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
#include "CSP/Multiplayer/Components/CodeSpaceComponent.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "Events/EventId.h"
#include "Events/EventListener.h"
#include "Events/EventSystem.h"
#include "Multiplayer/NgxScript/NgxScriptSystem.h"

#include <cmath>
#include <fmt/format.h>

namespace
{

constexpr const char* CODECOMPONENT_ASSET_REGISTRY_MODULE = "/scripts/engine/registry.js";
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

std::string EscapeJSStringLiteral(const std::string& Input)
{
    std::string Escaped;
    Escaped.reserve(Input.size());

    for (const char Character : Input)
    {
        switch (Character)
        {
        case '\\':
            Escaped += "\\\\";
            break;
        case '\'':
            Escaped += "\\'";
            break;
        case '\n':
            Escaped += "\\n";
            break;
        case '\r':
            Escaped += "\\r";
            break;
        case '\t':
            Escaped += "\\t";
            break;
        default:
            Escaped += Character;
            break;
        }
    }

    return Escaped;
}

std::string BuildAttributeValueLiteral(const csp::multiplayer::CodeAttribute& Attribute)
{
    switch (Attribute.Type)
    {
    case csp::multiplayer::CodePropertyType::Boolean:
        return Attribute.BooleanValue ? "true" : "false";
    case csp::multiplayer::CodePropertyType::Integer:
        return std::to_string(Attribute.IntegerValue);
    case csp::multiplayer::CodePropertyType::Float:
        if (!std::isfinite(Attribute.FloatValue))
        {
            return "0";
        }
        return fmt::format("{:.9g}", static_cast<double>(Attribute.FloatValue));
    case csp::multiplayer::CodePropertyType::String:
        return "'" + EscapeJSStringLiteral(Attribute.StringValue.c_str()) + "'";
    default:
        return "undefined";
    }
}

std::string BuildAttributesObjectLiteral(const std::unordered_map<std::string, csp::multiplayer::CodeAttribute>& Attributes)
{
    std::string Literal = "{";
    bool bIsFirst = true;

    for (const auto& AttributePair : Attributes)
    {
        if (!bIsFirst)
        {
            Literal += ",";
        }

        bIsFirst = false;
        Literal += "'";
        Literal += EscapeJSStringLiteral(AttributePair.first);
        Literal += "':";
        Literal += BuildAttributeValueLiteral(AttributePair.second);
    }

    Literal += "}";
    return Literal;
}

std::string BuildBootstrapSnippet(const std::string& ModulePath)
{
    const std::string EscapedModulePath = EscapeJSStringLiteral(ModulePath);

    return "import * as __ngxRegistryModule from '" + EscapedModulePath + "';\n"
           "const __ngxFactory = (typeof __ngxRegistryModule.createScriptRegistry === 'function')\n"
           "    ? __ngxRegistryModule.createScriptRegistry\n"
           "    : ((typeof __ngxRegistryModule.default === 'function') ? __ngxRegistryModule.default : null);\n"
           "if (typeof globalThis.scriptRegistry === 'undefined') {\n"
           "    if (__ngxFactory !== null) {\n"
           "        globalThis.scriptRegistry = __ngxFactory();\n"
           "    } else if (__ngxRegistryModule && typeof __ngxRegistryModule.default === 'object') {\n"
           "        globalThis.scriptRegistry = __ngxRegistryModule.default;\n"
           "    }\n"
           "}\n";
}

constexpr const char* CODECOMPONENT_REGISTRY_SOURCE = R"(
export function createScriptRegistry() {
    const codeComponents = new Map();

    const safeCall = (callback, payload, label) => {
        if (typeof callback !== 'function') {
            return;
        }

        try {
            callback(payload);
        } catch (error) {
            console.error(`NgxCodeComponentRuntime: ${label} failed`, error);
        }
    };

    const cloneAttributes = (attributes) => ({ ...(attributes || {}) });

    return {
        addCodeComponent(entityId, payload = {}) {
            const existing = codeComponents.get(entityId);
            if (existing && existing.module) {
                safeCall(existing.module.onDispose, { entityId, attributes: cloneAttributes(existing.attributes) }, "onDispose");
            }

            const scriptAssetPath = typeof payload.scriptAssetPath === 'string' ? payload.scriptAssetPath : '';
            const attributes = cloneAttributes(payload.attributes);
            const entry = {
                scriptAssetPath,
                attributes,
                module: null,
            };

            codeComponents.set(entityId, entry);

            if (!scriptAssetPath) {
                console.warn(`NgxCodeComponentRuntime: Missing scriptAssetPath for entity ${entityId}.`);
                return;
            }

            import(scriptAssetPath)
                .then((module) => {
                    const current = codeComponents.get(entityId);
                    if (!current || current.scriptAssetPath !== scriptAssetPath) {
                        return;
                    }

                    current.module = module;
                    safeCall(module.onInit, { entityId, attributes: cloneAttributes(current.attributes) }, "onInit");
                })
                .catch((error) => {
                    console.error(`NgxCodeComponentRuntime: Failed to import module ${scriptAssetPath}`, error);
                });
        },
        updateAttributeForEntity(entityId, key, value) {
            const entry = codeComponents.get(entityId);
            if (!entry) {
                return;
            }

            entry.attributes[key] = value;
            if (entry.module) {
                safeCall(entry.module.onUpdateAttributes,
                    { entityId, key, value, attributes: cloneAttributes(entry.attributes) },
                    "onUpdateAttributes");
            }
        },
        removeCodeComponent(entityId) {
            const entry = codeComponents.get(entityId);
            if (!entry) {
                return;
            }

            if (entry.module) {
                safeCall(entry.module.onDispose, { entityId, attributes: cloneAttributes(entry.attributes) }, "onDispose");
            }

            codeComponents.delete(entityId);
        },
        tick(timestampMs) {
            for (const [entityId, entry] of codeComponents) {
                if (entry.module) {
                    safeCall(entry.module.onTick,
                        { entityId, timestampMs, attributes: cloneAttributes(entry.attributes) },
                        "onTick");
                }
            }
        },
        destroy() {
            for (const [entityId, entry] of codeComponents) {
                if (entry.module) {
                    safeCall(entry.module.onDispose, { entityId, attributes: cloneAttributes(entry.attributes) }, "onDispose");
                }
            }

            codeComponents.clear();
        }
    };
}

export default createScriptRegistry;
)";

