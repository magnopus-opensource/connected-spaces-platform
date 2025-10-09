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
#include "CSP/Common/Systems/Log/LogSystem.h"
#include <gmock/gmock.h>

class MockHttpResponseHandler : public csp::web::IHttpResponseHandler
{
public:
    MockHttpResponseHandler() { }

    MOCK_METHOD(void, OnHttpProgress, (csp::web::HttpRequest & Request), (override));
    MOCK_METHOD(void, OnHttpResponse, (csp::web::HttpResponse & Response), (override));
    MOCK_METHOD(bool, ShouldDelete, (), (const, override));
};

class WebClientMock : public csp::web::WebClient
{
public:
    WebClientMock(const uint32_t InPort, const csp::web::ETransferProtocol Tp, csp::common::LogSystem* LogSystem, bool AutoRefresh)
        : WebClient(InPort, Tp, LogSystem, AutoRefresh)
    {
    }

    MOCK_METHOD(void, SendRequest,
        (csp::web::ERequestVerb Verb, const csp::web::Uri& InUri, csp::web::HttpPayload& Payload, csp::web::IHttpResponseHandler* ResponseCallback,
            csp::common::CancellationToken& CancellationToken, bool AsyncResponse),
        (override));

    MOCK_METHOD(void, SetAuthContext, (csp::common::IAuthContext & AuthContext), (override));

    MOCK_METHOD(std::string, MD5Hash, (const void* Data, const size_t Size), (override));

    MOCK_METHOD(void, SetFileUploadContentFromFile,
        (csp::web::HttpPayload * Payload, const char* FilePath, const char* Version, const csp::common::String& MediaType), (override));

    MOCK_METHOD(void, SetFileUploadContentFromString,
        (csp::web::HttpPayload * Payload, const csp::common::String& StringSource, const csp::common::String& FileName, const char* Version,
            const csp::common::String& MediaTypeType),
        (override));

    MOCK_METHOD(void, SetFileUploadContentFromBuffer,
        (csp::web::HttpPayload * Payload, const char* Buffer, size_t BufferLength, const csp::common::String& FileName, const char* Version,
            const csp::common::String& MediaType),
        (override));

protected:
    MOCK_METHOD(void, Send, (csp::web::HttpRequest & Request), (override));
};
