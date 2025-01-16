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
#include "Debug/Logging.h"

using namespace csp::systems;

namespace csp::multiplayer
{

EntityScriptInterface::EntityScriptInterface(SpaceEntity* InEntity)
    : Entity(InEntity)
{
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

int64_t EntityScriptInterface::GetParentId()
{
    if (const auto Parent = Entity->GetParentEntity())
    {
        return Parent->GetId();
    }

    return 0;
}

void EntityScriptInterface::SetParentId(int64_t ParentId)
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

SpaceEntity* EntityScriptInterface::GetParentEntity() const
{
    if (Entity)
    {
        return Entity->GetParentEntity();
    }

    return nullptr;
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

int64_t EntityScriptInterface::GetId() const { return Entity->GetId(); }

void EntityScriptInterface::SubscribeToPropertyChange(int32_t ComponentId, int32_t PropertyKey, std::string Message)
{
    Entity->GetScript()->SubscribeToPropertyChange(ComponentId, PropertyKey, Message.c_str());
}

void EntityScriptInterface::SubscribeToMessage(std::string Message, std::string MessageParamsJson)
{
    Entity->GetScript()->SubscribeToMessage(Message.c_str(), MessageParamsJson.c_str());
}

void EntityScriptInterface::PostMessageToScript(std::string Message, std::string MessageParamsJson)
{
    Entity->GetScript()->PostMessageToScript(Message.c_str(), MessageParamsJson.c_str());
}

void EntityScriptInterface::ClaimScriptOwnership() { Entity->ClaimScriptOwnership(); }

std::vector<ComponentScriptInterface*> EntityScriptInterface::GetComponents()
{
    std::vector<ComponentScriptInterface*> Components;

    if (Entity)
    {
        const csp::common::Map<uint16_t, ComponentBase*>& ComponentMap = *Entity->GetComponents();
        const auto ComponentKeys = ComponentMap.Keys();

        for (int i = 0; i < ComponentKeys->Size(); ++i)
        {
            ComponentBase* Component = ComponentMap[ComponentKeys->operator[](i)];

            if (Component->GetScriptInterface() != nullptr)
            {
                Components.push_back(Component->GetScriptInterface());
            }
        }

        CSP_DELETE(ComponentKeys);
    }

    return Components;
}

} // namespace csp::multiplayer
