/*
 * Copyright 2025 Magnopus LLC

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

#include "CSP/CSPCommon.h"
#include "CSP/Common/Map.h"
#include "CSP/Common/String.h"

#include <map>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <stack>
#include <string>
#include <vector>

namespace csp::json
{
class JsonSerializer;
class JsonDeserializer;
} // namespace csp::json

template <typename T> void ToJson(csp::json::JsonSerializer& serializer, T object);
template <typename T> void ToJson(csp::json::JsonSerializer& serializer, const T& object);
template <typename T> void ToJson(csp::json::JsonSerializer& serializer, const T* object);

template <typename T> void FromJson(const csp::json::JsonDeserializer& deserializer, T& object);
template <typename T> void FromJson(const csp::json::JsonDeserializer& deserializer, T* object);

namespace csp::json
{

class JsonSerializer
{
public:
    /// @brief Generates a json string from an object.
    /// @details A global ToJson function in the root namespace should be created to work with this function:
    /// void ToJson(csp::json::JsonSerializer& Serializer, const T& Value);
    /// @param Object const T& : The object to serialize.
    /// @return String : The serialized json string.
    /// @return common::String : Returns the serialized string.
    template <typename T> static common::String Serialize(const T& object)
    {
        JsonSerializer serializer;

        serializer.SerializeValue(object);
        return serializer.m_buffer.GetString();
    }

    /// @brief Should be called within custom ToJson function.
    /// @details This will serialize a member.
    /// If the member is another custom type, this was internally call ToJson on that type.
    /// @param Key const char * Key : The key to reference the item.
    /// @param Value const T& : The value to serialize.
    template <typename T> void SerializeMember(const char* key, const T& value)
    {
        m_writer.String(key);
        SerializeValue(value);
    }

private:
    rapidjson::StringBuffer m_buffer;
    rapidjson::Writer<rapidjson::StringBuffer> m_writer;

    JsonSerializer()
        : m_writer(m_buffer)
    {
    }

    template <typename T> void SerializeValue(const T& value)
    {
        // If T isn't one of the internal supported types,
        // assume this is a custom object
        m_writer.StartObject();
        ToJson(*this, value);
        m_writer.EndObject();
    }

    void SerializeValue(int32_t value);
    void SerializeValue(uint32_t value);
    void SerializeValue(int64_t value);
    void SerializeValue(uint64_t value);
    void SerializeValue(bool value);
    void SerializeValue(float value);
    void SerializeValue(double value);
    void SerializeValue(const char* value);
    void SerializeValue(const csp::common::String& value);
    void SerializeValue(std::nullptr_t value);

    void SerializeValue(const std::string& value);

    template <typename T> void SerializeValue(const csp::common::Array<T>& value);
    template <typename T> void SerializeValue(const csp::common::List<T>& value);
    template <typename T> void SerializeValue(const csp::common::Map<csp::common::String, T>& value);

    template <typename T> void SerializeValue(const std::vector<T>& value);
    template <typename T> void SerializeValue(const std::map<std::string, T>& value);
};

class JsonDeserializer
{
public:
    /// @brief Converts a given Json string into the specified object.
    /// @details A global FromJson function in the root namespace should be created to work with this function:
    /// void FromJson(const csp::json::JsonDeserializer& Deserializer, T& Value);
    /// @param Data const char* : The json string to deserialize.
    /// @param Object T& : The object to convert to.
    /// @return bool : Returns true if the Deserializer RapidJson Document has no errors.
    template <typename T> static bool Deserialize(const char* data, T& object)
    {
        JsonDeserializer deserializer { data };

        if (deserializer.m_doc.HasParseError())
        {
            // Error parsing Json
            return false;
        }

        rapidjson::Value root { deserializer.m_doc.GetObj() };

        deserializer.m_valueStack.push(&root);
        deserializer.DeserializeValue(object);
        deserializer.m_valueStack.pop();

        return true;
    }

    /// @brief Should be called within custom FromJson function.
    /// @details This will deserialize a member with the given key.
    /// If the member is another custom type, this was internally call FromJson on that type.
    /// @param Key const char* : The key which references this member.
    /// @param Val T& : The member to deserialize to.
    template <typename T> void DeserializeMember(const char* key, T& val) const
    {
        m_valueStack.push(&(*m_valueStack.top())[key]);
        DeserializeValue(val);
        m_valueStack.pop();
    }

    /// @brief Should be called within custom FromJson function.
    /// @details This will safely deserialize a member with the given key by first checking that the member exists.
    /// If the member is another custom type, this was internally call FromJson on that type.
    /// @param Key const char* : The key which references this member.
    /// @param Val T& : The member to deserialize to.
    /// @return bool : Returns true if the key exists in this object and the member has been deserialized.
    template <typename T> bool SafeDeserializeMember(const char* key, T& val) const
    {
        if (HasProperty(key))
        {
            DeserializeMember(key, val);
            return true;
        }
        return false;
    }

    /// @brief Should be called within custom FromJson function.
    /// @details This will return true if the given key exists
    /// @param Key const char* : The key to check.
    /// @return bool : Returns true if the key exists in this object.
    bool HasProperty(const char* key) const { return m_valueStack.top()->HasMember(key); }

    std::string GetMemberAsString(const char* key) const;

    /// @brief Starts the deserialization process of the member with the given key.
    /// Subsequent calls with deserialize members within the given member.
    /// @param Key const char* : The Key of the member to enter.
    void EnterMember(const char* key) const;

    /// @brief Stops deserialization of the current object and goes back to the parent object.
    /// Subsequent calls with deserialize members within the parent object.
    void ExitMember() const;

private:
    JsonDeserializer(const char* data) { m_doc.Parse(data); }

    template <typename T> inline void DeserializeValue(T& value) const { FromJson(*this, value); }

    rapidjson::Document m_doc;
    mutable std::stack<const rapidjson::Value*> m_valueStack;

    void DeserializeValue(int32_t& value) const;
    void DeserializeValue(uint32_t& value) const;
    void DeserializeValue(int64_t& value) const;
    void DeserializeValue(uint64_t& value) const;
    void DeserializeValue(bool& value) const;
    void DeserializeValue(float& value) const;
    void DeserializeValue(double& value) const;
    void DeserializeValue(csp::common::String& value) const;

    void DeserializeValue(std::string& value) const;

    template <typename T> void DeserializeValue(csp::common::Array<T>& value) const;
    template <typename T> void DeserializeValue(csp::common::List<T>& value) const;

    template <typename T> void DeserializeValue(std::vector<T>& value) const;
    template <typename T> void DeserializeValue(std::map<std::string, T>& value) const;
};

template <typename T> inline void JsonSerializer::SerializeValue(const csp::common::Array<T>& value)
{
    m_writer.StartArray();

    for (size_t i = 0; i < value.Size(); ++i)
    {
        SerializeValue(value[i]);
    }

    m_writer.EndArray();
}

template <typename T> inline void JsonSerializer::SerializeValue(const csp::common::List<T>& value)
{
    m_writer.StartArray();

    for (size_t i = 0; i < value.Size(); ++i)
    {
        SerializeValue(value[i]);
    }

    m_writer.EndArray();
}

template <typename T> inline void JsonSerializer::SerializeValue(const csp::common::Map<csp::common::String, T>& value)
{
    m_writer.StartObject();

    for (const std::pair<csp::common::String, T>& pair : value.GetUnderlying())
    {
        SerializeMember(pair.first.c_str(), pair.second);
    }

    m_writer.EndObject();
}

template <typename T> inline void JsonSerializer::SerializeValue(const std::vector<T>& value)
{
    m_writer.StartArray();

    for (size_t i = 0; i < value.size(); ++i)
    {
        SerializeValue(value[i]);
    }

    m_writer.EndArray();
}

template <typename T> inline void JsonSerializer::SerializeValue(const std::map<std::string, T>& value)
{
    m_writer.StartObject();

    for (const auto& pair : value)
    {
        SerializeMember(pair.first.c_str(), pair.second);
    }

    m_writer.EndObject();
}

template <typename T> inline void JsonDeserializer::DeserializeValue(csp::common::Array<T>& values) const
{
    rapidjson::SizeType size = m_valueStack.top()->Size();
    values = csp::common::Array<T>(size);

    for (rapidjson::SizeType i = 0; i < size; ++i)
    {
        // Get the json vale in the current array.
        auto jsonValue = &(*m_valueStack.top())[i];
        // Push this value to our stack, so subsequent calls affect read the inner object.
        m_valueStack.push(jsonValue);

        T newVal;
        DeserializeValue(newVal);
        values[i] = newVal;

        // Pop the element as we are finished reading.
        m_valueStack.pop();
    }
}

template <typename T> inline void JsonDeserializer::DeserializeValue(csp::common::List<T>& values) const
{
    rapidjson::SizeType size = m_valueStack.top()->Size();

    for (rapidjson::SizeType i = 0; i < size; ++i)
    {
        // Get the json vale in the current array.
        auto jsonValue = &(*m_valueStack.top())[i];
        // Push this value to our stack, so subsequent calls affect read the inner object.
        m_valueStack.push(jsonValue);

        T newVal;
        DeserializeValue(newVal);
        values.Append(newVal);

        // Pop the element as we are finished reading.
        m_valueStack.pop();
    }
}

template <typename T> inline void JsonDeserializer::DeserializeValue(std::vector<T>& values) const
{
    rapidjson::SizeType size = m_valueStack.top()->Size();
    values.resize(size);

    for (rapidjson::SizeType i = 0; i < size; ++i)
    {
        // Get the json vale in the current array.
        auto jsonValue = &(*m_valueStack.top())[i];
        // Push this value to our stack, so subsequent calls affect read the inner object.
        m_valueStack.push(jsonValue);

        T newVal;
        DeserializeValue(newVal);
        values[i] = newVal;

        // Pop the element as we are finished reading.
        m_valueStack.pop();
    }
}

template <typename T> inline void JsonDeserializer::DeserializeValue(std::map<std::string, T>& values) const
{
    for (auto itr = m_valueStack.top()->MemberBegin(); itr != m_valueStack.top()->MemberEnd(); ++itr)
    {
        const char* key = itr->name.GetString();

        m_valueStack.push(&itr->value);

        T newVal;
        DeserializeValue(newVal);

        values[key] = newVal;

        m_valueStack.pop();
    }
}

} // namespace csp::json
