#include "MCSComponentPacker.h"
#include "CSP/Multiplayer/ComponentBase.h"
#include "SpaceEntityKeys.h"

namespace csp::multiplayer {

MCSComponentUnpacker::MCSComponentUnpacker(const std::map<uint16_t, mcs::ItemComponentData>& Components)
    : Components{Components}
{
}

bool MCSComponentUnpacker::TryReadValue(uint16_t Key, csp::common::ReplicatedValue& Value) const
{
    auto ComponentDataIt = Components.find(Key);

    if (ComponentDataIt == Components.end())
    {
        return false;
    }

    const mcs::ItemComponentData& ComponentData = ComponentDataIt->second;
    Value = ToReplicatedValue(ComponentData);

    return true;
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

const std::map<uint16_t, mcs::ItemComponentData>& MCSComponentPacker::GetComponents() const {
    return Components;
}

csp::common::ReplicatedValue ToReplicatedValue(double) {
    throw std::runtime_error("Unsupported");
}

csp::common::ReplicatedValue ToReplicatedValue(uint64_t Value) {
    return csp::common::ReplicatedValue{static_cast<int64_t>(Value)};
}

csp::common::ReplicatedValue ToReplicatedValue(const std::string& Value) {
    return csp::common::ReplicatedValue{Value.c_str()};
}

csp::common::ReplicatedValue ToReplicatedValue(const std::vector<float>& Value)
{
    if (Value.size() == 2)
    {
        return csp::common::ReplicatedValue{csp::common::Vector2{Value[0], Value[1]}};
    }
    else if (Value.size() == 3)
    {
        return csp::common::ReplicatedValue{csp::common::Vector3{Value[0], Value[1], Value[2]}};
    }
    else if (Value.size() == 4)
    {
        return csp::common::ReplicatedValue{csp::common::Vector4{Value[0], Value[1], Value[2], Value[3]}};
    }
    else
    {
        throw std::runtime_error("Unsupported float array size.");
    }
}

csp::common::ReplicatedValue ToReplicatedValue(const mcs::ItemComponentData& Value)
{
    return std::visit([](const auto& ValueType) { return ToReplicatedValue(ValueType); }, Value.GetValue());
}

csp::common::ReplicatedValue ToReplicatedValue(const std::map<uint16_t, mcs::ItemComponentData>&) {
    throw std::runtime_error("Not yet implemented");
}

csp::common::ReplicatedValue ToReplicatedValue(const std::map<std::string, mcs::ItemComponentData>& Value)
{
    // Convert string map of ItemComponentData to csp string map of ReplicatedValue.
    csp::common::Map<csp::common::String, csp::common::ReplicatedValue> Map;

    for (const auto& Pair : Value)
    {
        Map[Pair.first.c_str()] = ToReplicatedValue(Pair.second);
    }

    return csp::common::ReplicatedValue{Map};
}

mcs::ItemComponentData ToItemComponentData(ComponentBase* Value)
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

    return mcs::ItemComponentData{ComponentPacker.GetComponents()};
}

mcs::ItemComponentData ToItemComponentData(const csp::common::ReplicatedValue& Value)
{
    mcs::ItemComponentData Data;
    // Extract the internal type from the variant and parse using its corrosponding typed CreateItemComponentData.
    std::visit([&Data, this](const auto& InternalType) { Data = CreateItemComponentData(InternalType); }, Value.GetValue());
    return Data;
}

mcs::ItemComponentData ToItemComponentData(bool Value) {
    return mcs::ItemComponentData{Value};
}

mcs::ItemComponentData ToItemComponentData(uint64_t Value) {
    return mcs::ItemComponentData{Value};
}

mcs::ItemComponentData ToItemComponentData(int64_t Value) {
    return mcs::ItemComponentData{Value};
}

mcs::ItemComponentData ToItemComponentData(float Value) {
    return mcs::ItemComponentData{Value};
}

mcs::ItemComponentData ToItemComponentData(const csp::common::String& Value) {
    return mcs::ItemComponentData{std::string{Value.c_str()}};
}

mcs::ItemComponentData ToItemComponentData(const csp::common::Vector3& Value)
{
    return mcs::ItemComponentData{std::vector<float> { Value.X, Value.Y, Value.Z }};
}

mcs::ItemComponentData ToItemComponentData(const csp::common::Vector4& Value)
{
    return mcs::ItemComponentData{std::vector<float> { Value.X, Value.Y, Value.Z, Value.W }};
}

mcs::ItemComponentData ToItemComponentData(const csp::common::Vector2& Value)
{
    return mcs::ItemComponentData{std::vector<float> { Value.X, Value.Y }};
}

mcs::ItemComponentData ToItemComponentData(const csp::common::Map<csp::common::String, csp::common::ReplicatedValue>& Value)
{
    std::map<std::string, mcs::ItemComponentData> Map;
    std::unique_ptr<common::Array<csp::common::String>> Keys(const_cast<common::Array<csp::common::String>*>(Value.Keys()));

    for (auto Key : (*Keys))
    {
        Map[Key.c_str()] = ToItemComponentData(Value[Key]);
    }

    return mcs::ItemComponentData{Map};
}

}
