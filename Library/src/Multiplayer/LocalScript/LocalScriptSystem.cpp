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
#include "CSP/Multiplayer/LocalScript/signals.h"
#include "CSP/Multiplayer/Script/EntityScriptMessages.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Multiplayer/SpaceEntitySystem.h"
#include "CSP/Systems/Assets/AssetSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "Debug/Logging.h"
#include "Multiplayer/Script/EntityScriptBinding.h"
#include "quickjspp.hpp"
#include <algorithm>
#include <async++.h>
#include <chrono>
#include <exception>
#include <functional>
#include <future>
#include <limits>
#include <string>
#include <thread>


using namespace std;

namespace csp::systems
{

LocalScriptSystem::LocalScriptSystem(csp::multiplayer::SpaceEntitySystem* InEntitySystem)
    : EntitySystem(InEntitySystem)
{
    this->SpaceId = "";
    Runtime = nullptr;
    Context = nullptr;
    ScriptBinding = nullptr;
    
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
    try
    {
        if (ScriptBinding != nullptr)
        {
            delete ScriptBinding;
            ScriptBinding = nullptr;
        }
        if (Context != nullptr)
        {
            delete Context;
            Context = nullptr;
        }
        if (Runtime != nullptr)
        {
            delete Runtime;
            Runtime = nullptr;
        }
        Runtime = new qjs::Runtime();
        Context = new qjs::Context(*Runtime);
        ScriptBinding = new csp::multiplayer::EntityScriptBinding(EntitySystem);
        // Set the custom module loader on the context
        Context->moduleLoader = [this](std::string_view filename) -> qjs::Context::ModuleData
        {
            CSP_LOG_FORMAT(csp::systems::LogLevel::Log, "Loading module: %s", filename.data());
            csp::common::String Url = csp::common::String(filename.data(), filename.length());
            if (LoadedScripts.HasKey(Url.c_str()))
            {
                const auto& source_code = LoadedScripts[Url];
                return qjs::Context::ModuleData { Url.c_str(), source_code.c_str(), std::nullopt };
            }
            if (Url == "@preact/signals-core")
            {
                // Return the CSP signals module
                return qjs::Context::ModuleData { Url.c_str(), csp::systems::SignalsScriptCode.c_str(), std::nullopt };
            }
            // Return empty module if not found
            return qjs::Context::ModuleData { Url.c_str(), "", std::nullopt };
        };

        // Built in library
        qjs::Context::Module& CSP_Module = Context->addModule("csp");
        Context->eval(csp::systems::SignalsScriptCode.c_str(), "@preact/signals-core", JS_EVAL_TYPE_MODULE);
        // Context->eval(csp::systems::RegistryScriptCode.c_str(), "@csp/registry", JS_EVAL_TYPE_MODULE);
        // Context->eval(csp::systems::ScriptRef.c_str(), "@csp/script-registry", JS_EVAL_TYPE_MODULE);

        // Bind the existing script functions to the context
        ScriptBinding->BindLocalScriptRoot(Context, &CSP_Module);
    }
    catch (qjs::exception& e)
    { // Catch by reference
        CSP_LOG_ERROR_MSG("QuickJS exception");
    }
    catch (const std::exception& e)
    {
        CSP_LOG_ERROR_FORMAT("std::exception caught: %s", e.what());
    }
    catch (...)
    {
        CSP_LOG_ERROR_MSG("Unknown C++ exception caught.");
    }
}

void LocalScriptSystem::ParseAttributes(csp::multiplayer::CodeSpaceComponent* CodeComponent)
{

    if (LoadedScripts.HasKey(CodeComponent->GetScriptAssetPath().c_str()))
    {
        const auto& source_code = LoadedScripts[CodeComponent->GetScriptAssetPath().c_str()];
        (void)source_code;
    }
}

void LocalScriptSystem::TickAnimationFrame(float timestamp)
{
    if (Runtime == nullptr || Context == nullptr)
    {
        return;
    }
    // This is the javascript "Event Loop"
    // needed to process promises and async/await operations
    while (Runtime->isJobPending()) {
        Runtime->executePendingJob();
    }

    std::stringstream ss;
    ss << "typeof scriptRegistry !== 'undefined' && scriptRegistry.tick(" << timestamp << ");\n";
    try
    {
        Context->eval(ss.str(), "<import>", JS_EVAL_TYPE_MODULE);
    }
    catch (qjs::exception& e)
    {
        CSP_LOG_ERROR_MSG("QuickJS exception while running script");
    }
    catch (const std::exception& e)
    {
        CSP_LOG_ERROR_FORMAT("std::exception caught: %s", e.what());
    }
    catch (...)
    {
        CSP_LOG_ERROR_MSG("Unknown C++ exception caught while running script.");
    }
}

void LocalScriptSystem::LoadScriptModules()
{
    // Get asset system to load scripts
    csp::systems::AssetSystem* assetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
    if (!assetSystem)
    {
        CSP_LOG_ERROR_MSG("Failed to get AssetSystem");
        return;
    }

    // Create callback for processing loaded scripts
    auto scriptLoadedCallback = [this](const csp::systems::LocalScriptResult& Result)
    {
        if (Result.GetResultCode() == csp::systems::EResultCode::Success)
        {
            // Get all the loaded scripts
            const auto& scripts = Result.GetLocalScripts();
            // Store the scripts in the class instance to ensure they remain in memory
            this->LoadedScripts = scripts;
            std::stringstream ss;
            ss << "import { createScriptRegistry } from '/scripts/engine/registry.js';\n";
            ss << "const scriptRegistry = createScriptRegistry();\n";
            ss << "globalThis.scriptRegistry = scriptRegistry;\n";
            Context->eval(ss.str(), "<import>", JS_EVAL_TYPE_MODULE);
        }
        else
        {
            CSP_LOG_ERROR_FORMAT("Failed to load scripts: Result code %d", (int)Result.GetResultCode());
        }
    };

    // Load scripts from the asset system
    assetSystem->LoadScripts(this->SpaceId, scriptLoadedCallback);
}

void LocalScriptSystem::RegisterCodeComponentInRegistry(uint64_t EntityId, const csp::common::String& scriptAssetPath)
{   
    // Convert EntityId to string for proper concatenation
    std::string entityIdStr = std::to_string(EntityId);
    std::string scriptPathStr = scriptAssetPath.c_str();

    std::string out = R"(
        import { Log } from 'csp';
        
            const scriptAssetPath = ')" + scriptPathStr + R"(';
            const EntityId = ')" + entityIdStr + R"(';
            const registerModule = async () => {
            const module = await import(scriptAssetPath);
            const {attributes} = module;
            const types = {
                'string': 'setAttributeString',
                'number': 'setAttributeFloat',
                'slider': 'setAttributeFloat',
                'boolean': 'setAttributeBoolean',
                'vector3': 'setAttributeVector3',
                'quaternion': 'setAttributeVector4',
                'entity': 'setAttributeString',
            };
            for (const [key, value] of Object.entries(attributes)) {
                const fn = types[value.type];
                if (!fn) {  
                    Log(`Unknown attribute type: ${value.type} for key: ${key}`);
                    continue;
                }
               // TheEntitySystem[fn](EntityId, key, value.type, value.defaultValue, value.min ?? 0, value.max ?? 0, value.description ?? '');
            }
        };
        Log(`Registering code component ${scriptAssetPath} for EntityId: ${EntityId}`);
        registerModule().then(() => {
          scriptRegistry.addCodeComponent(EntityId);
        });
    )";
       
