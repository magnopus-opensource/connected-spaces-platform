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

template <class T> inline csp::common::String TypeToJsonString(const std::shared_ptr<T>& value) { return value->ToJson(); }

template <class T>
[[deprecated("Unsupported type for JSON (string) serialisation! You should probably add support for it :)")]] inline csp::common::String
TypeToJsonString(const T& /*Value*/)
{
    return csp::common::String("");
}

// Forward declarations for clang to not be stupid
template <typename U> rapidjson::Value TypeToJsonValue(const std::optional<U>& value, RapidJsonAlloc& allocator);

template <typename U> rapidjson::Value TypeToJsonValue(const std::shared_ptr<U>& value, RapidJsonAlloc& allocator);

template <typename U> rapidjson::Value TypeToJsonValue(const std::vector<U>& value, RapidJsonAlloc& allocator);

template <typename U, typename V> rapidjson::Value TypeToJsonValue(const std::map<U, V>& value, RapidJsonAlloc& allocator);

template <class T, typename std::enable_if_t<std::is_base_of_v<csp::services::DtoBase, T>>* = nullptr>
rapidjson::Value TypeToJsonValue(const T& value, RapidJsonAlloc& allocator);

template <class T, typename std::enable_if_t<std::is_base_of_v<csp::services::EnumBase, T>>* = nullptr>
rapidjson::Value TypeToJsonValue(const T& value, RapidJsonAlloc& allocator);

template <class T,
    typename std::enable_if_t<!std::is_base_of_v<csp::services::DtoBase, T> && !std::is_base_of_v<csp::services::EnumBase, T>>* = nullptr>
rapidjson::Value TypeToJsonValue(const T& value, RapidJsonAlloc& allocator);

template <> rapidjson::Value TypeToJsonValue(const bool& value, RapidJsonAlloc& allocator);

template <> rapidjson::Value TypeToJsonValue(const int32_t& value, RapidJsonAlloc& allocator);

template <> rapidjson::Value TypeToJsonValue(const uint32_t& value, RapidJsonAlloc& allocator);

template <> rapidjson::Value TypeToJsonValue(const int64_t& value, RapidJsonAlloc& allocator);

template <> rapidjson::Value TypeToJsonValue(const uint64_t& value, RapidJsonAlloc& allocator);

template <> rapidjson::Value TypeToJsonValue(const float& value, RapidJsonAlloc& allocator);

template <> rapidjson::Value TypeToJsonValue(const double& value, RapidJsonAlloc& allocator);

template <> rapidjson::Value TypeToJsonValue(const csp::common::String& value, RapidJsonAlloc& allocator);

template <> rapidjson::Value TypeToJsonValue(const rapidjson::Document& value, RapidJsonAlloc& allocator);

template <typename U> inline void JsonValueToType(const rapidjson::Value& value, std::optional<U>& type);

template <typename U> inline void JsonValueToType(const rapidjson::Value& value, std::shared_ptr<U>& type);

template <typename U> inline void JsonValueToType(const rapidjson::Value& value, std::vector<U>& type);

template <typename U, typename V> inline void JsonValueToType(const rapidjson::Value& value, std::map<U, V>& type);

template <typename T, typename std::enable_if_t<std::is_base_of_v<csp::services::DtoBase, T>>* = nullptr>
inline void JsonValueToType(const rapidjson::Value& value, T& type);

template <typename T, typename std::enable_if_t<std::is_base_of_v<csp::services::EnumBase, T>>* = nullptr>
inline void JsonValueToType(const rapidjson::Value& value, T& type);

template <class T,
    typename std::enable_if_t<!std::is_base_of_v<csp::services::DtoBase, T> && !std::is_base_of_v<csp::services::EnumBase, T>>* = nullptr>
inline void JsonValueToType(const rapidjson::Value& value, T& type);

template <> inline void JsonValueToType(const rapidjson::Value& value, bool& type);

template <> inline void JsonValueToType(const rapidjson::Value& value, int32_t& type);

template <> inline void JsonValueToType(const rapidjson::Value& value, uint32_t& type);

template <> inline void JsonValueToType(const rapidjson::Value& value, int64_t& type);

template <> inline void JsonValueToType(const rapidjson::Value& value, uint64_t& type);

template <> inline void JsonValueToType(const rapidjson::Value& value, float& type);

template <> inline void JsonValueToType(const rapidjson::Value& value, double& type);

template <> inline void JsonValueToType(const rapidjson::Value& value, csp::common::String& type);

