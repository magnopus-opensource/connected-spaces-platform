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
    Value = std::get<uint64_t>(ComponentData.GetValue());
}

void MCSComponentUnpacker::ReadValue(const mcs::ItemComponentData& ComponentData, int64_t& Value)
{
    Value = std::get<int64_t>(ComponentData.GetValue());
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

void MCSComponentUnpacker::ReadValue(const mcs::ItemComponentData& ComponentData, ReplicatedValue& Value)
{
    std::visit([&Value](const auto& ValueType) { CreateReplicatedValueFromType(ValueType, Value); }, ComponentData.GetValue());
}

void MCSComponentUnpacker::CreateReplicatedValueFromType(const std::vector<float>& Type, ReplicatedValue& Value)
{
    if (Type.size() == 2)
    {
        Value = ReplicatedValue { csp::common::Vector2 { Type[0], Type[1] } };
    }
    else if (Type.size() == 3)
    {
        Value = ReplicatedValue { csp::common::Vector3 { Type[0], Type[1], Type[2] } };
    }
    else if (Type.size() == 4)
    {
        Value = ReplicatedValue { csp::common::Vector4 { Type[0], Type[1], Type[2], Type[3] } };
    }
    else
    {
        throw std::runtime_error("Unsupported float array.");
    }
}

void MCSComponentUnpacker::CreateReplicatedValueFromType(uint64_t Type, ReplicatedValue& Value)
{
    // ReplicatedValue only supported signed integers.
    CreateReplicatedValueFromType(static_cast<int64_t>(Type), Value);
}

void MCSComponentUnpacker::CreateReplicatedValueFromType(double, ReplicatedValue&) { throw std::runtime_error("Unsupported"); }

void MCSComponentUnpacker::CreateReplicatedValueFromType(const std::string& Type, ReplicatedValue& Value)
{
    Value = ReplicatedValue { csp::common::String { Type.c_str() } };
}

void MCSComponentUnpacker::CreateReplicatedValueFromType(const std::map<uint16_t, mcs::ItemComponentData>&, ReplicatedValue&)
{
    throw std::runtime_error("Unsupported");
}

void MCSComponentUnpacker::CreateReplicatedValueFromType(const std::map<std::string, mcs::ItemComponentData>& Type, ReplicatedValue& Value)
{
    // Convert string map of ItemComponentData to csp string map of ReplicatedValue.
    csp::common::Map<csp::common::String, ReplicatedValue> Map;

    for (const auto& Pair : Type)
    {
        ReplicatedValue ChildValue;
        CreateReplicatedValueFromType(Pair.second, ChildValue);
        Map[Pair.first.c_str()] = ChildValue;
    }

    CreateReplicatedValueFromType(Map, Value);
}

void MCSComponentUnpacker::CreateReplicatedValueFromType(const mcs::ItemComponentData& ComponentData, ReplicatedValue& Value)
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
mcs::ItemComponentData MCSComponentPacker::CreateItemComponentData(const ReplicatedValue& Value)
{
    if (Value.GetReplicatedValueType() == ReplicatedValueType::Boolean)
    {
        return CreateItemComponentData(Value.GetBool());
    }
    else if (Value.GetReplicatedValueType() == ReplicatedValueType::Integer)
    {
        return CreateItemComponentData(Value.GetInt());
    }
    else if (Value.GetReplicatedValueType() == ReplicatedValueType::Float)
    {
        return CreateItemComponentData(Value.GetFloat());
    }
    else if (Value.GetReplicatedValueType() == ReplicatedValueType::String)
    {
        return CreateItemComponentData(Value.GetString());
    }
    else if (Value.GetReplicatedValueType() == ReplicatedValueType::Vector3)
    {
        return CreateItemComponentData(Value.GetVector3());
    }
    else if (Value.GetReplicatedValueType() == ReplicatedValueType::Vector4)
    {
        return CreateItemComponentData(Value.GetVector4());
    }
    else if (Value.GetReplicatedValueType() == ReplicatedValueType::Vector2)
    {
        return CreateItemComponentData(Value.GetVector2());
    }
    else if (Value.GetReplicatedValueType() == ReplicatedValueType::StringMap)
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

mcs::ItemComponentData MCSComponentPacker::CreateItemComponentData(const csp::common::Map<csp::common::String, ReplicatedValue>& Value)
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
