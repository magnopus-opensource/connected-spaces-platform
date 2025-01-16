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
#include "Common/StlString.h"
#include "Memory/StlAllocator.h"
#include "Web/Json.h"

#include <map>
#include <rapidjson/document.h>

namespace csp::web
{

/// Headers and Content for a HttpRequest or Response
class HttpPayload
{
public:
    using HeadersMap = std::map<csp::StlString, csp::StlString, std::less<csp::StlString>,
        csp::memory::StlAllocator<std::pair<const csp::StlString, csp::StlString>>>;

    HttpPayload();
    HttpPayload(const char* InContent);
    HttpPayload(const csp::common::String& InContent);
    ~HttpPayload();

    void SetContent(const rapidjson::Document& InJson);
    void SetContent(const csp::common::String& InContent);

    void AddContent(const csp::common::String& InContent);

    const csp::common::String& GetContent() const;
    const csp::common::String& ToJson() const;

    void SetContent(const char* Data, size_t DataLength);

    void AllocateContent(size_t DataLength);
    void WriteContent(size_t Offset, const char* Data, size_t DataLength);
    size_t ReadContent(size_t Offset, void* Data, size_t DataLength) const;

    void AddHeader(const csp::common::String& Key, const csp::common::String& Value);

    void AddFormParam(const char* Name, const std::shared_ptr<csp::web::HttpPayload>& formFile);
    void AddFormParam(const char* Name, const csp::common::String& param);

    const HeadersMap& GetHeaders() const;

    /// Sets that this requires a CHS bearer token header
    void SetBearerToken();
    // Returns whenther this requires a CHS bearer token header
    bool GetRequiresBearerToken() const;
    /// Ensures that the bearer token header is set, if required, with the latest access token
    void RefreshBearerToken();

    /// Reset the Payload, clearing all content and headers
    void Reset();

    void SetBoundary(const csp::common::String& InBoundary);

    bool IsJsonPayload() const;

private:
    HeadersMap Headers;
    csp::common::String Content;
    csp::common::String Boundary;

    bool RequiresBearerToken = false;
};

} // namespace csp::web
