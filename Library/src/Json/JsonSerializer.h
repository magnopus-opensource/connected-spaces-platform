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

#include "CSP/CSPCommon.h"
#include "CSP/Common/String.h"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <stack>

namespace csp::json
{
class JsonSerializer;
class JsonDeserializer;
} // namespace csp::json

void ToJson(csp::json::JsonSerializer& Serializer, int32_t Value);
void ToJson(csp::json::JsonSerializer& Serializer, uint32_t Value);
void ToJson(csp::json::JsonSerializer& Serializer, int64_t Value);
void ToJson(csp::json::JsonSerializer& Serializer, uint64_t Value);
void ToJson(csp::json::JsonSerializer& Serializer, bool Value);
void ToJson(csp::json::JsonSerializer& Serializer, float Value);
void ToJson(csp::json::JsonSerializer& Serializer, double Value);
void ToJson(csp::json::JsonSerializer& Serializer, const csp::common::String& Value);
void ToJson(csp::json::JsonSerializer& Serializer, const char* Value);
void ToJson(csp::json::JsonSerializer& Serializer, std::nullptr_t Value);

void FromJson(const csp::json::JsonDeserializer& Deserializer, int32_t& Value);
void FromJson(const csp::json::JsonDeserializer& Deserializer, uint32_t& Value);
void FromJson(const csp::json::JsonDeserializer& Deserializer, int64_t& Value);
void FromJson(const csp::json::JsonDeserializer& Deserializer, uint64_t& Value);
void FromJson(const csp::json::JsonDeserializer& Deserializer, bool& Value);
void FromJson(const csp::json::JsonDeserializer& Deserializer, float& Value);
void FromJson(const csp::json::JsonDeserializer& Deserializer, double& Value);
void FromJson(const csp::json::JsonDeserializer& Deserializer, csp::common::String& Value);
void FromJson(const csp::json::JsonDeserializer& Deserializer, const char* Value);

template <typename T> void ToJson(csp::json::JsonSerializer& Serializer, const csp::common::Array<T>& Value)
{
	Serializer.Writer.StartArray();

	for (size_t i = 0; i < Value.Size(); ++i)
	{
		ToJson(Serializer, Value[i]);
	}

	Serializer.Writer.EndArray();
}

template <typename T> void ToJson(csp::json::JsonSerializer& Serializer, const csp::common::List<T>& Value)
{
	Serializer.Writer.StartArray();

	for (size_t i = 0; i < Value.Size(); ++i)
	{
		ToJson(Serializer, Value[i]);
	}

	Serializer.Writer.EndArray();
}

template <typename T> void FromJson(const csp::json::JsonDeserializer& Deserializer, csp::common::Array<T>& Values)
{
	int Size = Deserializer.ValueStack.top()->Size();
	Values	 = csp::common::Array<T>(Size);

	for (int i = 0; i < Size; ++i)
	{
		auto JsonValue = &(*Deserializer.ValueStack.top())[i];
		Deserializer.ValueStack.push(JsonValue);

		T NewVal;
		FromJson(Deserializer, NewVal);
		Values[i] = NewVal;

		Deserializer.ValueStack.pop();
	}
}

template <typename T> void FromJson(const csp::json::JsonDeserializer& Deserializer, csp::common::List<T>& Values)
{
	int Size = Deserializer.ValueStack.top()->Size();

	for (int i = 0; i < Size; ++i)
	{
		auto JsonValue = &(*Deserializer.ValueStack.top())[i];
		Deserializer.ValueStack.push(JsonValue);

		T NewVal;
		FromJson(Deserializer, NewVal);
		Values.Append(NewVal);

		Deserializer.ValueStack.pop();
	}
}

namespace csp::json
{

class JsonSerializer
{
public:
	/// @brief Generates a json string from an object
	/// A global ToJson function in the root namespace should be created to work with this function:
	/// void ToJson(csp::json::JsonSerializer& Serializer, const T& Value);
	/// @param Object const T& : The object to serialize
	/// @return String : The serialized json string
	template <typename T> static common::String Serialize(const T& Object)
	{
		JsonSerializer Serializer;

		Serializer.Writer.StartObject();
		::ToJson(Serializer, Object);
		Serializer.Writer.EndObject();
		return Serializer.Buffer.GetString();
	}

	/// @brief Should be called within custon ToJson function
	/// This will serialize an object member
	/// If the member is another custom type, this was internally call ToJson on that type
	/// @param Key const char * Key : The key to reference the object
	/// @param Value const T& : The object value to serialize
	template <typename T> void SerializeObject(const char* Key, const T& Value)
	{
		Writer.String(Key);

		Writer.StartObject();
		::ToJson(*this, Value);
		Writer.EndObject();
	}

	/// @brief Should be called within custon ToJson function
	/// This will serialize a property member
	/// If the member is another custom type, this was internally call ToJson on that type
	/// @param Key const char * Key : The key to reference the object
	/// @param Value const T& : The property value to serialize
	template <typename Val> void SerializeProperty(const char* Key, const Val& Value)
	{
		Writer.String(Key);
		SerializeValue(Value);
	}

private:
	rapidjson::StringBuffer Buffer;
	rapidjson::Writer<rapidjson::StringBuffer> Writer;

