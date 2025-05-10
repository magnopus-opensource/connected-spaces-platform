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

#include <signalrclient/signalr_value.h>

#include <optional>
#include <type_traits>
#include <variant>
#include <vector>

namespace csp::multiplayer
{
class ISignalRSerializable;
class ISignalRDeserializable;

// Type Checks ----------------------------------------------------------------------------------------------------------

// All suported numeric types.
// clang-format off
template <typename T>
struct IsInteger : std::bool_constant<
    std::is_same_v<T, int8_t> ||
    std::is_same_v<T, uint8_t> ||
    std::is_same_v<T, int16_t> ||
    std::is_same_v<T, uint16_t> ||
    std::is_same_v<T, int32_t> ||
    std::is_same_v<T, uint32_t> ||
    std::is_same_v<T, int64_t> ||
    std::is_same_v<T, uint64_t> ||
    std::is_same_v<T, long long> ||
    std::is_same_v<T, unsigned long long>>
 {};
// clang-format on
//
// Is type an integer.
template <typename T> constexpr bool IsIntegerV = IsInteger<T>::value;

// Is type an unsigned integer.
template <typename T> struct IsUnsignedInteger : std::conjunction<IsInteger<T>, std::negation<std::is_signed<T>>>
{
};

template <typename T> constexpr bool IsUnsignedIntegerV = IsUnsignedInteger<T>::value;

// Is type a signed integer.
template <typename T> struct IsSignedInteger : std::conjunction<IsInteger<T>, std::is_signed<T>>
{
};

template <typename T> constexpr bool IsSignedIntegerV = IsSignedInteger<T>::value;

// Is type a custom serializable/deserializable type.
template <typename T> constexpr bool IsCustomSerializableV = std::is_base_of_v<ISignalRSerializable, T>;
template <typename T> constexpr bool IsCustomDeserializableV = std::is_base_of_v<ISignalRDeserializable, T>;

// Supported types ------------------------------------------------------------------------------------------------

// All supported basic types our serializer supports.
// clang-format off
template <typename T>
constexpr bool IsSupportedSignalRBasicType = 
    IsIntegerV<T> || 
    std::is_same_v<T, double> || 
    std::is_same_v<T, float> ||
    std::is_same_v<T, bool> || 
    std::is_same_v<T, std::string> || 
    std::is_same_v<T, nullptr_t> ||
    std::is_base_of_v<ISignalRSerializable, T> ||
    std::is_base_of_v<ISignalRDeserializable, T>;
// clang-format on

// Add supported container types.
template <typename T, typename = void> struct IsSupportedSignalRType : std::false_type
{
};

template <typename T> struct IsSupportedSignalRType<T, std::enable_if_t<IsSupportedSignalRBasicType<T>>> : std::true_type
{
};

// Optional
template <typename T> struct IsSupportedSignalRType<std::optional<T>, std::enable_if_t<IsSupportedSignalRType<T>::value>> : std::true_type
{
};

// Vector
template <typename T> struct IsSupportedSignalRType<std::vector<T>, std::enable_if_t<IsSupportedSignalRType<T>::value>> : std::true_type
{
};

// Uint map that can support any of our unsigned integers as a key
template <typename K, typename T>
struct IsSupportedSignalRType<std::map<K, T>, std::enable_if_t<IsUnsignedIntegerV<K> && IsSupportedSignalRType<T>::value>> : std::true_type
{
};

// String map
template <typename T> struct IsSupportedSignalRType<std::map<std::string, T>, std::enable_if_t<IsSupportedSignalRType<T>::value>> : std::true_type
{
};

}
