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

#include "CSP/Multiplayer/SpaceEntity.h"
#include "quickjspp.hpp"

#include <map>
#include <string>
#include <vector>

namespace csp::multiplayer
{

class SpaceEntity;
class ComponentScriptInterface;
class AttachmentSpaceComponentScriptInterface;
class LightSpaceComponentScriptInterface;
class ButtonSpaceComponentScriptInterface;
class VideoPlayerSpaceComponentScriptInterface;
class AudioSpaceComponentScriptInterface;
class StaticModelSpaceComponentScriptInterface;
class AnimatedModelSpaceComponentScriptInterface;
class CinematicCameraSpaceComponentScriptInterface;
class CollisionSpaceComponentScriptInterface;
class ExternalLinkSpaceComponentScriptInterface;
class FogSpaceComponentScriptInterface;
class GaussianSplatSpaceComponentScriptInterface;
class ImageSpaceComponentScriptInterface;
class PortalSpaceComponentScriptInterface;
class ReflectionSpaceComponentScriptInterface;
class SplineSpaceComponentScriptInterface;
class TextSpaceComponentScriptInterface;

class EntityScriptInterface
{
public:
    EntityScriptInterface(SpaceEntity* InEntity = nullptr, bool IsLocal = false);

    using Vector3 = std::vector<float>;
    using Vector4 = std::vector<float>;

    Vector3 GetPosition() const;
    void SetPosition(Vector3 Pos);

    Vector3 GetGlobalPosition() const;

    Vector3 GetScale() const;
    void SetScale(Vector3 Scale);

    Vector3 GetGlobalScale() const;

    Vector4 GetRotation() const;
    void SetRotation(Vector4 Rot);

    Vector4 GetGlobalRotation() const;

    int64_t GetParentId();
    void SetParentId(int64_t ParentId);

    void RemoveParentEntity();

    EntityScriptInterface* GetParentEntity() const;

    const std::string GetName() const;
    int64_t GetId() const;
    bool IsLocal() const;
    void SetLocal(bool InIsLocal);
    std::vector<std::string> GetTags() const;
    void SetTags(const std::vector<std::string>& Tags);
    bool HasTag(const std::string& Tag) const;
    bool AddTag(const std::string& Tag);
    bool RemoveTag(const std::string& Tag);

    std::vector<EntityScriptInterface*> GetChildEntitiesByQuery(qjs::Value Query, qjs::rest<bool> Recursive);

    void SubscribeToPropertyChange(int32_t ComponentId, int32_t PropertyKey, std::string Message);

    void SubscribeToMessage(std::string Message, std::string OnMessageCallback);
    void PostMessageToScript(std::string Message, std::string MessageParamsJson);

    void SetLocalScope(bool IsLocal) { LocalScope = IsLocal; }

    void ClaimScriptOwnership();

    void On(const std::string& EventName, qjs::Value Callback);
    void Off(const std::string& EventName, qjs::Value Callback);
    void Fire(const std::string& EventName, qjs::Value EventData);

    /// @brief Removes all registered event listeners. Called when the script
    ///        context is rebuilt so stale callbacks from the old context are
    ///        discarded before new scripts re-register.
    void ClearEventListeners();

    std::vector<ComponentScriptInterface*> GetComponents();
    AttachmentSpaceComponentScriptInterface* AddAttachmentComponent();
    StaticModelSpaceComponentScriptInterface* AddStaticModelComponent();
    AnimatedModelSpaceComponentScriptInterface* AddAnimatedModelComponent();
    AudioSpaceComponentScriptInterface* AddAudioComponent();
    ButtonSpaceComponentScriptInterface* AddButtonComponent();
    CinematicCameraSpaceComponentScriptInterface* AddCinematicCameraComponent();
    CollisionSpaceComponentScriptInterface* AddCollisionComponent();
    ExternalLinkSpaceComponentScriptInterface* AddExternalLinkComponent();
    FogSpaceComponentScriptInterface* AddFogComponent();
    GaussianSplatSpaceComponentScriptInterface* AddGaussianSplatComponent();
    ImageSpaceComponentScriptInterface* AddImageComponent();
    LightSpaceComponentScriptInterface* AddLightComponent();
    PortalSpaceComponentScriptInterface* AddPortalComponent();
    ReflectionSpaceComponentScriptInterface* AddReflectionComponent();
    SplineSpaceComponentScriptInterface* AddSplineComponent();
    TextSpaceComponentScriptInterface* AddTextComponent();
    VideoPlayerSpaceComponentScriptInterface* AddVideoComponent();

    template <typename ScriptInterface, ComponentType Type> std::vector<ScriptInterface*> GetComponentsOfType();

private:
    void CommitEntityUpdate();
    template <typename ScriptInterface, ComponentType Type> ScriptInterface* AddComponentForScript();

    SpaceEntity* Entity;
    bool LocalScope = false;
    std::map<std::string, std::vector<qjs::Value>> EventListeners;
};

template <typename ScriptInterface, ComponentType Type> std::vector<ScriptInterface*> EntityScriptInterface::GetComponentsOfType()
{
    std::vector<ScriptInterface*> Components;

    if (Entity)
    {
        const ComponentType ThisType = Type;

        const auto& ComponentMap = *Entity->GetComponents();
        const auto ComponentKeys = ComponentMap.Keys();

        for (size_t i = 0; i < ComponentKeys->Size(); ++i)
        {
            ComponentBase* Component = ComponentMap[ComponentKeys->operator[](i)];

            if ((Component != nullptr) && (Component->GetComponentType() == ThisType) && (Component->GetScriptInterface() != nullptr))
            {
                auto* Iface = (ScriptInterface*)Component->GetScriptInterface();
                Iface->SetLocalScope(LocalScope || Entity->IsLocal());
                Components.push_back(Iface);
            }
        }

        delete (ComponentKeys);
    }

    return Components;
}

template <typename ScriptInterface, ComponentType Type> ScriptInterface* EntityScriptInterface::AddComponentForScript()
{
    if (Entity == nullptr)
    {
        return nullptr;
    }

    auto* Component = Entity->AddComponent(Type);
    if ((Component == nullptr) || (Component->GetScriptInterface() == nullptr))
    {
        return nullptr;
    }

    auto* Iface = static_cast<ScriptInterface*>(Component->GetScriptInterface());
    Iface->SetLocalScope(LocalScope || Entity->IsLocal());

    // Flush the dirty add-component patch so EntityUpdateCallback fires on the
    // next tick and downstream JS wrappers (cspwa OlySpaceEntity, Electra's
    // ToRenderingCommon component factories) actually learn about the new
    // component. Without this, AddComponent sits as a dirty entry on the
    // StatePatcher and the renderer-side never builds a body for it.
    if (LocalScope || Entity->IsLocal())
    {
        Entity->ApplyLocalPropertyPatch();
    }
    else
    {
        Entity->QueueUpdate();
    }

    return Iface;
}

} // namespace csp::multiplayer
