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

#include "JsonSerializer.h"

void ToJson(csp::json::JsonSerializer& Serializer, int32_t Value)
{
	Serializer.Writer.Int(Value);
}

void ToJson(csp::json::JsonSerializer& Serializer, uint32_t Value)
{
	Serializer.Writer.Uint(Value);
}

void ToJson(csp::json::JsonSerializer& Serializer, int64_t Value)
{
	Serializer.Writer.Int64(Value);
}

void ToJson(csp::json::JsonSerializer& Serializer, uint64_t Value)
{
	Serializer.Writer.Uint64(Value);
}

void ToJson(csp::json::JsonSerializer& Serializer, bool Value)
{
	Serializer.Writer.Bool(Value);
}

void ToJson(csp::json::JsonSerializer& Serializer, float Value)
{
	Serializer.Writer.Double(Value);
}

void ToJson(csp::json::JsonSerializer& Serializer, double Value)
{
	Serializer.Writer.Double(Value);
}

void ToJson(csp::json::JsonSerializer& Serializer, const csp::common::String& Value)
{
	Serializer.Writer.String(Value);
}

void ToJson(csp::json::JsonSerializer& Serializer, const char* Value)
{
	Serializer.Writer.String(Value);
}

void ToJson(csp::json::JsonSerializer& Serializer, std::nullptr_t Value)
{
	Serializer.Writer.Null();
}

void FromJson(const csp::json::JsonDeserializer& Deserializer, int32_t& Value)
{
	Value = Deserializer.ValueStack.top()->GetInt();
}

void FromJson(const csp::json::JsonDeserializer& Deserializer, uint32_t& Value)
{
	Value = Deserializer.ValueStack.top()->GetUint();
}

void FromJson(const csp::json::JsonDeserializer& Deserializer, int64_t& Value)
{
	Value = Deserializer.ValueStack.top()->GetInt64();
}

void FromJson(const csp::json::JsonDeserializer& Deserializer, uint64_t& Value)
{
	Value = Deserializer.ValueStack.top()->GetUint64();
}

void FromJson(const csp::json::JsonDeserializer& Deserializer, bool& Value)
{
	Value = Deserializer.ValueStack.top()->GetBool();
}

void FromJson(const csp::json::JsonDeserializer& Deserializer, float& Value)
{
	Value = static_cast<float>(Deserializer.ValueStack.top()->GetDouble());
}

void FromJson(const csp::json::JsonDeserializer& Deserializer, double& Value)
{
	Value = Deserializer.ValueStack.top()->GetDouble();
}

void FromJson(const csp::json::JsonDeserializer& Deserializer, csp::common::String& Value)
{
	Value = Deserializer.ValueStack.top()->GetString();
}

void FromJson(const csp::json::JsonDeserializer& Deserializer, const char* Value)
{
	Value = Deserializer.ValueStack.top()->GetString();
}

namespace csp::json
{

}
