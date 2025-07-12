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
#include "Debug/Logging.h"
#include "quickjspp.hpp"
#include "CSP/Multiplayer/ComponentBase.h"
#include "CSP/Multiplayer/SpaceEntity.h"

#include <map>
#include <string>
#include <vector>

namespace csp::multiplayer
{

class SpaceEntity;
class ComponentScriptInterface;
class LightSpaceComponentScriptInterface;
class ButtonSpaceComponentScriptInterface;
class VideoPlayerSpaceComponentScriptInterface;

class EntityScriptInterface
{
public:
    EntityScriptInterface(SpaceEntity* InEntity = nullptr);

    void SetContext(qjs::Context* InContext);

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

    int32_t GetParentId();
    void SetParentId(int32_t ParentId);

    bool IsLocal() const;
    void SetLocal(bool bLocal);

    void RemoveParentEntity();

    std::vector<EntityScriptInterface*> GetChildEntities() const;

    EntityScriptInterface* GetParentEntity() const;

    const std::string GetName() const;
    int32_t GetId() const;

    void SubscribeToPropertyChange(int32_t ComponentId, int32_t PropertyKey, std::string Message);

    void SubscribeToMessage(std::string Message, std::string OnMessageCallback);
    void PostMessageToScript(std::string Message, std::string MessageParamsJson);

    void On(const std::string& EventName, qjs::Value Callback);
    void Off(const std::string& EventName, qjs::Value Callback);
    void Fire(const std::string& EventName, qjs::Value& EventData);

    std::vector<ComponentScriptInterface*> GetComponents();

    template <typename ScriptInterface, ComponentType Type> ScriptInterface* CreateComponentOfType();
    template <typename ScriptInterface, ComponentType Type> std::vector<ScriptInterface*> GetComponentsOfType();

private:
    SpaceEntity* Entity;
    qjs::Context* Context = nullptr;
    std::map<std::string, std::vector<qjs::Value>> EventListeners;
};

template <typename ScriptInterface, ComponentType Type>
ScriptInterface* EntityScriptInterface::CreateComponentOfType()
{
    if (Entity && Entity->IsLocal())
    {
        ComponentBase* Component = Entity->AddComponent(Type);
        return (ScriptInterface*)Component->GetScriptInterface();
    }
    return nullptr;
}

template <typename ScriptInterface, ComponentType Type>
std::vector<ScriptInterface*> EntityScriptInterface::GetComponentsOfType()
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
                Components.push_back((ScriptInterface*)Component->GetScriptInterface());
            }
        }

        delete (ComponentKeys);
    }

    return Components;
}

} // namespace csp::multiplayer
