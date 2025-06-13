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
#include "CSP/Multiplayer/ComponentBase.h"

#include "CSP/Common/List.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Common/fmt_Formatters.h"
#include "CSP/Multiplayer/Script/EntityScript.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "ComponentBaseKeys.h"
#include "Multiplayer/Script/ComponentScriptInterface.h"

#include <fmt/format.h>

namespace csp::multiplayer
{

static const ReplicatedValue InvalidValue = ReplicatedValue();

ComponentBase::ComponentBase()
    : Parent(nullptr)
    , Id(0)
    , Type(ComponentType::Invalid)
    , ScriptInterface(nullptr)
    , LogSystem(nullptr)
{
    InitialiseProperties();
}

ComponentBase::ComponentBase(ComponentType Type, csp::common::LogSystem* LogSystem, SpaceEntity* Parent)
    : Parent(Parent)
    , Id(0)
    , Type(Type)
    , ScriptInterface(nullptr)
    , LogSystem(LogSystem)
{
    InitialiseProperties();
}

ComponentBase::~ComponentBase()
{
    if (ScriptInterface)
    {
        delete (ScriptInterface);
    }
}

uint16_t ComponentBase::GetId() { return Id; }

ComponentType ComponentBase::GetComponentType() const { return Type; }

const csp::common::Map<uint32_t, ReplicatedValue>* ComponentBase::GetProperties() const { return &Properties; }

const ReplicatedValue& ComponentBase::GetProperty(uint32_t Key) const
{
    if (Properties.HasKey(Key))
    {
        return Properties[Key];
    }

    if (LogSystem != nullptr)
    {
        LogSystem->LogMsg(csp::common::LogLevel::Error, fmt::format("No Property with this key: {}", Key).c_str());
    }

    return InvalidValue;
}

bool ComponentBase::GetBooleanProperty(uint32_t Key) const
{
    const auto& RepVal = GetProperty(Key);

    if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Boolean)
    {
        return RepVal.GetBool();
    }

    if (LogSystem != nullptr)
    {
        LogSystem->LogMsg(csp::common::LogLevel::Error, "Underlying ReplicatedValue not a valid Boolean type");
    }

    return false;
}

int64_t ComponentBase::GetIntegerProperty(uint32_t Key) const
{
    const auto& RepVal = GetProperty(Key);

    if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Integer)
    {
        return RepVal.GetInt();
    }

    if (LogSystem != nullptr)
    {
        LogSystem->LogMsg(csp::common::LogLevel::Error, "Underlying ReplicatedValue not a valid Integer type");
    }

    return 0;
}

float ComponentBase::GetFloatProperty(uint32_t Key) const
{
    const auto& RepVal = GetProperty(Key);

    if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Float)
    {
        return RepVal.GetFloat();
    }

    if (LogSystem != nullptr)
    {
        LogSystem->LogMsg(csp::common::LogLevel::Error, "Underlying ReplicatedValue not a valid Float type");
    }

    return 0.0f;
}

const csp::common::String& ComponentBase::GetStringProperty(uint32_t Key) const
{
    const auto& RepVal = GetProperty(Key);

    if (RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
    {
        return RepVal.GetString();
    }

    if (LogSystem != nullptr)
    {
        LogSystem->LogMsg(csp::common::LogLevel::Error, "Underlying ReplicatedValue not a valid String type");
    }

    return ReplicatedValue::GetDefaultString();
}

const csp::common::Vector2& ComponentBase::GetVector2Property(uint32_t Key) const
{
    const auto& RepVal = GetProperty(Key);

    if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector2)
    {
        return RepVal.GetVector2();
    }

    if (LogSystem != nullptr)
    {
        LogSystem->LogMsg(csp::common::LogLevel::Error, "Underlying ReplicatedValue not a valid Vector2 type");
    }

    return ReplicatedValue::GetDefaultVector2();
}

const csp::common::Vector3& ComponentBase::GetVector3Property(uint32_t Key) const
{
    const auto& RepVal = GetProperty(Key);

    if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector3)
    {
        return RepVal.GetVector3();
    }

    if (LogSystem != nullptr)
    {
        LogSystem->LogMsg(csp::common::LogLevel::Error, "Underlying ReplicatedValue not a valid Vector3 type");
    }

    return ReplicatedValue::GetDefaultVector3();
}

const csp::common::Vector4& ComponentBase::GetVector4Property(uint32_t Key) const
{
    const auto& RepVal = GetProperty(Key);

    if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector4)
    {
        return RepVal.GetVector4();
    }

    if (LogSystem != nullptr)
    {
        LogSystem->LogMsg(csp::common::LogLevel::Error, "Underlying ReplicatedValue not a valid Vector4 type");
    }

    return ReplicatedValue::GetDefaultVector4();
}

