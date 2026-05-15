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
#include "Multiplayer/Script/ComponentScriptInterface.h"

#include "CSP/Multiplayer/SpaceEntity.h"

namespace csp::multiplayer
{

ComponentScriptInterface::ComponentScriptInterface(ComponentBase* inComponenty)
    : m_component(inComponenty)
{
}

void ComponentScriptInterface::SubscribeToPropertyChange(int32_t propertyKey, std::string message)
{
    if (m_component)
    {
        m_component->SubscribeToPropertyChange(propertyKey, message.c_str());
    }
}

void ComponentScriptInterface::InvokeAction(std::string actionId, std::string actionParams)
{
    if (m_component)
    {
        m_component->InvokeAction(actionId.c_str(), actionParams.c_str());
    }
}

int64_t ComponentScriptInterface::GetComponentId() const
{
    if (m_component)
    {
        return m_component->GetId();
    }

    return INVALID_COMPONENT_ID;
}

int64_t ComponentScriptInterface::GetComponentType() const
{
    if (m_component)
    {
        return (int64_t)m_component->GetComponentType();
    }

    return (int64_t)ComponentType::Invalid;
}

void ComponentScriptInterface::SetComponentName(std::string name)
{
    if (m_component)
    {
        m_component->SetComponentName(name.c_str());
    }
}

std::string ComponentScriptInterface::GetComponentName() const
{
    if (m_component)
    {
        return m_component->GetComponentName().c_str();
    }

    return "";
}

void ComponentScriptInterface::SendPropertyUpdate()
{
    if (m_component)
    {
        m_component->GetParent()->QueueUpdate();
    }
}

std::optional<ComponentScriptInterface::Value> ComponentScriptInterface::GetProperty(uint16_t key) const
{
    if (!m_component)
    {
        return {};
    }

    const auto& maybeValue = m_component->GetProperty(key);
    if (maybeValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::InvalidType)
    {
        return std::nullopt;
    }

    struct Visitor final
    {
        std::optional<Value> operator()(bool value) const
        {
            return value;
        }

        std::optional<Value> operator()(float value) const
        {
            return value;
        }

        std::optional<Value> operator()(int64_t value) const
        {
            return value;
        }

        std::optional<Value> operator()(const csp::common::String& value) const
        {
            return std::string(value.c_str());
        }

        std::optional<Value> operator()(const csp::common::Vector2& value) const
        {
            return std::vector<float>{value.X, value.Y};
        }

        std::optional<Value> operator()(const csp::common::Vector3& value) const
        {
            return std::vector<float>{value.X, value.Y, value.Z};
        }

        std::optional<Value> operator()(const csp::common::Vector4& value) const
        {
            return std::vector<float>{value.X, value.Y, value.Z, value.W};
        }
        
        std::optional<Value> operator()(const csp::common::Map<csp::common::String, csp::common::ReplicatedValue>&) const
        {
            return {};
        }
    };

    return std::visit(Visitor{}, maybeValue.GetValue());
}

void ComponentScriptInterface::SetProperty(uint16_t key, Value desiredValue)
{
    if (!m_component)
    {
        return;
    }

    struct Visitor final
    {
        std::optional<csp::common::ReplicatedValue> operator()(bool value) const
        {
            return value;
        }

        std::optional<csp::common::ReplicatedValue> operator()(float value) const
        {
            return value;
        }

        std::optional<csp::common::ReplicatedValue> operator()(int64_t value) const
        {
            return value;
        }

        std::optional<csp::common::ReplicatedValue> operator()(const std::string& value) const
        {
            return csp::common::String(value.c_str());
        }

        std::optional<csp::common::ReplicatedValue> operator()(const std::vector<float>& value) const
        {
            if (value.size() == 2)
            {
                return csp::common::Vector2{value[0], value[1]};
            }

            if (value.size() == 3)
            {
                return csp::common::Vector3{value[0], value[1], value[2]};
            }

            if (value.size() == 4)
            {
                return csp::common::Vector4{value[0], value[1], value[2], value[3]};
            }
 
            return {};
        }
    };

    if (auto maybeMappedValue = std::visit(Visitor{}, std::move(desiredValue)))
    {
        m_component->SetProperty(key, std::move(*maybeMappedValue));
        SendPropertyUpdate();
    }
}

} // namespace csp::multiplayer
