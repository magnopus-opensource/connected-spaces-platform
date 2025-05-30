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
#include "CSP/Systems/Script/ScriptSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "Debug/Logging.h"
#include "quickjspp.hpp"
#include "CSP/Systems/Assets/AssetSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "Memory/Memory.h"
#include "Memory/MemoryManager.h"
#include "Systems/Script/ScriptContext.h"
#include <functional>

namespace csp::systems
{

// Define a constant for the local script context ID
constexpr const int64_t LOCAL_SCRIPT_CONTEXT_ID = 6666666666;

LocalScriptSystem::LocalScriptSystem()
    : ScriptSystem(csp::systems::SystemsManager::Get().GetScriptSystem())
{
    // Defer module initialization to avoid memory issues in WASM during construction
    CSP_LOG_MSG(csp::systems::LogLevel::Log, "LocalScriptSystem initialized");
    
    // Initialize the script system and local context
    Initialize();
}

LocalScriptSystem::~LocalScriptSystem() 
{
    // Cleanup will be handled by ScriptSystem's destruction
}

void LocalScriptSystem::Initialize()
{
    try 
    {
        CSP_LOG_MSG(csp::systems::LogLevel::Log, "LocalScriptSystem::Initialize starting");
        
        // Make sure ScriptSystem is initialized
        ScriptSystem->Initialise();
        
        // Create or get the local script context with the specific ID
        if (!ScriptSystem->HasContext(LOCAL_SCRIPT_CONTEXT_ID))
        {
            // Create a new context with our specific ID
            bool created = ScriptSystem->CreateContext(LOCAL_SCRIPT_CONTEXT_ID);
            if (!created)
            {
                CSP_LOG_ERROR_MSG("Failed to create local script context");
                return;
            }
        }
        
        CSP_LOG_MSG(csp::systems::LogLevel::Log, "LocalScriptSystem::Initialize completed successfully");
    }
    catch(const std::exception& e)
    {
        CSP_LOG_ERROR_FORMAT("Exception in LocalScriptSystem::Initialize: %s", e.what());
    }
}

void ScriptLog(qjs::rest<std::string> Args)
{
    std::stringstream Str;

    for (auto const& Arg : Args)
    {
        Str << Arg << " ";
    }

    CSP_LOG_FORMAT(csp::systems::LogLevel::Log, "%s", Str.str().c_str());
}

void LocalScriptSystem::LoadScriptModules(const csp::common::String& SpaceId)
{
    CSP_LOG_FORMAT(csp::systems::LogLevel::Log, "Loading script modules for space: %s", SpaceId.c_str());

    if (!ScriptSystem->HasContext(LOCAL_SCRIPT_CONTEXT_ID))
    {
        ScriptSystem->CreateContext(LOCAL_SCRIPT_CONTEXT_ID);
        CSP_LOG_MSG(csp::systems::LogLevel::Log, "Create Local Script Context");

        ScriptModule* cspModule = (ScriptModule*)ScriptSystem->AddModuleToContext(LOCAL_SCRIPT_CONTEXT_ID, "csp");
        qjs::Context* context = (qjs::Context*)ScriptSystem->GetContext(LOCAL_SCRIPT_CONTEXT_ID);
        auto Fn = [this](const char* Str)
        {
            ScriptSystem->FireLocalScriptCommand(Str);
        };
    
        try {
            cspModule->Module->function("log", ScriptLog);
            cspModule->Module->function("sendMessage", Fn);
            std::stringstream ss;
            ss << "import * as csp from 'csp';\n";
            ss << "globalThis.csp = csp;";
            context->eval(ss.str(), "<import>", JS_EVAL_TYPE_MODULE);
        } catch (const std::exception& e) {
            CSP_LOG_FORMAT(csp::systems::LogLevel::Error, "EntityScript::Exception called for %s", e.what());
        }
    }
    
    // Get asset system to load scripts
    csp::systems::AssetSystem* assetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
    if (!assetSystem)
    {
        CSP_LOG_ERROR_MSG("Failed to get AssetSystem");
        return;
    }
    
    // Create callback for processing loaded scripts
    auto scriptLoadedCallback = [this, SpaceId](const csp::systems::LocalScriptResult& Result) {
        if (Result.GetResultCode() == csp::systems::EResultCode::Success)
        {
            // Get all the loaded scripts
            const auto& scripts = Result.GetLocalScripts();
            size_t totalScripts = scripts.Keys()->Size();
            
            CSP_LOG_FORMAT(csp::systems::LogLevel::Log, "Loaded %zu scripts from space %s", totalScripts, SpaceId.c_str());
            
            // First pass: Register all modules so they're available for imports
            for (size_t i = 0; i < totalScripts; ++i)
            {
                auto scriptName = scripts.Keys()->operator[](i);
                auto scriptContent = scripts[scriptName];
                
                // Register with the script system
                ScriptSystem->SetModuleSource(scriptName, scriptContent);
                ScriptSystem->AddModuleToContext(LOCAL_SCRIPT_CONTEXT_ID, scriptName);
            }
            
            // Second pass: Evaluate and import all modules
            for (size_t i = 0; i < totalScripts; ++i)
            {
                try
                {
                    auto scriptName = scripts.Keys()->operator[](i);
                    auto scriptContent = scripts[scriptName];
                    
                    CSP_LOG_FORMAT(csp::systems::LogLevel::Log, "Importing script module: %s", scriptName.c_str());
                }
                catch (const std::exception& e)
                {
                    CSP_LOG_ERROR_FORMAT("Exception processing script: %s", e.what());
                }
            }
        }
        else 
        {
            CSP_LOG_ERROR_FORMAT("Failed to load scripts: Result code %d", (int)Result.GetResultCode());
        }
    };
    
    // Load scripts from the asset system
    assetSystem->LoadScripts(SpaceId, scriptLoadedCallback);
}

} // namespace csp::systems
