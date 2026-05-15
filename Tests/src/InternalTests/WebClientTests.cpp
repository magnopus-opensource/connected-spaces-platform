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

    csp::common::LogSystem* logSystem = csp::systems::SystemsManager::Get().GetLogSystem();

    WebClientMock* mockClient = new WebClientMock(80, ETransferProtocol::HTTP, logSystem, true);
    MockHttpResponseHandler mockHandler;

    HttpResponse response;

    std::promise<bool> onHttpResponsePromise;
    std::future<bool> onHttpResponseFuture = onHttpResponsePromise.get_future();

    EXPECT_CALL(*mockClient, SendRequest)
        .WillOnce(
            [](ERequestVerb /*Verb*/, const Uri& /*InUri*/, HttpPayload& /*Payload*/, IHttpResponseHandler* responseCallback,
                const csp::common::CancellationToken& /*CancellationToken*/, bool /*AsyncResponse*/)
            {
                HttpResponse mockResponse;
                mockResponse.SetResponseCode(EResponseCodes::ResponseOK);

                // Mock payload data
                csp::common::String jsonString = "{\"payloadData\":[{\"id\":123,\"email\":\"mock.user@magnopus.com\"}]}";
                HttpPayload mockPayload;
                mockPayload.SetContent(jsonString);
                ((HttpResponse&)mockResponse).GetMutablePayload() = mockPayload;

                responseCallback->OnHttpResponse(mockResponse);
            });

    EXPECT_CALL(mockHandler, OnHttpResponse)
        .WillOnce(
            [&](csp::web::HttpResponse& inResponse)
            {
                response = inResponse;

                onHttpResponsePromise.set_value(true);
            });

    csp::web::HttpPayload payload;
    payload.AddHeader(CSP_TEXT("x-api-key"), CSP_TEXT("MockApiKey"));

    mockClient->SendRequest(
        csp::web::ERequestVerb::Get, Uri("https://mock.service/api/users"), payload, &mockHandler, csp::common::CancellationToken::Dummy(), true);

    onHttpResponseFuture.wait();
    EXPECT_TRUE(onHttpResponseFuture.get() == true);

    std::string responseContent = response.GetPayload().GetContent().c_str();
    EXPECT_TRUE(responseContent.find("payloadData") != std::string::npos) << "PayloadData was not found.";

    delete mockClient;

    csp::CSPFoundation::Shutdown();
}

