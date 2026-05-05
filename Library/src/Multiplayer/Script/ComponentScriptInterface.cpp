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

ComponentScriptInterface::ComponentScriptInterface(ComponentBase* InComponenty)
    : Component(InComponenty)
{
}

void ComponentScriptInterface::SubscribeToPropertyChange(int32_t PropertyKey, std::string Message)
{
    if (Component)
    {
        Component->SubscribeToPropertyChange(PropertyKey, Message.c_str());
    }
}

void ComponentScriptInterface::InvokeAction(std::string ActionId, std::string ActionParams)
{
    if (Component)
    {
        Component->InvokeAction(ActionId.c_str(), ActionParams.c_str());
    }
}

int64_t ComponentScriptInterface::GetComponentId() const
{
    if (Component)
    {
        return Component->GetId();
    }

    return INVALID_COMPONENT_ID;
}

int64_t ComponentScriptInterface::GetComponentType() const
{
    if (Component)
    {
        return (int64_t)Component->GetComponentType();
    }

    return (int64_t)ComponentType::Invalid;
}

void ComponentScriptInterface::SetComponentName(std::string name)
{
    if (Component)
    {
        Component->SetComponentName(name.c_str());
    }
}

std::string ComponentScriptInterface::GetComponentName() const
{
    if (Component)
    {
        return Component->GetComponentName().c_str();
    }

    return "";
}

void ComponentScriptInterface::SendPropertyUpdate()
{
    if (Component)
    {
        Component->GetParent()->QueueUpdate();
    }
}

std::optional<ComponentScriptInterface::Value> ComponentScriptInterface::GetProperty(uint16_t Key) const
{
    if (!Component)
    {
        return {};
    }

    const auto& MaybeValue = Component->GetProperty(Key);
    if (MaybeValue.GetReplicatedValueType() == csp::common::ReplicatedValueType::InvalidType)
    {
        return std::nullopt;
    }

    struct Visitor final
    {
        std::optional<Value> operator()(bool Value) const
        {
            return Value;
        }

        std::optional<Value> operator()(float Value) const
        {
            return Value;
        }

        std::optional<Value> operator()(int64_t Value) const
        {
            return Value;
        }

        std::optional<Value> operator()(const csp::common::String& Value) const
        {
            return std::string(Value.c_str());
        }

        std::optional<Value> operator()(const csp::common::Vector2& Value) const
        {
            return std::vector<float>{Value.X, Value.Y};
        }

        std::optional<Value> operator()(const csp::common::Vector3& Value) const
        {
            return std::vector<float>{Value.X, Value.Y, Value.Z};
        }

        std::optional<Value> operator()(const csp::common::Vector4& Value) const
        {
            return std::vector<float>{Value.X, Value.Y, Value.Z, Value.W};
        }
        
        std::optional<Value> operator()(const csp::common::Map<csp::common::String, csp::common::ReplicatedValue>&) const
        {
            return {};
        }
    };

    return std::visit(Visitor{}, MaybeValue.GetValue());
}

void ComponentScriptInterface::SetProperty(uint16_t Key, Value DesiredValue)
{
    if (!Component)
    {
        return;
    }

    struct Visitor final
    {
        std::optional<csp::common::ReplicatedValue> operator()(bool Value) const
        {
            return Value;
        }

        std::optional<csp::common::ReplicatedValue> operator()(float Value) const
        {
            return Value;
        }

        std::optional<csp::common::ReplicatedValue> operator()(int64_t Value) const
        {
            return Value;
        }

        std::optional<csp::common::ReplicatedValue> operator()(const std::string& Value) const
        {
            return csp::common::String(Value.c_str());
        }

        std::optional<csp::common::ReplicatedValue> operator()(const std::vector<float>& Value) const
        {
            if (Value.size() == 2)
            {
                return csp::common::Vector2{Value[0], Value[1]};
            }

            if (Value.size() == 3)
            {
                return csp::common::Vector3{Value[0], Value[1], Value[2]};
            }

            if (Value.size() == 4)
            {
                return csp::common::Vector4{Value[0], Value[1], Value[2], Value[3]};
            }
 
            return {};
        }
    };

    if (auto MaybeMappedValue = std::visit(Visitor{}, std::move(DesiredValue)))
    {
        Component->SetProperty(Key, std::move(*MaybeMappedValue));
        SendPropertyUpdate();
    }
}

} // namespace csp::multiplayer
