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
constexpr const char* ATTRIBUTE_ENTITY_QUERY_VALUE_KEY = "entityQueryValue";
constexpr const char* ATTRIBUTE_MODEL_ASSET_VALUE_KEY = "modelAssetValue";
constexpr const char* ATTRIBUTE_IMAGE_ASSET_VALUE_KEY = "imageAssetValue";

constexpr const char* ENTITY_QUERY_KIND_KEY = "kind";
constexpr const char* ENTITY_QUERY_ID_KEY = "id";
constexpr const char* ENTITY_QUERY_NAME_KEY = "name";
constexpr const char* ENTITY_QUERY_TAG_KEY = "tag";
constexpr const char* ENTITY_QUERY_COMPONENT_TYPE_KEY = "componentType";
constexpr const char* ENTITY_QUERY_OPERANDS_KEY = "operands";
constexpr const char* ENTITY_QUERY_OPERAND_KEY = "operand";

constexpr const char* ENTITY_QUERY_KIND_ID = "id";
constexpr const char* ENTITY_QUERY_KIND_NAME = "name";
constexpr const char* ENTITY_QUERY_KIND_TAG = "tag";
constexpr const char* ENTITY_QUERY_KIND_COMPONENT_TYPE = "componentType";
constexpr const char* ENTITY_QUERY_KIND_AND = "and";
constexpr const char* ENTITY_QUERY_KIND_OR = "or";
constexpr const char* ENTITY_QUERY_KIND_NOT = "not";

constexpr int32_t ENTITY_QUERY_MAX_DEPTH = 16;
constexpr const char* MODEL_ASSET_COLLECTION_ID_KEY = "assetCollectionId";
constexpr const char* MODEL_ASSET_ASSET_ID_KEY = "assetId";
constexpr const char* IMAGE_ASSET_COLLECTION_ID_KEY = "assetCollectionId";
constexpr const char* IMAGE_ASSET_ID_KEY = "imageAssetId";

using EntityQueryValueType = csp::multiplayer::CodeAttribute::EntityQueryValueType;
using ModelAssetValueType = csp::multiplayer::CodeAttribute::ModelAssetValueType;
using ImageAssetValueType = csp::multiplayer::CodeAttribute::ImageAssetValueType;

template<typename TValueMap>
bool TryGetStringField(const TValueMap& Value, const char* Key, csp::common::String& OutValue)
{
    const auto ValueIt = Value.Find(Key);
    if ((ValueIt == Value.end()) || (ValueIt->second.GetReplicatedValueType() != csp::common::ReplicatedValueType::String))
    {
        return false;
    }

    OutValue = ValueIt->second.GetString();
    return true;
}

bool IsStringMapReplicatedValue(const csp::common::ReplicatedValue& Value)
{
    return Value.GetReplicatedValueType() == csp::common::ReplicatedValueType::StringMap;
}

bool IsValidEntityQueryValueImpl(const EntityQueryValueType& Value, int32_t Depth)
{
    if (Depth > ENTITY_QUERY_MAX_DEPTH)
    {
        return false;
    }

    csp::common::String Kind;
    if (!TryGetStringField(Value, ENTITY_QUERY_KIND_KEY, Kind))
    {
        return false;
    }

    if (Kind == ENTITY_QUERY_KIND_ID)
    {
        const auto IdIt = Value.Find(ENTITY_QUERY_ID_KEY);
        return (IdIt != Value.end()) && (IdIt->second.GetReplicatedValueType() == csp::common::ReplicatedValueType::Integer)
            && (IdIt->second.GetInt() > 0);
    }

    if (Kind == ENTITY_QUERY_KIND_NAME)
    {
        const auto NameIt = Value.Find(ENTITY_QUERY_NAME_KEY);
        return (NameIt != Value.end()) && (NameIt->second.GetReplicatedValueType() == csp::common::ReplicatedValueType::String)
            && !NameIt->second.GetString().IsEmpty();
    }

    if (Kind == ENTITY_QUERY_KIND_TAG)
    {
        const auto TagIt = Value.Find(ENTITY_QUERY_TAG_KEY);
        return (TagIt != Value.end()) && (TagIt->second.GetReplicatedValueType() == csp::common::ReplicatedValueType::String)
            && !TagIt->second.GetString().IsEmpty();
    }

    if (Kind == ENTITY_QUERY_KIND_COMPONENT_TYPE)
    {
        const auto ComponentTypeIt = Value.Find(ENTITY_QUERY_COMPONENT_TYPE_KEY);
        return (ComponentTypeIt != Value.end()) && (ComponentTypeIt->second.GetReplicatedValueType() == csp::common::ReplicatedValueType::Integer)
            && (ComponentTypeIt->second.GetInt() > 0);
    }

    if (Kind == ENTITY_QUERY_KIND_NOT)
    {
        const auto OperandIt = Value.Find(ENTITY_QUERY_OPERAND_KEY);
        if ((OperandIt == Value.end()) || !IsStringMapReplicatedValue(OperandIt->second))
        {
            return false;
        }

        return IsValidEntityQueryValueImpl(OperandIt->second.GetStringMap(), Depth + 1);
    }

    if ((Kind == ENTITY_QUERY_KIND_AND) || (Kind == ENTITY_QUERY_KIND_OR))
    {
        const auto OperandsIt = Value.Find(ENTITY_QUERY_OPERANDS_KEY);
        if ((OperandsIt == Value.end()) || !IsStringMapReplicatedValue(OperandsIt->second))
        {
            return false;
        }

        const auto& Operands = OperandsIt->second.GetStringMap();
        if (Operands.Size() == 0)
        {
            return false;
        }

        for (const auto& OperandPair : Operands)
        {
            if (!IsStringMapReplicatedValue(OperandPair.second))
            {
                return false;
            }

            if (!IsValidEntityQueryValueImpl(OperandPair.second.GetStringMap(), Depth + 1))
            {
                return false;
            }
        }

        return true;
    }

    return false;
}

