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

#include "CSP/Multiplayer/ComponentSchema.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "Common/Convert.h"
#include "Json/JsonSerializer.h"

#include <fmt/format.h>
#include <rapidjson/document.h>

#include <algorithm>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace csp::multiplayer
{

namespace
{
    void SerializeStringProperty(csp::json::JsonSerializer& Serializer, const csp::common::String& Value)
    {
        Serializer.SerializeMember("type", "string");
        Serializer.SerializeMember("defaultValue", Value);
    }

    void SerializeFloatProperty(csp::json::JsonSerializer& Serializer, float Value)
    {
        Serializer.SerializeMember("type", "float");
        Serializer.SerializeMember("defaultValue", Value);
    }

    void SerializeIntProperty(csp::json::JsonSerializer& Serializer, int64_t Value)
    {
        Serializer.SerializeMember("type", "int");
        Serializer.SerializeMember("defaultValue", Value);
    }

    void SerializeBoolProperty(csp::json::JsonSerializer& Serializer, bool Value)
    {
        Serializer.SerializeMember("type", "bool");
        Serializer.SerializeMember("defaultValue", Value);
    }

    void SerializeVec2Property(csp::json::JsonSerializer& Serializer, const csp::common::Vector2& Vector)
    {
        Serializer.SerializeMember("type", "vec2");
        Serializer.SerializeMember("defaultValue", std::vector<float> { Vector.X, Vector.Y });
    }

    void SerializeVec3Property(csp::json::JsonSerializer& Serializer, const csp::common::Vector3& Vector)
    {
        Serializer.SerializeMember("type", "vec3");
        Serializer.SerializeMember("defaultValue", std::vector<float> { Vector.X, Vector.Y, Vector.Z });
    }

    void SerializeVec4Property(csp::json::JsonSerializer& Serializer, const csp::common::Vector4& Vector)
    {
        Serializer.SerializeMember("type", "vec4");
        Serializer.SerializeMember("defaultValue", std::vector<float> { Vector.X, Vector.Y, Vector.Z, Vector.W });
    }

    bool IsFloatArray(const rapidjson::Value& Value, rapidjson::SizeType ExpectedSize)
    {
        if (!Value.IsArray() || Value.Size() != ExpectedSize)
        {
            return false;
        }

        const auto& Array = Value.GetArray();
        return std::all_of(Array.begin(), Array.end(), [](const auto& Element) { return Element.IsNumber(); });
    }

    template <typename T> std::optional<T> GetRequired(const rapidjson::Value& Object, const char* Key)
    {
        if (!Object.HasMember(Key) || !Object[Key].Is<T>())
        {
            return std::nullopt;
        }

        return Object[Key].Get<T>();
    }

    template <typename T> T GetOrDefault(const rapidjson::Value& Object, const char* Key, T Default)
    {
        if (!Object.HasMember(Key) || !Object[Key].Is<T>())
        {
            return std::move(Default);
        }

        return Object[Key].Get<T>();
    }

    template <typename T> std::optional<csp::common::ReplicatedValue> AsReplicatedValue(std::optional<T> Value)
    {
        if (!Value)
        {
            return std::nullopt;
        }

        return std::move(*Value);
    }

    std::optional<csp::common::String> TryParseString(const rapidjson::Value& Value)
    {
        if (!Value.IsString())
        {
            return std::nullopt;
        }

        return csp::common::String { Value.GetString() };
    }

    std::optional<float> TryParseFloat(const rapidjson::Value& Value)
    {
        if (!Value.IsNumber())
        {
            return std::nullopt;
        }

        return Value.GetFloat();
    }

    std::optional<int64_t> TryParseInt(const rapidjson::Value& Value)
    {
        if (!Value.IsInt64())
        {
            return std::nullopt;
        }

        return Value.GetInt64();
    }

    std::optional<bool> TryParseBool(const rapidjson::Value& Value)
    {
        if (!Value.IsBool())
        {
            return std::nullopt;
        }

        return Value.GetBool();
    }

    std::optional<csp::common::Vector2> TryParseVec2(const rapidjson::Value& Value)
    {
        if (!IsFloatArray(Value, 2))
        {
            return std::nullopt;
        }

        return csp::common::Vector2 {
            Value[0].GetFloat(),
            Value[1].GetFloat(),
        };
    }

    std::optional<csp::common::Vector3> TryParseVec3(const rapidjson::Value& Value)
    {
        if (!IsFloatArray(Value, 3))
        {
            return std::nullopt;
        }

        return csp::common::Vector3 {
            Value[0].GetFloat(),
            Value[1].GetFloat(),
            Value[2].GetFloat(),
        };
    }

    std::optional<csp::common::Vector4> TryParseVec4(const rapidjson::Value& Value)
    {
        if (!IsFloatArray(Value, 4))
        {
            return std::nullopt;
        }

        return csp::common::Vector4 {
            Value[0].GetFloat(),
            Value[1].GetFloat(),
            Value[2].GetFloat(),
            Value[3].GetFloat(),
        };
    }

    std::optional<csp::common::ReplicatedValue> TryParse(const std::string& Type, const rapidjson::Value& Value)
    {
        if (Type == "string")
        {
            return AsReplicatedValue(TryParseString(Value));
        }

        if (Type == "float")
        {
            return AsReplicatedValue(TryParseFloat(Value));
        }

        if (Type == "int")
        {
            return AsReplicatedValue(TryParseInt(Value));
        }

        if (Type == "bool")
        {
            return AsReplicatedValue(TryParseBool(Value));
        }

        if (Type == "vec2")
        {
            return AsReplicatedValue(TryParseVec2(Value));
        }

        if (Type == "vec3")
        {
            return AsReplicatedValue(TryParseVec3(Value));
        }

        if (Type == "vec4")
        {
            return AsReplicatedValue(TryParseVec4(Value));
        }

        return std::nullopt;
    }

    std::optional<std::pair<csp::common::ReplicatedValue, csp::common::ReplicatedValue>> TryParseRange(
        const rapidjson::Value& RangeValue, const std::string& Type)
    {
        if (!RangeValue.IsObject() || !RangeValue.HasMember("min") || !RangeValue.HasMember("max"))
        {
            return std::nullopt;
        }

        const auto Min = TryParse(Type, RangeValue["min"]);
        const auto Max = TryParse(Type, RangeValue["max"]);

        if (!Min || !Max)
        {
            return std::nullopt;
        }

        return std::make_pair(*Min, *Max);
    }

    std::optional<SchemaOption> TryParseOption(const rapidjson::Value& OptionValue, const std::string& Type)
    {
        if (!OptionValue.IsObject())
        {
            return std::nullopt;
        }

        const auto Name = GetRequired<const char*>(OptionValue, "name");

        if (!Name)
        {
            return std::nullopt;
        }

        if (!OptionValue.HasMember("value"))
        {
            return std::nullopt;
        }

        const auto Value = TryParse(Type, OptionValue["value"]);

        if (!Value)
        {
            return std::nullopt;
        }

        return SchemaOption {
            /*Name =*/*Name,
            /*Value =*/*Value,
        };
    }

    std::optional<csp::common::Array<SchemaOption>> TryParseOptions(rapidjson::Value::ConstArray JsonOptions, const std::string& Type)
    {
        auto Options = std::vector<SchemaOption> {};

        for (const auto& Element : JsonOptions)
        {
            const auto Option = TryParseOption(Element, Type);

            if (!Option)
            {
                return std::nullopt;
            }

            Options.push_back(*Option);
        }

        return csp::common::Convert(Options);
    }

    std::optional<ComponentProperty> TryParseProperty(const rapidjson::Value& Value, csp::common::LogSystem* LogSystem = nullptr)
    {
        if (!Value.IsObject())
        {
            if (LogSystem)
            {
                LogSystem->LogMsg(csp::common::LogLevel::Warning, "TryParseProperty: not a JSON object");
            }

            return std::nullopt;
        }

        const auto Key = GetRequired<unsigned int>(Value, "key");

        if (!Key)
        {
            if (LogSystem)
            {
                LogSystem->LogMsg(csp::common::LogLevel::Warning, "TryParseProperty: 'key' must be an unsigned integer");
            }

            return std::nullopt;
        }

        const auto Description = GetOrDefault<const char*>(Value, "description", "");
        const auto IsDeprecated = GetOrDefault<bool>(Value, "deprecated", false);

        if (GetOrDefault<bool>(Value, "reserved", false))
        {
            return ComponentProperty {
                /*Key =*/static_cast<ComponentProperty::KeyType>(*Key),
                /*Name =*/ {},
                /*DefaultValue =*/ {},
                /*Description =*/Description,
                /*IsScriptable =*/false,
                /*IsDeprecated =*/IsDeprecated,
                /*IsReserved =*/true,
            };
        }

        const auto Name = GetRequired<const char*>(Value, "name");

        if (!Name)
        {
            if (LogSystem)
            {
                LogSystem->LogMsg(csp::common::LogLevel::Warning, "TryParseProperty: 'name' must be a string");
            }

            return std::nullopt;
        }

        const auto Type = GetRequired<const char*>(Value, "type");

        if (!Type)
        {
            if (LogSystem)
            {
                LogSystem->LogMsg(csp::common::LogLevel::Warning, "TryParseProperty: 'type' must be a string");
            }

            return std::nullopt;
        }

        if (!Value.HasMember("defaultValue"))
        {
            if (LogSystem)
            {
                LogSystem->LogMsg(csp::common::LogLevel::Warning, "TryParseProperty: missing 'defaultValue'");
            }

            return std::nullopt;
        }

        const auto DefaultValue = TryParse(*Type, Value["defaultValue"]);

        if (!DefaultValue)
        {
            if (LogSystem)
            {
                LogSystem->LogMsg(
                    csp::common::LogLevel::Warning, fmt::format("TryParseProperty: 'defaultValue' is not valid for type '{}'", *Type).c_str());
            }

            return std::nullopt;
        }

        const auto IsScriptable = GetOrDefault<bool>(Value, "scripting", true);

        using csp::common::ReplicatedValueType;
        const auto ValueType = DefaultValue->GetReplicatedValueType();

        auto RangeMin = csp::common::ReplicatedValue {};
        auto RangeMax = csp::common::ReplicatedValue {};

        if (Value.HasMember("range"))
        {
            if (ValueType != ReplicatedValueType::Integer && ValueType != ReplicatedValueType::Float)
            {
                if (LogSystem)
                {
                    LogSystem->LogMsg(
                        csp::common::LogLevel::Warning, fmt::format("TryParseProperty: 'range' is not supported for type '{}'", *Type).c_str());
                }

                return std::nullopt;
            }

            const auto Range = TryParseRange(Value["range"], *Type);

            if (!Range)
            {
                if (LogSystem)
                {
                    LogSystem->LogMsg(csp::common::LogLevel::Warning, "TryParseProperty: 'range' is malformed");
                }

                return std::nullopt;
            }

            RangeMin = Range->first;
            RangeMax = Range->second;
        }

        auto Options = csp::common::Convert(std::vector<SchemaOption> {});

        if (Value.HasMember("options"))
        {
            if (ValueType != ReplicatedValueType::Integer && ValueType != ReplicatedValueType::Float && ValueType != ReplicatedValueType::String)
            {
                if (LogSystem)
                {
                    LogSystem->LogMsg(
                        csp::common::LogLevel::Warning, fmt::format("TryParseProperty: 'options' is not supported for type '{}'", *Type).c_str());
                }

                return std::nullopt;
            }

            const auto JsonOptions = GetRequired<rapidjson::Value::ConstArray>(Value, "options");

            if (!JsonOptions)
            {
                if (LogSystem)
                {
                    LogSystem->LogMsg(csp::common::LogLevel::Warning, "TryParseProperty: 'options' must be an array");
                }

                return std::nullopt;
            }

            const auto ParsedOptions = TryParseOptions(*JsonOptions, *Type);

            if (!ParsedOptions)
            {
                if (LogSystem)
                {
                    LogSystem->LogMsg(csp::common::LogLevel::Warning, "TryParseProperty: 'options' contains an invalid entry");
                }

                return std::nullopt;
            }

            Options = *ParsedOptions;
        }

        return ComponentProperty {
            /*Key =*/static_cast<ComponentProperty::KeyType>(*Key),
            /*Name =*/*Name,
            /*DefaultValue =*/*DefaultValue,
            /*Description =*/Description,
            /*IsScriptable =*/IsScriptable,
            /*IsDeprecated =*/IsDeprecated,
            /*IsReserved =*/false,
            /*RangeMin =*/RangeMin,
            /*RangeMax =*/RangeMax,
            /*Options =*/std::move(Options),
        };
    }

    std::optional<csp::common::Array<ComponentProperty>> TryParseProperties(
        rapidjson::Value::ConstArray JsonProperties, csp::common::LogSystem* LogSystem = nullptr)
    {
        auto Properties = std::vector<ComponentProperty> {};

        for (const auto& Element : JsonProperties)
        {
            const auto Property = TryParseProperty(Element, LogSystem);

            if (!Property)
            {
                return std::nullopt;
            }

            Properties.push_back(*Property);
        }

        return csp::common::Convert(Properties);
    }

    std::optional<ComponentSchema> TryParseSchema(const rapidjson::Value& Value, csp::common::LogSystem* LogSystem = nullptr)
    {
        if (!Value.IsObject())
        {
            if (LogSystem)
            {
                LogSystem->LogMsg(csp::common::LogLevel::Warning, "TryParseSchema: not a JSON object");
            }

            return std::nullopt;
        }

        const auto TypeId = GetRequired<uint64_t>(Value, "typeId");

        if (!TypeId)
        {
            if (LogSystem)
            {
                LogSystem->LogMsg(csp::common::LogLevel::Warning, "TryParseSchema: 'typeId' must be a uint64");
            }

            return std::nullopt;
        }

        const auto IsReserved = GetOrDefault<bool>(Value, "reserved", false);
        const auto Description = GetOrDefault<const char*>(Value, "description", "");
        const auto IsScriptable = GetOrDefault<bool>(Value, "scripting", true);
        const auto IsDeprecated = GetOrDefault<bool>(Value, "deprecated", false);

        if (IsReserved)
        {
            return ComponentSchema {
                /*TypeId =*/*TypeId,
                /*Name =*/ {},
                /*Properties =*/ {},
                /*Description =*/Description,
                /*IsScriptable =*/false,
                /*IsReserved =*/true,
            };
        }

        const auto Name = GetRequired<const char*>(Value, "name");

        if (!Name)
        {
            if (LogSystem)
            {
                LogSystem->LogMsg(csp::common::LogLevel::Warning, "TryParseSchema: 'name' must be a string");
            }

            return std::nullopt;
        }

        const auto JsonProperties = GetRequired<rapidjson::Value::ConstArray>(Value, "properties");

        if (!JsonProperties)
        {
            if (LogSystem)
            {
                LogSystem->LogMsg(csp::common::LogLevel::Warning, "TryParseSchema: 'properties' must be an array");
            }

            return std::nullopt;
        }

        const auto Properties = TryParseProperties(*JsonProperties, LogSystem);

        if (!Properties)
        {
            return std::nullopt;
        }

        return ComponentSchema {
            /*TypeId =*/*TypeId,
            /*Name =*/*Name,
            /*Properties =*/*Properties,
            /*Description =*/Description,
            /*IsScriptable =*/IsScriptable,
            /*IsReserved =*/false,
            /*IsDeprecated =*/IsDeprecated,
        };
    }

    void SerializeReplicatedValueMember(csp::json::JsonSerializer& Serializer, const char* Key, const csp::common::ReplicatedValue& Value)
    {
        using csp::common::ReplicatedValueType;

        switch (Value.GetReplicatedValueType())
        {
        case ReplicatedValueType::Float:
            Serializer.SerializeMember(Key, Value.GetFloat());
            break;
        case ReplicatedValueType::Integer:
            Serializer.SerializeMember(Key, Value.GetInt());
            break;
        case ReplicatedValueType::String:
            Serializer.SerializeMember(Key, Value.GetString());
            break;
        default:
            break;
        }
    }

} // namespace

struct PropertyRange
{
    csp::common::ReplicatedValue Min;
    csp::common::ReplicatedValue Max;
};

void ToJson(csp::json::JsonSerializer& Serializer, const PropertyRange& Range)
{
    SerializeReplicatedValueMember(Serializer, "min", Range.Min);
    SerializeReplicatedValueMember(Serializer, "max", Range.Max);
}

void ToJson(csp::json::JsonSerializer& Serializer, const SchemaOption& Option)
{
    Serializer.SerializeMember("name", Option.Name);
    SerializeReplicatedValueMember(Serializer, "value", Option.Value);
}

void ToJson(csp::json::JsonSerializer& Serializer, const ComponentProperty& Property)
{
    using csp::common::ReplicatedValueType;

    Serializer.SerializeMember("key", static_cast<uint32_t>(Property.Key));

    if (Property.IsReserved)
    {
        Serializer.SerializeMember("reserved", true);

        if (!Property.Description.IsEmpty())
        {
            Serializer.SerializeMember("description", Property.Description);
        }

        return;
    }

    Serializer.SerializeMember("name", Property.Name);

    switch (Property.DefaultValue.GetReplicatedValueType())
    {
    case ReplicatedValueType::String:
        SerializeStringProperty(Serializer, Property.DefaultValue.GetString());
        break;
    case ReplicatedValueType::Float:
        SerializeFloatProperty(Serializer, Property.DefaultValue.GetFloat());
        break;
    case ReplicatedValueType::Integer:
        SerializeIntProperty(Serializer, Property.DefaultValue.GetInt());
        break;
    case ReplicatedValueType::Boolean:
        SerializeBoolProperty(Serializer, Property.DefaultValue.GetBool());
        break;
    case ReplicatedValueType::Vector2:
        SerializeVec2Property(Serializer, Property.DefaultValue.GetVector2());
        break;
    case ReplicatedValueType::Vector3:
        SerializeVec3Property(Serializer, Property.DefaultValue.GetVector3());
        break;
    case ReplicatedValueType::Vector4:
        SerializeVec4Property(Serializer, Property.DefaultValue.GetVector4());
        break;
    default:
        break;
    }

    if (!Property.Description.IsEmpty())
    {
        Serializer.SerializeMember("description", Property.Description);
    }

    if (!Property.IsScriptable)
    {
        Serializer.SerializeMember("scripting", false);
    }

    if (Property.IsDeprecated)
    {
        Serializer.SerializeMember("deprecated", true);
    }

    if (Property.HasRange())
    {
        Serializer.SerializeMember("range", PropertyRange { Property.RangeMin, Property.RangeMax });
    }

    if (!Property.Options.IsEmpty())
    {
        Serializer.SerializeMember("options", Property.Options);
    }
}

void ToJson(csp::json::JsonSerializer& Serializer, const ComponentSchema& Schema)
{
    Serializer.SerializeMember("typeId", Schema.TypeId);

    if (Schema.IsReserved)
    {
        Serializer.SerializeMember("reserved", true);

        if (!Schema.Description.IsEmpty())
        {
            Serializer.SerializeMember("description", Schema.Description);
        }

        return;
    }

    Serializer.SerializeMember("name", Schema.Name);
    Serializer.SerializeMember("properties", Schema.Properties);

    if (!Schema.Description.IsEmpty())
    {
        Serializer.SerializeMember("description", Schema.Description);
    }

    if (!Schema.IsScriptable)
    {
        Serializer.SerializeMember("scripting", false);
    }

    if (Schema.IsDeprecated)
    {
        Serializer.SerializeMember("deprecated", true);
    }
}

csp::common::String ComponentSchema::ToJson(const ComponentSchema& Schema) { return csp::json::JsonSerializer::Serialize(Schema); }

csp::common::Optional<ComponentSchema> ComponentSchema::FromJson(const csp::common::String& Json)
{
    auto Doc = rapidjson::Document {};
    Doc.Parse(Json.c_str());

    if (Doc.HasParseError())
    {
        return {};
    }

    const auto Schema = TryParseSchema(Doc);

    if (!Schema)
    {
        return {};
    }

    return *Schema;
}

csp::common::Array<ComponentSchema> ComponentSchemasFromJson(
    const csp::common::List<csp::common::String>& JsonDocuments, csp::common::LogSystem* LogSystem)
{
    auto Collected = std::vector<ComponentSchema> {};

    for (const auto& JsonDoc : JsonDocuments)
    {
        auto Doc = rapidjson::Document {};
        Doc.Parse(JsonDoc.c_str());

        if (Doc.HasParseError() || !Doc.IsArray())
        {
            if (LogSystem)
            {
                LogSystem->LogMsg(csp::common::LogLevel::Warning, "ComponentSchemasFromJson: skipping document, expected a top-level JSON array");
            }
            continue;
        }

        for (const auto& Element : Doc.GetArray())
        {
            const auto Schema = TryParseSchema(Element, LogSystem);

            if (!Schema)
            {
                if (LogSystem)
                {
                    LogSystem->LogMsg(csp::common::LogLevel::Warning, "ComponentSchemasFromJson: skipping entry, failed to parse schema");
                }
                continue;
            }

            Collected.push_back(*Schema);
        }
    }

    return csp::common::Convert(Collected);
}

bool ComponentSchema::operator==(const ComponentSchema& Other) const
{
    return TypeId == Other.TypeId && Name == Other.Name && Properties == Other.Properties && Description == Other.Description
        && IsScriptable == Other.IsScriptable && IsReserved == Other.IsReserved && IsDeprecated == Other.IsDeprecated;
}

bool ComponentSchema::operator!=(const ComponentSchema& Other) const { return !(*this == Other); }

bool IsCompatible(const ComponentSchema& Original, const ComponentSchema& Updated, csp::common::LogSystem* LogSystem)
{
    if (Original.Name != Updated.Name)
    {
        if (LogSystem != nullptr)
        {
            LogSystem->LogMsg(csp::common::LogLevel::Warning,
                fmt::format("Schema name mismatch: expected '{}', got '{}'.", Original.Name.c_str(), Updated.Name.c_str()).c_str());
        }
        return false;
    }

    auto UpdatedByKey = std::unordered_map<ComponentProperty::KeyType, ComponentProperty> {};
    for (const auto& Property : Updated.Properties)
    {
        UpdatedByKey.emplace(Property.Key, Property);
    }

    const auto LogOnMismatch = [LogSystem](auto Predicate)
    {
        return [LogSystem, Predicate](const auto& Property)
        {
            const auto Result = Predicate(Property);
            if (!Result && LogSystem != nullptr)
            {
                LogSystem->LogMsg(csp::common::LogLevel::Warning,
                    fmt::format("Incompatible property: key {}, name '{}'.", Property.Key, Property.Name.c_str()).c_str());
            }
            return Result;
        };
    };

    return std::all_of(Original.Properties.begin(), Original.Properties.end(),
        LogOnMismatch(
            [&UpdatedByKey](const auto& Property)
            {
                const auto It = UpdatedByKey.find(Property.Key);
                return It != UpdatedByKey.end() && It->second == Property;
            }));
}

} // namespace csp::multiplayer
