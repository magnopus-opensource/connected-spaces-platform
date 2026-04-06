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

#include "CSP/Multiplayer/Components/CodeSpaceComponent.h"
#include "Multiplayer/Script/ComponentScriptInterface.h"

#include <rapidjson/document.h>

namespace csp::multiplayer
{

CodeSpaceComponent::CodeSpaceComponent(csp::common::LogSystem* LogSystem, SpaceEntity* Parent)
    : ComponentBase(ComponentType::Code, LogSystem, Parent)
{
    SetScriptInterface(new ComponentScriptInterface(this));
    Properties[static_cast<uint32_t>(CodeSpaceComponentPropertyKeys::ScriptAssetPath)] = "";
    Properties[static_cast<uint32_t>(CodeSpaceComponentPropertyKeys::CodeScopeType)] = static_cast<int64_t>(csp::multiplayer::CodeScopeType::Local);
    Properties[static_cast<uint32_t>(CodeSpaceComponentPropertyKeys::Attributes)]
        = csp::common::Map<csp::common::String, csp::common::ReplicatedValue>();
    Properties[static_cast<uint32_t>(CodeSpaceComponentPropertyKeys::Schema)] = "";
}

const csp::common::String& CodeSpaceComponent::GetScriptAssetPath() const
{
    return GetStringProperty(static_cast<uint32_t>(CodeSpaceComponentPropertyKeys::ScriptAssetPath));
}

void CodeSpaceComponent::SetScriptAssetPath(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(CodeSpaceComponentPropertyKeys::ScriptAssetPath), Value);
}

csp::multiplayer::CodeScopeType CodeSpaceComponent::GetCodeScopeType() const
{
    return static_cast<csp::multiplayer::CodeScopeType>(GetIntegerProperty(static_cast<uint32_t>(CodeSpaceComponentPropertyKeys::CodeScopeType)));
}

void CodeSpaceComponent::SetCodeScopeType(csp::multiplayer::CodeScopeType Value)
{
    SetProperty(static_cast<uint32_t>(CodeSpaceComponentPropertyKeys::CodeScopeType), static_cast<int64_t>(Value));
}

bool CodeSpaceComponent::HasAttribute(const csp::common::String& Key) const
{
    const auto& Attributes = GetStringMapProperty(static_cast<uint32_t>(CodeSpaceComponentPropertyKeys::Attributes));
    return Attributes.HasKey(Key);
}

bool CodeSpaceComponent::GetAttribute(const csp::common::String& Key, CodeAttribute& OutAttribute) const
{
    const auto& Attributes = GetStringMapProperty(static_cast<uint32_t>(CodeSpaceComponentPropertyKeys::Attributes));
    const auto AttributeIt = Attributes.Find(Key);
    if (AttributeIt == Attributes.end())
    {
        return false;
    }

    return CodeAttribute::TryFromReplicatedValue(AttributeIt->second, OutAttribute);
}

void CodeSpaceComponent::SetAttribute(const csp::common::String& Key, const CodeAttribute& Attribute)
{
    auto Attributes = GetStringMapProperty(static_cast<uint32_t>(CodeSpaceComponentPropertyKeys::Attributes));
    Attributes[Key] = Attribute.ToReplicatedValue();
    SetProperty(static_cast<uint32_t>(CodeSpaceComponentPropertyKeys::Attributes), Attributes);
}

void CodeSpaceComponent::RemoveAttribute(const csp::common::String& Key)
{
    auto Attributes = GetStringMapProperty(static_cast<uint32_t>(CodeSpaceComponentPropertyKeys::Attributes));
    Attributes.Remove(Key);
    SetProperty(static_cast<uint32_t>(CodeSpaceComponentPropertyKeys::Attributes), Attributes);
}

void CodeSpaceComponent::ClearAttributes()
{
    SetProperty(
        static_cast<uint32_t>(CodeSpaceComponentPropertyKeys::Attributes), csp::common::Map<csp::common::String, csp::common::ReplicatedValue>());
}

csp::common::List<csp::common::String> CodeSpaceComponent::GetAttributeKeys() const
{
    csp::common::List<csp::common::String> AttributeKeys;
    const auto& Attributes = GetStringMapProperty(static_cast<uint32_t>(CodeSpaceComponentPropertyKeys::Attributes));
    for (const auto& AttributePair : Attributes)
    {
        AttributeKeys.Append(AttributePair.first);
    }
    return AttributeKeys;
}

