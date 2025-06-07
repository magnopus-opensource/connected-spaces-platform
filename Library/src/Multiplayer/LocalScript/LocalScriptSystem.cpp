/*
 * Copyright 2023 Magnopus LLC

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "CSP/Multiplayer/LocalScript/LocalScriptSystem.h"
#include "CSP/Common/StringFormat.h"
#include "CSP/Multiplayer/Script/EntityScriptMessages.h"
#include "Multiplayer/Script/EntityScriptBinding.h"
#include "CSP/Systems/SystemsManager.h"
#include "Debug/Logging.h"
#include "quickjspp.hpp"
#include "CSP/Systems/Assets/AssetSystem.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Multiplayer/SpaceEntitySystem.h"
#include <functional>
#include <string>
#include <algorithm>
#include <async++.h>
#include <chrono>
#include <exception>
#include <future>
#include <limits>
#include <thread>

using namespace std;

void EntityScriptLog(qjs::rest<std::string> Args)
{
    std::stringstream Str;

    for (const auto& Arg : Args)
    {
        Str << Arg << " ";
    }

    CSP_LOG_FORMAT(csp::systems::LogLevel::Log, "%s", Str.str().c_str());
}

namespace csp::systems
{

// Define a constant for the local script context ID
//constexpr const int64_t LOCAL_SCRIPT_CONTEXT_ID = 6666666666;

// Don't forward declare ScriptLog as a free function - we'll make it a class method instead

LocalScriptSystem::LocalScriptSystem(csp::multiplayer::SpaceEntitySystem* InEntitySystem)
    : EntitySystem(InEntitySystem)
{
    // Defer module initialization to avoid memory issues in WASM during construction
    CSP_LOG_MSG(csp::systems::LogLevel::Log, "LocalScriptSystem initialized");
    this->SpaceId = "";
}

LocalScriptSystem::~LocalScriptSystem() 
{
    delete Context;
    delete Runtime;
    delete ScriptBinding;
}

void LocalScriptSystem::Initialize()
{
    CSP_LOG_MSG(csp::systems::LogLevel::Log, "LocalScriptSystem cleanup");
    try {
        Runtime = new qjs::Runtime(); 
        Context = new qjs::Context(*Runtime);
        ScriptBinding = new csp::multiplayer::EntityScriptBinding(EntitySystem);
        // Built in library
        qjs::Context::Module& CSP_Module = Context->addModule("csp");
        CSP_Module.function("log", &EntityScriptLog);
        // Bind the existing script functions to the context
        ScriptBinding->BindLocalScriptRoot(Context, &CSP_Module);

        // Set the custom module loader on the context
        Context->moduleLoader = [this](std::string_view filename) -> qjs::Context::ModuleData
            {
                csp::common::String Url = csp::common::String(filename.data(), filename.length());
                if (LoadedScripts.HasKey(Url.c_str()))
                {
                    const auto& source_code = LoadedScripts[Url];
                    return qjs::Context::ModuleData{Url.c_str(), source_code.c_str(), std::nullopt};
                }
                // Return empty module if not found
                return qjs::Context::ModuleData{ Url.c_str(), "", std::nullopt};
            };

    } catch (qjs::exception& e) { // Catch by reference
        CSP_LOG_ERROR_MSG("QuickJS exception");
    } catch (const std::exception& e) {
        CSP_LOG_ERROR_FORMAT("std::exception caught: %s", e.what());
    } catch (...) {
        CSP_LOG_ERROR_MSG("Unknown C++ exception caught.");
    }
}

void LocalScriptSystem::TickAnimationFrame(int32_t timestamp) {
    if (Context != nullptr) {
        std::stringstream ss;
        ss << "if (globalThis.tick) {\n";
        ss <<   "globalThis.tick(" << timestamp << ");\n";
        ss << "}\n";
        try {
            Context->eval(ss.str(),"<eval>", JS_EVAL_TYPE_MODULE);
        } catch (qjs::exception& e) {
            CSP_LOG_ERROR_MSG("QuickJS exception while running script");
        } catch (const std::exception& e) {
            CSP_LOG_ERROR_FORMAT("std::exception caught: %s", e.what());
        } catch (...) {
            CSP_LOG_ERROR_MSG("Unknown C++ exception caught while running script.");
        }
    }
}

// // update script modules from the AssetDetailBlobParams
// void LocalScriptSystem::UpdateScriptModule(AssetDetailBlobParams params, Callback)
// {
//     // Get asset system to load scripts
//     csp::systems::AssetSystem* assetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
//     if (!assetSystem)
//     {
//         CSP_LOG_ERROR_MSG("Failed to get AssetSystem");
//         return;
//     }
//     // get the asset using the AssetId and AssetCollectionId  using the assetSystem
//     assetSystem->GetAssetById(params.AssetCollectionId, params.AssetId, [this, params](const AssetResult& AssetResult)
//         {
//             if (CreateAssetResult.GetResultCode() != EResultCode::Success)
//             {
//                 Callback(NullResult(AssetResult.GetResultCode(), AssetResult.GetHttpResultCode()));
//                 return;
//             }

//             // 3. Upload material
//             auto UploadMaterialCallback
//                 = [Callback](const UriResult& UploadResult) { Callback(NullResult(UploadResult.GetResultCode(), UploadResult.GetHttpResultCode())); };

//             csp::common::String MaterialJson;

//             // Serialse material data.
//             SerializeMaterialOfType(Material.GetShaderType(), &Material, MaterialJson);

//             const Asset& CreatedAsset = CreateAssetResult.GetAsset();

//             // Create a new string to prevent const casting
//             std::string Buffer(MaterialJson.c_str());

//             BufferAssetDataSource AssetData;
//             AssetData.SetMimeType("application/json");
//             AssetData.Buffer = Buffer.data();
//             AssetData.BufferLength = MaterialJson.Length();

//             UploadAssetData(CreatedAssetCollection, CreatedAsset, AssetData, UploadMaterialCallback);
//         };
//     });   

//     CSP_LOG_FORMAT(csp::systems::LogLevel::Log, "Updating script module at path: %s in space: %s", Path.c_str(), SpaceId.c_str());

//     // Update the loaded scripts with the new source code
//     LoadedScripts[Path] = Source;
// }

void LocalScriptSystem::LoadScriptModules()
{
    CSP_LOG_FORMAT(csp::systems::LogLevel::Log, "Loading script modules for space: %s", this->SpaceId.c_str());

    // Get asset system to load scripts
    csp::systems::AssetSystem* assetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
    if (!assetSystem)
    {
        CSP_LOG_ERROR_MSG("Failed to get AssetSystem");
        return;
    }
    
    // Create callback for processing loaded scripts
    auto scriptLoadedCallback = [this](const csp::systems::LocalScriptResult& Result) {
        if (Result.GetResultCode() == csp::systems::EResultCode::Success)
        {
            // Get all the loaded scripts
            const auto& scripts = Result.GetLocalScripts();
            
            // Store the scripts in the class instance to ensure they remain in memory
            this->LoadedScripts = scripts;
            
            size_t totalScripts = LoadedScripts.Keys()->Size();
            
            CSP_LOG_FORMAT(csp::systems::LogLevel::Log, "Loaded %zu scripts from space %s", totalScripts, this->SpaceId.c_str());
            
            // First pass: Register all modules so they're available for imports
    
        }
        else 
        {
            CSP_LOG_ERROR_FORMAT("Failed to load scripts: Result code %d", (int)Result.GetResultCode());
        }
    };
    
    // Load scripts from the asset system
    assetSystem->LoadScripts(this->SpaceId, scriptLoadedCallback);
}

void LocalScriptSystem::RunScript(const csp::common::String& Path)
{
    CSP_LOG_FORMAT(csp::systems::LogLevel::Log, "Running script from path: %s in space: %s", Path.c_str(), this->SpaceId.c_str());
        // Initialize the script system and local context
    if (Context == nullptr || Runtime == nullptr)
    {
        CSP_LOG_ERROR_MSG("LocalScriptSystem not initialized. Call Initialize() first.");
        return;
    }
    // Check if the script is loaded
    if (!LoadedScripts.HasKey(Path))
    {
        CSP_LOG_ERROR_FORMAT("Script not found: %s", Path.c_str());
        return;
    }

    // Get the script source code
    const auto& scriptSource = LoadedScripts[Path];

    // Evaluate the script in the context
    try {
        Context->eval(scriptSource.c_str(), Path.c_str(), JS_EVAL_TYPE_MODULE);
    } catch (qjs::exception& e) {
        CSP_LOG_ERROR_MSG("QuickJS exception while running script");
    } catch (const std::exception& e) {
        CSP_LOG_ERROR_FORMAT("std::exception caught: %s", e.what());
    } catch (...) {
        CSP_LOG_ERROR_MSG("Unknown C++ exception caught while running script.");
    }
}

bool LocalScriptSystem::LoadScriptModulesWithTimeout(uint32_t TimeoutMs)
{
    try
    {

        // For non-WASM platforms, use the timeout mechanism
        // Create a future to represent the loading operation
        std::promise<bool> loadPromise;
        auto loadFuture = loadPromise.get_future();
        
        // Create a thread to perform the loading
        std::thread loadThread([this, &loadPromise]() {
            try
            {
                this->LoadScriptModules();
                loadPromise.set_value(true);
            }
            catch(const std::exception& e)
            {
                CSP_LOG_FORMAT(csp::systems::LogLevel::Error, "Exception in script loading: %s", e.what());
                loadPromise.set_value(false);
            }
        });
        
        // Wait for the future with timeout
        if (loadFuture.wait_for(std::chrono::milliseconds(TimeoutMs)) == std::future_status::ready)
        {
            bool result = loadFuture.get();
            loadThread.join();
            return result;
        }
        else
        {
            // We hit the timeout
            CSP_LOG_MSG(csp::systems::LogLevel::Warning, "Script module loading timed out");
            loadThread.detach(); // Can't safely join a potentially hung thread
            return false;
        }
    }
    catch(const std::exception& e)
    {
        CSP_LOG_FORMAT(csp::systems::LogLevel::Error, "Failed to schedule script loading: %s", e.what());
        return false;
    }
}
} // namespace csp::systems
