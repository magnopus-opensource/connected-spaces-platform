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
#include "CSP/Multiplayer/ComponentSchema.h"
#include "CSP/Multiplayer/Script/EntityScript.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "ComponentBaseKeys.h"
#include "Multiplayer/RealtimeEngineUtils.h"
#include "Multiplayer/Script/ComponentScriptHelpers.h"
#include "Multiplayer/Script/ComponentScriptInterface.h"

#include <fmt/format.h>

namespace csp::multiplayer
{

static const csp::common::ReplicatedValue InvalidValue = csp::common::ReplicatedValue();

ComponentBase::ComponentBase()
    : m_parent(nullptr)
    , m_id(0)
    , m_type(ComponentType::Invalid)
    , m_scriptInterface(nullptr)
    , m_logSystem(nullptr)
{
    InitialiseProperties();
}

ComponentBase::ComponentBase(ComponentType type, csp::common::LogSystem* logSystem, SpaceEntity* parent)
    : m_parent(parent)
    , m_id(0)
    , m_type(type)
    , m_scriptInterface(nullptr)
    , m_logSystem(logSystem)
{
    InitialiseProperties();
}

ComponentBase::ComponentBase(const ComponentSchema& schema, csp::common::LogSystem* logSystem, SpaceEntity* parent)
    : ComponentBase(static_cast<ComponentType>(schema.TypeId), logSystem, parent)
{
    for (const auto& property : schema.Properties)
    {
        m_properties[property.Key] = property.DefaultValue;
    }

    if (IsScriptable(schema))
    {
        m_scriptInterface = std::make_unique<ComponentScriptInterface>(this);
    }
}

ComponentBase::~ComponentBase() { }

uint16_t ComponentBase::GetId() const { return m_id; }

void ComponentBase::SetId(uint16_t newId) { this->m_id = newId; }

ComponentType ComponentBase::GetComponentType() const { return m_type; }

const csp::common::Map<uint32_t, csp::common::ReplicatedValue>* ComponentBase::GetProperties() const { return &m_properties; }

const csp::common::ReplicatedValue& ComponentBase::GetProperty(uint32_t key) const
{
    if (m_properties.HasKey(key))
    {
        return m_properties[key];
    }

    if (m_logSystem != nullptr)
    {
        m_logSystem->LogMsg(csp::common::LogLevel::Error, fmt::format("No Property with this key: {}", key).c_str());
    }

    return InvalidValue;
}

bool ComponentBase::GetBooleanProperty(uint32_t key) const
{
    const auto& repVal = GetProperty(key);

    if (repVal.GetReplicatedValueType() == csp::common::ReplicatedValueType::Boolean)
    {
        return repVal.GetBool();
    }

    if (m_logSystem != nullptr)
    {
        m_logSystem->LogMsg(csp::common::LogLevel::Error, "Underlying csp::common::ReplicatedValue not a valid Boolean type");
    }

    return false;
}

int64_t ComponentBase::GetIntegerProperty(uint32_t key) const
{
    const auto& repVal = GetProperty(key);

    if (repVal.GetReplicatedValueType() == csp::common::ReplicatedValueType::Integer)
    {
        return repVal.GetInt();
    }

    if (m_logSystem != nullptr)
    {
        m_logSystem->LogMsg(csp::common::LogLevel::Error, "Underlying csp::common::ReplicatedValue not a valid Integer type");
    }

    return 0;
}

float ComponentBase::GetFloatProperty(uint32_t key) const
{
    const auto& repVal = GetProperty(key);

    if (repVal.GetReplicatedValueType() == csp::common::ReplicatedValueType::Float)
    {
        return repVal.GetFloat();
    }

    if (m_logSystem != nullptr)
    {
        m_logSystem->LogMsg(csp::common::LogLevel::Error, "Underlying csp::common::ReplicatedValue not a valid Float type");
    }

    return 0.0f;
}

const csp::common::String& ComponentBase::GetStringProperty(uint32_t key) const
{
    const auto& repVal = GetProperty(key);

    if (repVal.GetReplicatedValueType() == csp::common::ReplicatedValueType::String)
    {
        return repVal.GetString();
    }

    if (m_logSystem != nullptr)
    {
        m_logSystem->LogMsg(csp::common::LogLevel::Error, "Underlying csp::common::ReplicatedValue not a valid String type");
    }

    return csp::common::ReplicatedValue::GetDefaultString();
}

const csp::common::Vector2& ComponentBase::GetVector2Property(uint32_t key) const
{
    const auto& repVal = GetProperty(key);

    if (repVal.GetReplicatedValueType() == csp::common::ReplicatedValueType::Vector2)
    {
        return repVal.GetVector2();
    }

    if (m_logSystem != nullptr)
    {
        m_logSystem->LogMsg(csp::common::LogLevel::Error, "Underlying csp::common::ReplicatedValue not a valid Vector2 type");
    }

    return csp::common::ReplicatedValue::GetDefaultVector2();
}

const csp::common::Vector3& ComponentBase::GetVector3Property(uint32_t key) const
{
    const auto& repVal = GetProperty(key);

    if (repVal.GetReplicatedValueType() == csp::common::ReplicatedValueType::Vector3)
    {
        return repVal.GetVector3();
    }

    if (m_logSystem != nullptr)
    {
        m_logSystem->LogMsg(csp::common::LogLevel::Error, "Underlying csp::common::ReplicatedValue not a valid Vector3 type");
    }

    return csp::common::ReplicatedValue::GetDefaultVector3();
}

const csp::common::Vector4& ComponentBase::GetVector4Property(uint32_t key) const
{
    const auto& repVal = GetProperty(key);

    if (repVal.GetReplicatedValueType() == csp::common::ReplicatedValueType::Vector4)
    {
        return repVal.GetVector4();
    }

    if (m_logSystem != nullptr)
    {
        m_logSystem->LogMsg(csp::common::LogLevel::Error, "Underlying csp::common::ReplicatedValue not a valid Vector4 type");
    }

    return csp::common::ReplicatedValue::GetDefaultVector4();
}

const csp::common::Map<csp::common::String, csp::common::ReplicatedValue>& ComponentBase::GetStringMapProperty(uint32_t key) const
{
    const auto& repVal = GetProperty(key);

    if (repVal.GetReplicatedValueType() == csp::common::ReplicatedValueType::StringMap)
    {
        return repVal.GetStringMap();
    }

    if (m_logSystem != nullptr)
    {
        m_logSystem->LogMsg(csp::common::LogLevel::Error, "Underlying csp::common::ReplicatedValue not a valid String Map type");
    }

    return csp::common::ReplicatedValue::GetDefaultStringMap();
}

void ComponentBase::SetProperty(uint32_t key, const csp::common::ReplicatedValue& value)
{
    if (m_properties.HasKey(key) && value.GetReplicatedValueType() != m_properties[key].GetReplicatedValueType())
    {
        if (m_logSystem != nullptr)
        {
            m_logSystem->LogMsg(csp::common::LogLevel::Error,
                fmt::format("ValueType is unexpected. Expected: {0} Received: {1}", static_cast<uint32_t>(m_properties[key].GetReplicatedValueType()),
                    static_cast<uint32_t>(value.GetReplicatedValueType()))
                    .c_str());
        }
    }

    // Ensure we can modify the entity. The criteria for this can be found on the specific RealtimeEngine::IsEntityModifiable overloads.
    ModifiableStatus modifiable = GetParent()->IsModifiable();
    if (modifiable != ModifiableStatus::Modifiable)
    {
        if (m_logSystem != nullptr)
        {
            m_logSystem->LogMsg(csp::common::LogLevel::Warning,
                fmt::format("Failed to set property on component: {0}, skipping update. Entity name: {1}",
                    RealtimeEngineUtils::ModifiableStatusToString(modifiable), GetParent()->GetName())
                    .c_str());
        }

        return;
    }

    if (!m_properties.HasKey(key) || m_properties[key] != value)
    {
        // Weird that this is instant and dosen't go through the regular lock/patch flow
        // I think it should ... note the lock above which every other SpaceEntity thing has in its setter methods.
        // This is the _one_ thing that in online mode, dosen't need ProcessPending() to be called ... _Weird_.
        // Note how `UpdateComponent` dosen't actually set the data, just does notification in this case. :(
        // TODO, fix. Look at `SetPropertyFromPatch` below, it's basically a `SetPropetyDirect`
        m_properties[key] = value;
        m_parent->UpdateComponent(this);

        // Hack alert
        // So, this is for the case where we already have a dirty component of update type ADD pending, but we've just updated it.
        // It just so happens that the ADD is enough to replicate the component well enough, as anything will do, BUT, we still
        // need to notify the scripting system here that a property has changed.
        m_parent->OnPropertyChanged(this, key);
    }
}

void ComponentBase::RemoveProperty(uint32_t key)
{
    // Weird that this is instant and dosen't go through the regular lock/patch flow
    m_properties.Remove(key);
    m_parent->UpdateComponent(this);
}

void ComponentBase::SetProperties(const csp::common::Map<uint32_t, csp::common::ReplicatedValue>& value) { m_properties = value; }

void ComponentBase::SetPropertyFromPatch(uint32_t key, const csp::common::ReplicatedValue& value) { m_properties[key] = value; }

void ComponentBase::OnCreated() { }

void ComponentBase::OnRemove() { }

SpaceEntity* ComponentBase::GetParent() { return m_parent; }

void ComponentBase::OnLocalDelete() { }

void ComponentBase::SetScriptInterface(ComponentScriptInterface* inScriptInterface) { m_scriptInterface.reset(inScriptInterface); }

ComponentScriptInterface* ComponentBase::GetScriptInterface() { return m_scriptInterface.get(); }

void ComponentBase::SubscribeToPropertyChange(uint32_t propertyKey, csp::common::String message)
{
    GetParent()->GetScript().SubscribeToPropertyChange(GetId(), propertyKey, message);
}

void ComponentBase::RegisterActionHandler(const csp::common::String& inAction, EntityActionHandler actionHandler)
{
    if (!m_actionMap.HasKey(inAction.c_str()))
    {
        m_actionMap[inAction.c_str()] = actionHandler;
    }
    else
    {
        // Already registered
        if (m_logSystem != nullptr)
        {
            m_logSystem->LogMsg(csp::common::LogLevel::Error, fmt::format("Action {} already registered\n", inAction).c_str());
        }
    }
}

void ComponentBase::UnregisterActionHandler(const csp::common::String& inAction)
{
    if (m_actionMap.HasKey(inAction.c_str()))
    {
        m_actionMap.Remove(inAction.c_str());
    }
    else
    {
        if (m_logSystem != nullptr)
        {
            m_logSystem->LogMsg(csp::common::LogLevel::Error, fmt::format("Action {} not found\n", inAction).c_str());
        }
    }
}

void ComponentBase::InvokeAction(const csp::common::String& inAction, const csp::common::String& inActionParams)
{
    if (m_actionMap.HasKey(inAction.c_str()))
    {
        EntityActionHandler actionHandler = m_actionMap[inAction.c_str()];
        actionHandler(this, inAction, inActionParams);
    }
}

const csp::common::String& ComponentBase::GetComponentName() const { return GetStringProperty(COMPONENT_KEY_NAME); }

void ComponentBase::SetComponentName(const csp::common::String& value) { SetProperty(COMPONENT_KEY_NAME, value); }

void ComponentBase::InitialiseProperties() { m_properties[COMPONENT_KEY_NAME] = ""; }

} // namespace csp::multiplayer
