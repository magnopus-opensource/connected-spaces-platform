/*
 * Copyright 2026 Magnopus LLC

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

#include "Multiplayer/ComponentSchema.h"

#include <algorithm>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

namespace csp::multiplayer
{

template <typename T> struct ScriptTypeMap;

template <typename T> using ScriptTypeT = typename ScriptTypeMap<T>::Type;

template <typename T, typename = void> inline constexpr bool IsScriptableV = false;

template <typename T> inline constexpr bool IsScriptableV<T, std::void_t<ScriptTypeT<T>>> = true;

template <> struct ScriptTypeMap<bool>
{
    using Type = bool;
};

template <> struct ScriptTypeMap<float>
{
    using Type = float;
};

template <> struct ScriptTypeMap<int64_t>
{
    using Type = int64_t;
};

template <> struct ScriptTypeMap<std::string>
{
    using Type = std::string;
};

template <> struct ScriptTypeMap<std::unordered_map<std::string, std::string>>
{
    using Type = std::unordered_map<std::string, std::string>;
};

template <> struct ScriptTypeMap<csp::common::Vector2>
{
    using Type = std::vector<float>;
};

template <> struct ScriptTypeMap<csp::common::Vector3>
{
    using Type = std::vector<float>;
};

template <> struct ScriptTypeMap<csp::common::Vector4>
{
    using Type = std::vector<float>;
};

template <typename T> struct ReplicatedTypeMap;

template <> struct ReplicatedTypeMap<bool>
{
    static constexpr auto ValueType = csp::common::ReplicatedValueType::Boolean;
};

template <> struct ReplicatedTypeMap<int64_t>
{
    static constexpr auto ValueType = csp::common::ReplicatedValueType::Integer;

    static std::optional<int64_t> From(const csp::common::ReplicatedValue& Value)
    {
        const auto* ActualValue = std::get_if<int64_t>(&Value.GetValue());

        return ActualValue != nullptr ? std::optional<int64_t> { *ActualValue } : std::nullopt;
    }
};

template <> struct ReplicatedTypeMap<float>
{
    static constexpr auto ValueType = csp::common::ReplicatedValueType::Float;

    static std::optional<float> From(const csp::common::ReplicatedValue& Value)
    {
        const auto* ActualValue = std::get_if<float>(&Value.GetValue());

        return ActualValue != nullptr ? std::optional<float> { *ActualValue } : std::nullopt;
    }
};

template <> struct ReplicatedTypeMap<std::string>
{
    static constexpr auto ValueType = csp::common::ReplicatedValueType::String;

    static std::optional<std::string> From(const csp::common::ReplicatedValue& Value)
    {
        const auto* ActualValue = std::get_if<csp::common::String>(&Value.GetValue());

        return ActualValue != nullptr ? std::optional<std::string> { ActualValue->c_str() } : std::nullopt;
    }
};

template <> struct ReplicatedTypeMap<csp::common::Vector2>
{
    static constexpr auto ValueType = csp::common::ReplicatedValueType::Vector2;
};

template <> struct ReplicatedTypeMap<csp::common::Vector3>
{
    static constexpr auto ValueType = csp::common::ReplicatedValueType::Vector3;
};

template <> struct ReplicatedTypeMap<csp::common::Vector4>
{
    static constexpr auto ValueType = csp::common::ReplicatedValueType::Vector4;
};

template <> struct ReplicatedTypeMap<std::unordered_map<std::string, std::string>>
{
    static constexpr auto ValueType = csp::common::ReplicatedValueType::StringMap;
};

inline csp::common::ReplicatedValue ToReplicatedValue(const PropertyValue& Value)
{
    return std::visit(
        [](const auto& V) -> csp::common::ReplicatedValue
        {
            using T = std::decay_t<decltype(V.Default)>;

            if constexpr (std::is_same_v<T, std::string>)
            {
                return { V.Default.c_str() };
            }
            else if constexpr (std::is_same_v<T, std::unordered_map<std::string, std::string>>)
            {
                auto Entries = csp::common::Map<csp::common::String, csp::common::ReplicatedValue> {};

                for (const auto& [Key, Entry] : V.Default)
                {
                    Entries[Key.c_str()] = Entry.c_str();
                }

                return { Entries };
            }
            else
            {
                return { V.Default };
            }
        },
        Value);
}

template <typename T> bool IsValidValue(const PlainValue<T>&, const csp::common::ReplicatedValue& Value)
{
    return Value.GetReplicatedValueType() == ReplicatedTypeMap<T>::ValueType;
}

template <typename T> bool IsValidValue(const BoundedValue<T>& SchemaValue, const csp::common::ReplicatedValue& Value)
{
    const auto ActualValue = ReplicatedTypeMap<T>::From(Value);

    return ActualValue && *ActualValue >= SchemaValue.Range.Min && *ActualValue <= SchemaValue.Range.Max;
}

template <typename T> bool IsValidValue(const EnumeratedValue<T>& SchemaValue, const csp::common::ReplicatedValue& Value)
{
    const auto ActualValue = ReplicatedTypeMap<T>::From(Value);
    const auto Matches = [&ActualValue](const auto& Option) { return Option.Value == *ActualValue; };

    return ActualValue && std::any_of(SchemaValue.Options.begin(), SchemaValue.Options.end(), Matches);
}

inline bool IsValidValue(const ComponentProperty& Property, const csp::common::ReplicatedValue& Value)
{
    return std::visit([&Value](const auto& SchemaValue) { return IsValidValue(SchemaValue, Value); }, Property.Value);
}

inline bool IsScriptable(const csp::common::String& Name) { return !Name.IsEmpty(); }

inline bool IsScriptable(const ComponentProperty& Property)
{
    return IsScriptable(Property.Name) && std::visit([](const auto& V) { return IsScriptableV<std::decay_t<decltype(V.Default)>>; }, Property.Value);
}

inline bool IsScriptable(const ComponentSchema& Schema)
{
    const auto HasScriptableProperties = [&](const auto& Properties)
    { return std::any_of(Properties.begin(), Properties.end(), [](const auto& Property) { return IsScriptable(Property); }); };

    return IsScriptable(Schema.Name) && HasScriptableProperties(Schema.Properties);
}

} // namespace csp::multiplayer
