/*
 * Copyright 2025 Magnopus LLC

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

#include "MCSTypes.h"

namespace csp::multiplayer::mcs
{
namespace
{
    // Convert from type to enum
    // Creating global functions gives us compile-time checking of types to ensure conversions are created for new types.
    ItemComponentDataType GetComponentEnum(bool) { return ItemComponentDataType::BOOL; }
    ItemComponentDataType GetComponentEnum(int64_t) { return ItemComponentDataType::INT64; }
    ItemComponentDataType GetComponentEnum(uint64_t) { return ItemComponentDataType::UINT64; }
    ItemComponentDataType GetComponentEnum(float) { return ItemComponentDataType::FLOAT; }
    ItemComponentDataType GetComponentEnum(const std::vector<float>&) { return ItemComponentDataType::FLOAT_ARRAY; }
    ItemComponentDataType GetComponentEnum(double) { return ItemComponentDataType::DOUBLE; }
    ItemComponentDataType GetComponentEnum(const std::string&) { return ItemComponentDataType::STRING; }
    ItemComponentDataType GetComponentEnum(const std::map<uint16_t, ItemComponentData>&) { return ItemComponentDataType::UINT16_DICTIONARY; }
    ItemComponentDataType GetComponentEnum(const std::map<std::string, ItemComponentData>&) { return ItemComponentDataType::STRING_DICTIONARY; }

    void SerializeComponentData(SignalRSerializer& serializer, bool value) { serializer.WriteValue(value); }
    void SerializeComponentData(SignalRSerializer& serializer, int64_t value) { serializer.WriteValue(value); }
    void SerializeComponentData(SignalRSerializer& serializer, uint64_t value) { serializer.WriteValue(value); }
    void SerializeComponentData(SignalRSerializer& serializer, float value) { serializer.WriteValue(value); }
    void SerializeComponentData(SignalRSerializer& serializer, const std::vector<float>& value) { serializer.WriteValue(value); }
    void SerializeComponentData(SignalRSerializer& serializer, double value) { serializer.WriteValue(value); }
    void SerializeComponentData(SignalRSerializer& serializer, const std::string& value) { serializer.WriteValue(value); }
    void SerializeComponentData(SignalRSerializer& serializer, const std::map<uint16_t, ItemComponentData>& value) { serializer.WriteValue(value); }
    void SerializeComponentData(SignalRSerializer& serializer, const std::map<std::string, ItemComponentData>& value)
    {
        serializer.WriteValue(value);
    }

    template <class T> void DeserializeComponentDataInternal(SignalRDeserializer& deserializer, ItemComponentDataVariant& outVal)
    {
        // It's important we construct the exact type we want to read from our variant,
        // as we want to make sure our variant is populated with the correct type.
        T deserializedValue {};
        deserializer.ReadValue(deserializedValue);
        outVal = deserializedValue;
    }

    void DeserializeComponentData(SignalRDeserializer& deserializer, ItemComponentDataType type, ItemComponentDataVariant& outVal)
    {
        switch (type)
        {
        case ItemComponentDataType::BOOL:
            DeserializeComponentDataInternal<bool>(deserializer, outVal);
            break;
        case ItemComponentDataType::INT64:
            // We can't guarantee MCS will give us back a signed integer, even if one is sent.
            if (deserializer.NextValueIsInt())
            {
                DeserializeComponentDataInternal<int64_t>(deserializer, outVal);
            }
            else
            {
                DeserializeComponentDataInternal<uint64_t>(deserializer, outVal);
            }
            break;
        case ItemComponentDataType::UINT64:
            // Due to us changing some of our types from int64->uint64, we may receive some unexpected int64 values here.
            // So we need to account for this here.
            if (deserializer.NextValueIsUint())
            {
                DeserializeComponentDataInternal<uint64_t>(deserializer, outVal);
            }
            else
            {
                DeserializeComponentDataInternal<int64_t>(deserializer, outVal);
            }

            break;
        case ItemComponentDataType::DOUBLE:
            DeserializeComponentDataInternal<double>(deserializer, outVal);
            break;
        case ItemComponentDataType::FLOAT:
            DeserializeComponentDataInternal<float>(deserializer, outVal);
            break;
        case ItemComponentDataType::FLOAT_ARRAY:
            DeserializeComponentDataInternal<std::vector<float>>(deserializer, outVal);
            break;
        case ItemComponentDataType::STRING:
            DeserializeComponentDataInternal<std::string>(deserializer, outVal);
            break;
        case ItemComponentDataType::UINT16_DICTIONARY:
            // If a dictionary is empty, we will receive null from MCS.
            if (deserializer.NextValueIsNull())
            {
                deserializer.Skip();
                outVal = std::map<uint16_t, ItemComponentData> {};
            }
            else
            {
                DeserializeComponentDataInternal<std::map<uint16_t, ItemComponentData>>(deserializer, outVal);
            }
            break;
        case ItemComponentDataType::STRING_DICTIONARY:
            // If a dictionary is empty, we will receive null from MCS.
            if (deserializer.NextValueIsNull())
            {
                deserializer.Skip();
                outVal = std::map<std::string, ItemComponentData> {};
            }
            else
            {
                DeserializeComponentDataInternal<std::map<std::string, ItemComponentData>>(deserializer, outVal);
            }
            break;
        default:
            throw std::invalid_argument("Trying to deserialize unsupported ItemComponentDataType");
        }
    }

    void DeserializeComponents(SignalRDeserializer& deserializer, std::optional<std::map<PropertyKeyType, ItemComponentData>>& components)
    {
        if (deserializer.NextValueIsNull() == false)
        {
            components = std::map<PropertyKeyType, ItemComponentData> {};

            size_t componentsSize = 0;
            deserializer.StartReadUintMap(componentsSize);

            for (size_t i = 0; i < componentsSize; ++i)
            {
                try
                {
                    std::pair<PropertyKeyType, ItemComponentData> componentKeyValue;
                    deserializer.ReadKeyValue(componentKeyValue);

                    (*components)[componentKeyValue.first] = componentKeyValue.second;
                }
                catch (const std::exception&)
                {
                }
                catch(...)
                {
                    /* This sort of exception management isn't really advisable, we need to emit a callback when we find old data, 
                       it's a serious compatibility issue. This catch block is added just to unblock Unity, which has done something
                       funky with sentry integration making derived exceptions not catch for them on iOS.
                       Sort of good it happened though, as it revealed the poor error management here:
                         - This is only one path of potentially many, why is there a special catch here?
                         - Just swallowing errors and loading the rest of a space is surely a recipe for terrifying bugs. */
                }
            }

            deserializer.EndReadUintMap();
        }
        else
        {
            components = std::nullopt;
            deserializer.Skip();
        }
    }
}

