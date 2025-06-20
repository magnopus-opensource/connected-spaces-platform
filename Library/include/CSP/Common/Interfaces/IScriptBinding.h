/*
 * Copyright 2025 Magnopus LLC

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
#include "CSP/Common/Interfaces/IJSScriptRunner.h"
#include "CSP/Common/String.h"

namespace csp::common
{

/**
 * @class IScriptBinding
 * @brief Represents an object capable of performing a script binding
 *
 * This type was migrated during the modularisation effort in order to seperate
 * the script binding logic in the RealtimeEngine module from the ScriptSystem
 * in the Systems module. It is perhaps not the best architectural concept
 *
 * In all cases at time of writing, IJSScriptRunner would be a CSP ScriptSystem.
 */
class IScriptBinding
{
public:
    virtual ~IScriptBinding() = default;
    virtual void Bind(int64_t ContextId, csp::common::IJSScriptRunner& ScriptRunner) = 0;
};
}