	JsonSerializer() : Writer(Buffer)
	{
	}

	template <typename T> void SerializeValue(const T& Value)
	{
		::ToJson(*this, Value);
	}

	// Default types should be friend to access internal json writer
	friend void ::ToJson(csp::json::JsonSerializer& Serializer, int32_t Value);
	friend void ::ToJson(csp::json::JsonSerializer& Serializer, uint32_t Value);
	friend void ::ToJson(csp::json::JsonSerializer& Serializer, int64_t Value);
	friend void ::ToJson(csp::json::JsonSerializer& Serializer, uint64_t Value);
	friend void ::ToJson(csp::json::JsonSerializer& Serializer, bool Value);
	friend void ::ToJson(csp::json::JsonSerializer& Serializer, float Value);
	friend void ::ToJson(csp::json::JsonSerializer& Serializer, double Value);
	friend void ::ToJson(csp::json::JsonSerializer& Serializer, const csp::common::String& Value);
	friend void ::ToJson(csp::json::JsonSerializer& Serializer, const char* Value);
	friend void ::ToJson(csp::json::JsonSerializer& Serializer, std::nullptr_t Value);

	template <typename T> friend void ::ToJson(csp::json::JsonSerializer& Serializer, const csp::common::Array<T>& Value);
	template <typename T> friend void ::ToJson(csp::json::JsonSerializer& Serializer, const csp::common::List<T>& Value);
};

template <typename T> inline void ToJson(csp::json::JsonSerializer& Serializer, T Object);
template <typename T> inline void ToJson(csp::json::JsonSerializer& Serializer, const T& Object);
template <typename T> inline void ToJson(csp::json::JsonSerializer& Serializer, const T* Object);

class JsonDeserializer
{
public:
	/// @brief Converts a given Json string into the specified object
	/// A global FromJson function in the root namespace should be created to work with this function:
	/// void FromJson(const csp::json::JsonDeserializer& Deserializer, T& Value);
	/// @param Data const char* : The json string to deserialize
	/// @param Object T& : The object to convert to
	template <typename T> static bool Deserialize(const char* Data, T& Object)
	{
		JsonDeserializer Deserializer {Data};

		if (Deserializer.Doc.HasParseError())
		{
			// Error parsing Json
			return false;
		}

		rapidjson::Value Root = Deserializer.Doc.GetObject();

		Deserializer.ValueStack.push(&Root);
		FromJson(Deserializer, Object);
		Deserializer.ValueStack.pop();
	}

	/// @brief Should be called within custom FromJson function
	/// This will deserialize a member with the given key
	/// If the member is another custom type, this was internally call FromJson on that type
	/// @param Key const char* : The key which references this member
	/// @param Val T& : The member to deserialize to
	template <typename T> const void DeserializeMember(const char* Key, T& Val) const
	{
		ValueStack.push(&(*ValueStack.top())[Key]);
		::FromJson(*this, Val);
		ValueStack.pop();
	}

	/// @brief Should be called within custon FromJson function
	/// This will return true if the given key exists
	/// @param Key const char* : The key to check
	/// @return bool : Returns true if the key exists in this object
	bool HasProperty(const char* Key) const
	{
		return Doc.HasMember(Key);
	}

private:
	JsonDeserializer(const char* Data)
	{
		Doc.Parse(Data);
	}

	rapidjson::Document Doc;
	mutable std::stack<const rapidjson::Value*> ValueStack;

	friend void ::FromJson(const csp::json::JsonDeserializer& Deserializer, int32_t& Value);
	friend void ::FromJson(const csp::json::JsonDeserializer& Deserializer, uint32_t& Value);
	friend void ::FromJson(const csp::json::JsonDeserializer& Deserializer, int64_t& Value);
	friend void ::FromJson(const csp::json::JsonDeserializer& Deserializer, uint64_t& Value);
	friend void ::FromJson(const csp::json::JsonDeserializer& Deserializer, bool& Value);
	friend void ::FromJson(const csp::json::JsonDeserializer& Deserializer, float& Value);
	friend void ::FromJson(const csp::json::JsonDeserializer& Deserializer, double& Value);
	friend void ::FromJson(const csp::json::JsonDeserializer& Deserializer, csp::common::String& Value);
	friend void ::FromJson(const csp::json::JsonDeserializer& Deserializer, const char* Value);

	template <typename T> friend void ::FromJson(const csp::json::JsonDeserializer& Deserializer, csp::common::Array<T>& Value);
	template <typename T> friend void ::FromJson(const csp::json::JsonDeserializer& Deserializer, csp::common::List<T>& Value);
};

template <typename T> inline void FromJson(const csp::json::JsonDeserializer& Deserializer, T& Object);
template <typename T> inline void FromJson(const csp::json::JsonDeserializer& Deserializer, T* Object);
} // namespace csp::json
