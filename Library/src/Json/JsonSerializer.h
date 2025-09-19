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

template <typename T> inline void ToJson(csp::json::JsonSerializer& Serializer, T Object);
template <typename T> inline void ToJson(csp::json::JsonSerializer& Serializer, const T& Object);
template <typename T> inline void ToJson(csp::json::JsonSerializer& Serializer, const T* Object);

template <typename T> inline void FromJson(const csp::json::JsonDeserializer& Deserializer, T& Object);
template <typename T> inline void FromJson(const csp::json::JsonDeserializer& Deserializer, T* Object);

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
    template <typename T> static common::String Serialize(const T& Object)
    {
        JsonSerializer Serializer;

        Serializer.SerializeValue(Object);
        return Serializer.Buffer.GetString();
    }

    /// @brief Should be called within custom ToJson function.
    /// @details This will serialize a member.
    /// If the member is another custom type, this was internally call ToJson on that type.
    /// @param Key const char * Key : The key to reference the item.
    /// @param Value const T& : The value to serialize.
    template <typename T> void SerializeMember(const char* Key, const T& Value)
    {
        Writer.String(Key);
        SerializeValue(Value);
    }

private:
    rapidjson::StringBuffer Buffer;
    rapidjson::Writer<rapidjson::StringBuffer> Writer;

    JsonSerializer()
        : Writer(Buffer)
    {
    }

    template <typename T> void SerializeValue(const T& Value)
    {
        // If T isn't one of the internal supported types,
        // assume this is a custom object
        Writer.StartObject();
        ::ToJson(*this, Value);
        Writer.EndObject();
    }

    void SerializeValue(int32_t Value);
    void SerializeValue(uint32_t Value);
    void SerializeValue(int64_t Value);
    void SerializeValue(uint64_t Value);
    void SerializeValue(bool Value);
    void SerializeValue(float Value);
    void SerializeValue(double Value);
    void SerializeValue(const char* Value);
    void SerializeValue(const csp::common::String& Value);
    void SerializeValue(std::nullptr_t Value);

    void SerializeValue(const std::string& Value);

    template <typename T> void SerializeValue(const csp::common::Array<T>& Value);
    template <typename T> void SerializeValue(const csp::common::List<T>& Value);
    template <typename T> void SerializeValue(const csp::common::Map<csp::common::String, T>& Value);

    template <typename T> void SerializeValue(const std::vector<T>& Value);
    template <typename T> void SerializeValue(const std::map<std::string, T>& Value);
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
    template <typename T> static bool Deserialize(const char* Data, T& Object)
    {
        JsonDeserializer Deserializer { Data };

        if (Deserializer.Doc.HasParseError())
        {
            // Error parsing Json
            return false;
        }

        rapidjson::Value Root = Deserializer.Doc.GetObject();

        Deserializer.ValueStack.push(&Root);
        Deserializer.DeserializeValue(Object);
        Deserializer.ValueStack.pop();

        return true;
    }

    /// @brief Should be called within custom FromJson function.
    /// @details This will deserialize a member with the given key.
    /// If the member is another custom type, this was internally call FromJson on that type.
    /// @param Key const char* : The key which references this member.
    /// @param Val T& : The member to deserialize to.
    template <typename T> void DeserializeMember(const char* Key, T& Val) const
    {
        ValueStack.push(&(*ValueStack.top())[Key]);
        DeserializeValue(Val);
        ValueStack.pop();
    }

    /// @brief Should be called within custom FromJson function.
    /// @details This will safely deserialize a member with the given key by first checking that the member exists.
    /// If the member is another custom type, this was internally call FromJson on that type.
    /// @param Key const char* : The key which references this member.
    /// @param Val T& : The member to deserialize to.
    /// @return bool : Returns true if the key exists in this object and the member has been deserialized.
    template <typename T> bool SafeDeserializeMember(const char* Key, T& Val) const
    {
        if (HasProperty(Key))
        {
            DeserializeMember(Key, Val);
            return true;
        }
        return false;
    }

    /// @brief Should be called within custom FromJson function.
    /// @details This will return true if the given key exists
    /// @param Key const char* : The key to check.
    /// @return bool : Returns true if the key exists in this object.
    bool HasProperty(const char* Key) const { return ValueStack.top()->HasMember(Key); }

    std::string GetMemberAsString(const char* Key) const;

    /// @brief Starts the deserialization process of the member with the given key.
    /// Subsequent calls with deserialize members within the given member.
    /// @param Key const char* : The Key of the member to enter.
    void EnterMember(const char* Key) const;

    /// @brief Stops deserialization of the current object and goes back to the parent object.
    /// Subsequent calls with deserialize members within the parent object.
    void ExitMember() const;