CSP_INTERNAL_TEST(CSPEngine, WebClientTests, MockWebClientRequestResponseVeryVerboseLoggingTest)
{
    InitialiseFoundation();

    {
        RAIIMockLogger mockLogger {};

        csp::common::LogSystem* logSystem = csp::systems::SystemsManager::Get().GetLogSystem();
        logSystem->SetSystemLevel(csp::common::LogLevel::VeryVerbose);

        WebClientMock* mockClient = new WebClientMock(80, ETransferProtocol::HTTP, logSystem, true);

        // Request/Response logs we expect to receive for our HTTP calls.
        // We are only checking against a substring of the request/response logs.
        // Get logs
        csp::common::String cspLogMsgGetRequestSubstring = "HTTP Request\nGET https://mock.service/api/users";
        csp::common::String cspLogMsgGetResponseSubstring = "HTTP Response\nGET https://mock.service/api/users";
        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::VeryVerbose, testing::HasSubstr(cspLogMsgGetRequestSubstring)));
        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::VeryVerbose, testing::HasSubstr(cspLogMsgGetResponseSubstring)));
        // Put logs
        csp::common::String cspLogMsgPutRequestSubstring = "HTTP Request\nPUT https://mock.service/api/users";
        csp::common::String cspLogMsgPutResponseSubstring = "HTTP Response\nPUT https://mock.service/api/users";
        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::VeryVerbose, testing::HasSubstr(cspLogMsgPutRequestSubstring)));
        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::VeryVerbose, testing::HasSubstr(cspLogMsgPutResponseSubstring)));
        // Post logs
        csp::common::String cspLogMsgPostRequestSubstring = "HTTP Request\nPOST https://mock.service/api/login";
        csp::common::String cspLogMsgPostResponseSubstring = "HTTP Response\nPOST https://mock.service/api/login";
        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::VeryVerbose, testing::HasSubstr(cspLogMsgPostRequestSubstring)));
        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::VeryVerbose, testing::HasSubstr(cspLogMsgPostResponseSubstring)));
        // Delete logs
        csp::common::String cspLogMsgDeleteRequestSubstring = "HTTP Request\nDELETE https://mock.service/api/users";
        csp::common::String cspLogMsgDeleteResponseSubstring = "HTTP Response\nDELETE https://mock.service/api/users";
        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::VeryVerbose, testing::HasSubstr(cspLogMsgDeleteRequestSubstring)));
        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::VeryVerbose, testing::HasSubstr(cspLogMsgDeleteResponseSubstring)));

        EXPECT_CALL(*mockClient, SendRequest)
            .WillRepeatedly(testing::DoAll(
                // Capture: Verb (0), Uri (1), Payload (2), ResponseCallback (3), CancellationToken (4), AsyncResponse (5)
                testing::WithArgs<0, 1, 2, 3, 4, 5>(
                    [&mockClient, &logSystem](ERequestVerb verb, const Uri& inUri, HttpPayload& payload, IHttpResponseHandler* responseCallback,
                        csp::common::CancellationToken& cancellationToken, bool asyncResponse)
                    {
                        // Mimic the logging behaviour of the WebClient SendRequest method
                        auto request = std::make_unique<csp::web::HttpRequest>(
                            mockClient, verb, inUri, payload, responseCallback, cancellationToken, asyncResponse);

                        logSystem->LogMsg(csp::common::LogLevel::VeryVerbose, fmt::format("{}", *(request.get())).c_str());
                    }),
                // Capture: Verb (0), Uri (1), ResponseCallback (3)
                testing::WithArgs<0, 1, 3>(
                    [&](ERequestVerb verb, const Uri& inUri, IHttpResponseHandler* handler)
                    {
                        EResponseCodes mockedResponseCode
                            = verb == ERequestVerb::Delete ? EResponseCodes::ResponseNoContent : EResponseCodes::ResponseOK;

                        HttpResponse mockResponse;
                        mockResponse.SetResponseCode(mockedResponseCode);

                        // Log the response
                        logSystem->LogMsg(csp::common::LogLevel::VeryVerbose,
                            fmt::format("HTTP Response\n{0} {1}\nStatus: {2} - {3}", ERequestVerbToString(verb), inUri.GetAsString(),
                                static_cast<int>(mockResponse.GetResponseCode()), "Success")
                                .c_str());

                        handler->OnHttpResponse(mockResponse);
                    }),
                testing::Return()));

        // GET request
        MockHttpResponseHandler mockHandlerGet;
        HttpPayload payloadGet;

        payloadGet.AddHeader(CSP_TEXT("x-api-key"), CSP_TEXT("MockApiKey"));

        mockClient->SendRequest(csp::web::ERequestVerb::Get, Uri("https://mock.service/api/users"), payloadGet, &mockHandlerGet,
            csp::common::CancellationToken::Dummy(), true);

        // PUT request
        MockHttpResponseHandler mockHandlerPut;
        HttpPayload payloadPut;

        rapidjson::Document jsonDocPut(rapidjson::kObjectType);
        jsonDocPut.AddMember("name", "bob", jsonDocPut.GetAllocator());
        jsonDocPut.AddMember("job", "builder", jsonDocPut.GetAllocator());

        payloadPut.SetContent(jsonDocPut);
        payloadPut.AddHeader(CSP_TEXT("x-api-key"), CSP_TEXT("MockApiKey"));

        mockClient->SendRequest(csp::web::ERequestVerb::Put, Uri("https://mock.service/api/users"), payloadPut, &mockHandlerPut,
            csp::common::CancellationToken::Dummy(), true);

        // POST request
        MockHttpResponseHandler mockHandlerPost;
        HttpPayload payloadPost;

        rapidjson::Document jsonDocPost(rapidjson::kObjectType);
        jsonDocPost.AddMember("email", "mock.user@magnopus.com", jsonDocPost.GetAllocator());
        jsonDocPost.AddMember("password", "secret", jsonDocPost.GetAllocator());

        payloadPost.SetContent(jsonDocPost);

        payloadPost.AddHeader(CSP_TEXT("Content-Type"), CSP_TEXT("application/json"));
        payloadPost.AddHeader(CSP_TEXT("x-api-key"), CSP_TEXT("MockApiKey"));

        mockClient->SendRequest(csp::web::ERequestVerb::Post, Uri("https://mock.service/api/login"), payloadPost, &mockHandlerPost,
            csp::common::CancellationToken::Dummy(), true);

        // Delete request
        MockHttpResponseHandler mockHandlerDelete;
        HttpPayload payloadDelete;
        payloadDelete.AddHeader(CSP_TEXT("x-api-key"), CSP_TEXT("MockApiKey"));

        mockClient->SendRequest(csp::web::ERequestVerb::Delete, Uri("https://mock.service/api/users"), payloadDelete, &mockHandlerDelete,
            csp::common::CancellationToken::Dummy(), true);

        delete mockClient;
    }

    csp::CSPFoundation::Shutdown();
}
