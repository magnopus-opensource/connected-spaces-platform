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

#include "CSP/CSPFoundation.h"
#include "CSP/Common/fmt_Formatters.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "Debug/Logging.h"
#include "PlatformTestUtils.h"
#include "RAIIMockLogger.h"
#include "TestHelpers.h"

#include "gtest/gtest.h"
#include <chrono>
#include <future>
#include <gmock/gmock.h>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>

#include "Mocks/WebClientMock.h"

using namespace csp::web;

inline const char* TESTS_PAYLOAD_RESPONSE_CONTENT = "payloadData";

csp::common::String ERequestVerbToString(ERequestVerb verb)
{
    switch (verb)
    {
    case ERequestVerb::Get:
        return "GET";
    case ERequestVerb::Put:
        return "PUT";
    case ERequestVerb::Post:
        return "POST";
    case ERequestVerb::Delete:
        return "DELETE";
    case ERequestVerb::Head:
        return "HEAD";
    default:
        return "Unknown";
    }
}

CSP_INTERNAL_TEST(CSPEngine, WebClientTests, MockWebClientSendRequestTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    csp::common::LogSystem* LogSystem = csp::systems::SystemsManager::Get().GetLogSystem();

    WebClientMock* MockClient = new WebClientMock(80, ETransferProtocol::HTTP, LogSystem, true);
    MockHttpResponseHandler MockHandler;

    HttpResponse Response;

    std::promise<bool> OnHttpResponsePromise;
    std::future<bool> OnHttpResponseFuture = OnHttpResponsePromise.get_future();

    EXPECT_CALL(*MockClient, SendRequest)
        .WillOnce(
            [](ERequestVerb /*Verb*/, const Uri& /*InUri*/, HttpPayload& /*Payload*/, IHttpResponseHandler* ResponseCallback,
                const csp::common::CancellationToken& /*CancellationToken*/, bool /*AsyncResponse*/)
            {
                HttpResponse MockResponse;
                MockResponse.SetResponseCode(EResponseCodes::ResponseOK);

                // Mock payload data
                csp::common::String JsonString = "{\"payloadData\":[{\"id\":123,\"email\":\"mock.user@magnopus.com\"}]}";
                HttpPayload MockPayload;
                MockPayload.SetContent(JsonString);
                ((HttpResponse&)MockResponse).GetMutablePayload() = MockPayload;

                ResponseCallback->OnHttpResponse(MockResponse);
            });

    EXPECT_CALL(MockHandler, OnHttpResponse)
        .WillOnce(
            [&](csp::web::HttpResponse& InResponse)
            {
                Response = InResponse;

                OnHttpResponsePromise.set_value(true);
            });

    csp::web::HttpPayload Payload;
    Payload.AddHeader(CSP_TEXT("x-api-key"), CSP_TEXT("MockApiKey"));

    MockClient->SendRequest(
        csp::web::ERequestVerb::Get, Uri("https://mock.service/api/users"), Payload, &MockHandler, csp::common::CancellationToken::Dummy(), true);

    OnHttpResponseFuture.wait();
    EXPECT_TRUE(OnHttpResponseFuture.get() == true);

    std::string ResponseContent = Response.GetPayload().GetContent().c_str();
    EXPECT_TRUE(ResponseContent.find("payloadData") != std::string::npos) << "PayloadData was not found.";

    delete MockClient;

    csp::CSPFoundation::Shutdown();
}

