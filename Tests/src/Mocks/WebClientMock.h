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
#pragma once

#include "../../Library/src/Common/Web/HttpPayload.h"
#include "../../Library/src/Common/Web/HttpRequest.h"
#include "../../Library/src/Common/Web/HttpResponse.h"
#include "../../Library/src/Common/Web/WebClient.h"
#include "../../Library/src/Services/ApiBase/ApiBase.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include <gmock/gmock.h>

class MockHttpResponseHandler : public csp::web::IHttpResponseHandler
{
public:
    MockHttpResponseHandler() { }

    MOCK_METHOD(void, OnHttpProgress, (csp::web::HttpRequest& request), (override));
    MOCK_METHOD(void, OnHttpResponse, (csp::web::HttpResponse& response), (override));
    MOCK_METHOD(bool, ShouldDelete, (), (const, override));
};

class MockApiResponseHandler : public csp::services::ApiResponseHandlerBase
{
public:
    MockApiResponseHandler() { }

    MOCK_METHOD(void, OnHttpProgress, (csp::web::HttpRequest& request), (override));
    MOCK_METHOD(void, OnHttpResponse, (csp::web::HttpResponse& response), (override));
};

class WebClientMock : public csp::web::WebClient
{
public:
    WebClientMock(const uint32_t inPort, const csp::web::ETransferProtocol tp, csp::common::LogSystem* logSystem, bool autoRefresh)
        : WebClient(inPort, tp, logSystem, autoRefresh)
    {
    }

    MOCK_METHOD(void, SendRequest,
        (csp::web::ERequestVerb verb, const csp::web::Uri& inUri, csp::web::HttpPayload& payload, csp::web::IHttpResponseHandler* responseCallback,
            csp::common::CancellationToken& cancellationToken, bool asyncResponse),
        (override));

    MOCK_METHOD(void, SetAuthContext, (csp::common::IAuthContext & authContext), (override));

    MOCK_METHOD(std::string, MD5Hash, (const void* data, const size_t size), (override));

    MOCK_METHOD(void, SetFileUploadContentFromFile,
        (csp::web::HttpPayload * payload, const char* filePath, const char* version, const csp::common::String& mediaType), (override));

    MOCK_METHOD(void, SetFileUploadContentFromString,
        (csp::web::HttpPayload * payload, const csp::common::String& stringSource, const csp::common::String& fileName, const char* version,
            const csp::common::String& mediaTypeType),
        (override));

    MOCK_METHOD(void, SetFileUploadContentFromBuffer,
        (csp::web::HttpPayload * payload, const char* buffer, size_t bufferLength, const csp::common::String& fileName, const char* version,
            const csp::common::String& mediaType),
        (override));

    MOCK_METHOD(void, Send, (csp::web::HttpRequest & request), (override));
};
