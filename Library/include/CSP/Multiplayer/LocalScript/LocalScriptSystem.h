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

#include "CSP/Common/String.h"
#include "CSP/Common/Map.h"
#include "CSP/Multiplayer/LocalScript/LocalScriptResult.h"
#include "CSP/Systems/Script/ScriptSystem.h"

#include <map>
#include <string>

namespace csp::systems
{
class LocalScriptSystem;
class LocalScriptResult;
}

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
    LocalScriptSystem();
    
    /// @brief Destroy the instance of LocalScriptSystem.
    ~LocalScriptSystem();
    
    using ModuleSourceMap = std::map<std::string, std::string>;
    // Add a new method to initialize the module functions
    /// @brief Initializes the script module functions after construction
    /// This is called automatically by LoadAndRegisterScripts
    void InitializeModuleFunctions();


private:

    ModuleSourceMap Modules;

};



}

