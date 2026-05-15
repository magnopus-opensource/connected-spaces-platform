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

#include "CSP/Common/Variant.h"

namespace csp::common
{

Variant::Variant() { m_valueType = VariantType::InvalidType; }

Variant::~Variant()
{
    if (m_valueType == VariantType::String)
    {
        m_value.String.~String();
    }
}

Variant::Variant(bool inValue)
    : m_valueType(VariantType::Boolean)
{
    m_value.Bool = inValue;
}

Variant::Variant(double inValue)
    : m_valueType(VariantType::Float)
{
    m_value.Float = inValue;
}

Variant::Variant(int64_t inValue)
    : m_valueType(VariantType::Integer)
{
    m_value.Int = inValue;
}

Variant::Variant(const char* inValue)
    : m_valueType(VariantType::String)
{
    m_value.String = inValue;
}

Variant::Variant(const csp::common::String inValue)
    : m_valueType(VariantType::String)
{
    m_value.String = inValue;
}

Variant::Variant(Vector3 inValue)
    : m_valueType(VariantType::Vector3)
{
    m_value.Vector3 = inValue;
}

Variant::Variant(const Vector4 inValue)
    : m_valueType(VariantType::Vector4)
{
    m_value.Vector4 = inValue;
}

Variant::Variant(const Variant& otherValue) { *this = otherValue; }

Variant& Variant::operator=(const Variant& inValue)
{
    switch (inValue.GetValueType())
    {
    case VariantType::Boolean:
    {
        SetBool(inValue.GetBool());
        break;
    }
    case VariantType::Integer:
    {
        SetInt(inValue.GetInt());
        break;
    }
    case VariantType::Float:
    {
        SetFloat(inValue.GetFloat());
        break;
    }
    case VariantType::String:
    {
        SetString(inValue.GetString());
        break;
    }
    case VariantType::Vector3:
    {
        SetVector3(inValue.GetVector3());
        break;
    }
    case VariantType::Vector4:
    {
        SetVector4(inValue.GetVector4());
        break;
    }
    case VariantType::InvalidType:
    {
        m_valueType = VariantType::InvalidType;
        break;
    }
    default:
    {
        assert(0 && "Unhandled replicated value type!");
        break;
    }
    }

    return *this;
}

bool Variant::operator==(const Variant& otherValue) const
{
    bool isEqual = GetValueType() == otherValue.GetValueType();

    if (isEqual)
    {
        switch (otherValue.GetValueType())
        {
        case VariantType::Boolean:
        {
            isEqual = GetBool() == otherValue.GetBool();
            break;
        }
        case VariantType::Integer:
        {
            isEqual = GetInt() == otherValue.GetInt();
            break;
        }
        case VariantType::Float:
        {
            isEqual = GetFloat() == otherValue.GetFloat();
            break;
        }
        case VariantType::String:
        {
            isEqual = GetString() == otherValue.GetString();
            break;
        }
        case VariantType::Vector3:
        {
            isEqual = GetVector3() == otherValue.GetVector3();
            break;
        }
        case VariantType::Vector4:
        {
            isEqual = GetVector4() == otherValue.GetVector4();
            break;
        }
        default:
        {
            assert(0 && "Unhandled replicated value type!");
            break;
        }
        }
    }

    return isEqual;
}

bool Variant::operator!=(const Variant& otherValue) const { return (*this == otherValue) == false; }

void Variant::SetBool(bool inValue)
{
    m_valueType = VariantType::Boolean;
    m_value.Bool = inValue;
}

bool Variant::GetBool() const
{
    assert(m_valueType == VariantType::Boolean);
    return m_value.Bool;
}

void Variant::SetFloat(double inValue)
{
    m_valueType = VariantType::Float;
    m_value.Float = inValue;
}

double Variant::GetFloat() const
{
    assert(m_valueType == VariantType::Float);
    return m_value.Float;
}

void Variant::SetInt(int64_t inValue)
{
    m_valueType = VariantType::Integer;
    m_value.Int = inValue;
}

int64_t Variant::GetInt() const
{
    assert(m_valueType == VariantType::Integer);
    return m_value.Int;
}

void Variant::SetString(const char* inValue)
{
    m_valueType = VariantType::String;
    m_value.String = inValue;
}

void Variant::SetString(const csp::common::String& inValue)
{
    m_valueType = VariantType::String;
    m_value.String = inValue;
}

const csp::common::String& Variant::GetString() const
{
    assert(m_valueType == VariantType::String);
    return m_value.String;
}

void Variant::SetVector3(Vector3 inValue)
{
    m_valueType = VariantType::Vector3;
    m_value.Vector3 = inValue;
}

Vector3 Variant::GetVector3() const
{
    assert(m_valueType == VariantType::Vector3);
    return m_value.Vector3;
}

void Variant::SetVector4(Vector4 inValue)
{
    m_valueType = VariantType::Vector4;
    m_value.Vector4 = inValue;
}

Vector4 Variant::GetVector4() const
{
    assert(m_valueType == VariantType::Vector4);
    return m_value.Vector4;
}

size_t Variant::GetSizeOfInternalValue() { return sizeof(InternalValue); }

// This is a real problem, don't ignore this, it needs fixed. (Or this entire type needs deleted)
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma clang diagnostic ignored "-Wnontrivial-memcall"
#endif
Variant::InternalValue::InternalValue() { memset(this, 0x0, sizeof(InternalValue)); }
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

Variant::InternalValue::~InternalValue() { }

} // namespace csp::common
