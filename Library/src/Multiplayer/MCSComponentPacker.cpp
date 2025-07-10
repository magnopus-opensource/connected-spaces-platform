#include "MCSComponentPacker.h"
#include "CSP/Multiplayer/ComponentBase.h"
#include "SpaceEntityKeys.h"

namespace csp::multiplayer
{

MCSComponentUnpacker::MCSComponentUnpacker(const std::map<uint16_t, mcs::ItemComponentData>& Components)
    : Components { Components }
{
}

uint64_t MCSComponentUnpacker::GetRuntimeComponentsCount() const
{
    uint64_t ComponentCount = 0;

    for (const auto& [Key, Component] : Components)
    {
        if (Key < COMPONENT_KEY_END_COMPONENTS)
        {
            ++ComponentCount;
        }
    }

    return ComponentCount;
}

void MCSComponentUnpacker::ReadValue(const mcs::ItemComponentData& ComponentData, uint64_t& Value)
{
    /*
        These checks are used to internally convert between integer types,
        as the type we want to read into may not be the exact integer type that exists in the variant, which will cause a crash.
        This is largly due to a quirk with the way MCS deserializes signed integer types, as they sometimes come back as unsigned.
        It also enforces backwards compatibility if we ever change integer types.
    */
    if (std::holds_alternative<uint64_t>(ComponentData.GetValue()))
    {
        Value = std::get<uint64_t>(ComponentData.GetValue());
    }
    else if (std::holds_alternative<int64_t>(ComponentData.GetValue()))
    {
        Value = static_cast<uint64_t>(std::get<int64_t>(ComponentData.GetValue()));
    }
    else
    {
        throw std::runtime_error("Invalid Integer Type");
    }
}

void MCSComponentUnpacker::ReadValue(const mcs::ItemComponentData& ComponentData, int64_t& Value)
{
    /*
        These checks are used to internally convert between integer types,
        as the type we want to read into may not be the exact integer type that exists in the variant, which will cause a crash.
        This is largly due to a quirk with the way MCS deserializes signed integer types, as they sometimes come back as unsigned.
        It also enforces backwards compatibility if we ever change integer types.
    */
    if (std::holds_alternative<int64_t>(ComponentData.GetValue()))
    {
        Value = std::get<int64_t>(ComponentData.GetValue());
    }
    else if (std::holds_alternative<uint64_t>(ComponentData.GetValue()))
    {
        Value = static_cast<int64_t>(std::get<uint64_t>(ComponentData.GetValue()));
    }
    else
    {
        throw std::runtime_error("Invalid Integer Type");
    }
}

void MCSComponentUnpacker::ReadValue(const mcs::ItemComponentData& ComponentData, csp::common::Vector2& Value)
{
    const auto& Vector = std::get<std::vector<float>>(ComponentData.GetValue());
    Value = common::Vector2 { Vector[0], Vector[1] };
}
void MCSComponentUnpacker::ReadValue(const mcs::ItemComponentData& ComponentData, csp::common::Vector3& Value)
{
    const auto& Vector = std::get<std::vector<float>>(ComponentData.GetValue());
    Value = common::Vector3 { Vector[0], Vector[1], Vector[2] };
}
void MCSComponentUnpacker::ReadValue(const mcs::ItemComponentData& ComponentData, csp::common::Vector4& Value)
{
    const auto& Vector = std::get<std::vector<float>>(ComponentData.GetValue());
    Value = common::Vector4 { Vector[0], Vector[1], Vector[2], Vector[3] };
}
void MCSComponentUnpacker::ReadValue(const mcs::ItemComponentData& ComponentData, csp::common::String& Value)
{
    const auto& String = std::get<std::string>(ComponentData.GetValue());
    Value = String.c_str();
}

void MCSComponentUnpacker::ReadValue(const mcs::ItemComponentData& ComponentData, csp::common::ReplicatedValue& Value)
{
    std::visit([&Value](const auto& ValueType) { CreateReplicatedValueFromType(ValueType, Value); }, ComponentData.GetValue());
}

void MCSComponentUnpacker::CreateReplicatedValueFromType(const std::vector<float>& Type, csp::common::ReplicatedValue& Value)
{
    if (Type.size() == 2)
    {
        Value = csp::common::ReplicatedValue { csp::common::Vector2 { Type[0], Type[1] } };
    }
    else if (Type.size() == 3)
    {
        Value = csp::common::ReplicatedValue { csp::common::Vector3 { Type[0], Type[1], Type[2] } };
    }
    else if (Type.size() == 4)
    {
        Value = csp::common::ReplicatedValue { csp::common::Vector4 { Type[0], Type[1], Type[2], Type[3] } };
    }
    else
    {
        throw std::runtime_error("Unsupported float array.");
    }
}

void MCSComponentUnpacker::CreateReplicatedValueFromType(uint64_t Type, csp::common::ReplicatedValue& Value)
{
    // ReplicatedValue only supported signed integers.
    CreateReplicatedValueFromType(static_cast<int64_t>(Type), Value);
}

void MCSComponentUnpacker::CreateReplicatedValueFromType(double, csp::common::ReplicatedValue&) { throw std::runtime_error("Unsupported"); }

void MCSComponentUnpacker::CreateReplicatedValueFromType(const std::string& Type, csp::common::ReplicatedValue& Value)
{
    Value = csp::common::ReplicatedValue { csp::common::String { Type.c_str() } };
}

void MCSComponentUnpacker::CreateReplicatedValueFromType(const std::map<uint16_t, mcs::ItemComponentData>&, csp::common::ReplicatedValue&)
{
    throw std::runtime_error("Unsupported");
}

void MCSComponentUnpacker::CreateReplicatedValueFromType(
    const std::map<std::string, mcs::ItemComponentData>& Type, csp::common::ReplicatedValue& Value)
{
    // Convert string map of ItemComponentData to csp string map of ReplicatedValue.
    csp::common::Map<csp::common::String, csp::common::ReplicatedValue> Map;

    for (const auto& Pair : Type)
    {
        csp::common::ReplicatedValue ChildValue;
        CreateReplicatedValueFromType(Pair.second, ChildValue);
        Map[Pair.first.c_str()] = ChildValue;
    }

    CreateReplicatedValueFromType(Map, Value);
}

void MCSComponentUnpacker::CreateReplicatedValueFromType(const mcs::ItemComponentData& ComponentData, csp::common::ReplicatedValue& Value)
{
    std::visit([&Value](const auto& ValueType) { CreateReplicatedValueFromType(ValueType, Value); }, ComponentData.GetValue());
}

mcs::ItemComponentData MCSComponentPacker::CreateItemComponentData(ComponentBase* Value)
{
    // Create a nested map to represent the component properties.
    MCSComponentPacker ComponentPacker;

    // Manually write the component type, as this isn't stored in the component properties.
    // This is currently the ONLY value that uses a uint64 types as a key for some reason. The rest use int64.
    ComponentPacker.WriteValue(COMPONENT_KEY_COMPONENTTYPE, static_cast<uint64_t>(Value->GetComponentType()));

    // Our current component keys are stores as uint32s when they should really be stored as uint16, as this is what we support.
    std::unique_ptr<common::Array<uint32_t>> Keys(const_cast<common::Array<uint32_t>*>(Value->GetProperties()->Keys()));

    for (uint32_t Key : *Keys)
    {
        ComponentPacker.WriteValue(static_cast<uint16_t>(Key), (*Value->GetProperties())[static_cast<uint32_t>(Key)]);
    }

    return mcs::ItemComponentData { ComponentPacker.GetComponents() };
}

// TODO: We can make a safer version of this function when we convert our ReplicatedValue to use a variant,
// as we can create compile-time checking by using std::visit and function overloads.
// This will prevent us forgetting to update this when we add new types.
// https://magnopus.atlassian.net/browse/OF-1511
mcs::ItemComponentData MCSComponentPacker::CreateItemComponentData(const csp::common::ReplicatedValue& Value)
{
    if (Value.GetReplicatedValueType() == csp::common::ReplicatedValueType::Boolean)
    {
        return CreateItemComponentData(Value.GetBool());
    }
    else if (Value.GetReplicatedValueType() == csp::common::ReplicatedValueType::Integer)
    {
        return CreateItemComponentData(Value.GetInt());
    }
    else if (Value.GetReplicatedValueType() == csp::common::ReplicatedValueType::Float)
    {
        return CreateItemComponentData(Value.GetFloat());
    }
    else if (Value.GetReplicatedValueType() == csp::common::ReplicatedValueType::String)
    {
        return CreateItemComponentData(Value.GetString());
    }
    else if (Value.GetReplicatedValueType() == csp::common::ReplicatedValueType::Vector3)
    {
        return CreateItemComponentData(Value.GetVector3());
    }
    else if (Value.GetReplicatedValueType() == csp::common::ReplicatedValueType::Vector4)
    {
        return CreateItemComponentData(Value.GetVector4());
    }
    else if (Value.GetReplicatedValueType() == csp::common::ReplicatedValueType::Vector2)
    {
        return CreateItemComponentData(Value.GetVector2());
    }
    else if (Value.GetReplicatedValueType() == csp::common::ReplicatedValueType::StringMap)
    {
        return CreateItemComponentData(Value.GetStringMap());
    }
    else
    {
        throw std::runtime_error("Invalid ReplicatedValue property");
    }
}

mcs::ItemComponentData MCSComponentPacker::CreateItemComponentData(bool Value) { return mcs::ItemComponentData { Value }; }

mcs::ItemComponentData MCSComponentPacker::CreateItemComponentData(uint64_t Value) { return mcs::ItemComponentData { Value }; }

mcs::ItemComponentData MCSComponentPacker::CreateItemComponentData(int64_t Value) { return mcs::ItemComponentData { Value }; }

mcs::ItemComponentData MCSComponentPacker::CreateItemComponentData(float Value) { return mcs::ItemComponentData { Value }; }

mcs::ItemComponentData MCSComponentPacker::CreateItemComponentData(const csp::common::String& Value)
{
    return mcs::ItemComponentData { std::string { Value.c_str() } };
}

mcs::ItemComponentData MCSComponentPacker::CreateItemComponentData(const csp::common::Vector3& Value)
{
    return mcs::ItemComponentData { std::vector<float> { Value.X, Value.Y, Value.Z } };
}

mcs::ItemComponentData MCSComponentPacker::CreateItemComponentData(const csp::common::Vector4& Value)
{
    return mcs::ItemComponentData { std::vector<float> { Value.X, Value.Y, Value.Z, Value.W } };
}

mcs::ItemComponentData MCSComponentPacker::CreateItemComponentData(const csp::common::Vector2& Value)
{
    return mcs::ItemComponentData { std::vector<float> { Value.X, Value.Y } };
}

mcs::ItemComponentData MCSComponentPacker::CreateItemComponentData(const csp::common::Map<csp::common::String, csp::common::ReplicatedValue>& Value)
{
    std::map<std::string, mcs::ItemComponentData> Map;
    std::unique_ptr<common::Array<csp::common::String>> Keys(const_cast<common::Array<csp::common::String>*>(Value.Keys()));

    for (auto Key : (*Keys))
    {
        Map[Key.c_str()] = CreateItemComponentData(Value[Key]);
    }

    return mcs::ItemComponentData { Map };
}

const std::map<uint16_t, mcs::ItemComponentData>& MCSComponentPacker::GetComponents() const { return Components; }

}