constexpr const char* CODECOMPONENT_BOOTSTRAP_SOURCE = R"(
import * as __ngxRegistryModule from '/__csp/internal/codecomponent/registry.js';
const __ngxFactory = (typeof __ngxRegistryModule.createScriptRegistry === 'function')
    ? __ngxRegistryModule.createScriptRegistry
    : ((typeof __ngxRegistryModule.default === 'function') ? __ngxRegistryModule.default : null);

if (typeof globalThis.scriptRegistry === 'undefined') {
    if (__ngxFactory !== null) {
        globalThis.scriptRegistry = __ngxFactory();
    } else if (__ngxRegistryModule && typeof __ngxRegistryModule.default === 'object') {
        globalThis.scriptRegistry = __ngxRegistryModule.default;
    }
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

class NgxCodeComponentRuntime::NgxCodeComponentTickEventHandler final : public csp::events::EventListener
{
public:
    explicit NgxCodeComponentTickEventHandler(NgxCodeComponentRuntime* InRuntime)
        : Runtime(InRuntime)
    {
    }

    void OnEvent(const csp::events::Event& InEvent) override
    {
        if (InEvent.GetId() == csp::events::FOUNDATION_TICK_EVENT_ID)
        {
            Runtime->OnTick();
        }
    }

private:
    NgxCodeComponentRuntime* Runtime;
};

NgxCodeComponentRuntime::NgxCodeComponentRuntime(csp::common::LogSystem& InLogSystem, NgxScriptSystem& InNgxScriptSystem)
    : LogSystem(InLogSystem)
    , ScriptSystem(InNgxScriptSystem)
    , ActiveRealtimeEngine(nullptr)
    , ActiveSpaceId("")
    , LastEntitySnapshots()
    , TickEventHandler(std::make_unique<NgxCodeComponentTickEventHandler>(this))
{
    RegisterBuiltInModules();
    csp::events::EventSystem::Get().RegisterListener(csp::events::FOUNDATION_TICK_EVENT_ID, TickEventHandler.get());
    LogSystem.LogMsg(csp::common::LogLevel::Log, "NgxCodeComponentRuntime Trace: Tick listener registered.");
}

NgxCodeComponentRuntime::~NgxCodeComponentRuntime()
{
    OnExitSpace();
    csp::events::EventSystem::Get().UnRegisterListener(csp::events::FOUNDATION_TICK_EVENT_ID, TickEventHandler.get());
}

void NgxCodeComponentRuntime::OnEnterSpace(const csp::common::String& InSpaceId, csp::common::IRealtimeEngine* InRealtimeEngine)
{
    const std::string EnterMessage = fmt::format("NgxCodeComponentRuntime Trace: OnEnterSpace(spaceId='{}', engineType={}).", InSpaceId.c_str(),
        RealtimeEngineTypeToString(InRealtimeEngine));
    LogSystem.LogMsg(csp::common::LogLevel::Log, EnterMessage.c_str());

    ActiveSpaceId = InSpaceId;
    ActiveRealtimeEngine = InRealtimeEngine;
    LastEntitySnapshots.clear();

    RegisterBuiltInModules();

    const bool bHasAssetRegistry = ScriptSystem.HasModuleSource(CODECOMPONENT_ASSET_REGISTRY_MODULE);
    if (bHasAssetRegistry)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Log,
            "NgxCodeComponentRuntime Trace: Bootstrap strategy uses asset module '/scripts/engine/registry.js'.");
        const std::string AssetBootstrapSnippet = BuildBootstrapSnippet(CODECOMPONENT_ASSET_REGISTRY_MODULE);
        if (!ScriptSystem.EvaluateSnippet(AssetBootstrapSnippet.c_str(), "<ngx-codecomponent-asset-bootstrap>"))
        {
            LogSystem.LogMsg(
                csp::common::LogLevel::Warning, "NgxCodeComponentRuntime: Failed to bootstrap script registry from asset module path.");
        }
        return;
    }

    LogSystem.LogMsg(csp::common::LogLevel::Log,
        "NgxCodeComponentRuntime Trace: Bootstrap strategy uses built-in fallback registry module.");
    if (!ScriptSystem.ExecuteModule(CODECOMPONENT_BOOTSTRAP_MODULE))
    {
        LogSystem.LogMsg(csp::common::LogLevel::Warning, "NgxCodeComponentRuntime: Failed to bootstrap built-in script registry module.");
    }
}

