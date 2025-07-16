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
#include "Multiplayer/Script/EntityScriptInterface.h"

#include "CSP/Multiplayer/Script/EntityScript.h"
#include "CSP/Multiplayer/SpaceEntity.h"

using namespace csp::systems;

namespace csp::multiplayer
{

EntityScriptInterface::EntityScriptInterface(SpaceEntity* InEntity)
    : Entity(InEntity)
{
}

void EntityScriptInterface::SetContext(qjs::Context* InContext)
{
    Context = InContext;
}

EntityScriptInterface::Vector3 EntityScriptInterface::GetPosition() const
{
    EntityScriptInterface::Vector3 Pos = { 0, 0, 0 };

    if (Entity)
    {
        const csp::common::Vector3& Position = Entity->GetPosition();

        Pos[0] = Position.X;
        Pos[1] = Position.Y;
        Pos[2] = Position.Z;
    }

    return Pos;
}

void EntityScriptInterface::SetPosition(EntityScriptInterface::Vector3 Pos)
{
    // CSP_LOG_FORMAT(LogLevel::VeryVerbose, "EntityScriptWrapper::SetPosition { %.2f, %.2f, %.2f }\n", Pos[0], Pos[1], Pos[2]);

    const csp::common::Vector3 NewPosition(Pos[0], Pos[1], Pos[2]);
    const csp::common::Vector3& CurrentPosition = Entity->GetPosition();

    if (CurrentPosition != NewPosition)
    {
        Entity->SetPosition(NewPosition);
        Entity->MarkForUpdate();
    }
}

EntityScriptInterface::Vector3 EntityScriptInterface::GetGlobalPosition() const
{
    EntityScriptInterface::Vector3 GlobalPos = { 0, 0, 0 };

    if (Entity)
    {
        const csp::common::Vector3& Position = Entity->GetGlobalPosition();

        GlobalPos[0] = Position.X;
        GlobalPos[1] = Position.Y;
        GlobalPos[2] = Position.Z;
    }

    return GlobalPos;
}

EntityScriptInterface::Vector4 EntityScriptInterface::GetRotation() const
{
    EntityScriptInterface::Vector4 Rot = { 0, 0, 0, 0 };

    if (Entity)
    {
        csp::common::Vector4 Rotation = Entity->GetRotation();

        Rot[0] = Rotation.X;
        Rot[1] = Rotation.Y;
        Rot[2] = Rotation.Z;
        Rot[3] = Rotation.W;
    }

    return Rot;
}

void EntityScriptInterface::SetRotation(EntityScriptInterface::Vector4 Rot)
{

    const csp::common::Vector4 NewRotation(Rot[0], Rot[1], Rot[2], Rot[3]);
    const csp::common::Vector4& CurrentRotation = Entity->GetRotation();

    if (CurrentRotation != NewRotation)
    {
        Entity->SetRotation(NewRotation);
        Entity->MarkForUpdate();
    }
}

EntityScriptInterface::Vector4 EntityScriptInterface::GetGlobalRotation() const
{
    EntityScriptInterface::Vector4 GlobalRot = { 0, 0, 0, 0 };

    if (Entity)
    {
        csp::common::Vector4 Rotation = Entity->GetGlobalRotation();

        GlobalRot[0] = Rotation.X;
        GlobalRot[1] = Rotation.Y;
        GlobalRot[2] = Rotation.Z;
        GlobalRot[3] = Rotation.W;
    }

    return GlobalRot;
}

int32_t EntityScriptInterface::GetParentId()
{
    if (const auto Parent = Entity->GetParentEntity())
    {
        return Parent->GetId();
    }

    return 0;
}

void EntityScriptInterface::SetParentId(int32_t ParentId)
{
    if (Entity)
    {
        Entity->SetParentId(ParentId);
    }
}

void EntityScriptInterface::RemoveParentEntity()
{
    if (Entity)
    {
        Entity->RemoveParentEntity();
    }
}

EntityScriptInterface* EntityScriptInterface::GetParentEntity() const
{
    if (Entity)
    {
        SpaceEntity* Parent = Entity->GetParentEntity();
        if (Parent)
        {
            return Parent->GetScriptInterface();
        }
    }
    return nullptr;
}

std::vector<EntityScriptInterface*> EntityScriptInterface::GetChildEntities() const
{
    std::vector<EntityScriptInterface*> ResultEntities;

    if (Entity)
    {
        const csp::common::List<SpaceEntity*>* ChildList = Entity->GetChildEntities();

        for (size_t i = 0; i < ChildList->Size(); ++i)
        {
            SpaceEntity* Child = ChildList->operator[](i);
            ResultEntities.push_back(Child->GetScriptInterface());
        }
    }

    return ResultEntities;
}

EntityScriptInterface::Vector3 EntityScriptInterface::GetScale() const
{
    EntityScriptInterface::Vector3 Scale = { 0, 0, 0 };

    if (Entity)
    {
        csp::common::Vector3 EntityScale = Entity->GetScale();

        Scale[0] = EntityScale.X;
        Scale[1] = EntityScale.Y;
        Scale[2] = EntityScale.Z;
    }

    return Scale;
}

void EntityScriptInterface::SetScale(EntityScriptInterface::Vector3 Scale)
{
    const csp::common::Vector3 NewScale(Scale[0], Scale[1], Scale[2]);
    const csp::common::Vector3& CurrentScale = Entity->GetScale();

    if (CurrentScale != NewScale)
    {
        Entity->SetScale(NewScale);
        Entity->MarkForUpdate();
    }
}

EntityScriptInterface::Vector3 EntityScriptInterface::GetGlobalScale() const
{
    EntityScriptInterface::Vector3 GlobalScale = { 0, 0, 0 };

    if (Entity)
    {
        csp::common::Vector3 EntityScale = Entity->GetGlobalScale();

        GlobalScale[0] = EntityScale.X;
        GlobalScale[1] = EntityScale.Y;
        GlobalScale[2] = EntityScale.Z;
    }

    return GlobalScale;
}

const std::string EntityScriptInterface::GetName() const { return Entity->GetName().c_str(); }

int32_t EntityScriptInterface::GetId() const { return static_cast<int32_t>(Entity->GetId()); }

bool EntityScriptInterface::IsLocal() const
{
    if (Entity)
    {
        return Entity->IsLocal();
    }
    return false;
}

void EntityScriptInterface::SetLocal(bool bLocal)
{
    if (Entity)
    {
        Entity->SetLocal(bLocal);
    }
}

void EntityScriptInterface::SubscribeToPropertyChange(int32_t ComponentId, int32_t PropertyKey, std::string Message)
{
    Entity->GetScript().SubscribeToPropertyChange(ComponentId, PropertyKey, Message.c_str());
}

void EntityScriptInterface::SubscribeToMessage(std::string Message, std::string MessageParamsJson)
{
    Entity->GetScript().SubscribeToMessage(Message.c_str(), MessageParamsJson.c_str());
}

void EntityScriptInterface::PostMessageToScript(std::string Message, std::string MessageParamsJson)
{
    Entity->GetScript().PostMessageToScript(Message.c_str(), MessageParamsJson.c_str());
}

void EntityScriptInterface::On(const std::string& EventName, qjs::Value Cb)
{
    if (!JS_IsFunction(Context->ctx, Cb.v))
    {
        return;
    }

    EventListeners[EventName].push_back(std::move(Cb));
}

void EntityScriptInterface::Off(const std::string& EventName, qjs::Value Cb)
{
    if (!JS_IsFunction(Context->ctx, Cb.v))
    {
        return;
    }

    auto it = EventListeners.find(EventName);
    if (it != EventListeners.end())
    {
        auto& listeners = it->second;
        listeners.erase(std::remove_if(listeners.begin(),
                                       listeners.end(),
                                       [&](const qjs::Value& storedCb)
                                       {
                                           return JS_VALUE_GET_PTR(storedCb.v) == JS_VALUE_GET_PTR(Cb.v);
                                       }),
                        listeners.end());
    }
}

void EntityScriptInterface::Fire(const std::string& EventName, qjs::Value& EventArgs)
{
    auto it = EventListeners.find(EventName);
    if (it != EventListeners.end())
    {
        for (auto& listener : it->second)
        {
            CSP_LOG_FORMAT(csp::systems::LogLevel::Log, "Firing event '%s' with args: %s", EventName.c_str(), EventArgs.toJSON().c_str());
            JSValueConst args[] = { EventArgs.v };
            JSValue result = JS_Call(Context->ctx, listener.v, JS_UNDEFINED, 1, args);

            if (JS_IsException(result))
            {
                Context->getException(); // This will log the exception through the context's handler
            }

            JS_FreeValue(Context->ctx, result);
        }
    }
}


std::vector<ComponentScriptInterface*> EntityScriptInterface::GetComponents()
{
    std::vector<ComponentScriptInterface*> Components;

    if (Entity)
    {
        const csp::common::Map<uint16_t, ComponentBase*>& ComponentMap = *Entity->GetComponents();
        const auto ComponentKeys = ComponentMap.Keys();

        for (size_t i = 0; i < ComponentKeys->Size(); ++i)
        {
            ComponentBase* Component = ComponentMap[ComponentKeys->operator[](i)];

            if (Component->GetScriptInterface() != nullptr)
            {
                Components.push_back(Component->GetScriptInterface());
            }
        }

        delete (ComponentKeys);
    }

    return Components;
}

} // namespace csp::multiplayer
