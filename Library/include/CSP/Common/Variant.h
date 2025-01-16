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
#include "CSP/Common/String.h"
#include "CSP/Common/Vector.h"

#include <functional>

namespace csp::common
{

/// @brief enum for representing a type stored by a csp::common::Variant.
enum class VariantType
{
    InvalidType,
    Boolean,
    Integer,
    Float,
    String,
    Vector3,
    Vector4,
};

/// @brief Variant is an intermediate class that enables clients to pack data into types that are supported by Connected Spaces Platform replication
/// systems.
class CSP_API Variant
{
public:
    /// @brief A default Variant will not have a valid type ("VariantType::InvalidType"), and will have no internal value associated.
    /// Do not use this constructor unless you know what you are doing!
    Variant();

    /// @brief Construct a Variant based on a bool type.
    /// @param InBoolValue bool : In value
    Variant(bool InBoolValue);

    /// @brief Construct a Variant based on a double-precision float type.
    /// @param InFloatValue double : In value
    Variant(double InFloatValue);

    /// @brief Construct a Variant based on a Long (uint64_t) type.
    /// @param InLongValue int64_t : In value
    Variant(int64_t InLongValue);

    /// @brief Construct a Variant based on a cstring (const char*) type.
    /// @param InStringValue const char* : In value
    CSP_NO_EXPORT Variant(const char* InStringValue);

    /// @brief Construct a Variant based on an csp::common::String type.
    /// @param InStringValue csp::common::String : In value
    Variant(String InStringValue);

    /// @brief Construct a Variant based on a csp::common::Vector3 type.
    /// @param InVector3Value csp::common::Vector3 : In value
    Variant(Vector3 InVector3Value);

    /// @brief Construct a Variant based on an csp::common::Vector4 type.
    /// @param InVector4Value csp::common::Vector4 : In value
    Variant(Vector4 InVector4Value);

    /// @brief Copy constructor
    /// @param Other const Variant&
    Variant(const Variant& Other);

    /// @brief Destructor
    ~Variant();

    Variant& operator=(const Variant& InValue);
    bool operator==(const Variant& OtherValue) const;
    bool operator!=(const Variant& OtherValue) const;

    /// @brief Gets the type of replicated value.
    /// @return VariantType : The current variant internal type
    VariantType GetValueType() const { return ValueType; }

    /// @brief Sets internal variant type as a bool.
    void SetBool(bool InValue);
    /// @brief Gets internal variant type as a bool.
    /// @return bool
    bool GetBool() const;

    /// @brief Sets internal variant type as a double-precision float.
    void SetFloat(double InValue);
    /// @brief Gets internal variant type as a double-precision float.
    /// @return double
    double GetFloat() const;

    /// @brief Sets internal variant type as an int64_t.
    void SetInt(int64_t InValue);
    /// @brief Gets internal variant type as a int64_t.
    /// @return int16_t
    int64_t GetInt() const;

    /// @brief Sets internal variant type as a String.
    CSP_NO_EXPORT void SetString(const char* InValue);

    /// @brief Sets internal variant type as a String.
    void SetString(const String& InValue);
    /// @brief Gets internal variant type as a String.
    /// @return const csp::common::String&
    const String& GetString() const;

    /// @brief Sets internal variant type as a Vector3.
    void SetVector3(Vector3 InValue);
    /// @brief Gets internal variant type as a Vector3.
    /// @return csp::common::Vector3
    Vector3 GetVector3() const;

    /// @brief Sets internal variant type as a Vector4.
    void SetVector4(Vector4 InValue);
    /// @brief Gets internal variant type as a Vector4.
    /// @return csp::common::Vector4
    Vector4 GetVector4() const;

    /// @brief Static utility function to get the size in bytes of an InternalValue.
    /// @return size_t
    CSP_NO_EXPORT static size_t GetSizeOfInternalValue();

private:
    VariantType ValueType;

    CSP_START_IGNORE
    union InternalValue
    {
        InternalValue();
        ~InternalValue();

        bool Bool;
        double Float;
        int64_t Int;
        String String;
        Vector3 Vector3;
        Vector4 Vector4;
    };

    InternalValue Value;
    CSP_END_IGNORE
};

} // namespace csp::common