void NgxCodeComponentRuntime::OnExitSpace()
{
    if ((ActiveRealtimeEngine == nullptr) && ActiveSpaceId.IsEmpty() && LastEntitySnapshots.empty())
    {
        return;
    }

    LogSystem.LogMsg(csp::common::LogLevel::Log, "NgxCodeComponentRuntime Trace: OnExitSpace().");
    LastEntitySnapshots.clear();
    ActiveSpaceId = "";
    ActiveRealtimeEngine = nullptr;

    ScriptSystem.ExecuteModule(CODECOMPONENT_SHUTDOWN_MODULE);
    ScriptSystem.PumpPendingJobs();
}

void NgxCodeComponentRuntime::RegisterBuiltInModules()
{
    ScriptSystem.RegisterStaticModuleSource(CODECOMPONENT_REGISTRY_MODULE, CODECOMPONENT_REGISTRY_SOURCE);
    ScriptSystem.RegisterStaticModuleSource(CODECOMPONENT_BOOTSTRAP_MODULE, CODECOMPONENT_BOOTSTRAP_SOURCE);
    ScriptSystem.RegisterStaticModuleSource(CODECOMPONENT_SHUTDOWN_MODULE, CODECOMPONENT_SHUTDOWN_SOURCE);
}

void NgxCodeComponentRuntime::OnTick()
{
    if (ActiveRealtimeEngine == nullptr)
    {
        return;
    }

    ScriptSystem.PumpPendingJobs();

    EntitySnapshotMap CurrentSnapshots;
    if (!CaptureEntitySnapshots(CurrentSnapshots))
    {
        return;
    }

    SyncSnapshots(CurrentSnapshots);

    ScriptSystem.PumpPendingJobs();
}