const csp::common::String& CodeSpaceComponent::GetSchema() const
{
    return GetStringProperty(static_cast<uint32_t>(CodeSpaceComponentPropertyKeys::Schema));
}

void CodeSpaceComponent::SetSchema(const csp::common::String& SchemaJson)
{
    SetProperty(static_cast<uint32_t>(CodeSpaceComponentPropertyKeys::Schema), SchemaJson);
}

namespace
{

constexpr const char* MODEL_ASSET_COLLECTION_ID_KEY = "assetCollectionId";
constexpr const char* MODEL_ASSET_ASSET_ID_KEY = "assetId";
constexpr const char* IMAGE_ASSET_COLLECTION_ID_KEY = "assetCollectionId";
constexpr const char* IMAGE_ASSET_ID_KEY = "imageAssetId";

ScriptAttributeType SchemaTypeStringToEnum(const char* TypeString)
{
    if (!TypeString)
    {
        return ScriptAttributeType::Invalid;
    }

    if (strcmp(TypeString, "boolean") == 0)
    {
        return ScriptAttributeType::Boolean;
    }

    if (strcmp(TypeString, "integer") == 0)
    {
        return ScriptAttributeType::Integer;
    }

    if (strcmp(TypeString, "float") == 0 || strcmp(TypeString, "number") == 0)
    {
        return ScriptAttributeType::Float;
    }

    if (strcmp(TypeString, "string") == 0)
    {
        return ScriptAttributeType::String;
    }

    if (strcmp(TypeString, "entity") == 0)
    {
        return ScriptAttributeType::EntityQuery;
    }

    if ((strcmp(TypeString, "modelAsset") == 0) || (strcmp(TypeString, "modelasset") == 0))
    {
        return ScriptAttributeType::ModelAsset;
    }

    if ((strcmp(TypeString, "imageAsset") == 0) || (strcmp(TypeString, "imageasset") == 0))
    {
        return ScriptAttributeType::ImageAsset;
    }

    return ScriptAttributeType::Invalid;
}

// Extract the bare scalar ReplicatedValue from a decoded CodeAttribute.
// This is distinct from CodeAttribute::ToReplicatedValue(), which returns the serialisation
// envelope (a StringMap wrapper) used for storage. NgxScriptAttribute::Value must hold the
// plain scalar so that consumers can directly read/compare/display it.
csp::common::ReplicatedValue ExtractScalarFromCodeAttribute(const CodeAttribute& Attr)
{
    switch (Attr.Type)
    {
    case CodePropertyType::Boolean:
        return csp::common::ReplicatedValue(Attr.BooleanValue);
    case CodePropertyType::Integer:
        return csp::common::ReplicatedValue(Attr.IntegerValue);
    case CodePropertyType::Float:
        return csp::common::ReplicatedValue(Attr.FloatValue);
    case CodePropertyType::String:
        return csp::common::ReplicatedValue(Attr.StringValue);
    case CodePropertyType::EntityQuery:
        return csp::common::ReplicatedValue(Attr.EntityQueryValue);
    case CodePropertyType::ModelAsset:
        return csp::common::ReplicatedValue(Attr.ModelAssetValue);
    case CodePropertyType::ImageAsset:
        return csp::common::ReplicatedValue(Attr.ImageAssetValue);
    default:
        return csp::common::ReplicatedValue();
    }
}

bool TryApplySchemaDefault(const rapidjson::Value& Entry, ScriptAttributeType Type, NgxScriptAttribute& OutAttribute)
{
    if (!Entry.HasMember("default"))
    {
        return false;
    }

    const rapidjson::Value& DefaultValue = Entry["default"];
    switch (Type)
    {
    case ScriptAttributeType::Boolean:
        if (!DefaultValue.IsBool())
        {
            return false;
        }
        OutAttribute.Value = csp::common::ReplicatedValue(DefaultValue.GetBool());
        return true;

    case ScriptAttributeType::Integer:
        if (!DefaultValue.IsNumber())
        {
            return false;
        }
        OutAttribute.Value = csp::common::ReplicatedValue(static_cast<int64_t>(DefaultValue.GetInt64()));
        return true;

    case ScriptAttributeType::Float:
        if (!DefaultValue.IsNumber())
        {
            return false;
        }
        OutAttribute.Value = csp::common::ReplicatedValue(static_cast<float>(DefaultValue.GetDouble()));
        return true;

    case ScriptAttributeType::String:
        if (!DefaultValue.IsString())
        {
            return false;
        }
        OutAttribute.Value = csp::common::ReplicatedValue(csp::common::String(DefaultValue.GetString()));
        return true;

    case ScriptAttributeType::EntityQuery:
        if (DefaultValue.IsString())
        {
            csp::common::Map<csp::common::String, csp::common::ReplicatedValue> QueryMap;
            QueryMap["id"] = csp::common::ReplicatedValue(csp::common::String(DefaultValue.GetString()));
            OutAttribute.Value = csp::common::ReplicatedValue(QueryMap);
            return true;
        }

        if (DefaultValue.IsObject() && DefaultValue.HasMember("id") && DefaultValue["id"].IsString())
        {
            csp::common::Map<csp::common::String, csp::common::ReplicatedValue> QueryMap;
            QueryMap["id"] = csp::common::ReplicatedValue(csp::common::String(DefaultValue["id"].GetString()));
            OutAttribute.Value = csp::common::ReplicatedValue(QueryMap);
            return true;
        }
        return false;

    case ScriptAttributeType::ModelAsset:
    {
        if (!DefaultValue.IsObject())
        {
            return false;
        }

        if (!DefaultValue.HasMember(MODEL_ASSET_COLLECTION_ID_KEY) || !DefaultValue[MODEL_ASSET_COLLECTION_ID_KEY].IsString())
        {
            return false;
        }

        if (!DefaultValue.HasMember(MODEL_ASSET_ASSET_ID_KEY) || !DefaultValue[MODEL_ASSET_ASSET_ID_KEY].IsString())
        {
            return false;
        }

        csp::common::Map<csp::common::String, csp::common::ReplicatedValue> ModelAssetMap;
        ModelAssetMap[MODEL_ASSET_COLLECTION_ID_KEY]
            = csp::common::ReplicatedValue(csp::common::String(DefaultValue[MODEL_ASSET_COLLECTION_ID_KEY].GetString()));
        ModelAssetMap[MODEL_ASSET_ASSET_ID_KEY]
            = csp::common::ReplicatedValue(csp::common::String(DefaultValue[MODEL_ASSET_ASSET_ID_KEY].GetString()));
        OutAttribute.Value = csp::common::ReplicatedValue(ModelAssetMap);
        return true;
    }

    case ScriptAttributeType::ImageAsset:
    {
        if (!DefaultValue.IsObject())
        {
            return false;
        }

        if (!DefaultValue.HasMember(IMAGE_ASSET_COLLECTION_ID_KEY) || !DefaultValue[IMAGE_ASSET_COLLECTION_ID_KEY].IsString())
        {
            return false;
        }

        if (!DefaultValue.HasMember(IMAGE_ASSET_ID_KEY) || !DefaultValue[IMAGE_ASSET_ID_KEY].IsString())
        {
            return false;
        }

        csp::common::Map<csp::common::String, csp::common::ReplicatedValue> ImageAssetMap;
        ImageAssetMap[IMAGE_ASSET_COLLECTION_ID_KEY]
            = csp::common::ReplicatedValue(csp::common::String(DefaultValue[IMAGE_ASSET_COLLECTION_ID_KEY].GetString()));
        ImageAssetMap[IMAGE_ASSET_ID_KEY]
            = csp::common::ReplicatedValue(csp::common::String(DefaultValue[IMAGE_ASSET_ID_KEY].GetString()));
        OutAttribute.Value = csp::common::ReplicatedValue(ImageAssetMap);
        return true;
    }

    default:
        return false;
    }
}

void ApplyBuiltInTypeDefault(ScriptAttributeType Type, NgxScriptAttribute& OutAttribute)
{
    switch (Type)
    {
    case ScriptAttributeType::Boolean:
        OutAttribute.Value = csp::common::ReplicatedValue(false);
        break;

    case ScriptAttributeType::Integer:
        OutAttribute.Value = csp::common::ReplicatedValue(static_cast<int64_t>(0));
        break;

    case ScriptAttributeType::Float:
        OutAttribute.Value = csp::common::ReplicatedValue(0.0f);
        break;

    case ScriptAttributeType::String:
        OutAttribute.Value = csp::common::ReplicatedValue(csp::common::String(""));
        break;

    case ScriptAttributeType::EntityQuery:
    {
        csp::common::Map<csp::common::String, csp::common::ReplicatedValue> QueryMap;
        QueryMap["id"] = csp::common::ReplicatedValue(csp::common::String(""));
        OutAttribute.Value = csp::common::ReplicatedValue(QueryMap);
        break;
    }

    case ScriptAttributeType::ModelAsset:
    {
        csp::common::Map<csp::common::String, csp::common::ReplicatedValue> ModelAssetMap;
        ModelAssetMap[MODEL_ASSET_COLLECTION_ID_KEY] = csp::common::ReplicatedValue(csp::common::String(""));
        ModelAssetMap[MODEL_ASSET_ASSET_ID_KEY] = csp::common::ReplicatedValue(csp::common::String(""));
        OutAttribute.Value = csp::common::ReplicatedValue(ModelAssetMap);
        break;
    }

    case ScriptAttributeType::ImageAsset:
    {
        csp::common::Map<csp::common::String, csp::common::ReplicatedValue> ImageAssetMap;
        ImageAssetMap[IMAGE_ASSET_COLLECTION_ID_KEY] = csp::common::ReplicatedValue(csp::common::String(""));
        ImageAssetMap[IMAGE_ASSET_ID_KEY] = csp::common::ReplicatedValue(csp::common::String(""));
        OutAttribute.Value = csp::common::ReplicatedValue(ImageAssetMap);
        break;
    }

    default:
        break;
    }
}

} // namespace

