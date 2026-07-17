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

        if (!Value.HasMember("key") || !Value["key"].IsUint())
        {
            if (LogSystem)
            {
                LogSystem->LogMsg(csp::common::LogLevel::Warning, "TryParseProperty: 'key' must be an unsigned integer");
            }

            return std::nullopt;
        }

        if (!Value.HasMember("name") || !Value["name"].IsString())
        {
            if (LogSystem)
            {
                LogSystem->LogMsg(csp::common::LogLevel::Warning, "TryParseProperty: 'name' must be a string");
            }

            return std::nullopt;
        }

        if (!Value.HasMember("type") || !Value["type"].IsString())
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

        const auto Key = static_cast<ComponentProperty::KeyType>(Value["key"].GetUint());
        const auto* Name = Value["name"].GetString();
        const auto* Type = Value["type"].GetString();
        const auto DefaultValue = TryParse(Type, Value["defaultValue"]);

        if (!DefaultValue)
        {
            if (LogSystem)
            {
                LogSystem->LogMsg(
                    csp::common::LogLevel::Warning, fmt::format("TryParseProperty: 'defaultValue' is not valid for type '{}'", Type).c_str());
            }

            return std::nullopt;
        }

        return ComponentProperty {
            Key,
            Name,
            *DefaultValue,
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

        if (!Value.HasMember("typeId") || !Value["typeId"].IsUint64())
        {
            if (LogSystem)
            {
                LogSystem->LogMsg(csp::common::LogLevel::Warning, "TryParseSchema: 'typeId' must be a uint64");
            }

            return std::nullopt;
        }

        if (!Value.HasMember("name") || !Value["name"].IsString())
        {
            if (LogSystem)
            {
                LogSystem->LogMsg(csp::common::LogLevel::Warning, "TryParseSchema: 'name' must be a string");
            }

            return std::nullopt;
        }

        if (!Value.HasMember("properties") || !Value["properties"].IsArray())
        {
            if (LogSystem)
            {
                LogSystem->LogMsg(csp::common::LogLevel::Warning, "TryParseSchema: 'properties' must be an array");
            }

            return std::nullopt;
        }

        const auto Properties = TryParseProperties(Value["properties"].GetArray(), LogSystem);

        if (!Properties)
        {
            return std::nullopt;
        }

        return ComponentSchema {
            Value["typeId"].GetUint64(),
            Value["name"].GetString(),
            *Properties,
        };
    }

} // namespace

void ToJson(csp::json::JsonSerializer& Serializer, const ComponentProperty& Property)
{
    using csp::common::ReplicatedValueType;

    Serializer.SerializeMember("key", static_cast<uint32_t>(Property.Key));
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
}

void ToJson(csp::json::JsonSerializer& Serializer, const ComponentSchema& Schema)
{
    Serializer.SerializeMember("typeId", Schema.TypeId);
    Serializer.SerializeMember("name", Schema.Name);
    Serializer.SerializeMember("properties", Schema.Properties);
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
    const csp::common::List<csp::common::String>& JsonDocuments, csp::common::LogSystem& LogSystem)
{
    auto Collected = std::vector<ComponentSchema> {};

    for (const auto& JsonDoc : JsonDocuments)
    {
        auto Doc = rapidjson::Document {};
        Doc.Parse(JsonDoc.c_str());

        if (Doc.HasParseError() || !Doc.IsArray())
        {
            LogSystem.LogMsg(csp::common::LogLevel::Warning, "ComponentSchemasFromJson: skipping document, expected a top-level JSON array");
            continue;
        }

        for (const auto& Element : Doc.GetArray())
        {
            const auto Schema = TryParseSchema(Element, &LogSystem);

            if (!Schema)
            {
                LogSystem.LogMsg(csp::common::LogLevel::Warning, "ComponentSchemasFromJson: skipping entry, failed to parse schema");
                continue;
            }

            Collected.push_back(*Schema);
        }
    }

    return csp::common::Convert(Collected);
}

bool ComponentSchema::operator==(const ComponentSchema& Other) const
{
    return TypeId == Other.TypeId && Name == Other.Name && Properties == Other.Properties;
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
