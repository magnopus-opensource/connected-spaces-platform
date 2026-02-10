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

#include "Multiplayer/Script/ComponentBinding/ComponentExtensionsScriptInterface.h"

#include "CSP/Common/List.h"
#include "CSP/Multiplayer/ComponentExtensions.h"
#include "CSP/Multiplayer/SpaceEntity.h"

using namespace csp::systems;

namespace
{

// Lifted from CustomSpaceComponentScriptInterface for consistency with how we're handling variant types in our script interfaces.
const uint8_t ValueType_Integer = 0;
const uint8_t ValueType_Float = 1;
const uint8_t ValueType_String = 2;
const uint8_t ValueType_Vector = 3;
const uint8_t ValueType_Boolean = 4;

} // namespace

namespace csp::multiplayer
{

ComponentExtensionsScriptInterface::ComponentExtensionsScriptInterface(ComponentExtensions* InExtensions)
    : Extensions(InExtensions)
{
}

const std::variant<bool, int64_t, float, std::string, std::vector<float>> ComponentExtensionsScriptInterface::GetProperty(
    const std::string& Key)
{
    csp::common::ReplicatedValue ReturnValue = Extensions->GetProperty(Key.c_str());

    switch (ReturnValue.GetReplicatedValueType())
    {
    case csp::common::ReplicatedValueType::Boolean:
        return ReturnValue.GetBool();
    case csp::common::ReplicatedValueType::Integer:
        return ReturnValue.GetInt();
    case csp::common::ReplicatedValueType::Float:
        return ReturnValue.GetFloat();
    case csp::common::ReplicatedValueType::String:
        return ReturnValue.GetString().c_str();
    case csp::common::ReplicatedValueType::Vector3:
    {
        std::vector<float> ReturnVector;
        ReturnVector = { ReturnValue.GetVector3().X, ReturnValue.GetVector3().Y, ReturnValue.GetVector3().Z };
        return ReturnVector;
    }
    case csp::common::ReplicatedValueType::Vector4:
    {
        std::vector<float> ReturnVector;
        ReturnVector = { ReturnValue.GetVector4().W, ReturnValue.GetVector4().X, ReturnValue.GetVector4().Y, ReturnValue.GetVector4().Z };
        return ReturnVector;
    }
    default:
        throw std::runtime_error("Unknown ReplicatedValue type!");
    }
}

void ComponentExtensionsScriptInterface::SetProperty(
    const std::string& Key, const std::variant<int64_t, float, std::string, std::vector<float>, bool>& Value)
{
    csp::common::ReplicatedValue SetValue;

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

    Extensions->SetProperty(Key.c_str(), SetValue);

    // Trigger an entity update/replication (this is consistent with how component property updates work in general).
    csp::multiplayer::ComponentBase* Component = Extensions->GetExtendedComponent();
    if (Component != nullptr)
    {
        Component->GetParent()->QueueUpdate();
    }
}

bool ComponentExtensionsScriptInterface::HasProperty(const std::string& Key) const
{
    return Extensions->HasProperty(Key.c_str());
}

} // namespace csp::multiplayer
