/*
 * Copyright 2026 Magnopus LLC

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

namespace csp::common
{
class IRealtimeEngine;
class LogSystem;
} // namespace csp::common

namespace csp::systems
{

class NgxScriptSystem;

// Opinionated runtime layer for CodeComponent behaviour.
class NgxCodeComponentRuntime
{
public:
    NgxCodeComponentRuntime(csp::common::LogSystem& InLogSystem, NgxScriptSystem& InNgxScriptSystem);

    void OnEnterSpace(const csp::common::String& InSpaceId, csp::common::IRealtimeEngine* InRealtimeEngine);
    void OnExitSpace();

private:
    void RegisterBuiltInModules();

    csp::common::LogSystem& LogSystem;
    NgxScriptSystem& ScriptSystem;
};

} // namespace csp::systems

