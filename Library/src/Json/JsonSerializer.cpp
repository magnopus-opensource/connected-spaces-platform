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

namespace csp::json
{

void JsonSerializer::SerializeValue(int32_t Value) { Writer.Int(Value); }

void JsonSerializer::SerializeValue(uint32_t Value) { Writer.Uint(Value); }

void JsonSerializer::SerializeValue(int64_t Value) { Writer.Int64(Value); }

void JsonSerializer::SerializeValue(uint64_t Value) { Writer.Uint64(Value); }

void JsonSerializer::SerializeValue(bool Value) { Writer.Bool(Value); }

void JsonSerializer::SerializeValue(float Value) { Writer.Double(Value); }

void JsonSerializer::SerializeValue(double Value) { Writer.Double(Value); }

void JsonSerializer::SerializeValue(const csp::common::String& Value) { Writer.String(Value); }

void JsonSerializer::SerializeValue(const char* Value) { Writer.String(Value); }

void JsonSerializer::SerializeValue(std::nullptr_t Value) { Writer.Null(); }

void JsonDeserializer::DeserializeValue(int32_t& Value) const { Value = ValueStack.top()->GetInt(); }

void JsonDeserializer::DeserializeValue(uint32_t& Value) const { Value = ValueStack.top()->GetUint(); }

void JsonDeserializer::DeserializeValue(int64_t& Value) const { Value = ValueStack.top()->GetInt64(); }

void JsonDeserializer::DeserializeValue(uint64_t& Value) const { Value = ValueStack.top()->GetUint64(); }

void JsonDeserializer::DeserializeValue(bool& Value) const { Value = ValueStack.top()->GetBool(); }

void JsonDeserializer::DeserializeValue(float& Value) const { Value = static_cast<float>(ValueStack.top()->GetDouble()); }

void JsonDeserializer::DeserializeValue(double& Value) const { Value = ValueStack.top()->GetDouble(); }

void JsonDeserializer::DeserializeValue(csp::common::String& Value) const { Value = ValueStack.top()->GetString(); }

void JsonDeserializer::DeserializeValue(const char* Value) const { Value = ValueStack.top()->GetString(); }
} // namespace csp::json