bool NgxCodeComponentRuntime::CaptureEntitySnapshots(EntitySnapshotMap& OutSnapshots) const
{
    OutSnapshots.clear();
    if (ActiveRealtimeEngine == nullptr)
    {
        return false;
    }

    ActiveRealtimeEngine->LockEntityUpdate();
    struct UnlockGuard
    {
        explicit UnlockGuard(csp::common::IRealtimeEngine* InEngine)
            : Engine(InEngine)
        {
        }

        ~UnlockGuard() { Engine->UnlockEntityUpdate(); }
        csp::common::IRealtimeEngine* Engine;
    };
    UnlockGuard Guard(ActiveRealtimeEngine);

    const size_t EntityCount = ActiveRealtimeEngine->GetNumEntities();
    for (size_t EntityIndex = 0; EntityIndex < EntityCount; ++EntityIndex)
    {
        auto* Entity = ActiveRealtimeEngine->GetEntityByIndex(EntityIndex);
        if (Entity == nullptr)
        {
            continue;
        }

        auto* Component = Entity->FindFirstComponentOfType(csp::multiplayer::ComponentType::Code);
        if (Component == nullptr)
        {
            continue;
        }

        auto* CodeComponent = static_cast<csp::multiplayer::CodeSpaceComponent*>(Component);

        CodeComponentSnapshot Snapshot;
        Snapshot.ScriptAssetPath = CodeComponent->GetScriptAssetPath().c_str();

        const auto AttributeKeys = CodeComponent->GetAttributeKeys();
        for (size_t KeyIndex = 0; KeyIndex < AttributeKeys.Size(); ++KeyIndex)
        {
            csp::multiplayer::CodeAttribute Attribute;
            if (CodeComponent->GetAttribute(AttributeKeys[KeyIndex], Attribute))
            {
                Snapshot.Attributes[AttributeKeys[KeyIndex].c_str()] = Attribute;
            }
        }

        OutSnapshots[Entity->GetId()] = std::move(Snapshot);
    }

    return true;
}

void NgxCodeComponentRuntime::SyncSnapshots(const EntitySnapshotMap& CurrentSnapshots)
{
    for (const auto& PreviousEntityPair : LastEntitySnapshots)
    {
        if (CurrentSnapshots.find(PreviousEntityPair.first) == CurrentSnapshots.end())
        {
            RemoveEntityFromRegistry(PreviousEntityPair.first);
        }
    }

    for (const auto& CurrentEntityPair : CurrentSnapshots)
    {
        const uint64_t EntityId = CurrentEntityPair.first;
        const CodeComponentSnapshot& CurrentSnapshot = CurrentEntityPair.second;

        const auto PreviousSnapshotIt = LastEntitySnapshots.find(EntityId);
        if (PreviousSnapshotIt == LastEntitySnapshots.end())
        {
            AddOrReplaceEntityInRegistry(EntityId, CurrentSnapshot);
            continue;
        }

        const CodeComponentSnapshot& PreviousSnapshot = PreviousSnapshotIt->second;
        if (CurrentSnapshot.ScriptAssetPath != PreviousSnapshot.ScriptAssetPath)
        {
            AddOrReplaceEntityInRegistry(EntityId, CurrentSnapshot);
            continue;
        }

        bool bHasRemovedAttributes = false;
        for (const auto& PreviousAttributePair : PreviousSnapshot.Attributes)
        {
            if (CurrentSnapshot.Attributes.find(PreviousAttributePair.first) == CurrentSnapshot.Attributes.end())
            {
                bHasRemovedAttributes = true;
                break;
            }
        }

        if (bHasRemovedAttributes)
        {
            RemoveEntityFromRegistry(EntityId);
            AddOrReplaceEntityInRegistry(EntityId, CurrentSnapshot);
            continue;
        }

        for (const auto& CurrentAttributePair : CurrentSnapshot.Attributes)
        {
            const auto PreviousAttributeIt = PreviousSnapshot.Attributes.find(CurrentAttributePair.first);
            if ((PreviousAttributeIt == PreviousSnapshot.Attributes.end()) || (PreviousAttributeIt->second != CurrentAttributePair.second))
            {
                UpdateAttributeInRegistry(EntityId, CurrentAttributePair.first, CurrentAttributePair.second);
            }
        }
    }

    LastEntitySnapshots = CurrentSnapshots;
}

