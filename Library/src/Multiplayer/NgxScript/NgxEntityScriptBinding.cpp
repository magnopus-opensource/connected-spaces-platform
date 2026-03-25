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

        if (CreatedEntity == nullptr)
        {
            return nullptr;
        }

        auto* ScriptInterface = CreatedEntity->GetScriptInterface();
        if (ScriptInterface != nullptr)
        {
            ScriptInterface->SetLocalScope(true);
        }

        return ScriptInterface;
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

const __ngxPlayerControllerConfigTag = "player-controller-config";
const __ngxCodeComponentType = 35;

function __ngxFindPlayerControllerHostComponent() {
  if (!globalThis.TheEntitySystem || typeof globalThis.TheEntitySystem.getEntities !== "function") {
    return null;
  }

  const entities = globalThis.TheEntitySystem.getEntities();
  if (!Array.isArray(entities)) {
    return null;
  }

  const configEntity = entities.find((entity) => entity && typeof entity.hasTag === "function" && entity.hasTag(__ngxPlayerControllerConfigTag));
  if (!configEntity || typeof configEntity.getComponents !== "function") {
    return null;
  }

  const components = configEntity.getComponents();
  if (!Array.isArray(components)) {
    return null;
  }

  return components.find(
    (component) =>
      component &&
      component.type === __ngxCodeComponentType &&
      typeof component.invokeAction === "function"
  ) ?? null;
}

if (typeof globalThis.__cspPlayerController === "undefined" || globalThis.__cspPlayerController === null) {
  globalThis.__cspPlayerController = {};
}

globalThis.__cspPlayerController.moveCharacter = (x, y, z, jump = false, isFlying = false) => {
  const component = __ngxFindPlayerControllerHostComponent();
  if (!component) {
    return false;
  }

  component.invokeAction(
    "moveCharacter",
    JSON.stringify({ x: Number(x), y: Number(y), z: Number(z), jump: !!jump, isFlying: !!isFlying })
  );
  return true;
};

globalThis.__cspPlayerController.teleportCharacter = (x, y, z) => {
  const component = __ngxFindPlayerControllerHostComponent();
  if (!component) {
    return false;
  }

  component.invokeAction("teleportCharacter", JSON.stringify({ x: Number(x), y: Number(y), z: Number(z) }));
  return true;
};

globalThis.__cspPlayerController.setFirstPersonEnabled = (enabled) => {
  const component = __ngxFindPlayerControllerHostComponent();
  if (!component) {
    return false;
  }

  component.invokeAction("setFirstPersonEnabled", JSON.stringify({ enabled: !!enabled }));
  return true;
};

globalThis.__cspPlayerController.getCameraPosition = () =>
  (globalThis.csp && typeof globalThis.csp.__getLocalPlayerCameraPosition === "function")
    ? globalThis.csp.__getLocalPlayerCameraPosition()
    : [0, 0, 0];

globalThis.__cspPlayerController.getCameraRotation = () =>
  (globalThis.csp && typeof globalThis.csp.__getLocalPlayerCameraRotation === "function")
    ? globalThis.csp.__getLocalPlayerCameraRotation()
    : [0, 0, 0, 1];

globalThis.__cspPlayerController.getCameraForward = () =>
  (globalThis.csp && typeof globalThis.csp.__getLocalPlayerCameraForward === "function")
    ? globalThis.csp.__getLocalPlayerCameraForward()
    : [0, 0, -1];

globalThis.__cspPlayerController.getCameraForwardFlat = () =>
  (globalThis.csp && typeof globalThis.csp.__getLocalPlayerCameraForwardFlat === "function")
    ? globalThis.csp.__getLocalPlayerCameraForwardFlat()
    : [0, 0, -1];

globalThis.__cspPlayerController.getCameraRight = () =>
  (globalThis.csp && typeof globalThis.csp.__getLocalPlayerCameraRight === "function")
    ? globalThis.csp.__getLocalPlayerCameraRight()
    : [1, 0, 0];

globalThis.__cspPlayerController.getCameraRightFlat = () =>
  (globalThis.csp && typeof globalThis.csp.__getLocalPlayerCameraRightFlat === "function")
    ? globalThis.csp.__getLocalPlayerCameraRightFlat()
    : [1, 0, 0];

globalThis.__cspPlayerController.getCameraUp = () =>
  (globalThis.csp && typeof globalThis.csp.__getLocalPlayerCameraUp === "function")
    ? globalThis.csp.__getLocalPlayerCameraUp()
    : [0, 1, 0];

async function __ngxWaitForMaterial(material) {
  for (;;) {
    if (!material || material.status !== "loading") {
      return material;
    }
    await new Promise((resolve) => requestAnimationFrame(resolve));
  }
}

function __ngxInstallMaterialHelpersOnPrototype(proto) {
  if (!proto) {
    return;
  }

  if (typeof proto.getMaterial !== "function") {
    proto.getMaterial = async function(materialPath) {
      if (typeof this.__getMaterial !== "function") {
        return null;
      }
      return await __ngxWaitForMaterial(this.__getMaterial(String(materialPath ?? "")));
    };
  }

  if (typeof proto.getMaterials !== "function") {
    proto.getMaterials = async function() {
      if (typeof this.__getMaterialPaths !== "function") {
        return [];
      }
      const paths = this.__getMaterialPaths();
      const result = [];
      for (const path of Array.isArray(paths) ? paths : []) {
        result.push(await this.getMaterial(path));
      }
      return result;
    };
  }
}

function __ngxPatchComponentArray(components) {
  if (!Array.isArray(components) || components.length === 0) {
    return components;
  }

  const sample = components[0];
  if (sample && typeof sample.__getMaterial === "function" && typeof sample.__getMaterialPaths === "function") {
    __ngxInstallMaterialHelpersOnPrototype(Object.getPrototypeOf(sample));
  }
  return components;
}

