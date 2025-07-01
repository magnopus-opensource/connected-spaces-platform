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

#include "CSP/Common/Map.h"
#include "CSP/Common/ReplicatedValue.h"
#include "CSP/Common/String.h"
#include "CSP/Common/Vector.h"
#include "MCS/MCSTypes.h"

#include <map>

namespace csp::multiplayer
{
class ComponentBase;

/// @brief Helper class to convert csp domain types to mcs ItemComponentData.
/// @details Builds a component map compatible with mcs::ObjectMessage and mcs::ObjectPatch.
class MCSComponentPacker
{
public:
    template <class T> void WriteValue(uint16_t Key, const T& Value);

    const std::map<uint16_t, mcs::ItemComponentData>& GetComponents() const;

private:
    mcs::ItemComponentData CreateItemComponentData(ComponentBase* Value);
    mcs::ItemComponentData CreateItemComponentData(const csp::common::ReplicatedValue& Value);
    mcs::ItemComponentData CreateItemComponentData(bool Value);
    mcs::ItemComponentData CreateItemComponentData(uint64_t Value);
    mcs::ItemComponentData CreateItemComponentData(int64_t Value);
    mcs::ItemComponentData CreateItemComponentData(float Value);
    mcs::ItemComponentData CreateItemComponentData(const csp::common::String& Value);
    mcs::ItemComponentData CreateItemComponentData(const csp::common::Vector3& Value);
    mcs::ItemComponentData CreateItemComponentData(const csp::common::Vector4& Value);
    mcs::ItemComponentData CreateItemComponentData(const csp::common::Vector2& Value);
    mcs::ItemComponentData CreateItemComponentData(const csp::common::Map<csp::common::String, csp::common::ReplicatedValue>& Value);

    // Case for enums
    template <typename T> std::enable_if_t<std::is_enum_v<T>, mcs::ItemComponentData> CreateItemComponentData(T Value);

    std::map<uint16_t, mcs::ItemComponentData> Components;
};

/// @brief Helper class to convert mcs domain types to csp types.
/// @details Reads value from a components maps retrieved from a mcs::ObjectMessage or mcs::ObjectPatch.
class MCSComponentUnpacker
{
public:
    MCSComponentUnpacker(const std::map<uint16_t, mcs::ItemComponentData>& Components);

    template <typename T> bool TryReadValue(uint16_t Key, T& Value) const;

    // Gets the count of all csp runtime components, excluding view components.
    uint64_t GetRuntimeComponentsCount() const;

    // Primitive types can be converted without changes
    template <class T> static void CreateReplicatedValueFromType(const T& Type, csp::common::ReplicatedValue& Value)
    {
        Value = csp::common::ReplicatedValue(Type);
    }

    static void CreateReplicatedValueFromType(const std::vector<float>& Type, csp::common::ReplicatedValue& Value);
    static void CreateReplicatedValueFromType(uint64_t, csp::common::ReplicatedValue&);
    static void CreateReplicatedValueFromType(double, csp::common::ReplicatedValue&);
    static void CreateReplicatedValueFromType(const std::string& Type, csp::common::ReplicatedValue& Value);
    static void CreateReplicatedValueFromType(const std::map<uint16_t, mcs::ItemComponentData>&, csp::common::ReplicatedValue&);
    static void CreateReplicatedValueFromType(const std::map<std::string, mcs::ItemComponentData>& Type, csp::common::ReplicatedValue& Value);
    static void CreateReplicatedValueFromType(const mcs::ItemComponentData& ComponentData, csp::common::ReplicatedValue& Value);

private:
    template <typename T> void ReadValue(uint16_t Key, T& Value) const;

    static void ReadValue(const mcs::ItemComponentData& ComponentData, uint64_t& Value);
    static void ReadValue(const mcs::ItemComponentData& ComponentData, int64_t& Value);
    static void ReadValue(const mcs::ItemComponentData& ComponentData, csp::common::Vector2& Value);
    static void ReadValue(const mcs::ItemComponentData& ComponentData, csp::common::Vector3& Value);
    static void ReadValue(const mcs::ItemComponentData& ComponentData, csp::common::Vector4& Value);
    static void ReadValue(const mcs::ItemComponentData& ComponentData, csp::common::String& Value);
    static void ReadValue(const mcs::ItemComponentData& ComponentData, csp::common::ReplicatedValue& Value);

    template <typename T> std::enable_if_t<std::is_enum_v<T>> ReadValue(const mcs::ItemComponentData& ComponentData, T& Value) const;

    std::map<uint16_t, mcs::ItemComponentData> Components;
};

template <class T> inline void MCSComponentPacker::WriteValue(uint16_t Key, const T& Value) { Components[Key] = CreateItemComponentData(Value); }

template <typename T> std::enable_if_t<std::is_enum_v<T>, mcs::ItemComponentData> MCSComponentPacker::CreateItemComponentData(T Value)
{
    return CreateItemComponentData(static_cast<uint64_t>(Value));
}

template <typename T> inline bool MCSComponentUnpacker::TryReadValue(uint16_t Key, T& Value) const
{
    if (Components.find(Key) == Components.end())
    {
        return false;
    }

    ReadValue(Key, Value);
    return true;
}

template <typename T> inline void MCSComponentUnpacker::ReadValue(uint16_t Key, T& Value) const
{
    const mcs::ItemComponentData& ComponentData = Components.find(Key)->second;
    ReadValue(ComponentData, Value);
}

template <typename T>
inline std::enable_if_t<std::is_enum_v<T>> MCSComponentUnpacker::ReadValue(const mcs::ItemComponentData& ComponentData, T& Value) const
{
    uint64_t RawEnumValue = 0;
    ReadValue(ComponentData, RawEnumValue);
    Value = static_cast<T>(RawEnumValue);
}

}
