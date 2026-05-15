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

void JsonSerializer::SerializeValue(int32_t value) { m_writer.Int(value); }

void JsonSerializer::SerializeValue(uint32_t value) { m_writer.Uint(value); }

void JsonSerializer::SerializeValue(int64_t value) { m_writer.Int64(value); }

void JsonSerializer::SerializeValue(uint64_t value) { m_writer.Uint64(value); }

void JsonSerializer::SerializeValue(bool value) { m_writer.Bool(value); }

void JsonSerializer::SerializeValue(float value) { m_writer.Double(value); }

void JsonSerializer::SerializeValue(double value) { m_writer.Double(value); }

void JsonSerializer::SerializeValue(const char* value) { m_writer.String(value); }

void JsonSerializer::SerializeValue(const csp::common::String& value) { m_writer.String(value); }

void JsonSerializer::SerializeValue(std::nullptr_t /*Value*/) { m_writer.Null(); }

void JsonSerializer::SerializeValue(const std::string& value) { m_writer.String(value.c_str()); }

std::string JsonDeserializer::GetMemberAsString(const char* key) const
{
    auto jsonValue = &(*m_valueStack.top())[key];

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    jsonValue->Accept(writer);
    return buffer.GetString();
}

void JsonDeserializer::EnterMember(const char* key) const
{
    auto jsonValue = &(*m_valueStack.top())[key];
    m_valueStack.push(jsonValue);
}

void JsonDeserializer::ExitMember() const { m_valueStack.pop(); }

void JsonDeserializer::DeserializeValue(int32_t& value) const { value = m_valueStack.top()->GetInt(); }

void JsonDeserializer::DeserializeValue(uint32_t& value) const { value = m_valueStack.top()->GetUint(); }

void JsonDeserializer::DeserializeValue(int64_t& value) const { value = m_valueStack.top()->GetInt64(); }

void JsonDeserializer::DeserializeValue(uint64_t& value) const { value = m_valueStack.top()->GetUint64(); }

void JsonDeserializer::DeserializeValue(bool& value) const { value = m_valueStack.top()->GetBool(); }

void JsonDeserializer::DeserializeValue(float& value) const { value = static_cast<float>(m_valueStack.top()->GetDouble()); }

void JsonDeserializer::DeserializeValue(double& value) const { value = m_valueStack.top()->GetDouble(); }

void JsonDeserializer::DeserializeValue(csp::common::String& value) const { value = m_valueStack.top()->GetString(); }
void JsonDeserializer::DeserializeValue(std::string& value) const { value = m_valueStack.top()->GetString(); }
} // namespace csp::json
