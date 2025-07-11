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
#include <Common/Web/HttpRequest.h>
#include <Common/Web/Json.h>

#include <fmt/base.h>
#include <string_view>

namespace fmt
{

// Formatter for csp::common::String.

template <> struct fmt::formatter<csp::common::String> : formatter<std::string_view>
{
    // parse is inherited from formatter<string_view>.

    auto format(const csp::common::String& s, format_context& ctx) const -> format_context::iterator
    {
        // wrap raw data in a string_view and forward to fmt
        return formatter<std::string_view>::format(std::string_view(s.c_str(), s.Length()), ctx);
    }
};

template <> struct fmt::formatter<csp::web::HttpRequest> : formatter<std::string_view>
{
    auto format(const csp::web::HttpRequest& Request, format_context& ctx) const -> format_context::iterator
    {
        csp::common::String Verb = "";
        switch (Request.GetVerb())
        {
        case csp::web::ERequestVerb::Get:
            Verb = "GET";
            break;
        case csp::web::ERequestVerb::Post:
            Verb = "POST";
            break;
        case csp::web::ERequestVerb::Put:
            Verb = "PUT";
            break;
        case csp::web::ERequestVerb::Delete:
            Verb = "DELETE";
            break;
        case csp::web::ERequestVerb::Head:
            Verb = "HEAD";
            break;
        default:
            Verb = "UNKNOWN";
            break;
        }

        csp::common::String Url = Request.GetUri().GetAsString();

        const auto& RequestHeaders = Request.GetPayload().GetHeaders();

        csp::common::String Headers;
        for (const auto& Header : RequestHeaders)
        {
            Headers.Append(fmt::format("\t{}: {}\n", Header.first, Header.second).c_str());
        }

        const csp::common::String& RequestPayload = Request.GetPayload().ToJson();

        csp::common::String RequestBody = "";

        if (!RequestPayload.IsEmpty())
        {
            rapidjson::Document RequestJson;
            RequestJson.Parse(RequestPayload);

            if (RequestJson.IsObject())
            {
                for (rapidjson::Value::ConstMemberIterator itr = RequestJson.MemberBegin(); itr != RequestJson.MemberEnd(); ++itr)
                {
                    // Obfuscate the users password when logging the Http request body
                    if (std::string_view(itr->name.GetString()) == "password")
                    {
                        RequestBody.Append(fmt::format("\t{}: ******\n", csp::web::JsonObjectToString(itr->name)).c_str());
                    }
                    else
                    {
                        RequestBody.Append(
                            fmt::format("\t{}: {}\n", csp::web::JsonObjectToString(itr->name), csp::web::JsonObjectToString(itr->value)).c_str());
                    }
                }
            }
        }

        csp::common::String FormattedRequestString
            = fmt::format("HTTP Request\n{0} {1}\nRequest Headers\n{2}Request Body\n{3}", Verb, Url, Headers, RequestBody).c_str();

        // wrap raw data in a string_view and forward to fmt
        return formatter<std::string_view>::format(std::string_view(FormattedRequestString.c_str(), FormattedRequestString.Length()), ctx);
    }
};

} // namespace fmt

CSP_END_IGNORE
