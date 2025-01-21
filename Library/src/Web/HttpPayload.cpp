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
#include "Web/HttpPayload.h"

#include "CSP/CSPFoundation.h"
#include "CSP/Systems/Log/LogSystem.h"
#include "Common/StlString.h"
#include "Debug/Logging.h"
#include "Web/HttpAuth.h"

#include <cstdio>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <regex>
#include <sstream>

namespace csp::web
{

HttpPayload::HttpPayload()
{
    std::string ResponseContent = csp::CSPFoundation::GetClientUserAgentString().c_str();

    if (ResponseContent.find("Unset") != std::string::npos)
    {
        CSP_LOG_MSG(csp::systems::LogLevel::Warning,
            "ClientUserAgentInfo was not provided by the client. Please call CSPFoundation::SetClientUserAgentInfo() after initialisation.");
    }

    // Using custom header as User-Agent is protected on web SKUs
    AddHeader(CSP_TEXT("X-AssetPlatform"), csp::common::String(ResponseContent.c_str()));
}

HttpPayload::HttpPayload(const char* InContent)
    : Content(InContent)
{
}

HttpPayload::HttpPayload(const csp::common::String& InContent)
    : Content(InContent)
{
}

HttpPayload::~HttpPayload() { }

void HttpPayload::SetContent(const rapidjson::Document& InJson) { SetContent(JsonDocToString(InJson)); }

void HttpPayload::SetContent(const csp::common::String& InContent) { Content = InContent; }

void HttpPayload::AddContent(const csp::common::String& InContent) { Content = InContent; }

const csp::common::String& HttpPayload::GetContent() const { return Content; }

const csp::common::String& HttpPayload::ToJson() const { return Content; }

void HttpPayload::SetContent(const char* Data, size_t DataLength) { Content = csp::common::String(Data, DataLength); }

void HttpPayload::AllocateContent(size_t DataLength) { Content = csp::common::String(DataLength); }

/// @brief Write content to the payload from the specified buffer
/// @param Offset
/// @param Data
/// @param DataLength
void HttpPayload::WriteContent(size_t Offset, const char* Data, size_t DataLength)
{
    const size_t Length = Content.AllocatedMemorySize();

    if (Offset > Length)
    {
        // RWD_ASSERT_MSG(false, "Invalid offset %ld in HttpPayload::WriteContent, Len=%ld", Offset, Length);
        return;
    }

    char* ContentPtr = (char*)Content.c_str() + Offset;

    const size_t AvailableLength = Length - Offset;
    const size_t LengthToCopy = std::min(DataLength, AvailableLength);

    memcpy(ContentPtr, Data, LengthToCopy);
}

/// @brief Read from the payload content into the specified buffer
/// @param Offset
/// @param Data
/// @param DataLength
/// @return
size_t HttpPayload::ReadContent(size_t Offset, void* Data, size_t DataLength) const
{
    const size_t Length = Content.Length();

    if (Offset > Length)
    {
        // RWD_ASSERT_MSG(false, "Invalid offset %ld in HttpPayload::ReadContent, Len=%ld", Offset, Length);
        return 0;
    }

    const char* ContentPtr = Content.c_str() + Offset;

    const size_t AvailableLength = Length - Offset;
    const size_t LengthToCopy = std::min(DataLength, AvailableLength);

    memcpy(Data, ContentPtr, LengthToCopy);

    return LengthToCopy;
}

void HttpPayload::AddHeader(const csp::common::String& Key, const csp::common::String& Value)
{
    if (Headers.find(Key.c_str()) == Headers.end())
    {
        Headers.insert(HeadersMap::value_type(Key.c_str(), Value.c_str()));
    }
    else
    {
        Headers[Key.c_str()] = Value.c_str();
    }
}

const HttpPayload::HeadersMap& HttpPayload::GetHeaders() const { return Headers; }

// This only refers to the CHS bearer token that is managed by the WebClient. At the point
// this is called we only set that the bearer token is required. Right before this is
// actually sent to CHS it is the responsibility of the WebClient to call RefreshBearerToken
// which will ensure that the latest access token is added as a bearer token header.
// Ideally this would be named like SetBearerTokenRequired but it is used in many places
// from when this actually performed the actions now performed in RefreshBearerToken.
void HttpPayload::SetBearerToken() { RequiresBearerToken = true; }

bool HttpPayload::GetRequiresBearerToken() const { return RequiresBearerToken; }

void HttpPayload::RefreshBearerToken()
{
    if (!RequiresBearerToken)
    {
        return;
    }

    if (HttpAuth::GetAccessToken().c_str() != nullptr)
    {
        char Str[1024];
        snprintf(Str, 1024, "Bearer %s", HttpAuth::GetAccessToken().c_str());
        AddHeader(CSP_TEXT("Authorization"), CSP_TEXT(Str));
    }
}

void HttpPayload::Reset()
{
    Content = csp::common::String("");
    RequiresBearerToken = false;
    Headers.clear();
}

void HttpPayload::AddFormParam(const char* Name, const std::shared_ptr<csp::web::HttpPayload>& formFile)
{
    // Set Multipart type and some boundary value
    std::string BoundaryText = "multipart/form-data; boundary=" + std::string(formFile->Boundary.c_str());
    AddHeader(CSP_TEXT("Content-Type"), CSP_TEXT(BoundaryText.c_str()));

    SetContent(formFile->GetContent());
}

void HttpPayload::SetBoundary(const csp::common::String& InBoundary) { Boundary = InBoundary; }

// This checks not only for "application/json" but also covers cases like "application/graphql+json" and "application/problem+json"
bool HttpPayload::IsJsonPayload() const
{
    const auto& ContentTypeHeader = Headers.find("content-type");
    if (ContentTypeHeader == Headers.end())
    {
        return false;
    }
    else
    {
        std::regex Regex("^application\\/([a-z]+\\+)?json");
        return std::regex_search(ContentTypeHeader->second.c_str(), Regex);
    }
}

} // namespace csp::web
