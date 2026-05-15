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
#include "Common/Web/HttpPayload.h"

#include "CSP/CSPFoundation.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "Common/Web/HttpAuth.h"
#include "Debug/Logging.h"

#include <cstdio>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <regex>
#include <sstream>

namespace csp::web
{

HttpPayload::HttpPayload()
{
    std::string responseContent = csp::CSPFoundation::GetClientUserAgentString().c_str();

    // Using custom header as User-Agent is protected on web SKUs
    AddHeader(CSP_TEXT("X-AssetPlatform"), csp::common::String(responseContent.c_str()));
}

HttpPayload::HttpPayload(const char* inContent)
    : m_content(inContent)
{
}

HttpPayload::HttpPayload(const csp::common::String& inContent)
    : m_content(inContent)
{
}

HttpPayload::~HttpPayload() { }

void HttpPayload::SetContent(const rapidjson::Document& inJson) { SetContent(JsonDocToString(inJson)); }

void HttpPayload::SetContent(const csp::common::String& inContent) { m_content = inContent; }

void HttpPayload::AddContent(const csp::common::String& inContent) { m_content = inContent; }

const csp::common::String& HttpPayload::GetContent() const { return m_content; }

const csp::common::String& HttpPayload::ToJson() const { return m_content; }

void HttpPayload::SetContent(const char* data, size_t dataLength) { m_content = csp::common::String(data, dataLength); }

void HttpPayload::AllocateContent(size_t dataLength) { m_content = csp::common::String(dataLength); }

/// @brief Write content to the payload from the specified buffer
/// @param Offset
/// @param Data
/// @param DataLength
void HttpPayload::WriteContent(size_t offset, const char* data, size_t dataLength)
{
    const size_t length = m_content.AllocatedMemorySize();

    if (offset > length)
    {
        // RWD_ASSERT_MSG(false, "Invalid offset %ld in HttpPayload::WriteContent, Len=%ld", Offset, Length);
        return;
    }

    char* contentPtr = (char*)m_content.c_str() + offset;

    const size_t availableLength = length - offset;
    const size_t lengthToCopy = std::min(dataLength, availableLength);

    memcpy(contentPtr, data, lengthToCopy);
}

/// @brief Read from the payload content into the specified buffer
/// @param Offset
/// @param Data
/// @param DataLength
/// @return
size_t HttpPayload::ReadContent(size_t offset, void* data, size_t dataLength) const
{
    const size_t length = m_content.Length();

    if (offset > length)
    {
        // RWD_ASSERT_MSG(false, "Invalid offset %ld in HttpPayload::ReadContent, Len=%ld", Offset, Length);
        return 0;
    }

    const char* contentPtr = m_content.c_str() + offset;

    const size_t availableLength = length - offset;
    const size_t lengthToCopy = std::min(dataLength, availableLength);

    memcpy(data, contentPtr, lengthToCopy);

    return lengthToCopy;
}

void HttpPayload::AddHeader(const csp::common::String& key, const csp::common::String& value)
{
    if (m_headers.find(key.c_str()) == m_headers.end())
    {
        m_headers.insert(HeadersMap::value_type(key.c_str(), value.c_str()));
    }
    else
    {
        m_headers[key.c_str()] = value.c_str();
    }
}

const HttpPayload::HeadersMap& HttpPayload::GetHeaders() const { return m_headers; }

// This only refers to the CHS bearer token that is managed by the WebClient. At the point
// this is called we only set that the bearer token is required. Right before this is
// actually sent to CHS it is the responsibility of the WebClient to call RefreshBearerToken
// which will ensure that the latest access token is added as a bearer token header.
// Ideally this would be named like SetBearerTokenRequired but it is used in many places
// from when this actually performed the actions now performed in RefreshBearerToken.
void HttpPayload::SetBearerToken() { m_requiresBearerToken = true; }

bool HttpPayload::GetRequiresBearerToken() const { return m_requiresBearerToken; }

void HttpPayload::RefreshBearerToken()
{
    if (!m_requiresBearerToken)
    {
        return;
    }

    if (HttpAuth::GetAccessToken().c_str() != nullptr)
    {
        char str[1024];
        snprintf(str, 1024, "Bearer %s", HttpAuth::GetAccessToken().c_str());
        AddHeader(CSP_TEXT("Authorization"), CSP_TEXT(str));
    }
}

void HttpPayload::Reset()
{
    m_content = csp::common::String("");
    m_requiresBearerToken = false;
    m_headers.clear();
}

void HttpPayload::AddFormParam(const char* /*Name*/, const std::shared_ptr<csp::web::HttpPayload>& formFile)
{
    // Set Multipart type and some boundary value
    std::string boundaryText = "multipart/form-data; boundary=" + std::string(formFile->m_boundary.c_str());
    AddHeader(CSP_TEXT("Content-Type"), CSP_TEXT(boundaryText.c_str()));

    SetContent(formFile->GetContent());
}

void HttpPayload::SetBoundary(const csp::common::String& inBoundary) { m_boundary = inBoundary; }

// This checks not only for "application/json" but also covers cases like "application/graphql+json" and "application/problem+json"
bool HttpPayload::IsJsonPayload() const
{
    const auto& contentTypeHeader = m_headers.find("content-type");
    if (contentTypeHeader == m_headers.end())
    {
        return false;
    }
    else
    {
        std::regex regex("^application\\/([a-z]+\\+)?json");
        return std::regex_search(contentTypeHeader->second.c_str(), regex);
    }
}

} // namespace csp::web
