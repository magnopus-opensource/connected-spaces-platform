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

    void SerializeComponentData(SignalRSerializer& Serializer, bool Value) { Serializer.WriteValue(Value); }
    void SerializeComponentData(SignalRSerializer& Serializer, int64_t Value) { Serializer.WriteValue(Value); }
    void SerializeComponentData(SignalRSerializer& Serializer, uint64_t Value) { Serializer.WriteValue(Value); }
    void SerializeComponentData(SignalRSerializer& Serializer, float Value) { Serializer.WriteValue(Value); }
    void SerializeComponentData(SignalRSerializer& Serializer, const std::vector<float>& Value) { Serializer.WriteValue(Value); }
    void SerializeComponentData(SignalRSerializer& Serializer, double Value) { Serializer.WriteValue(Value); }
    void SerializeComponentData(SignalRSerializer& Serializer, const std::string& Value) { Serializer.WriteValue(Value); }
    void SerializeComponentData(SignalRSerializer& Serializer, const std::map<uint16_t, ItemComponentData>& Value) { Serializer.WriteValue(Value); }
    void SerializeComponentData(SignalRSerializer& Serializer, const std::map<std::string, ItemComponentData>& Value)
    {
        Serializer.WriteValue(Value);
    }

    template <class T> void DeserializeComponentDataInternal(SignalRDeserializer& Deserializer, ItemComponentDataVariant& OutVal)
    {
        // It's important we construct the exact type we want to read from our variant,
        // as we want to make sure our variant is populated with the correct type.
        T DeserializedValue {};
        Deserializer.ReadValue(DeserializedValue);
        OutVal = DeserializedValue;
    }

    void DeserializeComponentData(SignalRDeserializer& Deserializer, ItemComponentDataType Type, ItemComponentDataVariant& OutVal)
    {
        switch (Type)
        {
        case ItemComponentDataType::BOOL:
            DeserializeComponentDataInternal<bool>(Deserializer, OutVal);
            break;
        case ItemComponentDataType::INT64:
            // We can't guarantee MCS will give us back a signed integer, even if one is sent.
            if (Deserializer.NextValueIsInt())
            {
                DeserializeComponentDataInternal<int64_t>(Deserializer, OutVal);
            }
            else
            {
                DeserializeComponentDataInternal<uint64_t>(Deserializer, OutVal);
            }
            break;
        case ItemComponentDataType::UINT64:
            // Due to us changing some of our types from int64->uint64, we may receive some unexpected int64 values here.
            // So we need to account for this here.
            if (Deserializer.NextValueIsUint())
            {
                DeserializeComponentDataInternal<uint64_t>(Deserializer, OutVal);
            }
            else
            {
                DeserializeComponentDataInternal<int64_t>(Deserializer, OutVal);
            }

            break;
        case ItemComponentDataType::DOUBLE:
            DeserializeComponentDataInternal<double>(Deserializer, OutVal);
            break;
        case ItemComponentDataType::FLOAT:
            DeserializeComponentDataInternal<float>(Deserializer, OutVal);
            break;
        case ItemComponentDataType::FLOAT_ARRAY:
            DeserializeComponentDataInternal<std::vector<float>>(Deserializer, OutVal);
            break;
        case ItemComponentDataType::STRING:
            DeserializeComponentDataInternal<std::string>(Deserializer, OutVal);
            break;
        case ItemComponentDataType::UINT16_DICTIONARY:
            // If a dictionary is empty, we will receive null from MCS.
            if (Deserializer.NextValueIsNull())
            {
                Deserializer.Skip();
                OutVal = std::map<uint16_t, ItemComponentData> {};
            }
            else
            {
                DeserializeComponentDataInternal<std::map<uint16_t, ItemComponentData>>(Deserializer, OutVal);
            }
            break;
        case ItemComponentDataType::STRING_DICTIONARY:
            // If a dictionary is empty, we will receive null from MCS.
            if (Deserializer.NextValueIsNull())
            {
                Deserializer.Skip();
                OutVal = std::map<std::string, ItemComponentData> {};
            }
            else
            {
                DeserializeComponentDataInternal<std::map<std::string, ItemComponentData>>(Deserializer, OutVal);
            }
            break;
        default:
            throw std::invalid_argument("Trying to deserialize unsupported ItemComponentDataType");
        }
    }
}

ItemComponentData::ItemComponentData(const ItemComponentDataVariant& Value)
    : Value { Value }
{
}

void ItemComponentData::Serialize(SignalRSerializer& Serializer) const
{
    // 1. Write an array for type-value pair.
    Serializer.StartWriteArray();
    {
        // Visit the variant to get the underlying type.
        std::visit(
            [&Serializer](const auto& ValueType)
            {
                // 2. Write the type.
                Serializer.WriteValue(static_cast<uint64_t>(GetComponentEnum(ValueType)));

                // 3. Write an array for the value.
                Serializer.StartWriteArray();
                {
                    // 4. Write the value.
                    SerializeComponentData(Serializer, ValueType);
                }
                Serializer.EndWriteArray();
            },
            Value);
    }
    Serializer.EndWriteArray();
}

void ItemComponentData::Deserialize(SignalRDeserializer& Deserializer)
{
    // 1. Read the type and value pair.
    size_t ArraySize = 0;
    Deserializer.StartReadArray(ArraySize);
    {
        // 2. Read the ItemComponentDataType.
        uint64_t RawType;
        Deserializer.ReadValue(RawType);

        const auto Type = static_cast<ItemComponentDataType>(RawType);

        // 3. Read the value inside an array.
        size_t ValueArraySize = 0;
        Deserializer.StartReadArray(ValueArraySize);
        {
            // 4. Deserialize the value.
            DeserializeComponentData(Deserializer, Type, Value);
        }
        Deserializer.EndReadArray();
    }
    Deserializer.EndReadArray();
}

