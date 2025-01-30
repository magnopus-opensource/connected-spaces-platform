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
#include "Debug/Logging.h"

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

CustomSpaceComponentScriptInterface::CustomSpaceComponentScriptInterface(CustomSpaceComponent* InComponent)
    : ComponentScriptInterface(InComponent)
{
}

uint32_t CustomSpaceComponentScriptInterface::GetCustomPropertySubscriptionKey(const std::string& Key)
{
    return static_cast<CustomSpaceComponent*>(Component)->GetCustomPropertySubscriptionKey(Key.c_str());
}

bool CustomSpaceComponentScriptInterface::HasCustomProperty(const std::string& Key) const
{
    return static_cast<CustomSpaceComponent*>(Component)->HasCustomProperty(Key.c_str());
}

void CustomSpaceComponentScriptInterface::RemoveCustomProperty(const std::string& Key)
{
    static_cast<CustomSpaceComponent*>(Component)->RemoveCustomProperty(Key.c_str());
}

const std::variant<bool, int64_t, float, std::string, std::vector<float>> CustomSpaceComponentScriptInterface::GetCustomProperty(
    const std::string& Key)
{
    ReplicatedValue ReturnValue = static_cast<CustomSpaceComponent*>(Component)->GetCustomProperty(Key.c_str());

    switch (ReturnValue.GetReplicatedValueType())
    {
    case ReplicatedValueType::Boolean:
        return ReturnValue.GetBool();
    case ReplicatedValueType::Integer:
        return ReturnValue.GetInt();
    case ReplicatedValueType::Float:
        return ReturnValue.GetFloat();
    case ReplicatedValueType::String:
        return ReturnValue.GetString().c_str();
    case ReplicatedValueType::Vector3:
    {
        std::vector<float> ReturnVector;
        ReturnVector = { ReturnValue.GetVector3().X, ReturnValue.GetVector3().Y, ReturnValue.GetVector3().Z };
        return ReturnVector;
    }
    case ReplicatedValueType::Vector4:
    {
        std::vector<float> ReturnVector;
        ReturnVector = { ReturnValue.GetVector4().W, ReturnValue.GetVector4().X, ReturnValue.GetVector4().Y, ReturnValue.GetVector4().Z };
        return ReturnVector;
    }
    default:
        throw std::runtime_error("Unknown ReplicatedValue type!");
    }
}

std::vector<std::string> CustomSpaceComponentScriptInterface::GetCustomPropertyKeys()
{
    std::vector<std::string> ReturnValue;
    csp::common::List<csp::common::String> Keys = static_cast<CustomSpaceComponent*>(Component)->GetCustomPropertyKeys();

    for (int i = 0; i < Keys.Size(); ++i)
    {
        ReturnValue.push_back(Keys[i].c_str());
    }

    return ReturnValue;
}

void CustomSpaceComponentScriptInterface::SetCustomProperty(
    const std::string& Key, const std::variant<int64_t, float, std::string, std::vector<float>, bool>& Value)
{
    ReplicatedValue SetValue;

    switch (Value.index())
    {
    case ValueType_Boolean:
        SetValue.SetBool(std::get<ValueType_Boolean>(Value));
        break;
    case ValueType_Integer:
        SetValue.SetInt(std::get<ValueType_Integer>(Value));
        break;
    case ValueType_Float:
        SetValue.SetFloat(std::get<ValueType_Float>(Value));
        break;
    case ValueType_String:
        SetValue.SetString(std::get<ValueType_String>(Value).c_str());
        break;
    case ValueType_Vector:
        if (std::get<ValueType_Vector>(Value).size() == 3)
        {
            SetValue.SetVector3({ std::get<ValueType_Vector>(Value)[0], std::get<ValueType_Vector>(Value)[1], std::get<ValueType_Vector>(Value)[2] });
        }
        else
        {
            SetValue.SetVector4({ std::get<ValueType_Vector>(Value)[0], std::get<ValueType_Vector>(Value)[1], std::get<ValueType_Vector>(Value)[2],
                std::get<ValueType_Vector>(Value)[3] });
        }
        break;
    }

    static_cast<CustomSpaceComponent*>(Component)->SetCustomProperty(Key.c_str(), SetValue);
    SendPropertyUpdate();
}

DEFINE_SCRIPT_PROPERTY_STRING(CustomSpaceComponent, ApplicationOrigin);

} // namespace csp::multiplayer
