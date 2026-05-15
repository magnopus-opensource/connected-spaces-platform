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

EntityScriptInterface::EntityScriptInterface(SpaceEntity* inEntity)
    : m_entity(inEntity)
{
}

EntityScriptInterface::Vector3 EntityScriptInterface::GetPosition() const
{
    EntityScriptInterface::Vector3 pos = { 0, 0, 0 };

    if (m_entity)
    {
        const csp::common::Vector3& position = m_entity->GetPosition();

        pos[0] = position.X;
        pos[1] = position.Y;
        pos[2] = position.Z;
    }

    return pos;
}

void EntityScriptInterface::SetPosition(EntityScriptInterface::Vector3 pos)
{
    // CSP_LOG_FORMAT(LogLevel::VeryVerbose, "EntityScriptWrapper::SetPosition { %.2f, %.2f, %.2f }\n", Pos[0], Pos[1], Pos[2]);

    const csp::common::Vector3 newPosition(pos[0], pos[1], pos[2]);
    const csp::common::Vector3& currentPosition = m_entity->GetPosition();

    if (currentPosition != newPosition)
    {
        m_entity->SetPosition(newPosition);
        m_entity->QueueUpdate();
    }
}

EntityScriptInterface::Vector3 EntityScriptInterface::GetGlobalPosition() const
{
    EntityScriptInterface::Vector3 globalPos = { 0, 0, 0 };

    if (m_entity)
    {
        const csp::common::Vector3& position = m_entity->GetGlobalPosition();

        globalPos[0] = position.X;
        globalPos[1] = position.Y;
        globalPos[2] = position.Z;
    }

    return globalPos;
}

EntityScriptInterface::Vector4 EntityScriptInterface::GetRotation() const
{
    EntityScriptInterface::Vector4 rot = { 0, 0, 0, 0 };

    if (m_entity)
    {
        csp::common::Vector4 rotation = m_entity->GetRotation();

        rot[0] = rotation.X;
        rot[1] = rotation.Y;
        rot[2] = rotation.Z;
        rot[3] = rotation.W;
    }

    return rot;
}

void EntityScriptInterface::SetRotation(EntityScriptInterface::Vector4 rot)
{

    const csp::common::Vector4 newRotation(rot[0], rot[1], rot[2], rot[3]);
    const csp::common::Vector4& currentRotation = m_entity->GetRotation();

    if (currentRotation != newRotation)
    {
        m_entity->SetRotation(newRotation);
        m_entity->QueueUpdate();
    }
}

EntityScriptInterface::Vector4 EntityScriptInterface::GetGlobalRotation() const
{
    EntityScriptInterface::Vector4 globalRot = { 0, 0, 0, 0 };

    if (m_entity)
    {
        csp::common::Vector4 rotation = m_entity->GetGlobalRotation();

        globalRot[0] = rotation.X;
        globalRot[1] = rotation.Y;
        globalRot[2] = rotation.Z;
        globalRot[3] = rotation.W;
    }

    return globalRot;
}

int64_t EntityScriptInterface::GetParentId()
{
    if (const auto parent = m_entity->GetParentEntity())
    {
        return parent->GetId();
    }

    return 0;
}

void EntityScriptInterface::SetParentId(int64_t parentId)
{
    if (m_entity)
    {
        m_entity->SetParentId(parentId);
    }
}

void EntityScriptInterface::RemoveParentEntity()
{
    if (m_entity)
    {
        m_entity->RemoveParentEntity();
    }
}

SpaceEntity* EntityScriptInterface::GetParentEntity() const
{
    if (m_entity)
    {
        return m_entity->GetParentEntity();
    }

    return nullptr;
}

EntityScriptInterface::Vector3 EntityScriptInterface::GetScale() const
{
    EntityScriptInterface::Vector3 scale = { 0, 0, 0 };

    if (m_entity)
    {
        csp::common::Vector3 entityScale = m_entity->GetScale();

        scale[0] = entityScale.X;
        scale[1] = entityScale.Y;
        scale[2] = entityScale.Z;
    }

    return scale;
}

void EntityScriptInterface::SetScale(EntityScriptInterface::Vector3 scale)
{
    const csp::common::Vector3 newScale(scale[0], scale[1], scale[2]);
    const csp::common::Vector3& currentScale = m_entity->GetScale();

    if (currentScale != newScale)
    {
        m_entity->SetScale(newScale);
        m_entity->QueueUpdate();
    }
}

EntityScriptInterface::Vector3 EntityScriptInterface::GetGlobalScale() const
{
    EntityScriptInterface::Vector3 globalScale = { 0, 0, 0 };

    if (m_entity)
    {
        csp::common::Vector3 entityScale = m_entity->GetGlobalScale();

        globalScale[0] = entityScale.X;
        globalScale[1] = entityScale.Y;
        globalScale[2] = entityScale.Z;
    }

    return globalScale;
}

const std::string EntityScriptInterface::GetName() const { return m_entity->GetName().c_str(); }

int64_t EntityScriptInterface::GetId() const { return m_entity->GetId(); }

void EntityScriptInterface::SubscribeToPropertyChange(int32_t componentId, int32_t propertyKey, std::string message)
{
    m_entity->GetScript().SubscribeToPropertyChange(componentId, propertyKey, message.c_str());
}

void EntityScriptInterface::SubscribeToMessage(std::string message, std::string messageParamsJson)
{
    m_entity->GetScript().SubscribeToMessage(message.c_str(), messageParamsJson.c_str());
}

void EntityScriptInterface::PostMessageToScript(std::string message, std::string messageParamsJson)
{
    m_entity->GetScript().PostMessageToScript(message.c_str(), messageParamsJson.c_str());
}

void EntityScriptInterface::ClaimScriptOwnership() { m_entity->ClaimScriptOwnership(); }

std::vector<ComponentScriptInterface*> EntityScriptInterface::GetComponents()
{
    std::vector<ComponentScriptInterface*> components;

    if (m_entity)
    {
        const csp::common::Map<uint16_t, ComponentBase*>& componentMap = *m_entity->GetComponents();
        const auto componentKeys = componentMap.Keys();

        for (size_t i = 0; i < componentKeys->Size(); ++i)
        {
            ComponentBase* component = componentMap[componentKeys->operator[](i)];

            if (component->GetScriptInterface() != nullptr)
            {
                components.push_back(component->GetScriptInterface());
            }
        }

        delete (componentKeys);
    }

    return components;
}

} // namespace csp::multiplayer