bool NgxCodeComponentRuntime::ExecuteRegistrySnippet(const std::string& Snippet, const char* DebugName) const
{
    if (!ScriptSystem.EvaluateSnippet(Snippet.c_str(), DebugName))
    {
        const std::string WarningMessage = fmt::format("NgxCodeComponentRuntime: Failed to evaluate registry snippet '{}'.", DebugName);
        LogSystem.LogMsg(csp::common::LogLevel::Warning, WarningMessage.c_str());
        return false;
    }

    return true;
}

void NgxCodeComponentRuntime::AddOrReplaceEntityInRegistry(uint64_t EntityId, const CodeComponentSnapshot& Snapshot)
{
    const std::string EntityIdString = std::to_string(EntityId);
    const std::string AttributesLiteral = BuildAttributesObjectLiteral(Snapshot.Attributes);
    const std::string Snippet = "if (globalThis.scriptRegistry && typeof globalThis.scriptRegistry.addCodeComponent === 'function') {\n"
        "    globalThis.scriptRegistry.addCodeComponent('"
        + EscapeJSStringLiteral(EntityIdString) + "', { scriptAssetPath: '" + EscapeJSStringLiteral(Snapshot.ScriptAssetPath)
        + "', attributes: " + AttributesLiteral + " });\n" + "}\n";

    ExecuteRegistrySnippet(Snippet, "<ngx-codecomponent-add>");
}

void NgxCodeComponentRuntime::UpdateAttributeInRegistry(
    uint64_t EntityId, const std::string& Key, const csp::multiplayer::CodeAttribute& Attribute)
{
    const std::string EntityIdString = std::to_string(EntityId);
    const std::string AttributeLiteral = BuildAttributeValueLiteral(Attribute);
    const std::string Snippet = "if (globalThis.scriptRegistry && typeof globalThis.scriptRegistry.updateAttributeForEntity === 'function') {\n"
        "    globalThis.scriptRegistry.updateAttributeForEntity('"
        + EscapeJSStringLiteral(EntityIdString) + "', '" + EscapeJSStringLiteral(Key) + "', " + AttributeLiteral + ");\n" + "}\n";

    ExecuteRegistrySnippet(Snippet, "<ngx-codecomponent-update>");
}

void NgxCodeComponentRuntime::RemoveEntityFromRegistry(uint64_t EntityId)
{
    const std::string EntityIdString = std::to_string(EntityId);
    const std::string Snippet = "if (globalThis.scriptRegistry && typeof globalThis.scriptRegistry.removeCodeComponent === 'function') {\n"
        "    globalThis.scriptRegistry.removeCodeComponent('"
        + EscapeJSStringLiteral(EntityIdString) + "');\n" + "}\n";

    ExecuteRegistrySnippet(Snippet, "<ngx-codecomponent-remove>");
}

} // namespace csp::systems

