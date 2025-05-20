#include "MCSComponentPacker.h"
#include "CSP/Multiplayer/ComponentBase.h"
#include "Memory/Memory.h"
#include "SpaceEntityKeys.h"

namespace csp::multiplayer
{

MCSComponentUnpacker::MCSComponentUnpacker(const std::map<uint16_t, mcs::ItemComponentData>& Components)
    : Components { Components }
{
}

uint64_t MCSComponentUnpacker::GetRealComponentsCount() const
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
    Value = static_cast<uint64_t>(std::get<int64_t>(ComponentData.GetValue()));
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
    std::visit([&Value](const auto& ValueType) { ReplicatedValueFromType(ValueType, Value); }, ComponentData.GetValue());
}

void MCSComponentUnpacker::ReplicatedValueFromType(const std::vector<float>& Type, ReplicatedValue& Value)
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

void MCSComponentUnpacker::ReplicatedValueFromType(uint64_t, ReplicatedValue&) { throw std::runtime_error("Unsupported"); }

void MCSComponentUnpacker::ReplicatedValueFromType(double, ReplicatedValue&) { throw std::runtime_error("Unsupported"); }

void MCSComponentUnpacker::ReplicatedValueFromType(const std::string& Type, ReplicatedValue& Value)
{
    Value = ReplicatedValue { csp::common::String { Type.c_str() } };
}

void MCSComponentUnpacker::ReplicatedValueFromType(const std::map<uint16_t, mcs::ItemComponentData>&, ReplicatedValue&)
{
    throw std::runtime_error("Unsupported");
}

void MCSComponentUnpacker::ReplicatedValueFromType(const std::map<std::string, mcs::ItemComponentData>& Type, ReplicatedValue& Value)
{
    // Convert string map of ItemComponentData to csp string map of ReplicatedValue.
    csp::common::Map<csp::common::String, ReplicatedValue> Map;

    for (const auto& Pair : Type)
    {
        ReplicatedValue ChildValue;
        ReplicatedValueFromType(Pair.second, ChildValue);
        Map[Pair.first.c_str()] = ChildValue;
    }

    ReplicatedValueFromType(Map, Value);
}

void MCSComponentUnpacker::ReplicatedValueFromType(const mcs::ItemComponentData& ComponentData, ReplicatedValue& Value)
{
    std::visit([&Value](const auto& ValueType) { ReplicatedValueFromType(ValueType, Value); }, ComponentData.GetValue());
}

mcs::ItemComponentData MCSComponentPacker::CreateItemComponentData(ComponentBase* Value)
{
    // Create a nested map to represent the component properties.
    MCSComponentPacker ComponentPacker;
    std::map<uint16_t, mcs::ItemComponentData> ComponentProperties;

    // Manually write the component type, as this isn't stored in the component properties.
    // This is currently the ONLY value that uses a uint64 types as a key for some reason. The rest use int64.
    ComponentPacker.WriteValue(COMPONENT_KEY_COMPONENTTYPE, static_cast<uint64_t>(Value->GetComponentType()));

    // Our current component keys are stores as uint32s when they should really be stored as uint16, as this is what we support.
    auto Deleter = [](const common::Array<uint32_t>* Ptr) { CSP_DELETE(Ptr); };
    std::unique_ptr<common::Array<uint32_t>, decltype(Deleter)> Keys(const_cast<common::Array<uint32_t>*>(Value->GetProperties()->Keys()), Deleter);

    for (size_t i = 0; i < Keys->Size(); ++i)
    {
        ComponentPacker.WriteValue(static_cast<uint16_t>((*Keys)[i]), (*Value->GetProperties())[static_cast<uint32_t>((*Keys)[i])]);
    }

    return mcs::ItemComponentData { ComponentProperties };
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
        return CreateItemComponentData(Value.GetDefaultVector3());
    }
    else if (Value.GetReplicatedValueType() == ReplicatedValueType::Vector4)
    {
        return CreateItemComponentData(Value.GetDefaultVector4());
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

mcs::ItemComponentData MCSComponentPacker::CreateItemComponentData(const csp::common::String& Value) { return std::string { Value.c_str() }; }

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

    auto Deleter = [](const common::Array<csp::common::String>* Ptr) { CSP_DELETE(Ptr); };
    std::unique_ptr<common::Array<csp::common::String>, decltype(Deleter)> Keys(
        const_cast<common::Array<csp::common::String>*>(Value.Keys()), Deleter);

    for (auto Key : (*Keys))
    {
        Map[Key.c_str()] = CreateItemComponentData(Value[Key]);
    }

    return mcs::ItemComponentData { Map };
}

const std::map<uint16_t, mcs::ItemComponentData>& MCSComponentPacker::GetComponents() const { return Components; }

}