function __ngxWrapEntityComponentGetter(name) {
  if (!globalThis.ThisEntity) {
    return;
  }

  const entityProto = Object.getPrototypeOf(globalThis.ThisEntity);
  if (!entityProto || typeof entityProto[name] !== "function" || entityProto[`__ngxWrapped_${name}`]) {
    return;
  }

  const original = entityProto[name];
  entityProto[name] = function(...args) {
    return __ngxPatchComponentArray(original.apply(this, args));
  };
  entityProto[`__ngxWrapped_${name}`] = true;
}

__ngxWrapEntityComponentGetter("getStaticModelComponents");
__ngxWrapEntityComponentGetter("getAnimatedModelComponents");

function __ngxPatchEntity(entity) {
  if (!entity) {
    return entity;
  }

  const entityProto = Object.getPrototypeOf(entity);
  if (!entityProto) {
    return entity;
  }

  for (const name of ["getStaticModelComponents", "getAnimatedModelComponents"]) {
    if (typeof entityProto[name] !== "function" || entityProto[`__ngxWrapped_${name}`]) {
      continue;
    }

    const original = entityProto[name];
    entityProto[name] = function(...args) {
      return __ngxPatchComponentArray(original.apply(this, args));
    };
    entityProto[`__ngxWrapped_${name}`] = true;
  }

  if (typeof entityProto.addComponent !== "function") {
    entityProto.addComponent = function(type) {
      const normalizedType = String(type ?? "").trim().toLowerCase().replace(/[\s_-]+/g, "");
      switch (normalizedType) {
        case "staticmodel":
        case "staticmodelspacecomponent":
          return typeof this.addStaticModelComponent === "function" ? this.addStaticModelComponent() : null;
        case "animatedmodel":
        case "animatedmodelspacecomponent":
          return typeof this.addAnimatedModelComponent === "function" ? this.addAnimatedModelComponent() : null;
        case "audio":
        case "audiospacecomponent":
          return typeof this.addAudioComponent === "function" ? this.addAudioComponent() : null;
        case "button":
        case "buttonspacecomponent":
          return typeof this.addButtonComponent === "function" ? this.addButtonComponent() : null;
        case "cinematiccamera":
        case "cinematiccameraspacecomponent":
          return typeof this.addCinematicCameraComponent === "function" ? this.addCinematicCameraComponent() : null;
        case "collision":
        case "collisionspacecomponent":
          return typeof this.addCollisionComponent === "function" ? this.addCollisionComponent() : null;
        case "externallink":
        case "externallinkspacecomponent":
          return typeof this.addExternalLinkComponent === "function" ? this.addExternalLinkComponent() : null;
        case "fog":
        case "fogspacecomponent":
          return typeof this.addFogComponent === "function" ? this.addFogComponent() : null;
        case "gaussiansplat":
        case "gaussiansplatspacecomponent":
          return typeof this.addGaussianSplatComponent === "function" ? this.addGaussianSplatComponent() : null;
        case "image":
        case "imagespacecomponent":
          return typeof this.addImageComponent === "function" ? this.addImageComponent() : null;
        case "light":
        case "lightspacecomponent":
          return typeof this.addLightComponent === "function" ? this.addLightComponent() : null;
        case "portal":
        case "portalspacecomponent":
          return typeof this.addPortalComponent === "function" ? this.addPortalComponent() : null;
        case "reflection":
        case "reflectionspacecomponent":
          return typeof this.addReflectionComponent === "function" ? this.addReflectionComponent() : null;
        case "spline":
        case "splinespacecomponent":
          return typeof this.addSplineComponent === "function" ? this.addSplineComponent() : null;
        case "text":
        case "textspacecomponent":
          return typeof this.addTextComponent === "function" ? this.addTextComponent() : null;
        case "video":
        case "videoplayer":
        case "videoplayerspacecomponent":
          return typeof this.addVideoComponent === "function" ? this.addVideoComponent() : null;
        case "script":
        case "scriptdata":
        case "scriptspacecomponent":
        case "code":
        case "codespacecomponent":
        case "codecomponent":
          return null;
        default:
          return null;
      }
    };
  }

  return entity;
}

if (globalThis.ThisEntity) {
  __ngxPatchEntity(globalThis.ThisEntity);
}

if (globalThis.TheEntitySystem) {
  for (const name of ["getEntityById", "getEntityByID", "getEntityByName"]) {
    if (typeof globalThis.TheEntitySystem[name] === "function" && !globalThis.TheEntitySystem[`__ngxWrapped_${name}`]) {
      const original = globalThis.TheEntitySystem[name];
      globalThis.TheEntitySystem[name] = function(...args) {
        return __ngxPatchEntity(original.apply(this, args));
      };
      globalThis.TheEntitySystem[`__ngxWrapped_${name}`] = true;
    }
  }

  for (const name of ["getEntities", "getObjects", "getAvatars", "getEntitiesByQuery", "getRootHierarchyEntities"]) {
    if (typeof globalThis.TheEntitySystem[name] === "function" && !globalThis.TheEntitySystem[`__ngxWrapped_${name}`]) {
      const original = globalThis.TheEntitySystem[name];
      globalThis.TheEntitySystem[name] = function(...args) {
        const entities = original.apply(this, args);
        if (Array.isArray(entities)) {
          for (const entity of entities) {
            __ngxPatchEntity(entity);
          }
        }
        return entities;
      };
      globalThis.TheEntitySystem[`__ngxWrapped_${name}`] = true;
    }
  }
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