const ItemComponentDataVariant& ItemComponentData::GetValue() const { return Value; }

bool ItemComponentData::operator==(const ItemComponentData& Other) const { return Value == Other.Value; }

ObjectMessage::ObjectMessage(uint64_t Id, uint64_t Type, bool IsTransferable, bool IsPersistent, uint64_t OwnerId, std::optional<uint64_t> ParentId,
    const std::map<PropertyKeyType, ItemComponentData>& Components)
    : Id { Id }
    , Type { Type }
    , IsTransferable { IsTransferable }
    , IsPersistent { IsPersistent }
    , OwnerId { OwnerId }
    , ParentId { ParentId }
    , Components { Components }
{
}

void ObjectMessage::Serialize(SignalRSerializer& Serializer) const
{
    Serializer.StartWriteArray();
    {
        Serializer.WriteValue(Id);
        Serializer.WriteValue(Type);
        Serializer.WriteValue(IsTransferable);
        Serializer.WriteValue(IsPersistent);
        Serializer.WriteValue(OwnerId);
        Serializer.WriteValue(ParentId);
        Serializer.WriteValue(Components);
    }
    Serializer.EndWriteArray();
}

void ObjectMessage::Deserialize(SignalRDeserializer& Deserializer)
{
    size_t ArraySize = 0;
    Deserializer.StartReadArray(ArraySize);
    {
        Deserializer.ReadValue(Id);
        Deserializer.ReadValue(Type);
        Deserializer.ReadValue(IsTransferable);
        Deserializer.ReadValue(IsPersistent);
        Deserializer.ReadValue(OwnerId);
        Deserializer.ReadValue(ParentId);
        Deserializer.ReadValue(Components);
    }
    Deserializer.EndReadArray();
}

bool ObjectMessage::operator==(const ObjectMessage& Other) const
{
    return Id == Other.Id && Type == Other.Type && IsTransferable == Other.IsTransferable && IsPersistent == Other.IsPersistent
        && OwnerId == Other.OwnerId && ParentId == Other.ParentId && Components == Other.Components;
}

uint64_t ObjectMessage::GetId() const { return Id; }

uint64_t ObjectMessage::GetType() const { return Type; }

bool ObjectMessage::GetIsTransferable() const { return IsTransferable; }

bool ObjectMessage::GetIsPersistent() const { return IsPersistent; }

uint64_t ObjectMessage::GetOwnerId() const { return OwnerId; }

std::optional<uint64_t> ObjectMessage::GetParentId() const { return ParentId; }

const std::optional<std::map<PropertyKeyType, ItemComponentData>>& ObjectMessage::GetComponents() const { return Components; }

ObjectPatch::ObjectPatch(uint64_t Id, uint64_t OwnerId, bool Destroy, bool ShouldUpdateParent, std::optional<uint64_t> ParentId,
    const std::map<PropertyKeyType, ItemComponentData>& Components)
    : Id { Id }
    , OwnerId { OwnerId }
    , Destroy { Destroy }
    , ShouldUpdateParent { ShouldUpdateParent }
    , ParentId { ParentId }
    , Components { Components }
{
}

void ObjectPatch::Serialize(SignalRSerializer& Serializer) const
{
    Serializer.StartWriteArray();
    {
        Serializer.WriteValue(Id);
        Serializer.WriteValue(OwnerId);
        Serializer.WriteValue(Destroy);

        // Parent changes need to be in a vector.
        Serializer.StartWriteArray();
        {
            Serializer.WriteValue(ShouldUpdateParent);
            Serializer.WriteValue(ParentId);
        }
        Serializer.EndWriteArray();

        Serializer.WriteValue(Components);
    }
    Serializer.EndWriteArray();
}

void ObjectPatch::Deserialize(SignalRDeserializer& Deserializer)
{
    size_t ArraySize = 0;
    Deserializer.StartReadArray(ArraySize);
    {
        Deserializer.ReadValue(Id);
        Deserializer.ReadValue(OwnerId);
        Deserializer.ReadValue(Destroy);

        // Array will be null from MCS if there is no parent update.
        if (Deserializer.NextValueIsNull() == false)
        {
            size_t ParentArraySize = 0;
            Deserializer.StartReadArray(ParentArraySize);
            {
                Deserializer.ReadValue(ShouldUpdateParent);
                Deserializer.ReadValue(ParentId);
            }
            Deserializer.EndReadArray();
        }

        Deserializer.ReadValue(Components);
    }
    Deserializer.EndReadArray();
}

bool ObjectPatch::operator==(const ObjectPatch& Other) const
{
    return Id == Other.Id && OwnerId == Other.OwnerId && Destroy == Other.Destroy && ShouldUpdateParent == Other.ShouldUpdateParent
        && ParentId == Other.ParentId && Components == Other.Components;
}

uint64_t ObjectPatch::GetId() const { return Id; }

uint64_t ObjectPatch::GetOwnerId() const { return OwnerId; }

bool ObjectPatch::GetDestroy() const { return Destroy; }

bool ObjectPatch::GetShouldUpdateParent() const { return ShouldUpdateParent; }

std::optional<uint64_t> ObjectPatch::GetParentId() const { return ParentId; }

const std::optional<std::map<PropertyKeyType, ItemComponentData>>& ObjectPatch::GetComponents() const { return Components; }

}
