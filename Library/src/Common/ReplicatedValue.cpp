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
#include "CSP/Common/ReplicatedValue.h"
#include "CSP/Common/ReplicatedValueException.h"

namespace csp::common
{

static const csp::common::Vector2 InvalidVector2 = csp::common::Vector2();
static const csp::common::Vector3 InvalidVector3 = csp::common::Vector3();
static const csp::common::Vector4 InvalidVector4 = csp::common::Vector4();
static const csp::common::Map<csp::common::String, ReplicatedValue> InvalidStringMap = csp::common::Map<csp::common::String, ReplicatedValue>();

ReplicatedValue::ReplicatedValue() { ReplicatedType = ReplicatedValueType::InvalidType; }

ReplicatedValue::~ReplicatedValue() { }

ReplicatedValue::ReplicatedValue(bool InValue)
    : Value { InValue }
    , ReplicatedType(ReplicatedValueType::Boolean)

{
}

ReplicatedValue::ReplicatedValue(float InValue)
    : Value { InValue }
    , ReplicatedType(ReplicatedValueType::Float)
{
}

ReplicatedValue::ReplicatedValue(int64_t InValue)
    : Value { InValue }
    , ReplicatedType(ReplicatedValueType::Integer)
{
}

ReplicatedValue::ReplicatedValue(const char* InValue)
    : Value { csp::common::String { InValue } }
    , ReplicatedType(ReplicatedValueType::String)
{
}

ReplicatedValue::ReplicatedValue(const csp::common::String& InValue)
    : Value { InValue }
    , ReplicatedType(ReplicatedValueType::String)
{
}

ReplicatedValue::ReplicatedValue(const csp::common::Vector2& InValue)
    : Value { InValue }
    , ReplicatedType(ReplicatedValueType::Vector2)
{
}

ReplicatedValue::ReplicatedValue(const csp::common::Vector3& InValue)
    : Value { InValue }
    , ReplicatedType(ReplicatedValueType::Vector3)
{
}

ReplicatedValue::ReplicatedValue(const csp::common::Vector4& InValue)
    : Value { InValue }
    , ReplicatedType(ReplicatedValueType::Vector4)
{
}

ReplicatedValue::ReplicatedValue(const csp::common::Map<csp::common::String, ReplicatedValue>& InValue)
    : Value { InValue }
    , ReplicatedType(ReplicatedValueType::StringMap)
{
}

ReplicatedValue::ReplicatedValue(const ReplicatedValue& Other)
{
    Value = Other.Value;
    this->ReplicatedType = Other.ReplicatedType;
}

ReplicatedValue::ReplicatedValue(ReplicatedValue&& Other)
{
    this->Value = std::move(Other.Value);

    this->ReplicatedType = std::move(Other.ReplicatedType);
    Other.ReplicatedType = ReplicatedValueType::InvalidType;
}

ReplicatedValue& ReplicatedValue::operator=(const ReplicatedValue& Other)
{
    this->Value = Other.Value;
    this->ReplicatedType = Other.ReplicatedType;
    return *this;
}

ReplicatedValue& ReplicatedValue::operator=(ReplicatedValue&& Other)
{
    this->Value = std::move(Other.Value);

    this->ReplicatedType = std::move(Other.ReplicatedType);
    Other.ReplicatedType = ReplicatedValueType::InvalidType;

    return *this;
}

bool ReplicatedValue::operator==(const ReplicatedValue& OtherValue) const { return (Value) == OtherValue.Value; }
bool ReplicatedValue::operator!=(const ReplicatedValue& OtherValue) const { return (*this == OtherValue) == false; }

void ReplicatedValue::SetBool(bool InValue)
{
    Value = InValue;
    ReplicatedType = ReplicatedValueType::Boolean;
}

bool ReplicatedValue::GetBool() const
{
    if (ReplicatedType != ReplicatedValueType::Boolean)
    {
        throw ReplicatedValueException(ReplicatedValueType::Boolean, ReplicatedType);
    }

    return Get<bool>();
}

void ReplicatedValue::SetFloat(float InValue)
{
    Value = InValue;
    ReplicatedType = ReplicatedValueType::Float;
}

float ReplicatedValue::GetFloat() const
{
    if (ReplicatedType != ReplicatedValueType::Float)
    {
        throw ReplicatedValueException(ReplicatedValueType::Float, ReplicatedType);
    }

    return Get<float>();
}

void ReplicatedValue::SetInt(int64_t InValue)
{
    Value = InValue;
    ReplicatedType = ReplicatedValueType::Integer;
}

int64_t ReplicatedValue::GetInt() const
{
    if (ReplicatedType != ReplicatedValueType::Integer)
    {
        throw ReplicatedValueException(ReplicatedValueType::Integer, ReplicatedType);
    }

    return Get<int64_t>();
}

void ReplicatedValue::SetString(const char* InValue)
{
    Value = csp::common::String { InValue };
    ReplicatedType = ReplicatedValueType::String;
}

void ReplicatedValue::SetString(const csp::common::String& InValue)
{
    Value = InValue;
    ReplicatedType = ReplicatedValueType::String;
}

const csp::common::String& ReplicatedValue::GetString() const
{
    if (ReplicatedType != ReplicatedValueType::String)
    {
        throw ReplicatedValueException(ReplicatedValueType::String, ReplicatedType);
    }

    return Get<csp::common::String>();
}

const csp::common::String& ReplicatedValue::GetDefaultString()
{
    static const csp::common::String InvalidString = csp::common::String();
    return InvalidString;
}

void ReplicatedValue::SetVector2(const csp::common::Vector2& InValue)
{
    Value = InValue;
    ReplicatedType = ReplicatedValueType::Vector2;
}

const csp::common::Vector2& ReplicatedValue::GetVector2() const
{
    if (ReplicatedType != ReplicatedValueType::Vector2)
    {
        throw ReplicatedValueException(ReplicatedValueType::Vector2, ReplicatedType);
    }

    return Get<csp::common::Vector2>();
}

const csp::common::Vector2& ReplicatedValue::GetDefaultVector2() { return InvalidVector2; }

void ReplicatedValue::SetVector3(const csp::common::Vector3& InValue)
{
    Value = InValue;
    ReplicatedType = ReplicatedValueType::Vector3;
}

const csp::common::Vector3& ReplicatedValue::GetVector3() const
{
    if (ReplicatedType != ReplicatedValueType::Vector3)
    {
        throw ReplicatedValueException(ReplicatedValueType::Vector3, ReplicatedType);
    }

    return Get<csp::common::Vector3>();
}

const csp::common::Vector3& ReplicatedValue::GetDefaultVector3() { return InvalidVector3; }

void ReplicatedValue::SetVector4(const csp::common::Vector4& InValue)
{
    Value = InValue;
    ReplicatedType = ReplicatedValueType::Vector4;
}

const csp::common::Vector4& ReplicatedValue::GetVector4() const
{
    if (ReplicatedType != ReplicatedValueType::Vector4)
    {
        throw ReplicatedValueException(ReplicatedValueType::Vector4, ReplicatedType);
    }

    return Get<csp::common::Vector4>();
}

const csp::common::Vector4& ReplicatedValue::GetDefaultVector4() { return InvalidVector4; }

const csp::common::Map<csp::common::String, ReplicatedValue>& ReplicatedValue::GetStringMap() const
{
    if (ReplicatedType != ReplicatedValueType::StringMap)
    {
        throw ReplicatedValueException(ReplicatedValueType::StringMap, ReplicatedType);
    }

    return Get<csp::common::Map<csp::common::String, ReplicatedValue>>();
}

void ReplicatedValue::SetStringMap(const csp::common::Map<csp::common::String, ReplicatedValue>& InValue)
{
    Value = InValue;
    ReplicatedType = ReplicatedValueType::StringMap;
}

const csp::common::Map<csp::common::String, ReplicatedValue>& ReplicatedValue::GetDefaultStringMap() { return InvalidStringMap; }

} // namespace csp::common
