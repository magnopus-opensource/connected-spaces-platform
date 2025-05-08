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

#pragma once

#include "Multiplayer/SignalRSerializer.h"

#include <map>
#include <memory>
#include <optional>

/*
    Read if you want to support new types!!!

    1. You will need to uncomment the ItemComponentDataType that matches the type you want to support.
       If the type you want to support doesn't match any items in the ItemComponentDataType,
       you'll need to convert to one of the supported types, as these types are defined by MCS!

    2. Add the new type to ItemComponentDataVariant. This will then cause a compile error in
       ItemComponentData::Serialize until you do the next step.

    3. Add a new GetComponentEnum and SerializeComponentData functions that supports your type to MCSTypes.cpp,
       so it can be serialized to a signalr value.

    4. Add a new case in DeserializeComponentData so it can be deserialized from a signalr value.
*/

namespace csp::multiplayer::mcs
{
/// @brief All supported MCS types
enum class ItemComponentDataType : uint64_t
{
    BOOL = 0,
    // NULLABLE_BOOL = 1,
    // BOOL_ARRAY = 2,
    // NULLABLE_BOOL_ARRAY = 3,
    // UINT8 = 4,
    // NULLABLE_UINT8 = 5,
    // UINT8_ARRAY = 6,
    // NULLABLE_UINT8_ARRAY = 7,
    // INT32 = 8,
    // NULLABLE_INT32 = 9,
    // INT32_ARRAY = 10,
    // NULLABLE_INT32_ARRAY = 11,
    // UINT32 = 12,
    // NULLABLE_UINT32 = 13,
    // UINT32_ARRAY = 14,
    // NULLABLE_UINT32_ARRAY = 15,
    INT64 = 16,
    // NULLABLE_INT64 = 17,
    // INT64_ARRAY = 18,
    // NULLABLE_INT64_ARRAY = 19,
    UINT64 = 20,
    // NULLABLE_UINT64 = 21,
    // UINT64_ARRAY = 22,
    // NULLABLE_UINT64_ARRAY = 23,
    FLOAT = 24,
    // NULLABLE_FLOAT = 25,
    FLOAT_ARRAY = 26,
    // NULLABLE_FLOAT_ARRAY = 27,
    DOUBLE = 28,
    // NULLABLE_DOUBLE = 29,
    // DOUBLE_ARRAY = 30,
    // NULLABLE_DOUBLE_ARRAY = 31,
    STRING = 32,
    // STRING_ARRAY = 33,
    // DATETIMEOFFSET = 34,
    // NULLABLE_DATETIMEOFFSET = 35,
    // DATETIMEOFFSET_ARRAY = 36,
    // NULLABLE_DATETIMEOFFSET_ARRAY = 37,
    // TIMESPAN = 38,
    // NULLABLE_TIMESPAN = 39,
    // TIMESPAN_ARRAY = 40,
    // NULLABLE_TIMESPAN_ARRAY = 41,
    // GUID = 42,
    // NULLABLE_GUID = 43,
    // GUID_ARRAY = 44,
    // NULLABLE_GUID_ARRAY = 45,
    // INT16 = 46,
    // NULLABLE_INT16 = 47,
    // INT16_ARRAY = 48,
    // NULLABLE_INT16_ARRAY = 49,
    // UINT16 = 50,
    // NULLABLE_UINT16 = 51,
    // UINT16_ARRAY = 52,
    // NULLABLE_UINT16_ARRAY = 53,
    UINT16_DICTIONARY = 54,
    STRING_DICTIONARY = 55
};

class ItemComponentData;

/// @brief Variant that holds all currently implemented MCS types by CSP.
/// @details This should be updated if we need to support more of the above types in the future.
/// All of our variant types must match the supported signalr serializer values, or we will get a compile error.
using ItemComponentDataVariant = std::variant<bool, int64_t, uint64_t, float, std::vector<float>, double, std::string,
    std::map<uint16_t, ItemComponentData>, std::map<std::string, ItemComponentData>>;

using PropertyKeyType = uint16_t;

/// @brief ItemComponentData which represents a MCS component which is stores as a variant.
/// More information about this type can be found here:
/// https://github.com/magnopus/Magnopus.Services/blob/e7fff2e1171bbe185c0ad1f50fadc1ec64a30a6a/Source/Magnopus.Service.Multiplayer.Contracts/Messages/Components/IComponentData.cs
class ItemComponentData : public ISignalRSerializable, public ISignalRDeserializable
{
public:
    ItemComponentData() = default;
    ItemComponentData(const ItemComponentDataVariant& Value);

    void Serialize(SignalRSerializer& Serializer) const override;
    void Deserialize(SignalRDeserializer& Deserializer) override;

    const ItemComponentDataVariant& GetValue() const;

    bool operator==(const ItemComponentData& Other) const;

private:
    ItemComponentDataVariant Value;
};

/// @brief Represents an MCS object message.
/// @details This should be used when an object is first created, and needs to be replicated to MCS and other clients.
/// More information about this type can be found here:
/// https://github.com/magnopus/Magnopus.Services/blob/e7fff2e1171bbe185c0ad1f50fadc1ec64a30a6a/Source/Magnopus.Service.Multiplayer.Contracts/Messages/ObjectMessage.cs
class ObjectMessage : public ISignalRSerializable, public ISignalRDeserializable
{
public:
    ObjectMessage(uint64_t Id, uint64_t Type, bool IsTransferable, bool IsPersistant, uint64_t OwnerId, std::optional<uint64_t> ParentId,
        const std::map<uint16_t, ItemComponentData>& Components);

    void Serialize(SignalRSerializer& Serializer) const override;
    void Deserialize(SignalRDeserializer& Deserializer) override;

    bool operator==(const ObjectMessage& Other) const;

    uint64_t GetId() const;
    uint64_t GetType() const;
    bool GetIsTransferable() const;
    bool GetIsPersistant() const;
    uint64_t GetOwnerId() const;
    std::optional<uint64_t> GetParentId() const;
    const std::map<PropertyKeyType, ItemComponentData>& GetComponents() const;

private:
    uint64_t Id;
    uint64_t Type;
    bool IsTransferable;
    bool IsPersistant;
    uint64_t OwnerId;
    std::optional<uint64_t> ParentId;
    std::map<PropertyKeyType, ItemComponentData> Components;
};

/// @brief Represents an MCS object patch.
/// @details This should be used when an object needs to be update, and needs to be replicated to MCS and other clients.
/// More information about this type can be found here:
class ObjectPatch : public ISignalRSerializable, public ISignalRDeserializable
{
public:
    ObjectPatch(uint64_t Id, uint64_t OwnerId, bool Destroy, bool ShouldUpdateParent, std::optional<uint64_t> ParentId,
        const std::map<uint16_t, ItemComponentData>& Components);

    void Serialize(SignalRSerializer& Serializer) const override;
    void Deserialize(SignalRDeserializer& Deserializer) override;

    bool operator==(const ObjectPatch& Other) const;

    uint64_t GetId() const;
    uint64_t GetOwnerId() const;
    bool GetDestroy() const;
    bool GetShouldUpdateParent() const;
    std::optional<uint64_t> GetParentId() const;
    const std::map<PropertyKeyType, ItemComponentData>& GetComponents() const;

private:
    uint64_t Id;
    uint64_t OwnerId;
    bool Destroy;
    bool ShouldUpdateParent;
    std::optional<uint64_t> ParentId;
    std::map<PropertyKeyType, ItemComponentData> Components;
};
}