private:
    JsonDeserializer(const char* Data) { Doc.Parse(Data); }

    template <typename T> inline void DeserializeValue(T& Value) const { ::FromJson(*this, Value); }

    rapidjson::Document Doc;
    mutable std::stack<const rapidjson::Value*> ValueStack;

    void DeserializeValue(int32_t& Value) const;
    void DeserializeValue(uint32_t& Value) const;
    void DeserializeValue(int64_t& Value) const;
    void DeserializeValue(uint64_t& Value) const;
    void DeserializeValue(bool& Value) const;
    void DeserializeValue(float& Value) const;
    void DeserializeValue(double& Value) const;
    void DeserializeValue(csp::common::String& Value) const;

    void DeserializeValue(std::string& Value) const;

    template <typename T> void DeserializeValue(csp::common::Array<T>& Value) const;
    template <typename T> void DeserializeValue(csp::common::List<T>& Value) const;

    template <typename T> void DeserializeValue(std::vector<T>& Value) const;
    template <typename T> void DeserializeValue(std::map<std::string, T>& Value) const;
};

template <typename T> inline void JsonSerializer::SerializeValue(const csp::common::Array<T>& Value)
{
    Writer.StartArray();

    for (size_t i = 0; i < Value.Size(); ++i)
    {
        SerializeValue(Value[i]);
    }

    Writer.EndArray();
}

template <typename T> inline void JsonSerializer::SerializeValue(const csp::common::List<T>& Value)
{
    Writer.StartArray();

    for (size_t i = 0; i < Value.Size(); ++i)
    {
        SerializeValue(Value[i]);
    }

    Writer.EndArray();
}

template <typename T> inline void JsonSerializer::SerializeValue(const csp::common::Map<csp::common::String, T>& Value)
{
    Writer.StartObject();

    for (const std::pair<csp::common::String, T>& Pair : Value.GetUnderlying())
    {
        SerializeMember(Pair.first.c_str(), Pair.second);
    }

    Writer.EndObject();
}

template <typename T> inline void JsonSerializer::SerializeValue(const std::vector<T>& Value)
{
    Writer.StartArray();

    for (size_t i = 0; i < Value.size(); ++i)
    {
        SerializeValue(Value[i]);
    }

    Writer.EndArray();
}

template <typename T> inline void JsonSerializer::SerializeValue(const std::map<std::string, T>& Value)
{
    Writer.StartObject();

    for (const auto& Pair : Value)
    {
        SerializeMember(Pair.first.c_str(), Pair.second);
    }

    Writer.EndObject();
}

template <typename T> inline void JsonDeserializer::DeserializeValue(csp::common::Array<T>& Values) const
{
    rapidjson::SizeType Size = ValueStack.top()->Size();
    Values = csp::common::Array<T>(Size);

    for (rapidjson::SizeType i = 0; i < Size; ++i)
    {
        // Get the json vale in the current array.
        auto JsonValue = &(*ValueStack.top())[i];
        // Push this value to our stack, so subsequent calls affect read the inner object.
        ValueStack.push(JsonValue);

        T NewVal;
        DeserializeValue(NewVal);
        Values[i] = NewVal;

        // Pop the element as we are finished reading.
        ValueStack.pop();
    }
}

template <typename T> inline void JsonDeserializer::DeserializeValue(csp::common::List<T>& Values) const
{
    rapidjson::SizeType Size = ValueStack.top()->Size();

    for (rapidjson::SizeType i = 0; i < Size; ++i)
    {
        // Get the json vale in the current array.
        auto JsonValue = &(*ValueStack.top())[i];
        // Push this value to our stack, so subsequent calls affect read the inner object.
        ValueStack.push(JsonValue);

        T NewVal;
        DeserializeValue(NewVal);
        Values.Append(NewVal);

        // Pop the element as we are finished reading.
        ValueStack.pop();
    }
}

template <typename T> inline void JsonDeserializer::DeserializeValue(std::vector<T>& Values) const
{
    rapidjson::SizeType Size = ValueStack.top()->Size();
    Values.resize(Size);

    for (rapidjson::SizeType i = 0; i < Size; ++i)
    {
        // Get the json vale in the current array.
        auto JsonValue = &(*ValueStack.top())[i];
        // Push this value to our stack, so subsequent calls affect read the inner object.
        ValueStack.push(JsonValue);

        T NewVal;
        DeserializeValue(NewVal);
        Values[i] = NewVal;

        // Pop the element as we are finished reading.
        ValueStack.pop();
    }
}

template <typename T> inline void JsonDeserializer::DeserializeValue(std::map<std::string, T>& Values) const
{
    for (auto Itr = ValueStack.top()->MemberBegin(); Itr != ValueStack.top()->MemberEnd(); ++Itr)
    {
        const char* Key = Itr->name.GetString();

        ValueStack.push(&Itr->value);

        T NewVal;
        DeserializeValue(NewVal);

        Values[Key] = NewVal;

        ValueStack.pop();
    }
}

} // namespace csp::json
