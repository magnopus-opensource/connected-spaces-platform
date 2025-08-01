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

#include "Common/Web/HttpPayload.h"
#include "Common/Web/Json.h"

#include <rapidjson/rapidjson.h>

namespace csp::web
{

template <> inline rapidjson::Value TypeToJsonValue(const csp::web::HttpPayload& Value, RapidJsonAlloc& Allocator)
{
    assert(false && "This function is just a stub. Please flesh it out!");

    return rapidjson::Value("", Allocator);
}

template <> inline void JsonValueToType(const rapidjson::Value& Value, csp::web::HttpPayload& Type)
{
    assert(false && "This function is just a stub. Please flesh it out!");
}

} // namespace csp::web
