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

ReplicatedValue::ReplicatedValue()
{
    ReplicatedType = ReplicatedValueType::InvalidType;
    Impl = CSP_NEW ReplicatedValueImpl();
}

ReplicatedValue::~ReplicatedValue() { }

ReplicatedValue::ReplicatedValue(bool InValue)
    : ReplicatedType(ReplicatedValueType::Boolean)
{
    Impl = CSP_NEW ReplicatedValueImpl(InValue);
}

ReplicatedValue::ReplicatedValue(float InValue)
    : ReplicatedType(ReplicatedValueType::Float)
{
    Impl = CSP_NEW ReplicatedValueImpl(InValue);
}

ReplicatedValue::ReplicatedValue(int64_t InValue)
    : ReplicatedType(ReplicatedValueType::Integer)
{
    Impl = CSP_NEW ReplicatedValueImpl(InValue);
}

ReplicatedValue::ReplicatedValue(const char* InValue)
    : ReplicatedType(ReplicatedValueType::String)
{
    Impl = CSP_NEW ReplicatedValueImpl(InValue);
}

ReplicatedValue::ReplicatedValue(const csp::common::String& InValue)
    : ReplicatedType(ReplicatedValueType::String)
{
    Impl = CSP_NEW ReplicatedValueImpl(InValue);
}

ReplicatedValue::ReplicatedValue(const csp::common::Vector2& InValue)
    : ReplicatedType(ReplicatedValueType::Vector2)
{
    Impl = CSP_NEW ReplicatedValueImpl(InValue);
}

ReplicatedValue::ReplicatedValue(const csp::common::Vector3& InValue)
    : ReplicatedType(ReplicatedValueType::Vector3)
{
    Impl = CSP_NEW ReplicatedValueImpl(InValue);
}

ReplicatedValue::ReplicatedValue(const csp::common::Vector4& InValue)
    : ReplicatedType(ReplicatedValueType::Vector4)
{
    Impl = CSP_NEW ReplicatedValueImpl(InValue);
}

ReplicatedValue::ReplicatedValue(const csp::common::Map<csp::common::String, ReplicatedValue>& InMapValue)
    : ReplicatedType(ReplicatedValueType::StringMap)
{
    Impl = CSP_NEW ReplicatedValueImpl(InMapValue);
}

ReplicatedValue::ReplicatedValue(const ReplicatedValue& OtherValue)
{
    this->ReplicatedType = OtherValue.ReplicatedType;
    this->Impl = CSP_NEW ReplicatedValueImpl(OtherValue.Impl);
}

ReplicatedValue& ReplicatedValue::operator=(const ReplicatedValue& InValue) { return *this; }

bool ReplicatedValue::operator==(const ReplicatedValue& OtherValue) const { return (*Impl) == OtherValue.Impl; }
bool ReplicatedValue::operator!=(const ReplicatedValue& OtherValue) const { return (*this == OtherValue) == false; }

bool ReplicatedValue::operator<(const ReplicatedValue& OtherValue) const { return (*Impl) < OtherValue.Impl; }

bool ReplicatedValue::operator>(const ReplicatedValue& OtherValue) const { return (*Impl) > OtherValue.Impl; }

void ReplicatedValue::SetBool(bool InValue)
{
    ReplicatedType = ReplicatedValueType::Boolean;
    Impl->Set(InValue);
}

bool ReplicatedValue::GetBool() const
{
    assert(ReplicatedType == ReplicatedValueType::Boolean);
    return Impl->Get<bool>();
}

void ReplicatedValue::SetFloat(float InValue)
{
    ReplicatedType = ReplicatedValueType::Float;
    Impl->Set(InValue);
}

float ReplicatedValue::GetFloat() const
{
    assert(ReplicatedType == ReplicatedValueType::Float);
    return Impl->Get<float>();
}

void ReplicatedValue::SetInt(int64_t InValue)
{
    ReplicatedType = ReplicatedValueType::Integer;
    Impl->Set(InValue);
}

int64_t ReplicatedValue::GetInt() const
{
    assert(ReplicatedType == ReplicatedValueType::Integer);
    return Impl->Get<int64_t>();
}

void ReplicatedValue::SetString(const char* InValue)
{
    ReplicatedType = ReplicatedValueType::String;
    Impl->Set(InValue);
}

void ReplicatedValue::SetString(const csp::common::String& InValue)
{
    ReplicatedType = ReplicatedValueType::String;
    Impl->Set(InValue.c_str());
}

const csp::common::String& ReplicatedValue::GetString() const
{
    assert(ReplicatedType == ReplicatedValueType::String);
    return Impl->Get<csp::common::String>();
}

const csp::common::String& ReplicatedValue::GetDefaultString()
{
    static const csp::common::String InvalidString = csp::common::String();
    return InvalidString;
}

void ReplicatedValue::SetVector2(const csp::common::Vector2& InValue)
{
    ReplicatedType = ReplicatedValueType::Vector2;
    Impl->Set(InValue);
}

const csp::common::Vector2& ReplicatedValue::GetVector2() const
{
    assert(ReplicatedType == ReplicatedValueType::Vector2);
    return Impl->Get<csp::common::Vector2>();
}

const csp::common::Vector2& ReplicatedValue::GetDefaultVector2() { return InvalidVector2; }

void ReplicatedValue::SetVector3(const csp::common::Vector3& InValue)
{
    ReplicatedType = ReplicatedValueType::Vector3;
    Impl->Set(InValue);
}

const csp::common::Vector3& ReplicatedValue::GetVector3() const
{
    assert(ReplicatedType == ReplicatedValueType::Vector3);
    return Impl->Get<csp::common::Vector3>();
}

const csp::common::Vector3& ReplicatedValue::GetDefaultVector3() { return InvalidVector3; }

void ReplicatedValue::SetVector4(const csp::common::Vector4& InValue)
{
    ReplicatedType = ReplicatedValueType::Vector4;
    Impl->Set(InValue);
}

const csp::common::Vector4& ReplicatedValue::GetVector4() const
{
    assert(ReplicatedType == ReplicatedValueType::Vector4);
    return Impl->Get<csp::common::Vector4>();
}

const csp::common::Vector4& ReplicatedValue::GetDefaultVector4() { return InvalidVector4; }

const csp::common::Map<csp::common::String, ReplicatedValue>& ReplicatedValue::GetStringMap() const
{
    assert(ReplicatedType == ReplicatedValueType::StringMap);
    return Impl->Get<csp::common::Map<csp::common::String, ReplicatedValue>>();
}

void ReplicatedValue::SetStringMap(const csp::common::Map<csp::common::String, ReplicatedValue>& InValue)
{
    ReplicatedType = ReplicatedValueType::StringMap;
    Impl->Set(InValue);
}

const csp::common::Map<csp::common::String, ReplicatedValue>& ReplicatedValue::GetDefaultStringMap() { return InvalidStringMap; }

size_t ReplicatedValue::GetSizeOfInternalValue() { return 0; }

} // namespace csp::multiplayer
