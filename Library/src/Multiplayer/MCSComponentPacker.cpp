#include "MCSComponentPacker.h"
#include "CSP/Multiplayer/ComponentBase.h"
#include "SpaceEntityKeys.h"

namespace csp::multiplayer
{

MCSComponentUnpacker::MCSComponentUnpacker(const std::map<uint16_t, mcs::ItemComponentData>& components)
    : m_components { components }
{
}

bool MCSComponentUnpacker::TryReadValue(uint16_t key, csp::common::ReplicatedValue& value) const
{
    auto componentDataIt = m_components.find(key);

    if (componentDataIt == m_components.end())
    {
        return false;
    }

    const mcs::ItemComponentData& componentData = componentDataIt->second;
    value = ToReplicatedValue(componentData);

    return true;
}

uint64_t MCSComponentUnpacker::GetRuntimeComponentsCount() const
{
    uint64_t componentCount = 0;

    for (const auto& [Key, Component] : m_components)
    {
        if (Key < COMPONENT_KEY_END_COMPONENTS)
        {
            ++componentCount;
        }
    }

    return componentCount;
}

const std::map<uint16_t, mcs::ItemComponentData>& MCSComponentPacker::GetComponents() const { return m_components; }

csp::common::ReplicatedValue ToReplicatedValue(double) { throw std::runtime_error("Unsupported"); }

csp::common::ReplicatedValue ToReplicatedValue(uint64_t value) { return csp::common::ReplicatedValue { static_cast<int64_t>(value) }; }

csp::common::ReplicatedValue ToReplicatedValue(const std::string& value) { return csp::common::ReplicatedValue { value.c_str() }; }

csp::common::ReplicatedValue ToReplicatedValue(const std::vector<float>& value)
{
    if (value.size() == 2)
    {
        return csp::common::ReplicatedValue { csp::common::Vector2 { value[0], value[1] } };
    }
    else if (value.size() == 3)
    {
        return csp::common::ReplicatedValue { csp::common::Vector3 { value[0], value[1], value[2] } };
    }
    else if (value.size() == 4)
    {
        return csp::common::ReplicatedValue { csp::common::Vector4 { value[0], value[1], value[2], value[3] } };
    }
    else
    {
        throw std::runtime_error("Unsupported float array size.");
    }
}

csp::common::ReplicatedValue ToReplicatedValue(const mcs::ItemComponentData& value)
{
    return std::visit([](const auto& valueType) { return ToReplicatedValue(valueType); }, value.GetValue());
}

csp::common::ReplicatedValue ToReplicatedValue(const std::map<uint16_t, mcs::ItemComponentData>&) { throw std::runtime_error("Not yet implemented"); }

csp::common::ReplicatedValue ToReplicatedValue(const std::map<std::string, mcs::ItemComponentData>& value)
{
    // Convert string map of ItemComponentData to csp string map of ReplicatedValue.
    csp::common::Map<csp::common::String, csp::common::ReplicatedValue> map;

    for (const auto& pair : value)
    {
        map[pair.first.c_str()] = ToReplicatedValue(pair.second);
    }

    return csp::common::ReplicatedValue { map };
}

mcs::ItemComponentData ToItemComponentData(ComponentBase* value)
{
    // Create a nested map to represent the component properties.
    MCSComponentPacker componentPacker;

    // Manually write the component type, as this isn't stored in the component properties.
    // This is currently the ONLY value that uses a uint64 types as a key for some reason. The rest use int64.
    componentPacker.WriteValue(COMPONENT_KEY_COMPONENTTYPE, static_cast<uint64_t>(value->GetComponentType()));

    // Our current component keys are stores as uint32s when they should really be stored as uint16, as this is what we support.
    std::unique_ptr<common::Array<uint32_t>> keys(const_cast<common::Array<uint32_t>*>(value->GetProperties()->Keys()));

    for (uint32_t key : *keys)
    {
        componentPacker.WriteValue(static_cast<uint16_t>(key), (*value->GetProperties())[static_cast<uint32_t>(key)]);
    }

    return mcs::ItemComponentData { componentPacker.GetComponents() };
}

mcs::ItemComponentData ToItemComponentData(const csp::common::ReplicatedValue& value)
{
    mcs::ItemComponentData data;
    // Extract the internal type from the variant and parse using its corrosponding typed CreateItemComponentData.
    std::visit([&data](const auto& internalType) { data = ToItemComponentData(internalType); }, value.GetValue());
    return data;
}

mcs::ItemComponentData ToItemComponentData(bool value) { return mcs::ItemComponentData { value }; }

mcs::ItemComponentData ToItemComponentData(uint64_t value) { return mcs::ItemComponentData { value }; }

mcs::ItemComponentData ToItemComponentData(int64_t value) { return mcs::ItemComponentData { value }; }

mcs::ItemComponentData ToItemComponentData(float value) { return mcs::ItemComponentData { value }; }

mcs::ItemComponentData ToItemComponentData(const csp::common::String& value) { return mcs::ItemComponentData { std::string { value.c_str() } }; }

mcs::ItemComponentData ToItemComponentData(const csp::common::Vector3& value)
{
    return mcs::ItemComponentData { std::vector<float> { value.X, value.Y, value.Z } };
}

mcs::ItemComponentData ToItemComponentData(const csp::common::Vector4& value)
{
    return mcs::ItemComponentData { std::vector<float> { value.X, value.Y, value.Z, value.W } };
}

mcs::ItemComponentData ToItemComponentData(const csp::common::Vector2& value)
{
    return mcs::ItemComponentData { std::vector<float> { value.X, value.Y } };
}

mcs::ItemComponentData ToItemComponentData(const csp::common::Map<csp::common::String, csp::common::ReplicatedValue>& value)
{
    std::map<std::string, mcs::ItemComponentData> map;
    std::unique_ptr<common::Array<csp::common::String>> keys(const_cast<common::Array<csp::common::String>*>(value.Keys()));

    for (auto key : (*keys))
    {
        map[key.c_str()] = ToItemComponentData(value[key]);
    }

    return mcs::ItemComponentData { map };
}

}
