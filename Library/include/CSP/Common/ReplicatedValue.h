/*
 * Copyright 2023 Magnopus LLC

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

#include "CSP/CSPCommon.h"
#include "CSP/Common/Array.h"
#include "CSP/Common/Map.h"
#include "CSP/Common/String.h"
#include "CSP/Common/Vector.h"

#include <stdexcept>
#include <variant>

namespace csp::common
{

CSP_START_IGNORE
class ReplicatedValue;
using ReplicatedValueImplType = std::variant<bool, float, int64_t, csp::common::String, csp::common::Vector2, csp::common::Vector3,
    csp::common::Vector4, csp::common::Map<csp::common::String, ReplicatedValue>>;
CSP_END_IGNORE

/// @brief Enum representing the type of a replicated value.
/// These values are serialized and stored as integers.
/// When adding new values, always add to the end
enum class ReplicatedValueType
{
    InvalidType = 0,
    Boolean = 1,
    Integer = 2,
    Float = 3,
    String = 4,
    Vector3 = 5,
    Vector4 = 6,
    Vector2 = 7,
    StringMap = 8
};

/// @brief ReplicatedValue is an intermediate class that enables clients to pack data into types that are supported by Connected Spaces Platform
/// replication systems.
class CSP_API ReplicatedValue
{
public:
    /// @brief A default ReplicatedValue will not have a valid type ("ReplicatedValueType::InvalidType"), and will have no internal value associated.
    /// This constuctor will create the value in an invalid state. Do not use unless you know what you are doing!
    ReplicatedValue();

    CSP_START_IGNORE

    // These are great for certain scenarios such as serialization/deserialization, as we can avoid conditionals.
    // Internal templated setter.
    CSP_NO_EXPORT template <class T> inline void Set(const T& InValue) { Value = InValue; }
    // Internal templated getter.
    CSP_NO_EXPORT template <class T> inline const T& Get() const { return std::get<T>(Value); }

    // Internal getter for variant.
    CSP_NO_EXPORT const ReplicatedValueImplType& GetValue() const { return Value; };

    CSP_END_IGNORE

    /// @brief Construct a ReplicatedValue based on a bool type.
    /// @param InBoolValue bool : Initial value.
    ReplicatedValue(bool InBoolValue);

    /// @brief Construct a ReplicatedValue based on a float type.
    /// @param InFloatValue float : Initial value.
    ReplicatedValue(float InFloatValue);

    /// @brief Construct a ReplicatedValue based on a Long (uint64_t) type.
    /// @param InLongValue int64_t : Initial value.
    ReplicatedValue(int64_t InLongValue);

    /// @brief Construct a ReplicatedValue based on a csp::common::String type derived from the given char*.
    /// @param InLongValue int64_t : Initial value.
    CSP_NO_EXPORT ReplicatedValue(const char* InStringValue);

    /// @brief Construct a ReplicatedValue based on an csp::common::String type.
    /// @param InStringValue csp::common::String : Initial value.
    ReplicatedValue(const csp::common::String& InStringValue);

    /// @brief Construct a ReplicatedValue based on a csp::common::Vector2 type.
    /// @param InVector2Value csp::common::Vector2 : Initial value.
    ReplicatedValue(const csp::common::Vector2& InVector2Value);

    /// @brief Construct a ReplicatedValue based on a csp::common::Vector3 type.
    /// @param InVector3Value csp::common::Vector3 : Initial value.
    ReplicatedValue(const csp::common::Vector3& InVector3Value);

    /// @brief Construct a ReplicatedValue based on an csp::common::Vector4 type.
    /// @param InVector4Value csp::common::Vector4 : Initial value.
    ReplicatedValue(const csp::common::Vector4& InVector4Value);

    /// @brief Construct a ReplicatedValue based on an csp::common::Map type with a string value as the key.
    /// @param InValue csp::common::Map : Initial value.
    ReplicatedValue(const csp::common::Map<csp::common::String, ReplicatedValue>& InValue);

    /// @brief Destroys the replicated value instance.
    ~ReplicatedValue();

    /// @brief Copy constructor
    /// @param Other csp::common::ReplicatedValue& : The value to copy.
    ReplicatedValue(const ReplicatedValue& Other);

    /// @brief Move constructor
    /// @param Other csp::common::ReplicatedValue& : The value to move.
    CSP_NO_EXPORT ReplicatedValue(ReplicatedValue&& Other);

    /// @brief Copy assignment operator overload.
    /// @param InValue ReplicatedValue : Other replicated value to set from.
    ReplicatedValue& operator=(const ReplicatedValue& Other);

    /// @brief Move assignment operator overload.
    /// @param InValue ReplicatedValue : Other replicated value to move from.
    ReplicatedValue& operator=(ReplicatedValue&& Other);

    /// @brief Equality operator overload.
    /// @param ReplicatedValue : Other value to compare to.
    bool operator==(const ReplicatedValue& OtherValue) const;

    /// @brief Inequality operator overload.
    /// @param ReplicatedValue : Other value to compare to.
    bool operator!=(const ReplicatedValue& OtherValue) const;

    /// @brief Gets the type of replicated value.
    /// @return ReplicatedValueType: Enum representing all supported replication base types.
    ReplicatedValueType GetReplicatedValueType() const { return ReplicatedType; }

    /// @brief Sets a bool value for this replicated value, will overwrite any previous value.
    /// @param InValue
    void SetBool(bool InValue);

    /// @brief Get a bool value from this replicated value, will assert if not a bool type.
    /// @return bool
    bool GetBool() const;

    /// @brief Sets a float value for this replicated value, will overwrite any previous value.
    /// @param InValue
    void SetFloat(float InValue);

    /// @brief Get a float value from this replicated value, will assert if not a float type.
    /// @return float value
    float GetFloat() const;

    /// @brief Sets a int64 value for this replicated value, will overwrite any previous value.
    /// @param InValue
    void SetInt(int64_t InValue);

    /// @brief Get a int64 value from this replicated value, will assert if not a int64 type.
    /// @return int64 value
    int64_t GetInt() const;

    /// @brief Set a string value for this replicated value from a const char*, will overwrite and previous value.
    CSP_NO_EXPORT void SetString(const char* InValue);

    ///@brief Set a string value for this replicated value from a csp::common::String&, will overwrite and previous value.
    void SetString(const csp::common::String& InValue);

    /// @brief Get a csp::common::String& value from this replicated value, will assert if not a csp::common::String type.
    /// @return csp::common::String&
    const csp::common::String& GetString() const;

    /// @brief Get a generic default string.
    /// @return The default string.
    CSP_NO_EXPORT static const csp::common::String& GetDefaultString();

    /// @brief Set a Vector2 value for this replicated value from a csp::common::Vector2, will overwrite and previous value.
    void SetVector2(const csp::common::Vector2& InValue);

    /// @brief Get a csp::common::Vector2 value from this replicated value, will assert if not a csp::common::Vector2 type.
    /// @return csp::common::Vector2
    const csp::common::Vector2& GetVector2() const;

    /// @brief Get a generic default Vector2.
    /// @return The default Vector2.
    CSP_NO_EXPORT static const csp::common::Vector2& GetDefaultVector2();

    /// @brief Set a Vector3 value for this replicated value from a csp::common::Vector3, will overwrite and previous value.
    void SetVector3(const csp::common::Vector3& InValue);

    /// @brief Get a csp::common::Vector3 value from this replicated value, will assert if not a csp::common::Vector3 type.
    /// @return csp::common::Vector3
    const csp::common::Vector3& GetVector3() const;

    /// @brief Get a generic default Vector3.
    /// @return The default Vector3.
    CSP_NO_EXPORT static const csp::common::Vector3& GetDefaultVector3();

    /// @brief Set a Vector4 value for this replicated value from a csp::common::Vector4, will overwrite and previous value.
    void SetVector4(const csp::common::Vector4& InValue);

    /// @brief Get a csp::common::Vector4 value from this replicated value, will assert if not a csp::common::Vector4 type.
    /// @return csp::common::Vector4
    const csp::common::Vector4& GetVector4() const;

    /// @brief Get a generic default Vector4.
    /// @return The default Vector4.
    CSP_NO_EXPORT static const csp::common::Vector4& GetDefaultVector4();

    /// @brief Get a csp::common::Map value with a string value as the key.
    /// This will assert if not a csp::common::Map type with a string value as the key.
    /// @return csp::common::Map
    const csp::common::Map<csp::common::String, ReplicatedValue>& GetStringMap() const;

    /// @brief Set a string Map value for this replicated value from a csp::common::Map, will overwrite any previous value.
    void SetStringMap(const csp::common::Map<csp::common::String, ReplicatedValue>& InValue);

    /// @brief Get a generic default StringMap.
    /// @return The default StringMap.
    CSP_NO_EXPORT static const csp::common::Map<csp::common::String, ReplicatedValue>& GetDefaultStringMap();

private:
    CSP_START_IGNORE
    ReplicatedValueImplType Value;
    CSP_END_IGNORE

    ReplicatedValueType ReplicatedType;
};

} // namespace csp::common