ItemComponentData::ItemComponentData(const ItemComponentDataVariant& value)
    : m_value { value }
{
}

void ItemComponentData::Serialize(SignalRSerializer& serializer) const
{
    // 1. Write an array for type-value pair.
    serializer.StartWriteArray();
    {
        // Visit the variant to get the underlying type.
        std::visit(
            [&serializer](const auto& valueType)
            {
                // 2. Write the type.
                serializer.WriteValue(static_cast<uint64_t>(GetComponentEnum(valueType)));

                // 3. Write an array for the value.
                serializer.StartWriteArray();
                {
                    // 4. Write the value.
                    SerializeComponentData(serializer, valueType);
                }
                serializer.EndWriteArray();
            },
            m_value);
    }
    serializer.EndWriteArray();
}

void ItemComponentData::Deserialize(SignalRDeserializer& deserializer)
{
    // 1. Read the type and value pair.
    size_t arraySize = 0;
    deserializer.StartReadArray(arraySize);
    {
        // 2. Read the ItemComponentDataType.
        uint64_t rawType;
        deserializer.ReadValue(rawType);

        const auto type = static_cast<ItemComponentDataType>(rawType);

        // 3. Read the value inside an array.
        size_t valueArraySize = 0;
        deserializer.StartReadArray(valueArraySize);
        {
            // 4. Deserialize the value.
            DeserializeComponentData(deserializer, type, m_value);
        }
        deserializer.EndReadArray();
    }
    deserializer.EndReadArray();
}

const ItemComponentDataVariant& ItemComponentData::GetValue() const { return m_value; }

bool ItemComponentData::operator==(const ItemComponentData& other) const { return m_value == other.m_value; }

ObjectMessage::ObjectMessage(uint64_t id, uint64_t type, bool isTransferable, bool isPersistent, uint64_t ownerId, std::optional<uint64_t> parentId,
    const std::optional<std::map<PropertyKeyType, ItemComponentData>>& components)
    : m_id { id }
    , m_type { type }
    , m_isTransferable { isTransferable }
    , m_isPersistent { isPersistent }
    , m_ownerId { ownerId }
    , m_parentId { parentId }
    , m_components { components }
{
}