bool IsValidModelAssetValueImpl(const ModelAssetValueType& Value)
{
    csp::common::String AssetCollectionId;
    csp::common::String AssetId;
    return TryGetStringField(Value, MODEL_ASSET_COLLECTION_ID_KEY, AssetCollectionId) && TryGetStringField(Value, MODEL_ASSET_ASSET_ID_KEY, AssetId);
}

bool IsValidImageAssetValueImpl(const ImageAssetValueType& Value)
{
    csp::common::String AssetCollectionId;
    csp::common::String ImageAssetId;
    return TryGetStringField(Value, IMAGE_ASSET_COLLECTION_ID_KEY, AssetCollectionId) && TryGetStringField(Value, IMAGE_ASSET_ID_KEY, ImageAssetId);
}

} // namespace

namespace csp::multiplayer
{

CodeAttribute::CodeAttribute()
    : Type(CodePropertyType::Invalid)
    , BooleanValue(false)
    , IntegerValue(0)
    , FloatValue(0.0f)
    , StringValue("")
    , EntityQueryValue()
    , ModelAssetValue()
    , ImageAssetValue()
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

CodeAttribute CodeAttribute::FromEntityQuery(const EntityQueryValueType& Value)
{
    CodeAttribute Attribute;
    Attribute.SetEntityQueryValue(Value);
    return Attribute;
}

CodeAttribute CodeAttribute::FromModelAsset(const ModelAssetValueType& Value)
{
    CodeAttribute Attribute;
    Attribute.SetModelAssetValue(Value);
    return Attribute;
}

CodeAttribute CodeAttribute::FromImageAsset(const ImageAssetValueType& Value)
{
    CodeAttribute Attribute;
    Attribute.SetImageAssetValue(Value);
    return Attribute;
}

bool CodeAttribute::IsValidEntityQueryValue(const EntityQueryValueType& Value) { return IsValidEntityQueryValueImpl(Value, 0); }

const CodeAttribute::EntityQueryValueType& CodeAttribute::GetEntityQueryValue() const { return EntityQueryValue; }

void CodeAttribute::SetEntityQueryValue(const EntityQueryValueType& Value)
{
    if (!IsValidEntityQueryValue(Value))
    {
        Type = CodePropertyType::Invalid;
        EntityQueryValue = EntityQueryValueType();
        return;
    }

    Type = CodePropertyType::EntityQuery;
    EntityQueryValue = Value;
}

bool CodeAttribute::IsValidModelAssetValue(const ModelAssetValueType& Value) { return IsValidModelAssetValueImpl(Value); }

const CodeAttribute::ModelAssetValueType& CodeAttribute::GetModelAssetValue() const { return ModelAssetValue; }

void CodeAttribute::SetModelAssetValue(const ModelAssetValueType& Value)
{
    if (!IsValidModelAssetValue(Value))
    {
        Type = CodePropertyType::Invalid;
        ModelAssetValue = ModelAssetValueType();
        return;
    }

    Type = CodePropertyType::ModelAsset;
    ModelAssetValue = Value;
}

bool CodeAttribute::IsValidImageAssetValue(const ImageAssetValueType& Value) { return IsValidImageAssetValueImpl(Value); }

const CodeAttribute::ImageAssetValueType& CodeAttribute::GetImageAssetValue() const { return ImageAssetValue; }

void CodeAttribute::SetImageAssetValue(const ImageAssetValueType& Value)
{
    if (!IsValidImageAssetValue(Value))
    {
        Type = CodePropertyType::Invalid;
        ImageAssetValue = ImageAssetValueType();
        return;
    }

    Type = CodePropertyType::ImageAsset;
    ImageAssetValue = Value;
}

csp::common::ReplicatedValue CodeAttribute::ToReplicatedValue() const
{
    csp::common::Map<csp::common::String, csp::common::ReplicatedValue> SerializedValue;
    SerializedValue[ATTRIBUTE_TYPE_KEY] = static_cast<int64_t>(Type);
    SerializedValue[ATTRIBUTE_BOOLEAN_VALUE_KEY] = BooleanValue;
    SerializedValue[ATTRIBUTE_INTEGER_VALUE_KEY] = IntegerValue;
    SerializedValue[ATTRIBUTE_FLOAT_VALUE_KEY] = FloatValue;
    SerializedValue[ATTRIBUTE_STRING_VALUE_KEY] = StringValue;
    SerializedValue[ATTRIBUTE_ENTITY_QUERY_VALUE_KEY] = EntityQueryValue;
    SerializedValue[ATTRIBUTE_MODEL_ASSET_VALUE_KEY] = ModelAssetValue;
    SerializedValue[ATTRIBUTE_IMAGE_ASSET_VALUE_KEY] = ImageAssetValue;
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

    if (OutAttribute.Type == CodePropertyType::EntityQuery)
    {
        const auto EntityQueryValueIt = SerializedValue.Find(ATTRIBUTE_ENTITY_QUERY_VALUE_KEY);
        if ((EntityQueryValueIt == SerializedValue.end()) || !IsStringMapReplicatedValue(EntityQueryValueIt->second))
        {
            return false;
        }

        const auto& ParsedEntityQueryValue = EntityQueryValueIt->second.GetStringMap();
        if (!IsValidEntityQueryValue(ParsedEntityQueryValue))
        {
            return false;
        }

        OutAttribute.EntityQueryValue = ParsedEntityQueryValue;
    }

    if (OutAttribute.Type == CodePropertyType::ModelAsset)
    {
        const auto ModelAssetValueIt = SerializedValue.Find(ATTRIBUTE_MODEL_ASSET_VALUE_KEY);
        if ((ModelAssetValueIt == SerializedValue.end()) || !IsStringMapReplicatedValue(ModelAssetValueIt->second))
        {
            return false;
        }

        const auto& ParsedModelAssetValue = ModelAssetValueIt->second.GetStringMap();
        if (!IsValidModelAssetValue(ParsedModelAssetValue))
        {
            return false;
        }

        OutAttribute.ModelAssetValue = ParsedModelAssetValue;
    }

    if (OutAttribute.Type == CodePropertyType::ImageAsset)
    {
        const auto ImageAssetValueIt = SerializedValue.Find(ATTRIBUTE_IMAGE_ASSET_VALUE_KEY);
        if ((ImageAssetValueIt == SerializedValue.end()) || !IsStringMapReplicatedValue(ImageAssetValueIt->second))
        {
            return false;
        }

        const auto& ParsedImageAssetValue = ImageAssetValueIt->second.GetStringMap();
        if (!IsValidImageAssetValue(ParsedImageAssetValue))
        {
            return false;
        }

        OutAttribute.ImageAssetValue = ParsedImageAssetValue;
    }

    return true;
}

bool CodeAttribute::operator==(const CodeAttribute& Other) const
{
    return (Type == Other.Type) && (BooleanValue == Other.BooleanValue) && (IntegerValue == Other.IntegerValue)
        && (FloatValue == Other.FloatValue) && (StringValue == Other.StringValue) && (EntityQueryValue == Other.EntityQueryValue)
        && (ModelAssetValue == Other.ModelAssetValue) && (ImageAssetValue == Other.ImageAssetValue);
}

bool CodeAttribute::operator!=(const CodeAttribute& Other) const { return !(*this == Other); }

} // namespace csp::multiplayer

