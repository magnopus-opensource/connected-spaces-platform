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

#include "Memory/Memory.h"

namespace csp::common
{

Variant::Variant() { ValueType = VariantType::InvalidType; }

Variant::~Variant()
{
    if (ValueType == VariantType::String)
    {
        Value.String.~String();
    }
}

Variant::Variant(bool InValue)
    : ValueType(VariantType::Boolean)
{
    Value.Bool = InValue;
}

Variant::Variant(double InValue)
    : ValueType(VariantType::Float)
{
    Value.Float = InValue;
}

Variant::Variant(int64_t InValue)
    : ValueType(VariantType::Integer)
{
    Value.Int = InValue;
}

Variant::Variant(const char* InValue)
    : ValueType(VariantType::String)
{
    Value.String = InValue;
}

Variant::Variant(const csp::common::String InValue)
    : ValueType(VariantType::String)
{
    Value.String = InValue;
}

Variant::Variant(Vector3 InValue)
    : ValueType(VariantType::Vector3)
{
    Value.Vector3 = InValue;
}

Variant::Variant(const Vector4 InValue)
    : ValueType(VariantType::Vector4)
{
    Value.Vector4 = InValue;
}

Variant::Variant(const Variant& OtherValue) { *this = OtherValue; }

Variant& Variant::operator=(const Variant& InValue)
{
    switch (InValue.GetValueType())
    {
    case VariantType::Boolean:
    {
        SetBool(InValue.GetBool());
        break;
    }
    case VariantType::Integer:
    {
        SetInt(InValue.GetInt());
        break;
    }
    case VariantType::Float:
    {
        SetFloat(InValue.GetFloat());
        break;
    }
    case VariantType::String:
    {
        SetString(InValue.GetString());
        break;
    }
    case VariantType::Vector3:
    {
        SetVector3(InValue.GetVector3());
        break;
    }
    case VariantType::Vector4:
    {
        SetVector4(InValue.GetVector4());
        break;
    }
    case VariantType::InvalidType:
    {
        ValueType = VariantType::InvalidType;
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

bool Variant::operator==(const Variant& OtherValue) const
{
    bool IsEqual = GetValueType() == OtherValue.GetValueType();

    if (IsEqual)
    {
        switch (OtherValue.GetValueType())
        {
        case VariantType::Boolean:
        {
            IsEqual = GetBool() == OtherValue.GetBool();
            break;
        }
        case VariantType::Integer:
        {
            IsEqual = GetInt() == OtherValue.GetInt();
            break;
        }
        case VariantType::Float:
        {
            IsEqual = GetFloat() == OtherValue.GetFloat();
            break;
        }
        case VariantType::String:
        {
            IsEqual = GetString() == OtherValue.GetString();
            break;
        }
        case VariantType::Vector3:
        {
            IsEqual = GetVector3() == OtherValue.GetVector3();
            break;
        }
        case VariantType::Vector4:
        {
            IsEqual = GetVector4() == OtherValue.GetVector4();
            break;
        }
        default:
        {
            assert(0 && "Unhandled replicated value type!");
            break;
        }
        }
    }

    return IsEqual;
}

bool Variant::operator!=(const Variant& OtherValue) const { return (*this == OtherValue) == false; }

void Variant::SetBool(bool InValue)
{
    ValueType = VariantType::Boolean;
    Value.Bool = InValue;
}

bool Variant::GetBool() const
{
    assert(ValueType == VariantType::Boolean);
    return Value.Bool;
}

void Variant::SetFloat(double InValue)
{
    ValueType = VariantType::Float;
    Value.Float = InValue;
}

double Variant::GetFloat() const
{
    assert(ValueType == VariantType::Float);
    return Value.Float;
}

void Variant::SetInt(int64_t InValue)
{
    ValueType = VariantType::Integer;
    Value.Int = InValue;
}

int64_t Variant::GetInt() const
{
    assert(ValueType == VariantType::Integer);
    return Value.Int;
}

void Variant::SetString(const char* InValue)
{
    ValueType = VariantType::String;
    Value.String = InValue;
}

void Variant::SetString(const csp::common::String& InValue)
{
    ValueType = VariantType::String;
    Value.String = InValue;
}

const csp::common::String& Variant::GetString() const
{
    assert(ValueType == VariantType::String);
    return Value.String;
}

void Variant::SetVector3(Vector3 InValue)
{
    ValueType = VariantType::Vector3;
    Value.Vector3 = InValue;
}

Vector3 Variant::GetVector3() const
{
    assert(ValueType == VariantType::Vector3);
    return Value.Vector3;
}

void Variant::SetVector4(Vector4 InValue)
{
    ValueType = VariantType::Vector4;
    Value.Vector4 = InValue;
}

Vector4 Variant::GetVector4() const
{
    assert(ValueType == VariantType::Vector4);
    return Value.Vector4;
}

size_t Variant::GetSizeOfInternalValue() { return sizeof(InternalValue); }

Variant::InternalValue::InternalValue() { memset(this, 0x0, sizeof(InternalValue)); }

Variant::InternalValue::~InternalValue() { }

} // namespace csp::common