csp::common::List<NgxScriptAttribute> CodeSpaceComponent::GetScriptAttributes() const
{
    csp::common::List<NgxScriptAttribute> Result;

    const csp::common::String& SchemaJson = GetSchema();
    if (SchemaJson.IsEmpty())
    {
        return Result;
    }

    rapidjson::Document SchemaDoc;
    SchemaDoc.Parse(SchemaJson.c_str());
    if (SchemaDoc.HasParseError() || !SchemaDoc.IsObject())
    {
        return Result;
    }

    const auto& Attributes = GetStringMapProperty(static_cast<uint32_t>(CodeSpaceComponentPropertyKeys::Attributes));

    for (auto SchemaIt = SchemaDoc.MemberBegin(); SchemaIt != SchemaDoc.MemberEnd(); ++SchemaIt)
    {
        if (!SchemaIt->value.IsObject())
        {
            continue;
        }

        const char* KeyName = SchemaIt->name.GetString();
        const auto& Entry = SchemaIt->value;

        if (!Entry.HasMember("type") || !Entry["type"].IsString())
        {
            continue;
        }

        const ScriptAttributeType ParsedType = SchemaTypeStringToEnum(Entry["type"].GetString());
        if (ParsedType == ScriptAttributeType::Invalid)
        {
            continue;
        }

        NgxScriptAttribute ScriptAttr;
        ScriptAttr.Name = KeyName;
        ScriptAttr.Type = ParsedType;

        if (Entry.HasMember("min") && Entry["min"].IsNumber())
        {
            ScriptAttr.Min = static_cast<float>(Entry["min"].GetDouble());
            ScriptAttr.HasMin = true;
        }

        if (Entry.HasMember("max") && Entry["max"].IsNumber())
        {
            ScriptAttr.Max = static_cast<float>(Entry["max"].GetDouble());
            ScriptAttr.HasMax = true;
        }

        if (Entry.HasMember("step") && Entry["step"].IsNumber())
        {
            ScriptAttr.Step = static_cast<float>(Entry["step"].GetDouble());
            ScriptAttr.HasStep = true;
        }

        if (Entry.HasMember("required") && Entry["required"].IsBool())
        {
            ScriptAttr.Required = Entry["required"].GetBool();
        }

        // Look up the current live value from the Attributes map.
        const auto ValueIt = Attributes.Find(KeyName);
        if (ValueIt != Attributes.end())
        {
            CodeAttribute CodeAttr;
            if (CodeAttribute::TryFromReplicatedValue(ValueIt->second, CodeAttr))
            {
                // Extract the plain scalar value, not the serialisation envelope.
                ScriptAttr.Value = ExtractScalarFromCodeAttribute(CodeAttr);
            }
            else
            {
                const bool bAppliedSchemaDefault = TryApplySchemaDefault(Entry, ScriptAttr.Type, ScriptAttr);
                if (!bAppliedSchemaDefault)
                {
                    ApplyBuiltInTypeDefault(ScriptAttr.Type, ScriptAttr);
                }
            }
        }
        else
        {
            const bool bAppliedSchemaDefault = TryApplySchemaDefault(Entry, ScriptAttr.Type, ScriptAttr);
            if (!bAppliedSchemaDefault)
            {
                ApplyBuiltInTypeDefault(ScriptAttr.Type, ScriptAttr);
            }
        }

        Result.Append(ScriptAttr);
    }

    return Result;
}

} // namespace csp::multiplayer
