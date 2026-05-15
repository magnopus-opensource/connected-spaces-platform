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

ReplicatedValue::ReplicatedValue() { m_replicatedType = ReplicatedValueType::InvalidType; }

ReplicatedValue::~ReplicatedValue() { }

ReplicatedValue::ReplicatedValue(bool inValue)
    : m_value { inValue }
    , m_replicatedType(ReplicatedValueType::Boolean)

{
}

ReplicatedValue::ReplicatedValue(float inValue)
    : m_value { inValue }
    , m_replicatedType(ReplicatedValueType::Float)
{
}

ReplicatedValue::ReplicatedValue(int64_t inValue)
    : m_value { inValue }
    , m_replicatedType(ReplicatedValueType::Integer)
{
}

ReplicatedValue::ReplicatedValue(const char* inValue)
    : m_value { csp::common::String { inValue } }
    , m_replicatedType(ReplicatedValueType::String)
{
}

ReplicatedValue::ReplicatedValue(const csp::common::String& inValue)
    : m_value { inValue }
    , m_replicatedType(ReplicatedValueType::String)
{
}

ReplicatedValue::ReplicatedValue(const csp::common::Vector2& inValue)
    : m_value { inValue }
    , m_replicatedType(ReplicatedValueType::Vector2)
{
}

ReplicatedValue::ReplicatedValue(const csp::common::Vector3& inValue)
    : m_value { inValue }
    , m_replicatedType(ReplicatedValueType::Vector3)
{
}

ReplicatedValue::ReplicatedValue(const csp::common::Vector4& inValue)
    : m_value { inValue }
    , m_replicatedType(ReplicatedValueType::Vector4)
{
}

ReplicatedValue::ReplicatedValue(const csp::common::Map<csp::common::String, ReplicatedValue>& inValue)
    : m_value { inValue }
    , m_replicatedType(ReplicatedValueType::StringMap)
{
}

ReplicatedValue::ReplicatedValue(const ReplicatedValue& other)
{
    m_value = other.m_value;
    this->m_replicatedType = other.m_replicatedType;
}

ReplicatedValue::ReplicatedValue(ReplicatedValue&& other)
{
    this->m_value = std::move(other.m_value);

    this->m_replicatedType = std::move(other.m_replicatedType);
    other.m_replicatedType = ReplicatedValueType::InvalidType;
}

ReplicatedValue& ReplicatedValue::operator=(const ReplicatedValue& other)
{
    this->m_value = other.m_value;
    this->m_replicatedType = other.m_replicatedType;
    return *this;
}

ReplicatedValue& ReplicatedValue::operator=(ReplicatedValue&& other)
{
    this->m_value = std::move(other.m_value);

    this->m_replicatedType = std::move(other.m_replicatedType);
    other.m_replicatedType = ReplicatedValueType::InvalidType;

    return *this;
}

bool ReplicatedValue::operator==(const ReplicatedValue& otherValue) const { return (m_value) == otherValue.m_value; }
bool ReplicatedValue::operator!=(const ReplicatedValue& otherValue) const { return (*this == otherValue) == false; }

void ReplicatedValue::SetBool(bool inValue)
{
    m_value = inValue;
    m_replicatedType = ReplicatedValueType::Boolean;
}

bool ReplicatedValue::GetBool() const
{
    if (m_replicatedType != ReplicatedValueType::Boolean)
    {
        throw ReplicatedValueException(ReplicatedValueType::Boolean, m_replicatedType);
    }

    return Get<bool>();
}

void ReplicatedValue::SetFloat(float inValue)
{
    m_value = inValue;
    m_replicatedType = ReplicatedValueType::Float;
}

float ReplicatedValue::GetFloat() const
{
    if (m_replicatedType != ReplicatedValueType::Float)
    {
        throw ReplicatedValueException(ReplicatedValueType::Float, m_replicatedType);
    }

    return Get<float>();
}

void ReplicatedValue::SetInt(int64_t inValue)
{
    m_value = inValue;
    m_replicatedType = ReplicatedValueType::Integer;
}

int64_t ReplicatedValue::GetInt() const
{
    if (m_replicatedType != ReplicatedValueType::Integer)
    {
        throw ReplicatedValueException(ReplicatedValueType::Integer, m_replicatedType);
    }

    return Get<int64_t>();
}

void ReplicatedValue::SetString(const char* inValue)
{
    m_value = csp::common::String { inValue };
    m_replicatedType = ReplicatedValueType::String;
}

void ReplicatedValue::SetString(const csp::common::String& inValue)
{
    m_value = inValue;
    m_replicatedType = ReplicatedValueType::String;
}

const csp::common::String& ReplicatedValue::GetString() const
{
    if (m_replicatedType != ReplicatedValueType::String)
    {
        throw ReplicatedValueException(ReplicatedValueType::String, m_replicatedType);
    }

    return Get<csp::common::String>();
}

const csp::common::String& ReplicatedValue::GetDefaultString()
{
    static const csp::common::String invalidString = csp::common::String();
    return invalidString;
}

void ReplicatedValue::SetVector2(const csp::common::Vector2& inValue)
{
    m_value = inValue;
    m_replicatedType = ReplicatedValueType::Vector2;
}

const csp::common::Vector2& ReplicatedValue::GetVector2() const
{
    if (m_replicatedType != ReplicatedValueType::Vector2)
    {
        throw ReplicatedValueException(ReplicatedValueType::Vector2, m_replicatedType);
    }

    return Get<csp::common::Vector2>();
}

const csp::common::Vector2& ReplicatedValue::GetDefaultVector2() { return InvalidVector2; }

void ReplicatedValue::SetVector3(const csp::common::Vector3& inValue)
{
    m_value = inValue;
    m_replicatedType = ReplicatedValueType::Vector3;
}

const csp::common::Vector3& ReplicatedValue::GetVector3() const
{
    if (m_replicatedType != ReplicatedValueType::Vector3)
    {
        throw ReplicatedValueException(ReplicatedValueType::Vector3, m_replicatedType);
    }

    return Get<csp::common::Vector3>();
}

const csp::common::Vector3& ReplicatedValue::GetDefaultVector3() { return InvalidVector3; }

void ReplicatedValue::SetVector4(const csp::common::Vector4& inValue)
{
    m_value = inValue;
    m_replicatedType = ReplicatedValueType::Vector4;
}

const csp::common::Vector4& ReplicatedValue::GetVector4() const
{
    if (m_replicatedType != ReplicatedValueType::Vector4)
    {
        throw ReplicatedValueException(ReplicatedValueType::Vector4, m_replicatedType);
    }

    return Get<csp::common::Vector4>();
}

const csp::common::Vector4& ReplicatedValue::GetDefaultVector4() { return InvalidVector4; }

const csp::common::Map<csp::common::String, ReplicatedValue>& ReplicatedValue::GetStringMap() const
{
    if (m_replicatedType != ReplicatedValueType::StringMap)
    {
        throw ReplicatedValueException(ReplicatedValueType::StringMap, m_replicatedType);
    }

    return Get<csp::common::Map<csp::common::String, ReplicatedValue>>();
}

void ReplicatedValue::SetStringMap(const csp::common::Map<csp::common::String, ReplicatedValue>& inValue)
{
    m_value = inValue;
    m_replicatedType = ReplicatedValueType::StringMap;
}

const csp::common::Map<csp::common::String, ReplicatedValue>& ReplicatedValue::GetDefaultStringMap() { return InvalidStringMap; }

} // namespace csp::common
