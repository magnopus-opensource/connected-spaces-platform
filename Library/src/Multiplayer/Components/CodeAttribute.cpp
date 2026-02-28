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

#include "CSP/Multiplayer/Components/CodeAttribute.h"

namespace
{

constexpr const char* ATTRIBUTE_TYPE_KEY = "type";
constexpr const char* ATTRIBUTE_BOOLEAN_VALUE_KEY = "boolValue";
constexpr const char* ATTRIBUTE_INTEGER_VALUE_KEY = "intValue";
constexpr const char* ATTRIBUTE_FLOAT_VALUE_KEY = "floatValue";
constexpr const char* ATTRIBUTE_STRING_VALUE_KEY = "stringValue";

} // namespace

namespace csp::multiplayer
{

CodeAttribute::CodeAttribute()
    : Type(CodePropertyType::Invalid)
    , BooleanValue(false)
    , IntegerValue(0)
    , FloatValue(0.0f)
    , StringValue("")
{
}

CodeAttribute CodeAttribute::FromBoolean(bool Value)
{
    CodeAttribute Attribute;
    Attribute.Type = CodePropertyType::Boolean;
    Attribute.BooleanValue = Value;
    return Attribute;
}

CodeAttribute CodeAttribute::FromInteger(int64_t Value)
{
    CodeAttribute Attribute;
    Attribute.Type = CodePropertyType::Integer;
    Attribute.IntegerValue = Value;
    return Attribute;
}

CodeAttribute CodeAttribute::FromFloat(float Value)
{
    CodeAttribute Attribute;
    Attribute.Type = CodePropertyType::Float;
    Attribute.FloatValue = Value;
    return Attribute;
}

CodeAttribute CodeAttribute::FromString(const csp::common::String& Value)
{
    CodeAttribute Attribute;
    Attribute.Type = CodePropertyType::String;
    Attribute.StringValue = Value;
    return Attribute;
}

csp::common::ReplicatedValue CodeAttribute::ToReplicatedValue() const
{
    csp::common::Map<csp::common::String, csp::common::ReplicatedValue> SerializedValue;
    SerializedValue[ATTRIBUTE_TYPE_KEY] = static_cast<int64_t>(Type);
    SerializedValue[ATTRIBUTE_BOOLEAN_VALUE_KEY] = BooleanValue;
    SerializedValue[ATTRIBUTE_INTEGER_VALUE_KEY] = IntegerValue;
    SerializedValue[ATTRIBUTE_FLOAT_VALUE_KEY] = FloatValue;
    SerializedValue[ATTRIBUTE_STRING_VALUE_KEY] = StringValue;
    return csp::common::ReplicatedValue(SerializedValue);
}

bool CodeAttribute::TryFromReplicatedValue(const csp::common::ReplicatedValue& InValue, CodeAttribute& OutAttribute)
{
    if (InValue.GetReplicatedValueType() != csp::common::ReplicatedValueType::StringMap)
    {
        return false;
    }

    const auto& SerializedValue = InValue.GetStringMap();
    const auto TypeIt = SerializedValue.Find(ATTRIBUTE_TYPE_KEY);
    if ((TypeIt == SerializedValue.end()) || (TypeIt->second.GetReplicatedValueType() != csp::common::ReplicatedValueType::Integer))
    {
        return false;
    }

    const int64_t TypeValue = TypeIt->second.GetInt();
    if ((TypeValue <= static_cast<int64_t>(CodePropertyType::Invalid)) || (TypeValue >= static_cast<int64_t>(CodePropertyType::Num)))
    {
        return false;
    }

    OutAttribute = CodeAttribute();
    OutAttribute.Type = static_cast<CodePropertyType>(TypeValue);

    const auto BooleanValueIt = SerializedValue.Find(ATTRIBUTE_BOOLEAN_VALUE_KEY);
    if ((BooleanValueIt != SerializedValue.end()) && (BooleanValueIt->second.GetReplicatedValueType() == csp::common::ReplicatedValueType::Boolean))
    {
        OutAttribute.BooleanValue = BooleanValueIt->second.GetBool();
    }

    const auto IntegerValueIt = SerializedValue.Find(ATTRIBUTE_INTEGER_VALUE_KEY);
    if ((IntegerValueIt != SerializedValue.end()) && (IntegerValueIt->second.GetReplicatedValueType() == csp::common::ReplicatedValueType::Integer))
    {
        OutAttribute.IntegerValue = IntegerValueIt->second.GetInt();
    }

    const auto FloatValueIt = SerializedValue.Find(ATTRIBUTE_FLOAT_VALUE_KEY);
    if ((FloatValueIt != SerializedValue.end()) && (FloatValueIt->second.GetReplicatedValueType() == csp::common::ReplicatedValueType::Float))
    {
        OutAttribute.FloatValue = FloatValueIt->second.GetFloat();
    }

    const auto StringValueIt = SerializedValue.Find(ATTRIBUTE_STRING_VALUE_KEY);
    if ((StringValueIt != SerializedValue.end()) && (StringValueIt->second.GetReplicatedValueType() == csp::common::ReplicatedValueType::String))
    {
        OutAttribute.StringValue = StringValueIt->second.GetString();
    }

    return true;
}

bool CodeAttribute::operator==(const CodeAttribute& Other) const
{
    return (Type == Other.Type) && (BooleanValue == Other.BooleanValue) && (IntegerValue == Other.IntegerValue)
        && (FloatValue == Other.FloatValue) && (StringValue == Other.StringValue);
}

bool CodeAttribute::operator!=(const CodeAttribute& Other) const { return !(*this == Other); }

} // namespace csp::multiplayer

