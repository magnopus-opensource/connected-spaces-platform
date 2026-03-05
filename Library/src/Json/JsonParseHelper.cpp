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

rapidjson::Document& ParseWithErrorLogging(rapidjson::Document& Document, const csp::common::String& JsonString, const char* LogPrefix)
{
    rapidjson::ParseResult ok = Document.Parse(JsonString.c_str());
    if (!ok)
    {
        // Log the error with some context around where in the string the error occurred.
        const size_t Offset = ok.Offset();
        const size_t TotalLen = JsonString.Length();
        const size_t Start = Offset > PARSE_ERROR_CONTEXT_CHARS ? Offset - PARSE_ERROR_CONTEXT_CHARS : 0;
        size_t End = Offset + PARSE_ERROR_CONTEXT_CHARS;
        if (End > TotalLen)
        {
            End = TotalLen;
        }

        const std::string Excerpt(JsonString.c_str() + Start, End - Start);

        CSP_LOG_ERROR_FORMAT("Error: %s: JSON parse error: %s (at offset %zu). Context: %s", LogPrefix, rapidjson::GetParseError_En(ok.Code()),
            ok.Offset(), Excerpt.c_str());
    }

    // Return the document to follow the chaining pattern from rapidjson.
    // The document will be in an invalid state if parsing failed.
    return Document;
}

} // namespace csp::json