CSP_INTERNAL_TEST(CSPEngine, WebClientTests, MockWebClientRequestResponseVeryVerboseLoggingTest)
{
    InitialiseFoundation();

    {
        RAIIMockLogger MockLogger {};

        csp::common::LogSystem* LogSystem = csp::systems::SystemsManager::Get().GetLogSystem();
        LogSystem->SetSystemLevel(csp::common::LogLevel::VeryVerbose);

        WebClientMock* MockClient = new WebClientMock(80, ETransferProtocol::HTTP, LogSystem, true);

        // Request/Response logs we expect to receive for our HTTP calls.
        // We are only checking against a substring of the request/response logs.
        // Get logs
        csp::common::String CSPLogMsgGetRequestSubstring = "HTTP Request\nGET https://mock.service/api/users";
        csp::common::String CSPLogMsgGetResponseSubstring = "HTTP Response\nGET https://mock.service/api/users";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::VeryVerbose, testing::HasSubstr(CSPLogMsgGetRequestSubstring)));
        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::VeryVerbose, testing::HasSubstr(CSPLogMsgGetResponseSubstring)));
        // Put logs
        csp::common::String CSPLogMsgPutRequestSubstring = "HTTP Request\nPUT https://mock.service/api/users";
        csp::common::String CSPLogMsgPutResponseSubstring = "HTTP Response\nPUT https://mock.service/api/users";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::VeryVerbose, testing::HasSubstr(CSPLogMsgPutRequestSubstring)));
        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::VeryVerbose, testing::HasSubstr(CSPLogMsgPutResponseSubstring)));
        // Post logs
        csp::common::String CSPLogMsgPostRequestSubstring = "HTTP Request\nPOST https://mock.service/api/login";
        csp::common::String CSPLogMsgPostResponseSubstring = "HTTP Response\nPOST https://mock.service/api/login";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::VeryVerbose, testing::HasSubstr(CSPLogMsgPostRequestSubstring)));
        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::VeryVerbose, testing::HasSubstr(CSPLogMsgPostResponseSubstring)));
        // Delete logs
        csp::common::String CSPLogMsgDeleteRequestSubstring = "HTTP Request\nDELETE https://mock.service/api/users";
        csp::common::String CSPLogMsgDeleteResponseSubstring = "HTTP Response\nDELETE https://mock.service/api/users";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::VeryVerbose, testing::HasSubstr(CSPLogMsgDeleteRequestSubstring)));
        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::VeryVerbose, testing::HasSubstr(CSPLogMsgDeleteResponseSubstring)));

        EXPECT_CALL(*MockClient, SendRequest)
            .WillRepeatedly(testing::DoAll(
                // Capture: Verb (0), Uri (1), Payload (2), ResponseCallback (3), CancellationToken (4), AsyncResponse (5)
                testing::WithArgs<0, 1, 2, 3, 4, 5>(
                    [&MockClient, &LogSystem](ERequestVerb Verb, const Uri& InUri, HttpPayload& Payload, IHttpResponseHandler* ResponseCallback,
                        csp::common::CancellationToken& CancellationToken, bool AsyncResponse)
                    {
                        // Mimic the logging behaviour of the WebClient SendRequest method
                        auto Request = std::make_unique<csp::web::HttpRequest>(
                            MockClient, Verb, InUri, Payload, ResponseCallback, CancellationToken, AsyncResponse);

                        LogSystem->LogMsg(csp::common::LogLevel::VeryVerbose, fmt::format("{}", *(Request.get())).c_str());
                    }),
                // Capture: Verb (0), Uri (1), ResponseCallback (3)
                testing::WithArgs<0, 1, 3>(
                    [&](ERequestVerb Verb, const Uri& InUri, IHttpResponseHandler* Handler)
                    {
                        EResponseCodes MockedResponseCode
                            = Verb == ERequestVerb::Delete ? EResponseCodes::ResponseNoContent : EResponseCodes::ResponseOK;

                        HttpResponse MockResponse;
                        MockResponse.SetResponseCode(MockedResponseCode);

                        // Log the response
                        LogSystem->LogMsg(csp::common::LogLevel::VeryVerbose,
                            fmt::format("HTTP Response\n{0} {1}\nStatus: {2} - {3}", ERequestVerbToString(Verb), InUri.GetAsString(),
                                static_cast<int>(MockResponse.GetResponseCode()), "Success")
                                .c_str());

                        Handler->OnHttpResponse(MockResponse);
                    }),
                testing::Return()));

        // GET request
        MockHttpResponseHandler MockHandlerGet;
        HttpPayload PayloadGet;

        PayloadGet.AddHeader(CSP_TEXT("x-api-key"), CSP_TEXT("MockApiKey"));

        MockClient->SendRequest(csp::web::ERequestVerb::Get, Uri("https://mock.service/api/users"), PayloadGet, &MockHandlerGet,
            csp::common::CancellationToken::Dummy(), true);

        // PUT request
        MockHttpResponseHandler MockHandlerPut;
        HttpPayload PayloadPut;

        rapidjson::Document JsonDocPut(rapidjson::kObjectType);
        JsonDocPut.AddMember("name", "bob", JsonDocPut.GetAllocator());
        JsonDocPut.AddMember("job", "builder", JsonDocPut.GetAllocator());

        PayloadPut.SetContent(JsonDocPut);
        PayloadPut.AddHeader(CSP_TEXT("x-api-key"), CSP_TEXT("MockApiKey"));

        MockClient->SendRequest(csp::web::ERequestVerb::Put, Uri("https://mock.service/api/users"), PayloadPut, &MockHandlerPut,
            csp::common::CancellationToken::Dummy(), true);

        // POST request
        MockHttpResponseHandler MockHandlerPost;
        HttpPayload PayloadPost;

        rapidjson::Document JsonDocPost(rapidjson::kObjectType);
        JsonDocPost.AddMember("email", "mock.user@magnopus.com", JsonDocPost.GetAllocator());
        JsonDocPost.AddMember("password", "secret", JsonDocPost.GetAllocator());

        PayloadPost.SetContent(JsonDocPost);

        PayloadPost.AddHeader(CSP_TEXT("Content-Type"), CSP_TEXT("application/json"));
        PayloadPost.AddHeader(CSP_TEXT("x-api-key"), CSP_TEXT("MockApiKey"));

        MockClient->SendRequest(csp::web::ERequestVerb::Post, Uri("https://mock.service/api/login"), PayloadPost, &MockHandlerPost,
            csp::common::CancellationToken::Dummy(), true);

        // Delete request
        MockHttpResponseHandler MockHandlerDelete;
        HttpPayload PayloadDelete;
        PayloadDelete.AddHeader(CSP_TEXT("x-api-key"), CSP_TEXT("MockApiKey"));

        MockClient->SendRequest(csp::web::ERequestVerb::Delete, Uri("https://mock.service/api/users"), PayloadDelete, &MockHandlerDelete,
            csp::common::CancellationToken::Dummy(), true);

        delete MockClient;
    }

    csp::CSPFoundation::Shutdown();
}
