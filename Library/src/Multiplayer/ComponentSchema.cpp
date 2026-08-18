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

#include "Multiplayer/ComponentSchema.h"

#include "CSP/Common/Systems/Log/LogSystem.h"
#include "Common/Convert.h"
#include "Json/JsonSerializer.h"

#include <fmt/format.h>
#include <rapidjson/document.h>

#include <algorithm>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace csp::multiplayer
{

namespace
{
    void SerializeDefaultValue(csp::json::JsonSerializer& Serializer, const std::string& Value)
    {
        Serializer.SerializeMember("type", "string");
        Serializer.SerializeMember("defaultValue", Value);
    }

    void SerializeDefaultValue(csp::json::JsonSerializer& Serializer, float Value)
    {
        Serializer.SerializeMember("type", "float");
        Serializer.SerializeMember("defaultValue", Value);
    }

    void SerializeDefaultValue(csp::json::JsonSerializer& Serializer, int64_t Value)
    {
        Serializer.SerializeMember("type", "int");
        Serializer.SerializeMember("defaultValue", Value);
    }

    void SerializeDefaultValue(csp::json::JsonSerializer& Serializer, bool Value)
    {
        Serializer.SerializeMember("type", "bool");
        Serializer.SerializeMember("defaultValue", Value);
    }

    void SerializeDefaultValue(csp::json::JsonSerializer& Serializer, const csp::common::Vector2& Vector)
    {
        Serializer.SerializeMember("type", "vec2");
        Serializer.SerializeMember("defaultValue", std::vector<float> { Vector.X, Vector.Y });
    }

    void SerializeDefaultValue(csp::json::JsonSerializer& Serializer, const csp::common::Vector3& Vector)
    {
        Serializer.SerializeMember("type", "vec3");
        Serializer.SerializeMember("defaultValue", std::vector<float> { Vector.X, Vector.Y, Vector.Z });
    }

    void SerializeDefaultValue(csp::json::JsonSerializer& Serializer, const csp::common::Vector4& Vector)
    {
        Serializer.SerializeMember("type", "vec4");
        Serializer.SerializeMember("defaultValue", std::vector<float> { Vector.X, Vector.Y, Vector.Z, Vector.W });
    }

    // TODO: implement along with map parsing.
    void SerializeDefaultValue(csp::json::JsonSerializer&, const std::unordered_map<std::string, std::string>&) { }

    bool IsFloatArray(const rapidjson::Value& Value, rapidjson::SizeType ExpectedSize)
    {
        if (!Value.IsArray() || Value.Size() != ExpectedSize)
        {
            return false;
        }

        const auto& Array = Value.GetArray();
        return std::all_of(Array.begin(), Array.end(), [](const auto& Element) { return Element.IsNumber(); });
    }

    template <typename T> std::optional<T> TryParse(const rapidjson::Value& Value);

    template <> std::optional<std::string> TryParse<std::string>(const rapidjson::Value& Value)
    {
        if (!Value.IsString())
        {
            return std::nullopt;
        }

        return std::string { Value.GetString() };
    }

    template <> std::optional<csp::common::String> TryParse<csp::common::String>(const rapidjson::Value& Value)
    {
        if (!Value.IsString())
        {
            return std::nullopt;
        }

        return csp::common::String { Value.GetString() };
    }

    template <> std::optional<float> TryParse<float>(const rapidjson::Value& Value)
    {
        if (!Value.IsNumber())
        {
            return std::nullopt;
        }

        return Value.GetFloat();
    }

    template <> std::optional<int64_t> TryParse<int64_t>(const rapidjson::Value& Value)
    {
        if (!Value.IsInt64())
        {
            return std::nullopt;
        }

        return Value.GetInt64();
    }

    template <> std::optional<bool> TryParse<bool>(const rapidjson::Value& Value)
    {
        if (!Value.IsBool())
        {
            return std::nullopt;
        }

        return Value.GetBool();
    }

    template <> std::optional<uint32_t> TryParse<uint32_t>(const rapidjson::Value& Value)
    {
        if (!Value.IsUint())
        {
            return std::nullopt;
        }

        return Value.GetUint();
    }

    template <> std::optional<uint64_t> TryParse<uint64_t>(const rapidjson::Value& Value)
    {
        if (!Value.IsUint64())
        {
            return std::nullopt;
        }

        return Value.GetUint64();
    }

    template <> std::optional<csp::common::Vector2> TryParse<csp::common::Vector2>(const rapidjson::Value& Value)
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

    template <> std::optional<csp::common::Vector3> TryParse<csp::common::Vector3>(const rapidjson::Value& Value)
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

    template <> std::optional<csp::common::Vector4> TryParse<csp::common::Vector4>(const rapidjson::Value& Value)
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

    struct ParseError final
    {
        std::string Description;
    };

    template <typename T> class ParseResult final
    {
    public:
        explicit ParseResult(T InValue)
            : Storage(std::move(InValue))
        {
        }

        ParseResult(ParseError InError)
            : Storage(std::move(InError))
        {
        }

        explicit operator bool() const { return std::holds_alternative<T>(Storage); }

        T& operator*() { return *std::get_if<T>(&Storage); }

        const T& operator*() const { return *std::get_if<T>(&Storage); }

        ParseError& GetError() { return *std::get_if<ParseError>(&Storage); }

        const ParseError& GetError() const { return *std::get_if<ParseError>(&Storage); }

    private:
        std::variant<T, ParseError> Storage;
    };

    template <typename T> ParseResult<T> TryParseRequired(const rapidjson::Value& Object, const char* Key)
    {
        if (!Object.HasMember(Key))
        {
            return ParseError { fmt::format("required field '{}' is missing", Key) };
        }

        auto Parsed = TryParse<T>(Object[Key]);

        if (!Parsed)
        {
            return ParseError { fmt::format("required field '{}' has unexpected type", Key) };
        }

        return ParseResult { std::move(*Parsed) };
    }

    template <typename T> ParseResult<PropertyValue> TryParseDefaultValue(const rapidjson::Value& Object)
    {
        auto Parsed = TryParseRequired<T>(Object, "defaultValue");

        if (!Parsed)
        {
            return std::move(Parsed.GetError());
        }

        return ParseResult<PropertyValue> { PlainValue<T> { std::move(*Parsed) } };
    }

    ParseResult<PropertyValue> TryParseValue(const std::string& Type, const rapidjson::Value& Object)
    {
        if (Type == "string")
        {
            return TryParseDefaultValue<std::string>(Object);
        }

        if (Type == "float")
        {
            return TryParseDefaultValue<float>(Object);
        }

        if (Type == "int")
        {
            return TryParseDefaultValue<int64_t>(Object);
        }

        if (Type == "bool")
        {
            return TryParseDefaultValue<bool>(Object);
        }

        if (Type == "vec2")
        {
            return TryParseDefaultValue<csp::common::Vector2>(Object);
        }

        if (Type == "vec3")
        {
            return TryParseDefaultValue<csp::common::Vector3>(Object);
        }

        if (Type == "vec4")
        {
            return TryParseDefaultValue<csp::common::Vector4>(Object);
        }

        return ParseError { fmt::format("unknown type '{}'", Type) };
    }

    ParseResult<ComponentProperty> TryParseProperty(const rapidjson::Value& Value)
    {
        if (!Value.IsObject())
        {
            return ParseError { "property must be a JSON object" };
        }

        const auto Key = TryParseRequired<uint32_t>(Value, "key");

        if (!Key)
        {
            return Key.GetError();
        }

        auto Name = TryParseRequired<csp::common::String>(Value, "name");

        if (!Name)
        {
            return std::move(Name.GetError());
        }

        const auto Type = TryParseRequired<std::string>(Value, "type");

        if (!Type)
        {
            return Type.GetError();
        }

        auto ParsedValue = TryParseValue(*Type, Value);

        if (!ParsedValue)
        {
            return std::move(ParsedValue.GetError());
        }

        return ParseResult { ComponentProperty {
            static_cast<ComponentProperty::KeyType>(*Key),
            std::move(*Name),
            std::move(*ParsedValue),
        } };
    }

    ParseResult<csp::common::Array<ComponentProperty>> TryParseProperties(rapidjson::Value::ConstArray JsonProperties)
    {
        auto Properties = std::vector<ComponentProperty> {};

        for (auto Index = rapidjson::SizeType { 0 }; Index < JsonProperties.Size(); ++Index)
        {
            auto Property = TryParseProperty(JsonProperties[Index]);

            if (!Property)
            {
                return ParseError { fmt::format("properties[{}]: {}", Index, Property.GetError().Description) };
            }

            Properties.push_back(std::move(*Property));
        }

        return ParseResult { csp::common::Convert(Properties) };
    }

    ParseResult<ComponentSchema> TryParseSchema(const rapidjson::Value& Value)
    {
        if (!Value.IsObject())
        {
            return ParseError { "schema must be a JSON object" };
        }

        const auto TypeId = TryParseRequired<uint64_t>(Value, "typeId");

        if (!TypeId)
        {
            return TypeId.GetError();
        }

        auto Name = TryParseRequired<csp::common::String>(Value, "name");

        if (!Name)
        {
            return std::move(Name.GetError());
        }

        if (!Value.HasMember("properties"))
        {
            return ParseError { "required field 'properties' is missing" };
        }

        if (!Value["properties"].IsArray())
        {
            return ParseError { "required field 'properties' has unexpected type" };
        }

        auto Properties = TryParseProperties(Value["properties"].GetArray());

        if (!Properties)
        {
            return ParseError { fmt::format("{}: {}", (*Name).c_str(), Properties.GetError().Description) };
        }

        return ParseResult { ComponentSchema {
            *TypeId,
            std::move(*Name),
            std::move(*Properties),
        } };
    }

} // namespace

void ToJson(csp::json::JsonSerializer& Serializer, const ComponentProperty& Property)
{
    Serializer.SerializeMember("key", static_cast<uint32_t>(Property.Key));
    Serializer.SerializeMember("name", Property.Name);

    std::visit([&Serializer](const auto& Value) { SerializeDefaultValue(Serializer, Value.Default); }, Property.Value);
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

    auto Schema = TryParseSchema(Doc);

    if (!Schema)
    {
        return {};
    }

    return std::move(*Schema);
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
            auto Schema = TryParseSchema(Element);

            if (!Schema)
            {
                LogSystem.LogMsg(csp::common::LogLevel::Warning,
                    fmt::format("ComponentSchemasFromJson: skipping entry: {}", Schema.GetError().Description).c_str());
                continue;
            }

            Collected.push_back(std::move(*Schema));
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
