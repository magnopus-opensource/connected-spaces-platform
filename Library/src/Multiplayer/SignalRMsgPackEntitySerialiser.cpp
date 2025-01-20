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
#include "SignalRMsgPackEntitySerialiser.h"

#include "Memory/Memory.h"
#include "Multiplayer/SpaceEntityKeys.h"
#include "MultiplayerConstants.h"

#include <Debug/Logging.h>
#include <cassert>

namespace csp::multiplayer
{
using namespace msgpack_typeids;

namespace
{
    std::pair<ItemComponentData, signalr::value> ReplicatedValueToSignalRValue(const ReplicatedValue& Value)
    {
        ItemComponentData ValueType;
        signalr::value NewValue;

        switch (Value.GetReplicatedValueType())
        {
        case ReplicatedValueType::Boolean:
            ValueType = ItemComponentData::BOOL;
            NewValue = signalr::value(Value.GetBool());
            break;
        case ReplicatedValueType::Integer:
            ValueType = ItemComponentData::INT64;
            NewValue = signalr::value(Value.GetInt());
            break;
        case ReplicatedValueType::Float:
            ValueType = ItemComponentData::FLOAT;
            NewValue = signalr::value(Value.GetFloat());
            break;
        case ReplicatedValueType::String:
            ValueType = ItemComponentData::STRING;
            NewValue = signalr::value(Value.GetString().c_str(), Value.GetString().Length());
            break;
        case ReplicatedValueType::Vector2:
        {
            ValueType = ItemComponentData::FLOAT_ARRAY;
            auto VValue = Value.GetVector2();
            NewValue = signalr::value(std::vector { signalr::value(VValue.X), signalr::value(VValue.Y) });
            break;
        }
        case ReplicatedValueType::Vector3:
        {
            ValueType = ItemComponentData::FLOAT_ARRAY;
            auto VValue = Value.GetVector3();
            NewValue = signalr::value(std::vector { signalr::value(VValue.X), signalr::value(VValue.Y), signalr::value(VValue.Z) });
            break;
        }
        case ReplicatedValueType::Vector4:
        {
            ValueType = ItemComponentData::FLOAT_ARRAY;
            auto VValue = Value.GetVector4();
            NewValue = signalr::value(
                std::vector { signalr::value(VValue.X), signalr::value(VValue.Y), signalr::value(VValue.Z), signalr::value(VValue.W) });
            break;
        }
        case ReplicatedValueType::StringMap:
        {
            ValueType = ItemComponentData::STRING_DICTIONARY;
            auto MValue = Value.GetStringMap();

            auto Deleter = [](const common::Array<common::String>* Ptr) { CSP_DELETE(Ptr); };

            std::map<std::string, signalr::value> Map;
            std::unique_ptr<common::Array<common::String>, decltype(Deleter)> Keys(
                const_cast<common::Array<common::String>*>(MValue.Keys()), Deleter);

            for (size_t i = 0; i < Keys->Size(); ++i)
            {
                auto ValuePair = ReplicatedValueToSignalRValue(MValue[(*Keys)[i]]);

                std::vector<signalr::value> Item = { signalr::value(ValuePair.second) };
                std::vector<signalr::value> Prop = { ValuePair.first, signalr::value(Item) };

                Map[(*Keys)[i].c_str()] = signalr::value(Prop);
            }

            NewValue = signalr::value(Map);

            break;
        }
        case ReplicatedValueType::InvalidType:
            CSP_LOG_ERROR_MSG("Received Invalid Type as SignalR Replicated Value");
            return std::make_pair(ValueType, signalr::value {});
        }

        return std::make_pair(ValueType, NewValue);
    }

