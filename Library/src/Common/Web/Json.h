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
#pragma once

#include "CSP/Common/String.h"

#include <map>
#include <memory>
#include <optional>
#include <rapidjson/allocators.h>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <string>
#include <vector>

namespace csp::services
{
class DtoBase;
class EnumBase;
} // namespace csp::services

namespace csp::web
{

using RapidJsonAlloc = rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>;

// Functions for converting to Json types from C++ types

template <class T> inline csp::common::String TypeToJsonString(const std::shared_ptr<T>& Value) { return Value->ToJson(); }

template <class T>
[[deprecated("Unsupported type for JSON (string) serialisation! You should probably add support for it :)")]] inline csp::common::String
TypeToJsonString(const T& Value)
{
    return csp::common::String("");
}

// Forward declarations for clang to not be stupid
template <typename U> rapidjson::Value TypeToJsonValue(const std::optional<U>& Value, RapidJsonAlloc& Allocator);

template <typename U> rapidjson::Value TypeToJsonValue(const std::shared_ptr<U>& Value, RapidJsonAlloc& Allocator);

template <typename U> rapidjson::Value TypeToJsonValue(const std::vector<U>& Value, RapidJsonAlloc& Allocator);

template <typename U, typename V> rapidjson::Value TypeToJsonValue(const std::map<U, V>& Value, RapidJsonAlloc& Allocator);

template <class T, typename std::enable_if_t<std::is_base_of_v<csp::services::DtoBase, T>>* = nullptr>
rapidjson::Value TypeToJsonValue(const T& Value, RapidJsonAlloc& Allocator);

template <class T, typename std::enable_if_t<std::is_base_of_v<csp::services::EnumBase, T>>* = nullptr>
rapidjson::Value TypeToJsonValue(const T& Value, RapidJsonAlloc& Allocator);

template <class T,
    typename std::enable_if_t<!std::is_base_of_v<csp::services::DtoBase, T> && !std::is_base_of_v<csp::services::EnumBase, T>>* = nullptr>
rapidjson::Value TypeToJsonValue(const T& Value, RapidJsonAlloc& Allocator);

template <> rapidjson::Value TypeToJsonValue(const bool& Value, RapidJsonAlloc& Allocator);

template <> rapidjson::Value TypeToJsonValue(const int32_t& Value, RapidJsonAlloc& Allocator);

template <> rapidjson::Value TypeToJsonValue(const uint32_t& Value, RapidJsonAlloc& Allocator);

template <> rapidjson::Value TypeToJsonValue(const int64_t& Value, RapidJsonAlloc& Allocator);

template <> rapidjson::Value TypeToJsonValue(const uint64_t& Value, RapidJsonAlloc& Allocator);

template <> rapidjson::Value TypeToJsonValue(const float& Value, RapidJsonAlloc& Allocator);

template <> rapidjson::Value TypeToJsonValue(const double& Value, RapidJsonAlloc& Allocator);

template <> rapidjson::Value TypeToJsonValue(const csp::common::String& Value, RapidJsonAlloc& Allocator);

template <> rapidjson::Value TypeToJsonValue(const rapidjson::Document& Value, RapidJsonAlloc& Allocator);

template <typename U> inline void JsonValueToType(const rapidjson::Value& Value, std::optional<U>& Type);

template <typename U> inline void JsonValueToType(const rapidjson::Value& Value, std::shared_ptr<U>& Type);

template <typename U> inline void JsonValueToType(const rapidjson::Value& Value, std::vector<U>& Type);

template <typename U, typename V> inline void JsonValueToType(const rapidjson::Value& Value, std::map<U, V>& Type);

template <typename T, typename std::enable_if_t<std::is_base_of_v<csp::services::DtoBase, T>>* = nullptr>
inline void JsonValueToType(const rapidjson::Value& Value, T& Type);

template <typename T, typename std::enable_if_t<std::is_base_of_v<csp::services::EnumBase, T>>* = nullptr>
inline void JsonValueToType(const rapidjson::Value& Value, T& Type);

template <class T,
    typename std::enable_if_t<!std::is_base_of_v<csp::services::DtoBase, T> && !std::is_base_of_v<csp::services::EnumBase, T>>* = nullptr>
inline void JsonValueToType(const rapidjson::Value& Value, T& Type);

template <> inline void JsonValueToType(const rapidjson::Value& Value, bool& Type);

template <> inline void JsonValueToType(const rapidjson::Value& Value, int32_t& Type);

template <> inline void JsonValueToType(const rapidjson::Value& Value, uint32_t& Type);

template <> inline void JsonValueToType(const rapidjson::Value& Value, int64_t& Type);

template <> inline void JsonValueToType(const rapidjson::Value& Value, uint64_t& Type);

template <> inline void JsonValueToType(const rapidjson::Value& Value, float& Type);

template <> inline void JsonValueToType(const rapidjson::Value& Value, double& Type);

template <> inline void JsonValueToType(const rapidjson::Value& Value, csp::common::String& Type);

template <> inline void JsonValueToType(const rapidjson::Value& Value, rapidjson::Document& Type);

// Catch-all template for notifying the user if no specialisation exists for serialising the specified type
template <class T, typename std::enable_if_t<!std::is_base_of_v<csp::services::DtoBase, T> && !std::is_base_of_v<csp::services::EnumBase, T>>*>
[[deprecated("Unsupported type for JSON serialisation! You should probably add support for it :)")]] inline rapidjson::Value TypeToJsonValue(
    const T& Value, RapidJsonAlloc& Allocator)
{
    assert(false && "Unsupported type for JSON serialisation! You should probably add support for it :(");
}

// Serialisation function for types that derive from DtoBase
template <class T, typename std::enable_if_t<std::is_base_of_v<csp::services::DtoBase, T>>*>
inline rapidjson::Value TypeToJsonValue(const T& Value, RapidJsonAlloc& Allocator)
{
    csp::common::String Json = Value.ToJson();
    rapidjson::Document JsonDocument(rapidjson::Type::kObjectType, &Allocator);
    JsonDocument.Parse<0>(Json.c_str());

    return JsonDocument.GetObject();
}

// Serialisation function for types that derive from EnumDtoBase
template <class T, typename std::enable_if_t<std::is_base_of_v<csp::services::EnumBase, T>>*>
inline rapidjson::Value TypeToJsonValue(const T& Value, RapidJsonAlloc& Allocator)
{
    csp::common::String Json = Value.ToJson();

    return rapidjson::Value(Json.c_str(), Allocator);
}

// Partial template specialisations for TypeToJsonValue

template <typename U> inline rapidjson::Value TypeToJsonValue(const std::optional<U>& Value, RapidJsonAlloc& Allocator)
{
    return TypeToJsonValue(Value.value(), Allocator);
}

template <typename U> inline rapidjson::Value TypeToJsonValue(const std::shared_ptr<U>& Value, RapidJsonAlloc& Allocator)
{
    return TypeToJsonValue(*Value, Allocator);
}

template <typename U> inline rapidjson::Value TypeToJsonValue(const std::vector<U>& Value, RapidJsonAlloc& Allocator)
{
    rapidjson::Value JsonValue(rapidjson::kArrayType);
    JsonValue.Reserve((rapidjson::SizeType)Value.size(), Allocator);

    for (size_t i = 0; i < Value.size(); ++i)
    {
        JsonValue.PushBack(TypeToJsonValue(Value[i], Allocator), Allocator);
    }

    return JsonValue;
}

template <typename U, typename V> inline rapidjson::Value TypeToJsonValue(const std::map<U, V>& Value, RapidJsonAlloc& Allocator)
{
    rapidjson::Value JsonValue(rapidjson::kObjectType);
    JsonValue.MemberReserve((rapidjson::SizeType)Value.size(), Allocator);

    for (auto& Item : Value)
    {
        JsonValue.AddMember(TypeToJsonValue(Item.first, Allocator), TypeToJsonValue(Item.second, Allocator), Allocator);
    }

    return JsonValue;
}

// Full template specialisations for TypeToJsonValue

template <> inline rapidjson::Value TypeToJsonValue(const bool& Value, RapidJsonAlloc& /*Allocator*/)
{
    rapidjson::Value JsonValue;
    JsonValue.SetBool(Value);

    return JsonValue;
}

template <> inline rapidjson::Value TypeToJsonValue(const int32_t& Value, RapidJsonAlloc& /*Allocator*/) { return rapidjson::Value(Value); }

template <> inline rapidjson::Value TypeToJsonValue(const uint32_t& Value, RapidJsonAlloc& /*Allocator*/) { return rapidjson::Value(Value); }

template <> inline rapidjson::Value TypeToJsonValue(const int64_t& Value, RapidJsonAlloc& /*Allocator*/) { return rapidjson::Value(Value); }

template <> inline rapidjson::Value TypeToJsonValue(const uint64_t& Value, RapidJsonAlloc& /*Allocator*/) { return rapidjson::Value(Value); }

template <> inline rapidjson::Value TypeToJsonValue(const float& Value, RapidJsonAlloc& /*Allocator*/) { return rapidjson::Value(Value); }

template <> inline rapidjson::Value TypeToJsonValue(const double& Value, RapidJsonAlloc& /*Allocator*/) { return rapidjson::Value(Value); }

template <> inline rapidjson::Value TypeToJsonValue(const csp::common::String& Value, RapidJsonAlloc& Allocator)
{
    return rapidjson::Value(Value.c_str(), Allocator);
}

template <> inline rapidjson::Value TypeToJsonValue(const rapidjson::Document& Value, RapidJsonAlloc& Allocator)
{
    rapidjson::Value Object(rapidjson::kObjectType);
    Object.CopyFrom(*Value.Begin(), Allocator);

    return Object;
}

// Functions for converting to C++ types from Json types

inline csp::common::String JsonDocToString(const rapidjson::Document& JsonDoc)
{
    rapidjson::StringBuffer Buffer;
    rapidjson::Writer<rapidjson::StringBuffer> Writer(Buffer);
    JsonDoc.Accept(Writer);

    return csp::common::String(Buffer.GetString());
}

inline csp::common::String JsonObjectToString(const rapidjson::Value& JsonObject)
{
    rapidjson::StringBuffer Buffer;
    rapidjson::Writer<rapidjson::StringBuffer> Writer(Buffer);
    JsonObject.Accept(Writer);

    return csp::common::String(Buffer.GetString());
}

template <class T> inline csp::common::String TypeToJsonString(const std::vector<std::shared_ptr<T>>& Value)
{
    RapidJsonAlloc Allocator;
    return JsonObjectToString(TypeToJsonValue(Value, Allocator));
}

// Deserialisation function for types that derive from EnumBase
template <typename T, typename std::enable_if_t<std::is_base_of_v<csp::services::EnumBase, T>>*>
inline void JsonValueToType(const rapidjson::Value& Value, T& Type)
{
    Type.FromJson(CSP_TEXT(Value.GetString()));
}

// Catch-all template for notifying the user if no specialisation exists for deserialising to the specified type
template <class T, typename std::enable_if_t<!std::is_base_of_v<csp::services::DtoBase, T> && !std::is_base_of_v<csp::services::EnumBase, T>>*>
[[deprecated("Unsupported type for JSON deserialisation! You should probably add support for it :)")]] inline void JsonValueToType(
    const rapidjson::Value& Value, T& Type)
{
    assert(false && "Unsupported type for JSON deserialisation! You should probably add support for it :)");
}

// Deserialisation function for types that derive from DtoBase
template <typename T, typename std::enable_if_t<std::is_base_of_v<csp::services::DtoBase, T>>*>
inline void JsonValueToType(const rapidjson::Value& Value, T& Type)
{
    Type.FromJson(Value.IsString() ? CSP_TEXT(Value.GetString()) : JsonObjectToString(Value));
}

// Partial template specialisations for JsonValueToType

template <typename U> inline void JsonValueToType(const rapidjson::Value& Value, std::optional<U>& Type)
{
    U Inner;
    JsonValueToType(Value, Inner);
    Type = std::make_optional(Inner);
}

template <typename U> inline void JsonValueToType(const rapidjson::Value& Value, std::shared_ptr<U>& Type)
{
    Type = std::make_shared<U>();
    JsonValueToType(Value, *Type);
}

template <typename U> inline void JsonValueToType(const rapidjson::Value& Value, std::vector<U>& Type)
{
    assert(Value.IsArray());

    for (auto i = 0U; i < Value.Size(); ++i)
    {
        U Element;
        JsonValueToType(Value[i], Element);
        Type.push_back(Element);
    }
}

template <typename U, typename V> inline void JsonValueToType(const rapidjson::Value& Value, std::map<U, V>& Type)
{
    assert(Value.IsObject());

    for (auto& Member : Value.GetObject())
    {
        U ElementKey;
        V ElementValue;
        JsonValueToType(Member.name, ElementKey);
        JsonValueToType(Member.value, ElementValue);
        Type.emplace(std::make_pair(ElementKey, ElementValue));
    }
}

// Full template specialisations for JsonValueToType

template <> inline void JsonValueToType(const rapidjson::Value& Value, bool& Type) { Type = Value.GetBool(); }

template <> inline void JsonValueToType(const rapidjson::Value& Value, int32_t& Type) { Type = Value.GetInt(); }

template <> inline void JsonValueToType(const rapidjson::Value& Value, uint32_t& Type) { Type = Value.GetUint(); }

template <> inline void JsonValueToType(const rapidjson::Value& Value, int64_t& Type) { Type = Value.GetInt64(); }

template <> inline void JsonValueToType(const rapidjson::Value& Value, uint64_t& Type) { Type = Value.GetUint64(); }

template <> inline void JsonValueToType(const rapidjson::Value& Value, float& Type) { Type = Value.GetFloat(); }

template <> inline void JsonValueToType(const rapidjson::Value& Value, double& Type) { Type = Value.GetDouble(); }

template <> inline void JsonValueToType(const rapidjson::Value& Value, csp::common::String& Type) { Type = Value.GetString(); }

template <> inline void JsonValueToType(const rapidjson::Value& Value, rapidjson::Document& Type) { Type.CopyFrom(Value, Type.GetAllocator()); }

} // namespace csp::web
