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
#include "Multiplayer/SpaceEntityKeys.h"

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
    template <class T> void WriteValue(SpaceEntityComponentKey Key, const T& Value);

    const std::map<uint16_t, mcs::ItemComponentData>& GetComponents() const;

private:
    std::map<uint16_t, mcs::ItemComponentData> Components;
};

/// @brief Helper class to convert mcs domain types to csp types.
/// @details Reads value from a components maps retrieved from a mcs::ObjectMessage or mcs::ObjectPatch.
class MCSComponentUnpacker
{
public:
    MCSComponentUnpacker(const std::map<uint16_t, mcs::ItemComponentData>& Components);

    bool TryReadValue(uint16_t Key, csp::common::ReplicatedValue& Value) const;

    // Gets the count of all csp runtime components, excluding view components.
    uint64_t GetRuntimeComponentsCount() const;

private:
    std::map<uint16_t, mcs::ItemComponentData> Components;
};

template <class T> inline void MCSComponentPacker::WriteValue(uint16_t Key, const T& Value) { Components[Key] = ToItemComponentData(Value); }
template <class T> inline void MCSComponentPacker::WriteValue(SpaceEntityComponentKey Key, const T& Value)
{
    WriteValue(static_cast<uint16_t>(Key), Value);
}

template <typename T> csp::common::ReplicatedValue ToReplicatedValue(const T& Value) { return csp::common::ReplicatedValue { Value }; }

template <typename T> std::enable_if_t<std::is_enum_v<T>, csp::common::ReplicatedValue> inline ToReplicatedValue(const T& Value)
{
    return csp::common::ReplicatedValue { static_cast<int64_t>(Value) };
}

csp::common::ReplicatedValue ToReplicatedValue(double);
csp::common::ReplicatedValue ToReplicatedValue(uint64_t Value);
csp::common::ReplicatedValue ToReplicatedValue(const std::string& Value);
csp::common::ReplicatedValue ToReplicatedValue(const std::vector<float>& Value);
csp::common::ReplicatedValue ToReplicatedValue(const mcs::ItemComponentData& Value);
csp::common::ReplicatedValue ToReplicatedValue(const std::map<uint16_t, mcs::ItemComponentData>&);
csp::common::ReplicatedValue ToReplicatedValue(const std::map<std::string, mcs::ItemComponentData>& Value);

template <typename T> std::enable_if_t<std::is_enum_v<T>, mcs::ItemComponentData> ToItemComponentData(T Value)
{
    return ToItemComponentData(static_cast<uint64_t>(Value));
}

mcs::ItemComponentData ToItemComponentData(ComponentBase* Value);
mcs::ItemComponentData ToItemComponentData(const csp::common::ReplicatedValue& Value);
mcs::ItemComponentData ToItemComponentData(bool Value);
mcs::ItemComponentData ToItemComponentData(uint64_t Value);
mcs::ItemComponentData ToItemComponentData(int64_t Value);
mcs::ItemComponentData ToItemComponentData(float Value);
mcs::ItemComponentData ToItemComponentData(const csp::common::String& Value);
mcs::ItemComponentData ToItemComponentData(const csp::common::Vector3& Value);
mcs::ItemComponentData ToItemComponentData(const csp::common::Vector4& Value);
mcs::ItemComponentData ToItemComponentData(const csp::common::Vector2& Value);
mcs::ItemComponentData ToItemComponentData(const csp::common::Map<csp::common::String, csp::common::ReplicatedValue>& Value);
}