    ReplicatedValue SignalRValueToReplicatedValue(ItemComponentData Type, const signalr::value& Value)
    {
        switch (Type)
        {
        case ItemComponentData::BOOL:
            return Value.as_bool();
        case ItemComponentData::INT64:
            return (Value.is_integer()) ? ReplicatedValue(Value.as_integer()) : ReplicatedValue((int64_t)Value.as_uinteger());
        case ItemComponentData::DOUBLE:
        case ItemComponentData::FLOAT:
            return (float)Value.as_double();
        case ItemComponentData::STRING:
            return csp::common::String(Value.as_string().c_str(), Value.as_string().length());
        case ItemComponentData::FLOAT_ARRAY:
        {
            auto& Array = Value.as_array();

            if (Array.size() == 3)
            {
                return csp::common::Vector3 { (float)Array[0].as_double(), (float)Array[1].as_double(), (float)Array[2].as_double() };
            }
            else if (Array.size() == 2)
            {
                return csp::common::Vector2 { (float)Array[0].as_double(), (float)Array[1].as_double() };
            }
            else
            {
                return csp::common::Vector4 { (float)Array[0].as_double(), (float)Array[1].as_double(), (float)Array[2].as_double(),
                    (float)Array[3].as_double() };
            }
        }
        case ItemComponentData::STRING_DICTIONARY:
        {
            csp::common::Map<csp::common::String, csp::multiplayer::ReplicatedValue> ReplicatedMap;

            // Value will be of type null if no elements exist within the map
            if (Value.is_null())
            {
                return ReplicatedMap;
            }

            const auto& Map = Value.as_string_map();

            for (const auto& Pair : Map)
            {
                auto ValuePair = Pair.second;
                const auto& ValueArray = ValuePair.as_array();

                ReplicatedMap[Pair.first.c_str()]
                    = SignalRValueToReplicatedValue(static_cast<ItemComponentData>(ValueArray[0].as_uinteger()), ValueArray[1].as_array()[0]);
            }

            return ReplicatedMap;
        }
        default:
            throw std::runtime_error("Unsupported property type!");
        }
    }

} // namespace

SignalRMsgPackEntitySerialiser::SignalRMsgPackEntitySerialiser()
    : CurrentState(SerialiserState::Initial)
    , CurrentComponentId(0)
{
}

void SignalRMsgPackEntitySerialiser::BeginEntity()
{
    assert(CurrentState == SerialiserState::Initial && "Entity already begun!");

    CurrentState = SerialiserState::InEntity;
    Fields.clear();
}

void SignalRMsgPackEntitySerialiser::EndEntity()
{
    assert(CurrentState == SerialiserState::InEntity && "Entity not yet begun!");

    CurrentState = SerialiserState::Initial;
}

void SignalRMsgPackEntitySerialiser::WriteBool(bool Value)
{
    switch (CurrentState)
    {
    case SerialiserState::InEntity:
        Fields.push_back(signalr::value(Value));
        break;
    case SerialiserState::InArray:
        CurrentArray.push_back(signalr::value(Value));
        break;
    default:
        throw std::runtime_error("WriteBool() function not supported in current state!");
    }
}

void SignalRMsgPackEntitySerialiser::WriteByte(uint8_t Value)
{
    assert(CurrentState == SerialiserState::InEntity && "WriteByte() function not supported in current state!");

    Fields.push_back(signalr::value((uint64_t)Value));
}

void SignalRMsgPackEntitySerialiser::WriteDouble(double Value)
{
    assert(CurrentState == SerialiserState::InEntity && "WriteDouble() function not supported in current state!");

    Fields.push_back(signalr::value(Value));
}

void SignalRMsgPackEntitySerialiser::WriteInt64(int64_t Value)
{
    assert(CurrentState == SerialiserState::InEntity && "WriteInt64() function not supported in current state!");

    Fields.push_back(signalr::value(Value));
}

void SignalRMsgPackEntitySerialiser::WriteUInt64(uint64_t Value)
{

    switch (CurrentState)
    {
    case SerialiserState::InEntity:
        Fields.push_back(signalr::value(Value));
        break;
    case SerialiserState::InArray:
        CurrentArray.push_back(signalr::value(Value));
        break;
    default:
        throw std::runtime_error("WriteUInt64() function not supported in current state!");
    }
}

void SignalRMsgPackEntitySerialiser::WriteString(const csp::common::String& Value)
{
    assert(CurrentState == SerialiserState::InEntity && "WriteString() function not supported in current state!");

    Fields.push_back(signalr::value(Value));
}

void SignalRMsgPackEntitySerialiser::WriteVector2(const csp::common::Vector2& Value)
{
    assert(CurrentState == SerialiserState::InEntity && "WriteVector2() function not supported in current state!");

    std::vector<signalr::value> ArrayValue { Value.X, Value.Y };
    Fields.push_back(std::move(ArrayValue));
}

void SignalRMsgPackEntitySerialiser::WriteVector3(const csp::common::Vector3& Value)
{
    assert(CurrentState == SerialiserState::InEntity && "WriteVector3() function not supported in current state!");

    std::vector<signalr::value> ArrayValue { Value.X, Value.Y, Value.Z };
    Fields.push_back(std::move(ArrayValue));
}

void SignalRMsgPackEntitySerialiser::WriteVector4(const csp::common::Vector4& Value)
{
    assert(CurrentState == SerialiserState::InEntity && "WriteVector4() function not supported in current state!");

    std::vector<signalr::value> ArrayValue { Value.X, Value.Y, Value.Z, Value.W };
    Fields.push_back(std::move(ArrayValue));
}

void SignalRMsgPackEntitySerialiser::WriteNull()
{
    switch (CurrentState)
    {
    case SerialiserState::InEntity:
        Fields.push_back(signalr::value(signalr::value_type::null));
        break;
    case SerialiserState::InArray:
        CurrentArray.push_back(signalr::value(signalr::value_type::null));
        break;
    default:
        throw std::runtime_error("WriteNull() function not supported in current state!");
    }
}

void SignalRMsgPackEntitySerialiser::BeginComponents()
{
    assert(CurrentState == SerialiserState::InEntity && "Entity not yet begun or components already begun!");

    CurrentState = SerialiserState::InComponents;
    Components.clear();
}

void SignalRMsgPackEntitySerialiser::EndComponents()
{
    assert(CurrentState == SerialiserState::InComponents && "Components not yet begun or component begun!");

    CurrentState = SerialiserState::InEntity;
    Fields.push_back(signalr::value(Components));
}

void SignalRMsgPackEntitySerialiser::BeginComponent(uint16_t Id, uint64_t Type)
{
    assert(CurrentState == SerialiserState::InComponents && "Components not yet begun or component already begun!");

    CurrentState = SerialiserState::InComponent;
    CurrentComponentId = Id;
    Properties.clear();

    // When we begin the component, take note of the component type and encode into a specific property key, so
    // that we can easily reference this info when deserialising.
    Properties[COMPONENT_KEY_COMPONENTTYPE] = std::make_pair(ItemComponentData::UINT64, Type);
}

void SignalRMsgPackEntitySerialiser::EndComponent()
{
    assert(CurrentState == SerialiserState::InComponent && "Component not yet begun or property begun!");

    CurrentState = SerialiserState::InComponents;

    std::map<uint64_t, signalr::value> ComponentFields;

    // Build our map of component properties in a format our server expects by looping over our gathered property collection
    for (auto const& Property : Properties)
    {
        // Store the value of the property as a signalr value (this has to be stored as a vector even though it's only 1 index we'll ever use)
        std::vector<signalr::value> Item = { signalr::value(Property.second.second) };
        // Store the value with the type identifier in a vector (again, needs to be a vector)
        std::vector<signalr::value> Prop = { Property.second.first, signalr::value(Item) };
        // Convert to signalr value and add to our final map structure indexing using the property ID
        ComponentFields[Property.first] = signalr::value(Prop);
    }

    // Pack the fields into a vector (required by the server)
    std::vector<signalr::value> ComponentFieldVector = { signalr::value(ComponentFields) };

    // Construct a vector that states the data type of our serialisation (Dictionary), along with the data
    std::vector<signalr::value> ComponentArray
        = { signalr::value((uint64_t)ItemComponentData::UINT16_DICTIONARY), signalr::value(ComponentFieldVector) };
    // Convert into a signalr value and store in the Components map to be eventually returned as part of the final data structure
    Components[CurrentComponentId] = signalr::value(ComponentArray);
}

void SignalRMsgPackEntitySerialiser::BeginArray()
{
    assert(CurrentState == SerialiserState::InEntity && "Entity not yet begun or array already begun!");

    CurrentState = SerialiserState::InArray;

    CurrentArray.clear();
}

void SignalRMsgPackEntitySerialiser::EndArray()
{
    assert(CurrentState == SerialiserState::InArray && "Array not yet begun!");

    CurrentState = SerialiserState::InEntity;

    Fields.push_back(signalr::value(CurrentArray));
}

/// <summary>
/// Write a property into the serialised format. Converts from ReplicatedValue to signalr::value.
/// It is important that we only use this for serialising properties inside a Component.
/// Entity-Level properties should use the functions such as WriteBool().
/// </summary>
/// <param name="Id">The property ID, used to apply this change to the right data when deserialising.</param>
/// <param name="Value">The ReplicatedValue to turn into a signalr value.</param>
void SignalRMsgPackEntitySerialiser::WriteProperty(uint64_t Id, const ReplicatedValue& Value)
{
    assert(CurrentState == SerialiserState::InComponent && "Component not yet begun!");

    auto SignalRValue = ReplicatedValueToSignalRValue(Value);

    // Place the data into the properties map to be converted into a correct SignalR formatted structure when we call EndComponent()
    Properties[static_cast<uint16_t>(Id)] = SignalRValue;
}

/// <summary>
/// View Components are data that is stored in specific keys on the server, it allows us to discretely update these
/// singular data pieces, rather than replicating larger chunks of data, and also allows us to always know where
/// in a data structure this data will be.
/// </summary>
/// <param name="Id">The ID of the component</param>
/// <param name="Value">The value of the component data to add</param>
void SignalRMsgPackEntitySerialiser::AddViewComponent(uint16_t Id, const ReplicatedValue& Value)
{
    uint64_t Type;
    signalr::value SValue;

    switch (Value.GetReplicatedValueType())
    {
    case csp::multiplayer::ReplicatedValueType::String:
        Type = ItemComponentData::STRING;
        SValue = Value.GetString().c_str();
        break;
    case csp::multiplayer::ReplicatedValueType::Vector2:
    {
        Type = ItemComponentData::FLOAT_ARRAY;
        auto Vector = Value.GetVector2();
        std::vector<signalr::value> Array = { Vector.X, Vector.Y };
        SValue = Array;
        break;
    }
    case csp::multiplayer::ReplicatedValueType::Vector3:
    {
        Type = ItemComponentData::FLOAT_ARRAY;
        auto Vector = Value.GetVector3();
        std::vector<signalr::value> Array = { Vector.X, Vector.Y, Vector.Z };
        SValue = Array;
        break;
    }
    case csp::multiplayer::ReplicatedValueType::Vector4:
    {
        Type = ItemComponentData::FLOAT_ARRAY;
        auto Vector = Value.GetVector4();
        std::vector<signalr::value> Array = { Vector.X, Vector.Y, Vector.Z, Vector.W };
        SValue = Array;
        break;
    }
    case csp::multiplayer::ReplicatedValueType::Integer:
        Type = ItemComponentData::INT64;
        SValue = Value.GetInt();
        break;
    default:
        throw std::runtime_error("Unsupported ViewComponent type!");
    }

    // Specific data packing for the component, we only store single values (though some are vectors) in these components,
    // as opposed to our regular components that contain many properties.
    std::vector<signalr::value> ComponentFields;
    ComponentFields.push_back(SValue);
    std::vector<signalr::value> ComponentArray = { signalr::value(Type), signalr::value(ComponentFields) };
    Components[Id] = signalr::value(ComponentArray);
}

/// <summary>
/// Return the signalr value that represents our serialised data structure, ready for adding to a message payload.
/// </summary>
signalr::value SignalRMsgPackEntitySerialiser::Finalise() { return signalr::value(Fields); }

SignalRMsgPackEntityDeserialiser::SignalRMsgPackEntityDeserialiser(const signalr::value& Object)
    : CurrentState(SerialiserState::Initial)
    , Object(&Object)
    , Fields(nullptr)
    , Components(nullptr)
    , ComponentPropertyCount(0)
    , CurrentArray(nullptr)
{
}

void SignalRMsgPackEntityDeserialiser::EnterEntity()
{
    assert(CurrentState == SerialiserState::Initial && "Entity already entered!");

    CurrentState = SerialiserState::InEntity;
    Fields = &Object->as_array();
    CurrentFieldIterator = Fields->cbegin();
}

void SignalRMsgPackEntityDeserialiser::LeaveEntity()
{
    assert(CurrentState == SerialiserState::InEntity && "Entity not entered!");

    CurrentState = SerialiserState::Initial;
    CurrentFieldIterator = Fields->cend();
    Fields = nullptr;
}

bool SignalRMsgPackEntityDeserialiser::ReadBool()
{
    switch (CurrentState)
    {
    case SerialiserState::InEntity:
        assert(CurrentFieldIterator->is_bool() && "Current field is not a boolean!");

        return (CurrentFieldIterator++)->as_bool();
    case SerialiserState::InArray:
        assert(CurrentArrayIterator->is_bool() && "Current array element is not a boolean!");

        return (CurrentArrayIterator++)->as_bool();
    case SerialiserState::InComponent:
        PropertyUnpacker.next(PropertyObjectHandle);

        assert(PropertyObjectHandle.get().type == msgpack::type::object_type::BOOLEAN);

        return PropertyObjectHandle.get().via.boolean;
    default:
        throw std::runtime_error("ReadBool() function not supported in current state!");
    }
}

uint8_t SignalRMsgPackEntityDeserialiser::ReadByte()
{
    switch (CurrentState)
    {
    case SerialiserState::InEntity:
        assert(CurrentFieldIterator->is_uinteger() && "Current field is not a byte!");

        return (CurrentFieldIterator++)->as_uinteger() & 0xFF;
    case SerialiserState::InComponent:
        PropertyUnpacker.next(PropertyObjectHandle);

        assert(PropertyObjectHandle.get().type == msgpack::type::object_type::POSITIVE_INTEGER);

        return PropertyObjectHandle.get().via.u64 & 0xFF;
    default:
        throw std::runtime_error("ReadByte() function not supported in current state!");
    }
}

double SignalRMsgPackEntityDeserialiser::ReadDouble()
{
    switch (CurrentState)
    {
    case SerialiserState::InEntity:
        assert(CurrentFieldIterator->is_double() && "Current field is not a double!");

        return (CurrentFieldIterator++)->as_double();
    case SerialiserState::InComponent:
        PropertyUnpacker.next(PropertyObjectHandle);

        assert(PropertyObjectHandle.get().type == msgpack::type::object_type::FLOAT64);

        return PropertyObjectHandle.get().via.f64;
    default:
        throw std::runtime_error("ReadDouble() function not supported in current state!");
    }
}

int64_t SignalRMsgPackEntityDeserialiser::ReadInt64()
{
    switch (CurrentState)
    {
    case SerialiserState::InEntity:
        assert(CurrentFieldIterator->is_integer() && "Current field is not an integer!");

        return (CurrentFieldIterator++)->as_integer();
    case SerialiserState::InComponent:
        PropertyUnpacker.next(PropertyObjectHandle);

        assert(PropertyObjectHandle.get().type == msgpack::type::object_type::POSITIVE_INTEGER
            || PropertyObjectHandle.get().type == msgpack::type::object_type::NEGATIVE_INTEGER);

        return PropertyObjectHandle.get().via.i64;
    default:
        throw std::runtime_error("ReadInt64() function not supported in current state!");
    }
}

uint64_t SignalRMsgPackEntityDeserialiser::ReadUInt64()
{
    switch (CurrentState)
    {
    case SerialiserState::InEntity:
        assert(CurrentFieldIterator->is_uinteger() && "Current field is not an unsigned integer!");

        return (CurrentFieldIterator++)->as_uinteger();
    case SerialiserState::InComponent:
        PropertyUnpacker.next(PropertyObjectHandle);

        assert(PropertyObjectHandle.get().type == msgpack::type::object_type::POSITIVE_INTEGER);

        return PropertyObjectHandle.get().via.u64;
    case SerialiserState::InArray:
        assert(CurrentArrayIterator->is_uinteger() && "Current array is not an unsigned integer!");

        return (CurrentArrayIterator++)->as_uinteger();
    default:
        throw std::runtime_error("ReadUInt64() function not supported in current state!");
    }
}

csp::common::String SignalRMsgPackEntityDeserialiser::ReadString()
{
    switch (CurrentState)
    {
    case SerialiserState::InEntity:
    {
        assert(CurrentFieldIterator->is_string() && "Current field is not a string!");

        auto& Value = (CurrentFieldIterator++)->as_string();

        return csp::common::String(Value.c_str(), Value.length());
    }
    case SerialiserState::InComponent:
        PropertyUnpacker.next(PropertyObjectHandle);

        assert(PropertyObjectHandle.get().type == msgpack::type::object_type::STR);

        return csp::common::String(PropertyObjectHandle.get().via.str.ptr, PropertyObjectHandle.get().via.str.size);
    default:
        throw std::runtime_error("ReadString() function not supported in current state!");
    }
}

csp::common::Vector2 SignalRMsgPackEntityDeserialiser::ReadVector2()
{
    switch (CurrentState)
    {
    case SerialiserState::InEntity:
    {
        assert(CurrentFieldIterator->is_array() && CurrentFieldIterator->as_array().size() == 2 && CurrentFieldIterator->as_array()[0].is_double()
            && "Current field is not a Vector2!");

        auto Value = (CurrentFieldIterator++)->as_array();

        return { (float)Value[0].as_double(), (float)Value[1].as_double() };
    }
    case SerialiserState::InComponent:
    {
        PropertyUnpacker.next(PropertyObjectHandle);

        assert(PropertyObjectHandle.get().type == msgpack::type::object_type::ARRAY && PropertyObjectHandle.get().via.array.size == 2
            && PropertyObjectHandle.get().via.array.ptr[0].type == msgpack::type::object_type::FLOAT64);

        auto* Array = PropertyObjectHandle.get().via.array.ptr;

        return { (float)Array[0].via.f64, (float)Array[1].via.f64 };
    }
    default:
        throw std::runtime_error("ReadVector2() function not supported in current state!");
    }
}

csp::common::Vector3 SignalRMsgPackEntityDeserialiser::ReadVector3()
{
    switch (CurrentState)
    {
    case SerialiserState::InEntity:
    {
        assert(CurrentFieldIterator->is_array() && CurrentFieldIterator->as_array().size() == 3 && CurrentFieldIterator->as_array()[0].is_double()
            && "Current field is not a Vector3!");

        auto Value = (CurrentFieldIterator++)->as_array();

        return { (float)Value[0].as_double(), (float)Value[1].as_double(), (float)Value[2].as_double() };
    }
    case SerialiserState::InComponent:
    {
        PropertyUnpacker.next(PropertyObjectHandle);

        assert(PropertyObjectHandle.get().type == msgpack::type::object_type::ARRAY && PropertyObjectHandle.get().via.array.size == 3
            && PropertyObjectHandle.get().via.array.ptr[0].type == msgpack::type::object_type::FLOAT64);

        auto* Array = PropertyObjectHandle.get().via.array.ptr;

        return { (float)Array[0].via.f64, (float)Array[1].via.f64, (float)Array[2].via.f64 };
    }
    default:
        throw std::runtime_error("ReadVector3() function not supported in current state!");
    }
}

csp::common::Vector4 SignalRMsgPackEntityDeserialiser::ReadVector4()
{
    switch (CurrentState)
    {
    case SerialiserState::InEntity:
    {
        assert(CurrentFieldIterator->is_array() && CurrentFieldIterator->as_array().size() == 4 && CurrentFieldIterator->as_array()[0].is_double()
            && "Current field is not a Vector4!");

        auto& Value = (CurrentFieldIterator++)->as_array();

        return { (float)Value[0].as_double(), (float)Value[1].as_double(), (float)Value[2].as_double(), (float)Value[3].as_double() };
    }
    case SerialiserState::InComponent:
    {
        PropertyUnpacker.next(PropertyObjectHandle);

        assert(PropertyObjectHandle.get().type == msgpack::type::object_type::ARRAY && PropertyObjectHandle.get().via.array.size == 4
            && PropertyObjectHandle.get().via.array.ptr[0].type == msgpack::type::object_type::FLOAT64);

        auto* Array = PropertyObjectHandle.get().via.array.ptr;

        return { (float)Array[0].via.f64, (float)Array[1].via.f64, (float)Array[2].via.f64, (float)Array[3].via.f64 };
    }
    default:
        throw std::runtime_error("ReadVector4() function not supported in current state!");
    }
}

bool SignalRMsgPackEntityDeserialiser::NextValueIsNull()
{
    switch (CurrentState)
    {
    case SerialiserState::InEntity:
        return CurrentFieldIterator->is_null();
    case SerialiserState::InArray:
        return CurrentArrayIterator->is_null();
    default:
        throw std::runtime_error("NextValueIsNull() function not supported in current state!");
    }
}

bool SignalRMsgPackEntityDeserialiser::NextValueIsArray()
{
    switch (CurrentState)
    {
    case SerialiserState::InEntity:
        return CurrentFieldIterator->is_array();
    case SerialiserState::InArray:
        return CurrentArrayIterator->is_array();
    default:
        throw std::runtime_error("NextValueIsArray() function not supported in current state!");
    }
}

void SignalRMsgPackEntityDeserialiser::EnterArray(CSP_OUT uint32_t& OutLength)
{
    assert(CurrentState == SerialiserState::InEntity && "Entity not entered or array already entered!");

    CurrentState = SerialiserState::InArray;
    CurrentArray = &CurrentFieldIterator->as_array();
    CurrentArrayIterator = CurrentArray->cbegin();

    OutLength = static_cast<uint32_t>(CurrentArray->size());
}

void SignalRMsgPackEntityDeserialiser::LeaveArray()
{
    assert(CurrentState == SerialiserState::InArray && "Array not entered!");

    CurrentState = SerialiserState::InEntity;
    CurrentArrayIterator = CurrentArray->cend();
    CurrentArray = nullptr;

    CurrentFieldIterator++;
}

void SignalRMsgPackEntityDeserialiser::EnterComponents()
{
    assert(CurrentState == SerialiserState::InEntity && "Entity not entered or components already entered!");

    CurrentState = SerialiserState::InComponents;
    Components = &CurrentFieldIterator->as_uint_map();
    CurrentComponentIterator = Components->cbegin();
}

void SignalRMsgPackEntityDeserialiser::LeaveComponents()
{
    assert(CurrentState == SerialiserState::InComponents && "Components not entered or component entered!");

    CurrentState = SerialiserState::InEntity;
    CurrentComponentIterator = Components->cend();
    Components = nullptr;
}

uint64_t SignalRMsgPackEntityDeserialiser::GetNumComponents()
{
    assert(CurrentState >= SerialiserState::InComponents && "Components not entered!");

    return Components->size();
}

uint64_t SignalRMsgPackEntityDeserialiser::GetNumRealComponents()
{
    assert(CurrentState >= SerialiserState::InComponents && "Components not entered!");

    uint64_t Count = 0;

    for (const auto& [Key, Component] : *Components)
    {
        if (Key < COMPONENT_KEY_END_COMPONENTS)
        {
            ++Count;
        }
    }

    return Count;
}

void SignalRMsgPackEntityDeserialiser::EnterComponent(CSP_OUT uint16_t& OutId, CSP_OUT uint64_t& OutType)
{
    assert(CurrentState == SerialiserState::InComponents && "Components not entered or component already entered!");

    IsMsgPackSerialiser = false;

    CurrentState = SerialiserState::InComponent;

    // Ensure our iterator does not parse values that are not in the regular component range (such as ViewComponents, which are
    // deserialised separately, see GetViewComponent()).
    // In addition, ensure the OutId is set here, to the component we are about to parse.
    for (;;)
    {
        OutId = static_cast<uint16_t>(CurrentComponentIterator->first);

        if (OutId <= COMPONENT_KEY_END_COMPONENTS)
        {
            break;
        }

        ++CurrentComponentIterator;
    }

    // Read specifically the data type set in the component.
    auto DataType = CurrentComponentIterator->second.as_array()[0].as_uinteger();

    if (DataType == ItemComponentData::UINT16_DICTIONARY)
    {
        Properties.clear();

        // Retrieve our property map from the component.
        auto _Properties = CurrentComponentIterator->second.as_array()[1].as_array()[0].as_uint_map();

        for (auto Property : _Properties)
        {
            auto PropertyID = Property.first;
            auto PropertyData = Property.second.as_array();

            // Find the component type when we parse it, and ensure it's output
            if (PropertyID == COMPONENT_KEY_COMPONENTTYPE)
            {
                OutType = PropertyData[1].as_array()[0].as_uinteger();
            }
            else
            {
                // push to the deserialisers own property map, which represents deserialised data
                Properties[PropertyID] = std::make_pair((ItemComponentData)PropertyData[0].as_uinteger(), PropertyData[1].as_array()[0]);
            }
        }

        ComponentPropertyCount = Properties.size();

        CurrentPropertyIterator = Properties.cbegin();
    }
    else if (DataType == ItemComponentData::UINT8_ARRAY) // Support for reading legacy, MsgPacked component data, in a raw binary format. Eventually
                                                         // this will be removed.
    {
        IsMsgPackSerialiser = true;

        size_t DataLen;
        auto Data = CurrentComponentIterator->second.as_array()[1].as_array()[0].as_raw(DataLen);

        ComponentUnpacker = msgpack::unpacker();
        ComponentUnpacker.reserve_buffer(DataLen);
        std::memcpy(ComponentUnpacker.buffer(), Data, DataLen);
        ComponentUnpacker.buffer_consumed(DataLen);

        ComponentObjectHandle = msgpack::object_handle();

        ComponentUnpacker.next(ComponentObjectHandle);
        OutType = ComponentObjectHandle.get().via.u64;

        ComponentUnpacker.next(ComponentObjectHandle);
        ComponentPropertyCount = ComponentObjectHandle.get().via.u64;

        ComponentUnpacker.next(ComponentObjectHandle);
        auto PropertyDataLen = ComponentObjectHandle.get().via.bin.size;
        auto PropertyData = ComponentObjectHandle.get().via.bin.ptr;

        PropertyUnpacker = msgpack::unpacker();
        PropertyUnpacker.reserve_buffer(PropertyDataLen);
        std::memcpy(PropertyUnpacker.buffer(), PropertyData, PropertyDataLen);
        PropertyUnpacker.buffer_consumed(PropertyDataLen);
    }
    else
    {
        CSP_LOG_ERROR_MSG("Unsupported data type of serialised data");
    }
}

void SignalRMsgPackEntityDeserialiser::LeaveComponent()
{
    assert(CurrentState == SerialiserState::InComponent && "Component not entered!");

    CurrentState = SerialiserState::InComponents;
    ++CurrentComponentIterator;
}

uint64_t SignalRMsgPackEntityDeserialiser::GetNumProperties()
{
    assert(CurrentState == SerialiserState::InComponent && "Component not entered!");

    return ComponentPropertyCount;
}

ReplicatedValue SignalRMsgPackEntityDeserialiser::ReadProperty(CSP_OUT uint64_t& OutId)
{
    assert(CurrentState == SerialiserState::InComponent && "Component not entered or property already entered!");

    if (IsMsgPackSerialiser) // Support for deserialising properties within a legacy MsgPacked component, this will be removed in future.
    {
        PropertyUnpacker.next(PropertyObjectHandle);
        OutId = PropertyObjectHandle.get().via.u64;

        PropertyUnpacker.next(PropertyObjectHandle);
        auto Type = (ReplicatedValueType)PropertyObjectHandle.get().via.u64;

        PropertyUnpacker.next(PropertyObjectHandle);

        switch (Type)
        {
        case ReplicatedValueType::Boolean:
            assert(PropertyObjectHandle.get().type == msgpack::type::object_type::BOOLEAN);

            return PropertyObjectHandle.get().via.boolean;
        case ReplicatedValueType::Integer:
            assert(PropertyObjectHandle.get().type == msgpack::type::object_type::POSITIVE_INTEGER
                || PropertyObjectHandle.get().type == msgpack::type::object_type::NEGATIVE_INTEGER);

            return PropertyObjectHandle.get().via.i64;
        case ReplicatedValueType::Float:
            assert(PropertyObjectHandle.get().type == msgpack::type::object_type::FLOAT64);

            return (float)PropertyObjectHandle.get().via.f64;
        case ReplicatedValueType::String:
            assert(PropertyObjectHandle.get().type == msgpack::type::object_type::STR);

            return csp::common::String(PropertyObjectHandle.get().via.str.ptr, PropertyObjectHandle.get().via.str.size);
        case ReplicatedValueType::Vector2:
        {
            assert(PropertyObjectHandle.get().type == msgpack::type::object_type::ARRAY && PropertyObjectHandle.get().via.array.size == 2
                && PropertyObjectHandle.get().via.array.ptr[0].type == msgpack::type::object_type::FLOAT64);

            auto* Array = PropertyObjectHandle.get().via.array.ptr;

            return csp::common::Vector2 { (float)Array[0].via.f64, (float)Array[1].via.f64 };
        }
        case ReplicatedValueType::Vector3:
        {
            assert(PropertyObjectHandle.get().type == msgpack::type::object_type::ARRAY && PropertyObjectHandle.get().via.array.size == 3
                && PropertyObjectHandle.get().via.array.ptr[0].type == msgpack::type::object_type::FLOAT64);

            auto* Array = PropertyObjectHandle.get().via.array.ptr;

            return csp::common::Vector3 { (float)Array[0].via.f64, (float)Array[1].via.f64, (float)Array[2].via.f64 };
        }
        case ReplicatedValueType::Vector4:
        {
            assert(PropertyObjectHandle.get().type == msgpack::type::object_type::ARRAY && PropertyObjectHandle.get().via.array.size == 4
                && PropertyObjectHandle.get().via.array.ptr[0].type == msgpack::type::object_type::FLOAT64);

            auto* Array = PropertyObjectHandle.get().via.array.ptr;

            return csp::common::Vector4 { (float)Array[0].via.f64, (float)Array[1].via.f64, (float)Array[2].via.f64, (float)Array[3].via.f64 };
        }
        default:
            throw std::runtime_error("Unsupported property type!");
        }
    }
    else
    {
        // Our CurrentPropertyIterator starts with the first value in our locally deserialiser built Properties map,
        // each time we read a property using this function, the iterator grabs the next property due for deserialisation.
        auto& KeyValue = *CurrentPropertyIterator++;
        // We return the propertyID with this out value.
        OutId = KeyValue.first;
        // We use the value type grabbed here, to convert the underlying signalr value correctly into a ReplicatedValue.
        auto ValueType = KeyValue.second.first;
        // We grab the Value data here, for simplicity of code.
        auto Value = KeyValue.second.second;

        return SignalRValueToReplicatedValue(ValueType, Value);
    }
}

ReplicatedValue SignalRMsgPackEntityDeserialiser::GetViewComponent(uint16_t Id)
{
    if (Components->find(Id) == Components->end())
    {
        return ReplicatedValue();
    }

    auto& Component = Components->at(Id);
    auto& ComponentValue = Component.as_array()[1].as_array()[0];

    switch (ComponentValue.type())
    {
    case signalr::value_type::string:
        return ReplicatedValue(ComponentValue.as_string().c_str());
    case signalr::value_type::array:
    {
        auto& Array = ComponentValue.as_array();

        if (Array.size() == 3)
        {
            return ReplicatedValue(csp::common::Vector3 { (float)Array[0].as_double(), (float)Array[1].as_double(), (float)Array[2].as_double() });
        }

        if (Array.size() == 4)
        {
            return ReplicatedValue(csp::common::Vector4 {
                (float)Array[0].as_double(), (float)Array[1].as_double(), (float)Array[2].as_double(), (float)Array[3].as_double() });
        }
        return ReplicatedValue();
    }
    case signalr::value_type::integer:
    {
        return ReplicatedValue(ComponentValue.as_integer());
    }
    case signalr::value_type::uinteger:
    {
        return ReplicatedValue(static_cast<int64_t>(ComponentValue.as_uinteger()));
    }
    default:
        throw std::runtime_error("Unsupported ViewComponent type!");
    }
}

bool SignalRMsgPackEntityDeserialiser::HasViewComponent(uint16_t Id) { return Components->count(Id); }

void SignalRMsgPackEntityDeserialiser::Skip()
{
    switch (CurrentState)
    {
    case SerialiserState::InEntity:
        ++CurrentFieldIterator;
        break;
    case SerialiserState::InArray:
        ++CurrentArrayIterator;
        break;
    default:
        throw std::runtime_error("Skip() function not supported in current state!");
    }
}

} // namespace csp::multiplayer
