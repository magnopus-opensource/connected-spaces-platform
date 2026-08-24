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

#include "Common/Result.h"
#include "Multiplayer/ComponentSchema.h"

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <algorithm>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

namespace csp::multiplayer::schema
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

using ValidationResult = csp::common::Result<void>;

template <typename T> ValidationResult::Error TypeMismatch(const csp::common::ReplicatedValue& Value)
{
    const auto ToString = [](csp::common::ReplicatedValueType Type)
    {
        switch (Type)
        {
        case csp::common::ReplicatedValueType::InvalidType:
            return "ReplicatedValueType::InvalidType";
        case csp::common::ReplicatedValueType::Boolean:
            return "ReplicatedValueType::Boolean";
        case csp::common::ReplicatedValueType::Integer:
            return "ReplicatedValueType::Integer";
        case csp::common::ReplicatedValueType::Float:
            return "ReplicatedValueType::Float";
        case csp::common::ReplicatedValueType::String:
            return "ReplicatedValueType::String";
        case csp::common::ReplicatedValueType::Vector3:
            return "ReplicatedValueType::Vector3";
        case csp::common::ReplicatedValueType::Vector4:
            return "ReplicatedValueType::Vector4";
        case csp::common::ReplicatedValueType::Vector2:
            return "ReplicatedValueType::Vector2";
        case csp::common::ReplicatedValueType::StringMap:
            return "ReplicatedValueType::StringMap";
        }

        return "Unknown";
    };

    return { fmt::format("expected {}, got {}", ToString(ReplicatedTypeMap<T>::ValueType), ToString(Value.GetReplicatedValueType())) };
}

template <typename T> bool TypeCheck(const csp::common::ReplicatedValue& Value)
{
    return Value.GetReplicatedValueType() == ReplicatedTypeMap<T>::ValueType;
}

template <typename T> ValidationResult Validate(const PlainValue<T>&, const csp::common::ReplicatedValue& Value)
{
    if (!TypeCheck<T>(Value))
    {
        return TypeMismatch<T>(Value);
    }

    return ValidationResult::Ok();
}

template <typename ElementType>
ValidationResult Validate(const PlainValue<std::unordered_map<std::string, ElementType>>&, const csp::common::ReplicatedValue& Value)
{
    static_assert(ReplicatedTypeMap<ElementType>::ValueType != csp::common::ReplicatedValueType::StringMap,
        "validating nested maps is not currently supported by this implementation, as element validation is not recursive");

    if (!TypeCheck<std::unordered_map<std::string, ElementType>>(Value))
    {
        return TypeMismatch<std::unordered_map<std::string, ElementType>>(Value);
    }

    for (const auto& [Key, Element] : Value.GetStringMap())
    {
        if (!TypeCheck<ElementType>(Element))
        {
            return ValidationResult::Error {
                fmt::format("value for key '{}': {}", Key.c_str(), TypeMismatch<ElementType>(Element).Description),
            };
        }
    }

    return ValidationResult::Ok();
}

template <typename T> ValidationResult Validate(const BoundedValue<T>& SchemaValue, const csp::common::ReplicatedValue& Value)
{
    const auto ActualValue = ReplicatedTypeMap<T>::From(Value);

    if (!ActualValue)
    {
        return TypeMismatch<T>(Value);
    }

    const auto IsValid = *ActualValue >= SchemaValue.Range.Min && *ActualValue <= SchemaValue.Range.Max;

    if (!IsValid)
    {
        return ValidationResult::Error { fmt::format(
            "value {} is outside the range [{}, {}]", *ActualValue, SchemaValue.Range.Min, SchemaValue.Range.Max) };
    }

    return ValidationResult::Ok();
}

template <typename T> ValidationResult Validate(const EnumeratedValue<T>& SchemaValue, const csp::common::ReplicatedValue& Value)
{
    const auto ActualValue = ReplicatedTypeMap<T>::From(Value);

    if (!ActualValue)
    {
        return TypeMismatch<T>(Value);
    }

    const auto Matches = [&ActualValue](const auto& Option) { return Option.Value == *ActualValue; };
    const auto IsValid = std::any_of(SchemaValue.Options.begin(), SchemaValue.Options.end(), Matches);

    if (!IsValid)
    {
        const auto ToString = [](const auto& Options)
        {
            auto Values = std::vector<T>(Options.size());
            std::transform(Options.begin(), Options.end(), Values.begin(), [](const auto& Option) { return Option.Value; });

            return fmt::format("{}", fmt::join(Values, ", "));
        };

        return ValidationResult::Error { fmt::format("value {} is not one of the options [{}]", *ActualValue, ToString(SchemaValue.Options)) };
    }

    return ValidationResult::Ok();
}

inline ValidationResult Validate(const ComponentProperty& Property, const csp::common::ReplicatedValue& Value)
{
    return std::visit([&Value](const auto& SchemaValue) { return Validate(SchemaValue, Value); }, Property.Value);
}

inline bool IsScriptable(const csp::common::String& Name) { return !Name.IsEmpty(); }

inline bool IsScriptable(const ComponentProperty& Property)
{
    return IsScriptable(Property.Name) && Property.IsScriptable.value_or(true)
        && std::visit([](const auto& V) { return IsScriptableV<std::decay_t<decltype(V.Default)>>; }, Property.Value);
}

inline bool IsScriptable(const ComponentSchema& Schema)
{
    const auto HasScriptableProperties = [&](const auto& Properties)
    { return std::any_of(Properties.begin(), Properties.end(), [](const auto& Property) { return IsScriptable(Property); }); };

    return IsScriptable(Schema.Name) && Schema.IsScriptable.value_or(true) && HasScriptableProperties(Schema.Properties);
}

} // namespace csp::multiplayer::schema