void ObjectMessage::Serialize(SignalRSerializer& serializer) const
{
    serializer.StartWriteArray();
    {
        serializer.WriteValue(m_id);
        serializer.WriteValue(m_type);
        serializer.WriteValue(m_isTransferable);
        serializer.WriteValue(m_isPersistent);
        serializer.WriteValue(m_ownerId);
        serializer.WriteValue(m_parentId);
        serializer.WriteValue(m_components);
    }
    serializer.EndWriteArray();
}

void ObjectMessage::Deserialize(SignalRDeserializer& deserializer)
{
    size_t arraySize = 0;
    deserializer.StartReadArray(arraySize);
    {
        deserializer.ReadValue(m_id);
        deserializer.ReadValue(m_type);
        deserializer.ReadValue(m_isTransferable);
        deserializer.ReadValue(m_isPersistent);
        deserializer.ReadValue(m_ownerId);
        deserializer.ReadValue(m_parentId);
        DeserializeComponents(deserializer, m_components);
    }
    deserializer.EndReadArray();
}

bool ObjectMessage::operator==(const ObjectMessage& other) const
{
    return m_id == other.m_id && m_type == other.m_type && m_isTransferable == other.m_isTransferable && m_isPersistent == other.m_isPersistent
        && m_ownerId == other.m_ownerId && m_parentId == other.m_parentId && m_components == other.m_components;
}

uint64_t ObjectMessage::GetId() const { return m_id; }

uint64_t ObjectMessage::GetType() const { return m_type; }

bool ObjectMessage::GetIsTransferable() const { return m_isTransferable; }

bool ObjectMessage::GetIsPersistent() const { return m_isPersistent; }

uint64_t ObjectMessage::GetOwnerId() const { return m_ownerId; }

std::optional<uint64_t> ObjectMessage::GetParentId() const { return m_parentId; }

const std::optional<std::map<PropertyKeyType, ItemComponentData>>& ObjectMessage::GetComponents() const { return m_components; }

ObjectPatch::ObjectPatch(uint64_t id, uint64_t ownerId, bool destroy, bool shouldUpdateParent, std::optional<uint64_t> parentId,
    const std::map<PropertyKeyType, ItemComponentData>& components)
    : m_id { id }
    , m_ownerId { ownerId }
    , m_destroy { destroy }
    , m_shouldUpdateParent { shouldUpdateParent }
    , m_parentId { parentId }
    , m_components { components }
{
}

void ObjectPatch::Serialize(SignalRSerializer& serializer) const
{
    serializer.StartWriteArray();
    {
        serializer.WriteValue(m_id);
        serializer.WriteValue(m_ownerId);
        serializer.WriteValue(m_destroy);

        // Parent changes need to be in a vector.
        serializer.StartWriteArray();
        {
            serializer.WriteValue(m_shouldUpdateParent);
            serializer.WriteValue(m_parentId);
        }
        serializer.EndWriteArray();

        serializer.WriteValue(m_components);
    }
    serializer.EndWriteArray();
}

void ObjectPatch::Deserialize(SignalRDeserializer& deserializer)
{
    size_t arraySize = 0;
    deserializer.StartReadArray(arraySize);
    {
        deserializer.ReadValue(m_id);
        deserializer.ReadValue(m_ownerId);
        deserializer.ReadValue(m_destroy);

        // Array will be null from MCS if there is no parent update.
        if (deserializer.NextValueIsNull() == false)
        {
            size_t parentArraySize = 0;
            deserializer.StartReadArray(parentArraySize);
            {
                deserializer.ReadValue(m_shouldUpdateParent);
                deserializer.ReadValue(m_parentId);
            }
            deserializer.EndReadArray();
        }

        DeserializeComponents(deserializer, m_components);
    }
    deserializer.EndReadArray();
}

bool ObjectPatch::operator==(const ObjectPatch& other) const
{
    return m_id == other.m_id && m_ownerId == other.m_ownerId && m_destroy == other.m_destroy && m_shouldUpdateParent == other.m_shouldUpdateParent
        && m_parentId == other.m_parentId && m_components == other.m_components;
}

uint64_t ObjectPatch::GetId() const { return m_id; }

uint64_t ObjectPatch::GetOwnerId() const { return m_ownerId; }

bool ObjectPatch::GetDestroy() const { return m_destroy; }

bool ObjectPatch::GetShouldUpdateParent() const { return m_shouldUpdateParent; }

std::optional<uint64_t> ObjectPatch::GetParentId() const { return m_parentId; }

const std::optional<std::map<PropertyKeyType, ItemComponentData>>& ObjectPatch::GetComponents() const { return m_components; }

}
