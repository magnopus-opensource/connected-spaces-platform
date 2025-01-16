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
#include "CSP/Multiplayer/ReplicatedValue.h"

#include "Memory/Memory.h"

namespace csp::multiplayer
{
static const csp::common::Vector2 InvalidVector2 = csp::common::Vector2();
static const csp::common::Vector3 InvalidVector3 = csp::common::Vector3();
static const csp::common::Vector4 InvalidVector4 = csp::common::Vector4();
static const csp::common::Map<csp::common::String, ReplicatedValue> InvalidStringMap = csp::common::Map<csp::common::String, ReplicatedValue>();

ReplicatedValue::ReplicatedValue() { ReplicatedType = ReplicatedValueType::InvalidType; }

ReplicatedValue::~ReplicatedValue()
{
    if (ReplicatedType == ReplicatedValueType::String)
    {
        Value.String.~String();
    }
    else if (ReplicatedType == ReplicatedValueType::StringMap)
    {
        Value.StringMap.~Map();
    }
}

ReplicatedValue::ReplicatedValue(bool InValue)
    : ReplicatedType(ReplicatedValueType::Boolean)
{
    Value.Bool = InValue;
}

ReplicatedValue::ReplicatedValue(float InValue)
    : ReplicatedType(ReplicatedValueType::Float)
{
    Value.Float = InValue;
}

ReplicatedValue::ReplicatedValue(int64_t InValue)
    : ReplicatedType(ReplicatedValueType::Integer)
{
    Value.Int = InValue;
}

ReplicatedValue::ReplicatedValue(const char* InValue)
    : ReplicatedType(ReplicatedValueType::String)
{
    Value.String = InValue;
}

ReplicatedValue::ReplicatedValue(const csp::common::String& InValue)
    : ReplicatedType(ReplicatedValueType::String)
{
    Value.String = InValue;
}

ReplicatedValue::ReplicatedValue(const csp::common::Vector2& InValue)
    : ReplicatedType(ReplicatedValueType::Vector2)
{
    Value.Vector2 = InValue;
}

ReplicatedValue::ReplicatedValue(const csp::common::Vector3& InValue)
    : ReplicatedType(ReplicatedValueType::Vector3)
{
    Value.Vector3 = InValue;
}

ReplicatedValue::ReplicatedValue(const csp::common::Vector4& InValue)
    : ReplicatedType(ReplicatedValueType::Vector4)
{
    Value.Vector4 = InValue;
}

ReplicatedValue::ReplicatedValue(const csp::common::Map<csp::common::String, ReplicatedValue>& InMapValue)
    : ReplicatedType(ReplicatedValueType::StringMap)
{
    Value.StringMap = InMapValue;
}

ReplicatedValue::ReplicatedValue(const ReplicatedValue& OtherValue) { *this = OtherValue; }

ReplicatedValue& ReplicatedValue::operator=(const ReplicatedValue& InValue)
{
    switch (InValue.GetReplicatedValueType())
    {
    case ReplicatedValueType::Boolean:
    {
        SetBool(InValue.GetBool());
        break;
    }
    case ReplicatedValueType::Integer:
    {
        SetInt(InValue.GetInt());
        break;
    }
    case ReplicatedValueType::Float:
    {
        SetFloat(InValue.GetFloat());
        break;
    }
    case ReplicatedValueType::String:
    {
        SetString(InValue.GetString());
        break;
    }
    case ReplicatedValueType::Vector2:
    {
        SetVector2(InValue.GetVector2());
        break;
    }
    case ReplicatedValueType::Vector3:
    {
        SetVector3(InValue.GetVector3());
        break;
    }
    case ReplicatedValueType::Vector4:
    {
        SetVector4(InValue.GetVector4());
        break;
    }
    case ReplicatedValueType::StringMap:
    {
        SetStringMap(InValue.GetStringMap());
        break;
    }
    case ReplicatedValueType::InvalidType:
    {
        ReplicatedType = ReplicatedValueType::InvalidType;
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

bool ReplicatedValue::operator==(const ReplicatedValue& OtherValue) const
{
    bool IsEqual = GetReplicatedValueType() == OtherValue.GetReplicatedValueType();
    if (IsEqual)
    {
        switch (OtherValue.GetReplicatedValueType())
        {
        case ReplicatedValueType::Boolean:
        {
            IsEqual = GetBool() == OtherValue.GetBool();
            break;
        }
        case ReplicatedValueType::Integer:
        {
            IsEqual = GetInt() == OtherValue.GetInt();
            break;
        }
        case ReplicatedValueType::Float:
        {
            IsEqual = GetFloat() == OtherValue.GetFloat();
            break;
        }
        case ReplicatedValueType::String:
        {
            IsEqual = GetString() == OtherValue.GetString();
            break;
        }
        case ReplicatedValueType::Vector2:
        {
            IsEqual = GetVector2() == OtherValue.GetVector2();
            break;
        }
        case ReplicatedValueType::Vector3:
        {
            IsEqual = GetVector3() == OtherValue.GetVector3();
            break;
        }
        case ReplicatedValueType::Vector4:
        {
            IsEqual = GetVector4() == OtherValue.GetVector4();
            break;
        }
        case ReplicatedValueType::StringMap:
        {
            IsEqual = GetStringMap() == OtherValue.GetStringMap();
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
bool ReplicatedValue::operator!=(const ReplicatedValue& OtherValue) const { return (*this == OtherValue) == false; }

bool ReplicatedValue::operator<(const ReplicatedValue& OtherValue) const
{
    switch (OtherValue.GetReplicatedValueType())
    {
    case ReplicatedValueType::Boolean:
    {
        return GetBool() < OtherValue.GetBool();
    }
    case ReplicatedValueType::Integer:
    {
        return GetInt() < OtherValue.GetInt();
    }
    case ReplicatedValueType::Float:
    {
        return GetFloat() < OtherValue.GetFloat();
    }
    case ReplicatedValueType::String:
    {
        return GetString() < OtherValue.GetString();
    }
    default:
    {
        assert(0 && "Unhandled replicated value type!");
        break;
    }
    }

    return false;
}

bool ReplicatedValue::operator>(const ReplicatedValue& OtherValue) const
{
    switch (OtherValue.GetReplicatedValueType())
    {
    case ReplicatedValueType::Boolean:
    {
        return GetBool() > OtherValue.GetBool();
    }
    case ReplicatedValueType::Integer:
    {
        return GetInt() > OtherValue.GetInt();
    }
    case ReplicatedValueType::Float:
    {
        return GetFloat() > OtherValue.GetFloat();
    }
    case ReplicatedValueType::String:
    {
        return GetString() > OtherValue.GetString();
    }
    default:
    {
        assert(0 && "Unhandled replicated value type!");
        break;
    }
    }

    return false;
}

void ReplicatedValue::SetBool(bool InValue)
{
    ReplicatedType = ReplicatedValueType::Boolean;
    Value.Bool = InValue;
}

bool ReplicatedValue::GetBool() const
{
    assert(ReplicatedType == ReplicatedValueType::Boolean);
    return Value.Bool;
}

void ReplicatedValue::SetFloat(float InValue)
{
    ReplicatedType = ReplicatedValueType::Float;
    Value.Float = InValue;
}

float ReplicatedValue::GetFloat() const
{
    assert(ReplicatedType == ReplicatedValueType::Float);
    return Value.Float;
}

void ReplicatedValue::SetInt(int64_t InValue)
{
    ReplicatedType = ReplicatedValueType::Integer;
    Value.Int = InValue;
}

int64_t ReplicatedValue::GetInt() const
{
    assert(ReplicatedType == ReplicatedValueType::Integer);
    return Value.Int;
}

void ReplicatedValue::SetString(const char* InValue)
{
    ReplicatedType = ReplicatedValueType::String;
    Value.String = InValue;
}

void ReplicatedValue::SetString(const csp::common::String& InValue)
{
    ReplicatedType = ReplicatedValueType::String;
    Value.String = InValue;
}

const csp::common::String& ReplicatedValue::GetString() const
{
    assert(ReplicatedType == ReplicatedValueType::String);
    return Value.String;
}

const csp::common::String& ReplicatedValue::GetDefaultString()
{
    static const csp::common::String InvalidString = csp::common::String();
    return InvalidString;
}

void ReplicatedValue::SetVector2(const csp::common::Vector2& InValue)
{
    ReplicatedType = ReplicatedValueType::Vector2;
    Value.Vector2 = InValue;
}

const csp::common::Vector2& ReplicatedValue::GetVector2() const
{
    assert(ReplicatedType == ReplicatedValueType::Vector2);
    return Value.Vector2;
}

const csp::common::Vector2& ReplicatedValue::GetDefaultVector2() { return InvalidVector2; }

void ReplicatedValue::SetVector3(const csp::common::Vector3& InValue)
{
    ReplicatedType = ReplicatedValueType::Vector3;
    Value.Vector3 = InValue;
}

const csp::common::Vector3& ReplicatedValue::GetVector3() const
{
    assert(ReplicatedType == ReplicatedValueType::Vector3);
    return Value.Vector3;
}

const csp::common::Vector3& ReplicatedValue::GetDefaultVector3() { return InvalidVector3; }

void ReplicatedValue::SetVector4(const csp::common::Vector4& InValue)
{
    ReplicatedType = ReplicatedValueType::Vector4;
    Value.Vector4 = InValue;
}

const csp::common::Vector4& ReplicatedValue::GetVector4() const
{
    assert(ReplicatedType == ReplicatedValueType::Vector4);
    return Value.Vector4;
}

const csp::common::Vector4& ReplicatedValue::GetDefaultVector4() { return InvalidVector4; }

const csp::common::Map<csp::common::String, ReplicatedValue>& ReplicatedValue::GetStringMap() const
{
    assert(ReplicatedType == ReplicatedValueType::StringMap);
    return Value.StringMap;
}

void ReplicatedValue::SetStringMap(const csp::common::Map<csp::common::String, ReplicatedValue>& InValue)
{
    ReplicatedType = ReplicatedValueType::StringMap;
    Value.StringMap = InValue;
}

const csp::common::Map<csp::common::String, ReplicatedValue>& ReplicatedValue::GetDefaultStringMap() { return InvalidStringMap; }

size_t ReplicatedValue::GetSizeOfInternalValue() { return sizeof(InternalValue); }

ReplicatedValue::InternalValue::InternalValue() { memset(this, 0x0, sizeof(InternalValue)); }

ReplicatedValue::InternalValue::~InternalValue() { }

} // namespace csp::multiplayer
