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
#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Common/String.h"
#include "CSP/Common/Map.h"
#include "CSP/Multiplayer/LocalScript/LocalScriptResult.h"
#include "CSP/Multiplayer/Components/CodeSpaceComponent.h"
#include "Debug/Logging.h"
#include "CSP/Common/StringFormat.h"
#include "quickjspp.hpp"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Multiplayer/SpaceEntitySystem.h"

#include <map>
#include <string>
#include <sstream>

namespace csp::systems
{
class LocalScriptSystem;
class LocalScriptResult;
}

// namespace qjs
// {
// class Runtime;
// class Context;
// using Context::Module = qjs::Context::Module;
// }

namespace csp::services
{


} // namespace csp::services

/// @brief Namespace that encompasses everything in the multiplayer system
namespace csp::systems
{

/// @brief Manages the local script system
///
/// Provides functions for setting the script source, subscribing to property changes and messages and other script management.
class CSP_API LocalScriptSystem
{

public:
    /// @brief Construct a new instance of LocalScriptSystem.
    LocalScriptSystem(csp::multiplayer::SpaceEntitySystem* InEntitySystem);
    
    /// @brief Destroy the instance of LocalScriptSystem.
    ~LocalScriptSystem();

    void TickAnimationFrame(float timestamp);

    using ModuleSourceMap = std::map<std::string, std::string>;

    /// @brief Initialize the LocalScriptSystem and create a local context
    void Initialize();

    void SetSpaceId(const csp::common::String& InSpaceId)
    {
        this->SpaceId = InSpaceId;
    }

    void ParseAttributesForEntity(uint64_t EntityId);
    void RegisterCodeComponentInRegistry(uint64_t EntityId);
    /// @brief Load and register script modules from the given space
    void LoadScriptModules();
    CSP_NO_EXPORT void UpdateAttributeForEntity(uint64_t EntityId, const csp::common::String& Key, const csp::multiplayer::CodeAttribute& Attribute);



private:
    /// @brief The QuickJS context for script execution
    qjs::Context* Context; 
    qjs::Runtime* Runtime;
    csp::multiplayer::EntityScriptBinding* ScriptBinding;
    multiplayer::SpaceEntitySystem* EntitySystem;
    csp::common::String SpaceId;
    //qjs::Context::Module* CSP_Module;
    // Store loaded scripts to ensure they remain in memory
    csp::common::Map<csp::common::String, csp::common::String> LoadedScripts;
    void evalScript(const csp::common::String& script);
};



}

