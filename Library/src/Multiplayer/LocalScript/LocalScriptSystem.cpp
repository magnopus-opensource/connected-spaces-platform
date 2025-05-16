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

//constexpr const int64_t LocalScriptContextId = 6666666666;


namespace csp::systems
{

LocalScriptSystem::LocalScriptSystem()
{
   
    // Register the local script system with the script system    
    // Defer module initialization to avoid memory issues in WASM during construction
    CSP_LOG_MSG(csp::systems::LogLevel::Log, "LocalScriptSystem initialized");

}

// Keep InitializeModuleFunctions but implement it more directly
void LocalScriptSystem::InitializeModuleFunctions()
{
    
}

LocalScriptSystem::~LocalScriptSystem() {  }



} // namespace csp::multiplayer
