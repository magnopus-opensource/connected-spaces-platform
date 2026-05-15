/*
 * Copyright 2026 Magnopus LLC

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

#include "JsonParseHelper.h"
#include "Debug/Logging.h"
#include <rapidjson/error/en.h>
#include <string>

namespace csp::json
{

const size_t PARSE_ERROR_CONTEXT_CHARS = 20;

rapidjson::Document& ParseWithErrorLogging(rapidjson::Document& document, const csp::common::String& jsonString, const char* logPrefix)
{
    rapidjson::ParseResult ok = document.Parse(jsonString.c_str());
    if (!ok)
    {
        // Log the error with some context around where in the string the error occurred.
        const size_t offset = ok.Offset();
        const size_t totalLen = jsonString.Length();
        const size_t start = offset > PARSE_ERROR_CONTEXT_CHARS ? offset - PARSE_ERROR_CONTEXT_CHARS : 0;
        size_t end = offset + PARSE_ERROR_CONTEXT_CHARS;
        if (end > totalLen)
        {
            end = totalLen;
        }

        const std::string excerpt(jsonString.c_str() + start, end - start);

        CSP_LOG_ERROR_FORMAT("Error: %s: JSON parse error: %s (at offset %zu). Context: %s", logPrefix, rapidjson::GetParseError_En(ok.Code()),
            ok.Offset(), excerpt.c_str());
    }

    // Return the document to follow the chaining pattern from rapidjson.
    // The document will be in an invalid state if parsing failed.
    return document;
}

} // namespace csp::json
