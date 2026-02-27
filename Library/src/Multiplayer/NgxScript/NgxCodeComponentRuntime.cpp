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

#include "Multiplayer/NgxScript/NgxCodeComponentRuntime.h"

#include "CSP/Common/Interfaces/IRealtimeEngine.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "Multiplayer/NgxScript/NgxScriptSystem.h"

#include <fmt/format.h>

namespace
{

constexpr const char* CODECOMPONENT_REGISTRY_MODULE = "/__csp/internal/codecomponent/registry.js";
constexpr const char* CODECOMPONENT_BOOTSTRAP_MODULE = "/__csp/internal/codecomponent/bootstrap.js";
constexpr const char* CODECOMPONENT_SHUTDOWN_MODULE = "/__csp/internal/codecomponent/shutdown.js";

const char* RealtimeEngineTypeToString(csp::common::IRealtimeEngine* RealtimeEngine)
{
    if (RealtimeEngine == nullptr)
    {
        return "Null";
    }

    switch (RealtimeEngine->GetRealtimeEngineType())
    {
    case csp::common::RealtimeEngineType::Online:
        return "Online";
    case csp::common::RealtimeEngineType::Offline:
        return "Offline";
    default:
        return "Unknown";
    }
}

constexpr const char* CODECOMPONENT_REGISTRY_SOURCE = R"(
export function createScriptRegistry() {
    const codeComponents = new Map();

    return {
        addCodeComponent(entityId, attributes = {}) {
            codeComponents.set(entityId, { attributes: { ...attributes } });
        },
        updateAttributeForEntity(entityId, key, value) {
            if (!codeComponents.has(entityId)) {
                return;
            }

            codeComponents.get(entityId).attributes[key] = value;
        },
        removeCodeComponent(entityId) {
            codeComponents.delete(entityId);
        },
        tick(_timestamp) {
            // Reserved for client-side update loop integration.
        },
        destroy() {
            codeComponents.clear();
        }
    };
}

export default createScriptRegistry;
)";

constexpr const char* CODECOMPONENT_BOOTSTRAP_SOURCE = R"(
import { createScriptRegistry } from '/__csp/internal/codecomponent/registry.js';

if (typeof globalThis.scriptRegistry === 'undefined') {
    globalThis.scriptRegistry = createScriptRegistry();
}
)";

constexpr const char* CODECOMPONENT_SHUTDOWN_SOURCE = R"(
if (typeof globalThis.scriptRegistry !== 'undefined') {
    if (typeof globalThis.scriptRegistry.destroy === 'function') {
        globalThis.scriptRegistry.destroy();
    }
    delete globalThis.scriptRegistry;
}
)";

} // namespace

namespace csp::systems
{

NgxCodeComponentRuntime::NgxCodeComponentRuntime(csp::common::LogSystem& InLogSystem, NgxScriptSystem& InNgxScriptSystem)
    : LogSystem(InLogSystem)
    , ScriptSystem(InNgxScriptSystem)
{
    RegisterBuiltInModules();
}

void NgxCodeComponentRuntime::OnEnterSpace(const csp::common::String& InSpaceId, csp::common::IRealtimeEngine* InRealtimeEngine)
{
    const std::string EnterMessage = fmt::format("NgxCodeComponentRuntime Trace: OnEnterSpace(spaceId='{}', engineType={}).", InSpaceId.c_str(),
        RealtimeEngineTypeToString(InRealtimeEngine));
    LogSystem.LogMsg(csp::common::LogLevel::Log, EnterMessage.c_str());

    RegisterBuiltInModules();

    if (!ScriptSystem.ExecuteModule(CODECOMPONENT_BOOTSTRAP_MODULE))
    {
        LogSystem.LogMsg(csp::common::LogLevel::Warning, "NgxCodeComponentRuntime: Failed to bootstrap script registry module.");
    }
}

void NgxCodeComponentRuntime::OnExitSpace()
{
    LogSystem.LogMsg(csp::common::LogLevel::Log, "NgxCodeComponentRuntime Trace: OnExitSpace().");
    ScriptSystem.ExecuteModule(CODECOMPONENT_SHUTDOWN_MODULE);
}

void NgxCodeComponentRuntime::RegisterBuiltInModules()
{
    ScriptSystem.RegisterStaticModuleSource(CODECOMPONENT_REGISTRY_MODULE, CODECOMPONENT_REGISTRY_SOURCE);
    ScriptSystem.RegisterStaticModuleSource(CODECOMPONENT_BOOTSTRAP_MODULE, CODECOMPONENT_BOOTSTRAP_SOURCE);
    ScriptSystem.RegisterStaticModuleSource(CODECOMPONENT_SHUTDOWN_MODULE, CODECOMPONENT_SHUTDOWN_SOURCE);
}

} // namespace csp::systems

