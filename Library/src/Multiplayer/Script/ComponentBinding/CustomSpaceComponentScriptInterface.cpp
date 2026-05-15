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

#include "Multiplayer/Script/ComponentBinding/CustomSpaceComponentScriptInterface.h"

#include "CSP/Common/List.h"
#include "CSP/Multiplayer/Components/CustomSpaceComponent.h"
#include "CSP/Multiplayer/SpaceEntity.h"

using namespace csp::systems;

namespace
{

const uint8_t ValueType_Integer = 0;
const uint8_t ValueType_Float = 1;
const uint8_t ValueType_String = 2;
const uint8_t ValueType_Vector = 3;
const uint8_t ValueType_Boolean = 4;

} // namespace

namespace csp::multiplayer
{

CustomSpaceComponentScriptInterface::CustomSpaceComponentScriptInterface(CustomSpaceComponent* inComponent)
    : ComponentScriptInterface(inComponent)
{
}

uint32_t CustomSpaceComponentScriptInterface::GetCustomPropertySubscriptionKey(const std::string& key)
{
    return static_cast<CustomSpaceComponent*>(m_component)->GetCustomPropertySubscriptionKey(key.c_str());
}

bool CustomSpaceComponentScriptInterface::HasCustomProperty(const std::string& key) const
{
    return static_cast<CustomSpaceComponent*>(m_component)->HasCustomProperty(key.c_str());
}

void CustomSpaceComponentScriptInterface::RemoveCustomProperty(const std::string& key)
{
    static_cast<CustomSpaceComponent*>(m_component)->RemoveCustomProperty(key.c_str());
}

const std::variant<bool, int64_t, float, std::string, std::vector<float>> CustomSpaceComponentScriptInterface::GetCustomProperty(
    const std::string& key)
{
    csp::common::ReplicatedValue returnValue = static_cast<CustomSpaceComponent*>(m_component)->GetCustomProperty(key.c_str());

    switch (returnValue.GetReplicatedValueType())
    {
    case csp::common::ReplicatedValueType::Boolean:
        return returnValue.GetBool();
    case csp::common::ReplicatedValueType::Integer:
        return returnValue.GetInt();
    case csp::common::ReplicatedValueType::Float:
        return returnValue.GetFloat();
    case csp::common::ReplicatedValueType::String:
        return returnValue.GetString().c_str();
    case csp::common::ReplicatedValueType::Vector3:
    {
        std::vector<float> returnVector;
        returnVector = { returnValue.GetVector3().X, returnValue.GetVector3().Y, returnValue.GetVector3().Z };
        return returnVector;
    }
    case csp::common::ReplicatedValueType::Vector4:
    {
        std::vector<float> returnVector;
        returnVector = { returnValue.GetVector4().W, returnValue.GetVector4().X, returnValue.GetVector4().Y, returnValue.GetVector4().Z };
        return returnVector;
    }
    default:
        throw std::runtime_error("Unknown ReplicatedValue type!");
    }
}

std::vector<std::string> CustomSpaceComponentScriptInterface::GetCustomPropertyKeys()
{
    std::vector<std::string> returnValue;
    csp::common::List<csp::common::String> keys = static_cast<CustomSpaceComponent*>(m_component)->GetCustomPropertyKeys();

    for (size_t i = 0; i < keys.Size(); ++i)
    {
        returnValue.push_back(keys[i].c_str());
    }

    return returnValue;
}

void CustomSpaceComponentScriptInterface::SetCustomProperty(
    const std::string& key, const std::variant<int64_t, float, std::string, std::vector<float>, bool>& value)
{
    csp::common::ReplicatedValue setValue;

    switch (value.index())
    {
    case ValueType_Boolean:
        setValue.SetBool(std::get<ValueType_Boolean>(value));
        break;
    case ValueType_Integer:
        setValue.SetInt(std::get<ValueType_Integer>(value));
        break;
    case ValueType_Float:
        setValue.SetFloat(std::get<ValueType_Float>(value));
        break;
    case ValueType_String:
        setValue.SetString(std::get<ValueType_String>(value).c_str());
        break;
    case ValueType_Vector:
        if (std::get<ValueType_Vector>(value).size() == 3)
        {
            setValue.SetVector3({ std::get<ValueType_Vector>(value)[0], std::get<ValueType_Vector>(value)[1], std::get<ValueType_Vector>(value)[2] });
        }
        else
        {
            setValue.SetVector4({ std::get<ValueType_Vector>(value)[0], std::get<ValueType_Vector>(value)[1], std::get<ValueType_Vector>(value)[2],
                std::get<ValueType_Vector>(value)[3] });
        }
        break;
    }

    static_cast<CustomSpaceComponent*>(m_component)->SetCustomProperty(key.c_str(), setValue);
    SendPropertyUpdate();
}

} // namespace csp::multiplayer
