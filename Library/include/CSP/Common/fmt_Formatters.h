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

/*
 * fmt Formatters to allow internal formatting of common types
 * If you are including csp as a library, you may wish to include this, but you will
 * need to have linked against the same fmt library CSP has.
 * This does not constitute public interface.
 */

#pragma once

CSP_START_IGNORE

#include "CSP/Common/String.h"
#include "Common/Web/HttpRequest.h"
#include "Common/Web/Json.h"

#include <fmt/base.h>
#include <fmt/format.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <string_view>

namespace fmt
{

// Formatter for csp::common::String.

template <> struct formatter<csp::common::String> : formatter<std::string_view>
{
    // parse is inherited from formatter<string_view>.

    auto format(const csp::common::String& s, format_context& ctx) const -> format_context::iterator
    {
        // wrap raw data in a string_view and forward to fmt
        return formatter<std::string_view>::format(std::string_view(s.c_str(), s.Length()), ctx);
    }
};

template <> struct formatter<csp::web::HttpRequest> : formatter<std::string_view>
{
    auto format(const csp::web::HttpRequest& request, format_context& ctx) const -> format_context::iterator
    {
        csp::common::String verb = "";
        switch (request.GetVerb())
        {
        case csp::web::ERequestVerb::Get:
            verb = "GET";
            break;
        case csp::web::ERequestVerb::Post:
            verb = "POST";
            break;
        case csp::web::ERequestVerb::Put:
            verb = "PUT";
            break;
        case csp::web::ERequestVerb::Delete:
            verb = "DELETE";
            break;
        case csp::web::ERequestVerb::Head:
            verb = "HEAD";
            break;
        case csp::web::ERequestVerb::Patch:
            verb = "PATCH";
            break;
        default:
            verb = "UNKNOWN";
            break;
        }

        csp::common::String url = request.GetUri().GetAsString();

        const auto& requestHeaders = request.GetPayload().GetHeaders();

        csp::common::String headers;
        for (const auto& header : requestHeaders)
        {
            headers.Append(fmt::format("\t{}: {}\n", header.first, header.second).c_str());
        }

        const csp::common::String& requestPayload = request.GetPayload().ToJson();

        csp::common::String requestBody = "";

        if (!requestPayload.IsEmpty())
        {
            rapidjson::Document requestJson;
            rapidjson::ParseResult ok = requestJson.Parse(requestPayload);
            if (!ok)
            {
                requestBody.Append(fmt::format("\tFailed to parse request body as JSON: {}\n", rapidjson::GetParseError_En(ok.Code())).c_str());
            }
            else if (requestJson.IsObject())
            {
                for (rapidjson::Value::ConstMemberIterator itr = requestJson.MemberBegin(); itr != requestJson.MemberEnd(); ++itr)
                {
                    // Obfuscate the users password when logging the Http request body
                    if (std::string_view(itr->name.GetString()) == "password")
                    {
                        requestBody.Append(fmt::format("\t{}: ******\n", csp::web::JsonObjectToString(itr->name)).c_str());
                    }
                    else
                    {
                        requestBody.Append(
                            fmt::format("\t{}: {}\n", csp::web::JsonObjectToString(itr->name), csp::web::JsonObjectToString(itr->value)).c_str());
                    }
                }
            }
        }

        csp::common::String formattedRequestString
            = fmt::format("HTTP Request\n{0} {1}\nRequest Headers\n{2}Request Body\n{3}", verb, url, headers, requestBody).c_str();

        // wrap raw data in a string_view and forward to fmt
        return formatter<std::string_view>::format(std::string_view(formattedRequestString.c_str(), formattedRequestString.Length()), ctx);
    }
};

} // namespace fmt

CSP_END_IGNORE
