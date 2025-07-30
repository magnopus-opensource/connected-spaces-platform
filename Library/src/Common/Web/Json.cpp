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
#include "Common/Web/Json.h"

namespace csp::web
{

template <class T, typename std::enable_if_t<!std::is_base_of_v<csp::services::DtoBase, T> && !std::is_base_of_v<csp::services::EnumBase, T>>*>
[[deprecated("Unsupported type for JSON serialisation! You should probably add support for it :)")]] rapidjson::Value TypeToJsonValue(
    const T& Value, RapidJsonAlloc& Allocator)
{
    assert(false && "Unsupported type for JSON serialisation! You should probably add support for it :(");
}

template <class T, typename std::enable_if_t<!std::is_base_of_v<csp::services::DtoBase, T> && !std::is_base_of_v<csp::services::EnumBase, T>>*>
[[deprecated("Unsupported type for JSON deserialisation! You should probably add support for it :)")]] inline void JsonValueToType(
    const rapidjson::Value& Value, T& Type)
{
    assert(false && "Unsupported type for JSON deserialisation! You should probably add support for it :)");
}

} // namespace csp::web
