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
#include "Multiplayer/NgxScript/NgxEntityScriptBinding.h"

#include "CSP/CSPFoundation.h"
#include "CSP/Common/Interfaces/IJSScriptRunner.h"
#include "CSP/Common/Interfaces/IRealtimeEngine.h"
#include "CSP/Systems/SystemsManager.h"

#include "fmt/format.h"
#include "CSP/Common/Vector.h"
#include "CSP/Multiplayer/OnlineRealtimeEngine.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "Multiplayer/Script/EntityScriptBinding.h"
#include "Multiplayer/Script/EntityScriptInterface.h"
#include "Multiplayer/Script/ScriptHelpers.h"
#include "quickjspp.hpp"

namespace
{
class SingleContextScriptRunner final : public csp::common::IJSScriptRunner
{
public:
    SingleContextScriptRunner(qjs::Context* InContext, qjs::Context::Module* InModule)
        : Context(InContext)
        , Module(InModule)
    {
    }

    void* GetContext(int64_t) override { return Context; }
    void* GetModule(int64_t, const csp::common::String&) override { return Module; }

private:
    qjs::Context* Context;
    qjs::Context::Module* Module;
};
} // namespace

namespace csp::multiplayer
{

NgxEntityScriptBinding::NgxEntityScriptBinding(csp::common::IRealtimeEngine* InEntitySystem, csp::common::LogSystem& InLogSystem, bool LocalScope)
    : EntitySystem(InEntitySystem)
    , LegacyBinding(std::make_unique<EntityScriptBinding>(InEntitySystem, InLogSystem, LocalScope))
{
}

NgxEntityScriptBinding::~NgxEntityScriptBinding() = default;

void NgxEntityScriptBinding::Bind(int64_t ContextId, csp::common::IJSScriptRunner& ScriptRunner)
{
    auto* Context = static_cast<qjs::Context*>(ScriptRunner.GetContext(ContextId));
    if (Context == nullptr)
    {
        return;
    }
    BindToContext(*Context, ContextId);
}

void NgxEntityScriptBinding::BindToContext(qjs::Context& Context, int64_t ContextId)
{
    if (EntitySystem == nullptr)
    {
        return;
    }

    // Reuse legacy binding registration for Entity / EntitySystem APIs in this context.
    auto& LegacyModule = Context.addModule(csp::systems::SCRIPT_NAMESPACE);
    SingleContextScriptRunner ScriptRunnerBridge(&Context, &LegacyModule);
    LegacyBinding->Bind(ContextId, ScriptRunnerBridge);

    const bool bHasBoundThisEntity = (ContextId != 0) && (EntitySystem->FindSpaceEntityById(static_cast<uint64_t>(ContextId)) != nullptr);
    if (!bHasBoundThisEntity)
    {
        Context.eval("globalThis.ThisEntity = null;", "<ngx-entity-script-binding-reset-this-entity>", JS_EVAL_TYPE_GLOBAL);
    }

    Context.global()["__ngxGetEntityIsLocal"] = [entitySystem = EntitySystem](int64_t EntityId) -> bool
    {
        auto* Entity = entitySystem->FindSpaceEntityById(static_cast<uint64_t>(EntityId));
        return Entity != nullptr && Entity->IsLocal();
    };

    Context.global()["__ngxSetEntityIsLocal"] = [entitySystem = EntitySystem](int64_t EntityId, bool InIsLocal) -> bool
    {
        auto* Entity = entitySystem->FindSpaceEntityById(static_cast<uint64_t>(EntityId));
        if (Entity == nullptr)
        {
            return false;
        }

        Entity->SetLocal(InIsLocal);
        return true;
    };

    Context.global()["__ngxFireEntityEvent"] = [entitySystem = EntitySystem](int64_t EntityId, const std::string& EventName, qjs::Value Payload)
    {
        auto* Entity = entitySystem->FindSpaceEntityById(static_cast<uint64_t>(EntityId));
        if (Entity == nullptr)
        {
            auto* LogSystem = csp::systems::SystemsManager::Get().GetLogSystem();
            LogSystem->LogMsg(csp::common::LogLevel::Error,
                fmt::format("NgxScript: __ngxFireEntityEvent('{}') — entity {} not found in entity system.",
                    EventName, EntityId).c_str());
            return;
        }
        Entity->GetScriptInterface()->Fire(EventName, std::move(Payload));
    };

    Context.global()["__ngxCreateLocalEntity"] = [entitySystem = EntitySystem](const std::string& Name) -> EntityScriptInterface*
    {
        if (entitySystem == nullptr || entitySystem->GetRealtimeEngineType() != csp::common::RealtimeEngineType::Online)
        {
            return nullptr;
        }
        auto* OnlineEngine = static_cast<csp::multiplayer::OnlineRealtimeEngine*>(entitySystem);

        const csp::multiplayer::SpaceTransform DefaultTransform
            = { csp::common::Vector3::Zero(), csp::common::Vector4::Identity(), csp::common::Vector3::One() };

        SpaceEntity* CreatedEntity = nullptr;
        OnlineEngine->CreateLocalEntity(
            Name.c_str(), DefaultTransform, csp::common::Optional<uint64_t> {}, [&CreatedEntity](SpaceEntity* Entity) { CreatedEntity = Entity; });

        return CreatedEntity != nullptr ? CreatedEntity->GetScriptInterface() : nullptr;
    };

    constexpr const char* NgxCompatibilityPatch = R"JS(
if (globalThis.ThisEntity && typeof globalThis.__ngxGetEntityIsLocal === "function" && typeof globalThis.__ngxSetEntityIsLocal === "function") {
  Object.defineProperty(Object.getPrototypeOf(globalThis.ThisEntity), "isLocal", {
    configurable: true,
    enumerable: true,
    get() { return globalThis.__ngxGetEntityIsLocal(this.id); },
    set(value) { globalThis.__ngxSetEntityIsLocal(this.id, !!value); }
  });
}

if (typeof globalThis.TheEntitySystem === "undefined" || globalThis.TheEntitySystem === null) {
  globalThis.TheEntitySystem = {};
}

if (typeof globalThis.TheEntitySystem.getEntityByID !== "function") {
  globalThis.TheEntitySystem.getEntityByID = (...args) => globalThis.TheEntitySystem.getEntityById(...args);
}

if (typeof globalThis.__ngxCreateLocalEntity === "function") {
  globalThis.TheEntitySystem.createLocalEntity = (name) => globalThis.__ngxCreateLocalEntity(name);
}
)JS";

    Context.eval(NgxCompatibilityPatch, "<ngx-entity-script-binding>", JS_EVAL_TYPE_GLOBAL);
}

NgxEntityScriptBinding* NgxEntityScriptBinding::BindEntitySystem(
    csp::common::IRealtimeEngine* InEntitySystem, csp::common::LogSystem& LogSystem, csp::common::IJSScriptRunner& ScriptRunner, bool LocalScope)
{
    auto* ScriptBinding = new NgxEntityScriptBinding(InEntitySystem, LogSystem, LocalScope);
    ScriptRunner.RegisterScriptBinding(ScriptBinding);
    return ScriptBinding;
}

void NgxEntityScriptBinding::RemoveBinding(NgxEntityScriptBinding* InEntityBinding, csp::common::IJSScriptRunner& ScriptRunner)
{
    if (csp::CSPFoundation::GetIsInitialised())
    {
        ScriptRunner.UnregisterScriptBinding(InEntityBinding);
    }
}

} // namespace csp::multiplayer
