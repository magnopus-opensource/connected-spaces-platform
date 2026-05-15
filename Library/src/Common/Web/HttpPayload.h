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

#include "CSP/Common/String.h"
#include "Common/Web/Json.h"

#include <map>
#include <rapidjson/document.h>

namespace csp::web
{

/// Headers and Content for a HttpRequest or Response
class HttpPayload
{
public:
    using HeadersMap = std::map<std::string, std::string>;

    HttpPayload();
    HttpPayload(const char* inContent);
    HttpPayload(const csp::common::String& inContent);
    ~HttpPayload();

    void SetContent(const rapidjson::Document& inJson);
    void SetContent(const csp::common::String& inContent);

    void AddContent(const csp::common::String& inContent);

    const csp::common::String& GetContent() const;
    const csp::common::String& ToJson() const;

    void SetContent(const char* data, size_t dataLength);

    void AllocateContent(size_t dataLength);
    void WriteContent(size_t offset, const char* data, size_t dataLength);
    size_t ReadContent(size_t offset, void* data, size_t dataLength) const;

    void AddHeader(const csp::common::String& key, const csp::common::String& value);

    void AddFormParam(const char* name, const std::shared_ptr<csp::web::HttpPayload>& formFile);
    void AddFormParam(const char* name, const csp::common::String& param);

    const HeadersMap& GetHeaders() const;

    /// Sets that this requires a CHS bearer token header
    void SetBearerToken();
    // Returns whenther this requires a CHS bearer token header
    bool GetRequiresBearerToken() const;
    /// Ensures that the bearer token header is set, if required, with the latest access token
    void RefreshBearerToken();

    /// Reset the Payload, clearing all content and headers
    void Reset();

    void SetBoundary(const csp::common::String& inBoundary);

    bool IsJsonPayload() const;

private:
    HeadersMap m_headers;
    csp::common::String m_content;
    csp::common::String m_boundary;

    bool m_requiresBearerToken = false;
};

} // namespace csp::web