template <> inline void JsonValueToType(const rapidjson::Value& value, rapidjson::Document& type);

// Catch-all template for notifying the user if no specialisation exists for serialising the specified type
template <class T, typename std::enable_if_t<!std::is_base_of_v<csp::services::DtoBase, T> && !std::is_base_of_v<csp::services::EnumBase, T>>*>
[[deprecated("Unsupported type for JSON serialisation! You should probably add support for it :)")]] inline rapidjson::Value TypeToJsonValue(
    const T& /*Value*/, RapidJsonAlloc& /*Allocator*/)
{
    assert(false && "Unsupported type for JSON serialisation! You should probably add support for it :(");
}

// Serialisation function for types that derive from DtoBase
template <class T, typename std::enable_if_t<std::is_base_of_v<csp::services::DtoBase, T>>*>
inline rapidjson::Value TypeToJsonValue(const T& value, RapidJsonAlloc& allocator)
{
    csp::common::String json = value.ToJson();
    rapidjson::Document jsonDocument(rapidjson::Type::kObjectType, &allocator);
    jsonDocument.Parse<0>(json.c_str());
    
    return rapidjson::Value { jsonDocument.GetObj() };
}

// Serialisation function for types that derive from EnumDtoBase
template <class T, typename std::enable_if_t<std::is_base_of_v<csp::services::EnumBase, T>>*>
inline rapidjson::Value TypeToJsonValue(const T& value, RapidJsonAlloc& allocator)
{
    csp::common::String json = value.ToJson();

    return rapidjson::Value(json.c_str(), allocator);
}

// Partial template specialisations for TypeToJsonValue

template <typename U> inline rapidjson::Value TypeToJsonValue(const std::optional<U>& value, RapidJsonAlloc& allocator)
{
    return TypeToJsonValue(value.value(), allocator);
}

template <typename U> inline rapidjson::Value TypeToJsonValue(const std::shared_ptr<U>& value, RapidJsonAlloc& allocator)
{
    return TypeToJsonValue(*value, allocator);
}

template <typename U> inline rapidjson::Value TypeToJsonValue(const std::vector<U>& value, RapidJsonAlloc& allocator)
{
    rapidjson::Value jsonValue(rapidjson::kArrayType);
    jsonValue.Reserve((rapidjson::SizeType)value.size(), allocator);

    for (size_t i = 0; i < value.size(); ++i)
    {
        jsonValue.PushBack(TypeToJsonValue(value[i], allocator), allocator);
    }

    return jsonValue;
}

template <typename U, typename V> inline rapidjson::Value TypeToJsonValue(const std::map<U, V>& value, RapidJsonAlloc& allocator)
{
    rapidjson::Value jsonValue(rapidjson::kObjectType);
    jsonValue.MemberReserve((rapidjson::SizeType)value.size(), allocator);

    for (auto& item : value)
    {
        jsonValue.AddMember(TypeToJsonValue(item.first, allocator), TypeToJsonValue(item.second, allocator), allocator);
    }

    return jsonValue;
}

// Full template specialisations for TypeToJsonValue

template <> inline rapidjson::Value TypeToJsonValue(const bool& value, RapidJsonAlloc& /*Allocator*/)
{
    rapidjson::Value jsonValue;
    jsonValue.SetBool(value);

    return jsonValue;
}

template <> inline rapidjson::Value TypeToJsonValue(const int32_t& value, RapidJsonAlloc& /*Allocator*/) { return rapidjson::Value(value); }

template <> inline rapidjson::Value TypeToJsonValue(const uint32_t& value, RapidJsonAlloc& /*Allocator*/) { return rapidjson::Value(value); }

template <> inline rapidjson::Value TypeToJsonValue(const int64_t& value, RapidJsonAlloc& /*Allocator*/) { return rapidjson::Value(value); }

template <> inline rapidjson::Value TypeToJsonValue(const uint64_t& value, RapidJsonAlloc& /*Allocator*/) { return rapidjson::Value(value); }

template <> inline rapidjson::Value TypeToJsonValue(const float& value, RapidJsonAlloc& /*Allocator*/) { return rapidjson::Value(value); }

template <> inline rapidjson::Value TypeToJsonValue(const double& value, RapidJsonAlloc& /*Allocator*/) { return rapidjson::Value(value); }

template <> inline rapidjson::Value TypeToJsonValue(const csp::common::String& value, RapidJsonAlloc& allocator)
{
    return rapidjson::Value(value.c_str(), allocator);
}