const csp::common::Map<csp::common::String, ReplicatedValue>& ComponentBase::GetStringMapProperty(uint32_t Key) const
{
    const auto& RepVal = GetProperty(Key);

    if (RepVal.GetReplicatedValueType() == ReplicatedValueType::StringMap)
    {
        return RepVal.GetStringMap();
    }

    if (LogSystem != nullptr)
    {
        LogSystem->LogMsg(csp::common::LogLevel::Error, "Underlying ReplicatedValue not a valid String Map type");
    }

    return ReplicatedValue::GetDefaultStringMap();
}

void ComponentBase::SetProperty(uint32_t Key, const ReplicatedValue& Value)
{
    if (Properties.HasKey(Key) && Value.GetReplicatedValueType() != Properties[Key].GetReplicatedValueType())
    {
        if (LogSystem != nullptr)
        {
            LogSystem->LogMsg(csp::common::LogLevel::Error,
                fmt::format("ValueType is unexpected. Expected: {0} Received: {1}", static_cast<uint32_t>(Properties[Key].GetReplicatedValueType()),
                    static_cast<uint32_t>(Value.GetReplicatedValueType()))
                    .c_str());
        }
    }

    // If the entity is not owned by us, and not a transferable entity, it is not allowed to modify the entity.
    if (!GetParent()->IsModifiable())
    {
        if (LogSystem != nullptr)
        {
            LogSystem->LogMsg(csp::common::LogLevel::Error,
                fmt::format("Error: Update attempted on a non-owned entity that is marked as non-transferable. Skipping update. Entity name: {}",
                    GetParent()->GetName())
                    .c_str());
        }

        return;
    }

    /*DirtyProperties.Remove(Key);

    if (Properties[Key] != Value)
    {
            DirtyProperties[Key] = Value;

            Parent->AddDirtyComponent(this);
    }*/

    if (!Properties.HasKey(Key) || Properties[Key] != Value)
    {
        Properties[Key] = Value;

        Parent->AddDirtyComponent(this);
        Parent->OnPropertyChanged(this, Key);
    }
}

void ComponentBase::RemoveProperty(uint32_t Key)
{
    // DirtyProperties.Remove(Key);
    Properties.Remove(Key);

    Parent->AddDirtyComponent(this);
}

void ComponentBase::SetProperties(const csp::common::Map<uint32_t, ReplicatedValue>& Value) { Properties = Value; }

void ComponentBase::SetPropertyFromPatch(uint32_t Key, const ReplicatedValue& Value) { Properties[Key] = Value; }

void ComponentBase::OnCreated() { }

void ComponentBase::OnRemove() { }

SpaceEntity* ComponentBase::GetParent() { return Parent; }

void ComponentBase::OnLocalDelete() { }

void ComponentBase::SetScriptInterface(ComponentScriptInterface* InScriptInterface) { ScriptInterface = InScriptInterface; }

ComponentScriptInterface* ComponentBase::GetScriptInterface() { return ScriptInterface; }

void ComponentBase::SubscribeToPropertyChange(uint32_t PropertyKey, csp::common::String Message)
{
    GetParent()->GetScript().SubscribeToPropertyChange(GetId(), PropertyKey, Message);
}

void ComponentBase::RegisterActionHandler(const csp::common::String& InAction, EntityActionHandler ActionHandler)
{
    if (!ActionMap.HasKey(InAction.c_str()))
    {
        ActionMap[InAction.c_str()] = ActionHandler;
    }
    else
    {
        // Already registered
        if (LogSystem != nullptr)
        {
            LogSystem->LogMsg(csp::common::LogLevel::Error, fmt::format("Action {} already registered\n", InAction).c_str());
        }
    }
}

void ComponentBase::UnregisterActionHandler(const csp::common::String& InAction)
{
    if (ActionMap.HasKey(InAction.c_str()))
    {
        ActionMap.Remove(InAction.c_str());
    }
    else
    {
        if (LogSystem != nullptr)
        {
            LogSystem->LogMsg(csp::common::LogLevel::Error, fmt::format("Action {} not found\n", InAction).c_str());
        }
    }
}

void ComponentBase::InvokeAction(const csp::common::String& InAction, const csp::common::String& InActionParams)
{
    if (ActionMap.HasKey(InAction.c_str()))
    {
        EntityActionHandler ActionHandler = ActionMap[InAction.c_str()];
        ActionHandler(this, InAction, InActionParams);
    }
}

const csp::common::String& ComponentBase::GetComponentName() const { return GetStringProperty(COMPONENT_KEY_NAME); }

void ComponentBase::SetComponentName(const csp::common::String& Value) { SetProperty(COMPONENT_KEY_NAME, Value); }

void ComponentBase::InitialiseProperties() { Properties[COMPONENT_KEY_NAME] = ""; }

} // namespace csp::multiplayer