    //CSP_LOG_FORMAT(csp::systems::LogLevel::Log,"main script:\n%s", out.c_str());
    try {
        Context->eval(out, "<import>", JS_EVAL_TYPE_MODULE); // Fixed variable name from out2 to out
    } catch (qjs::exception& e)
    {
        CSP_LOG_ERROR_FORMAT("main script:\n%s", out.c_str());
    }
    catch (const std::exception& e)
    {
        CSP_LOG_ERROR_FORMAT("main script:\n%s", out.c_str());
    }
    catch (...)
    {
        CSP_LOG_ERROR_MSG("Unknown C++ exception caught while running script.");
    }
}

void LocalScriptSystem::Eval(const csp::common::String& Code, const csp::common::String& Path)
{
    (void)Path;
    // Initialize the script system and local context
    if (Context == nullptr || Runtime == nullptr)
    {
        CSP_LOG_ERROR_MSG("LocalScriptSystem not initialized. Call Initialize() first.");
        return;
    }

    // Evaluate the script in the context
    try
    {
        Context->eval(Code.c_str());
    }
    catch (qjs::exception& e)
    {
        CSP_LOG_ERROR_MSG("QuickJS exception while running script");
    }
    catch (const std::exception& e)
    {
        CSP_LOG_ERROR_FORMAT("std::exception caught: %s", e.what());
    }
    catch (...)
    {
        CSP_LOG_ERROR_MSG("Unknown C++ exception caught while running script.");
    }
}

void LocalScriptSystem::RunScript(const csp::common::String& Path)
{
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
    try
    {
        Context->eval(scriptSource.c_str(), Path.c_str(), JS_EVAL_TYPE_MODULE);
    }
    catch (qjs::exception& e)
    {
        CSP_LOG_ERROR_MSG("QuickJS exception while running script");
    }
    catch (const std::exception& e)
    {
        CSP_LOG_ERROR_FORMAT("std::exception caught: %s", e.what());
    }
    catch (...)
    {
        CSP_LOG_ERROR_MSG("Unknown C++ exception caught while running script.");
    }
}

} // namespace csp::systems