template <> inline rapidjson::Value TypeToJsonValue(const rapidjson::Document& value, RapidJsonAlloc& allocator)
{
    rapidjson::Value object(rapidjson::kObjectType);
    object.CopyFrom(*value.Begin(), allocator);

    return object;
}

// Functions for converting to C++ types from Json types

inline csp::common::String JsonDocToString(const rapidjson::Document& jsonDoc)
{
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    jsonDoc.Accept(writer);

    return csp::common::String(buffer.GetString());
}

inline csp::common::String JsonObjectToString(const rapidjson::Value& jsonObject)
{
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    jsonObject.Accept(writer);

    return csp::common::String(buffer.GetString());
}

template <class T> inline csp::common::String TypeToJsonString(const std::vector<std::shared_ptr<T>>& value)
{
    RapidJsonAlloc allocator;
    return JsonObjectToString(TypeToJsonValue(value, allocator));
}

// Deserialisation function for types that derive from EnumBase
template <typename T, typename std::enable_if_t<std::is_base_of_v<csp::services::EnumBase, T>>*>
inline void JsonValueToType(const rapidjson::Value& value, T& type)
{
    type.FromJson(CSP_TEXT(value.GetString()));
}

// Catch-all template for notifying the user if no specialisation exists for deserialising to the specified type
template <class T, typename std::enable_if_t<!std::is_base_of_v<csp::services::DtoBase, T> && !std::is_base_of_v<csp::services::EnumBase, T>>*>
[[deprecated("Unsupported type for JSON deserialisation! You should probably add support for it :)")]] inline void JsonValueToType(
    const rapidjson::Value& /*Value*/, T& /*Type*/)
{
    assert(false && "Unsupported type for JSON deserialisation! You should probably add support for it :)");
}

// Deserialisation function for types that derive from DtoBase
template <typename T, typename std::enable_if_t<std::is_base_of_v<csp::services::DtoBase, T>>*>
inline void JsonValueToType(const rapidjson::Value& value, T& type)
{
    type.FromJson(value.IsString() ? CSP_TEXT(value.GetString()) : JsonObjectToString(value));
}

// Partial template specialisations for JsonValueToType

template <typename U> inline void JsonValueToType(const rapidjson::Value& value, std::optional<U>& type)
{
    U inner;
    JsonValueToType(value, inner);
    type = std::make_optional(inner);
}

template <typename U> inline void JsonValueToType(const rapidjson::Value& value, std::shared_ptr<U>& type)
{
    type = std::make_shared<U>();
    JsonValueToType(value, *type);
}

template <typename U> inline void JsonValueToType(const rapidjson::Value& value, std::vector<U>& type)
{
    assert(value.IsArray());

    for (auto i = 0U; i < value.Size(); ++i)
    {
        U element;
        JsonValueToType(value[i], element);
        type.push_back(element);
    }
}

template <typename U, typename V> inline void JsonValueToType(const rapidjson::Value& value, std::map<U, V>& type)
{
    assert(value.IsObject());

    for (auto& member : value.GetObj())
    {
        U elementKey;
        V elementValue;
        JsonValueToType(member.name, elementKey);
        JsonValueToType(member.value, elementValue);
        type.emplace(std::make_pair(elementKey, elementValue));
    }
}

// Full template specialisations for JsonValueToType

template <> inline void JsonValueToType(const rapidjson::Value& value, bool& type) { type = value.GetBool(); }

template <> inline void JsonValueToType(const rapidjson::Value& value, int32_t& type) { type = value.GetInt(); }

template <> inline void JsonValueToType(const rapidjson::Value& value, uint32_t& type) { type = value.GetUint(); }

template <> inline void JsonValueToType(const rapidjson::Value& value, int64_t& type) { type = value.GetInt64(); }

template <> inline void JsonValueToType(const rapidjson::Value& value, uint64_t& type) { type = value.GetUint64(); }

template <> inline void JsonValueToType(const rapidjson::Value& value, float& type) { type = value.GetFloat(); }

template <> inline void JsonValueToType(const rapidjson::Value& value, double& type) { type = value.GetDouble(); }

template <> inline void JsonValueToType(const rapidjson::Value& value, csp::common::String& type) { type = value.GetString(); }

template <> inline void JsonValueToType(const rapidjson::Value& value, rapidjson::Document& type) { type.CopyFrom(value, type.GetAllocator()); }

} // namespace csp::web
