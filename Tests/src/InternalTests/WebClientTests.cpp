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

#include "Awaitable.h"
#include "CSP/CSPFoundation.h"
#include "CSP/Common/fmt_Formatters.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "Debug/Logging.h"
#include "PlatformTestUtils.h"
#include "RAIIMockLogger.h"
#include "TestHelpers.h"

#include "gtest/gtest.h"
#include <atomic>
#include <chrono>
#include <future>
#include <gmock/gmock.h>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <vector>

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

    WebClientMock* MockClient = new WebClientMock(LogSystem, true);
    MockHttpResponseHandler MockHandler;

    HttpResponse Response;

    std::promise<bool> OnHttpResponsePromise;
    std::future<bool> OnHttpResponseFuture = OnHttpResponsePromise.get_future();

    EXPECT_CALL(*MockClient, SendRequest)
        .WillOnce(
            [](ERequestVerb /*Verb*/, const Uri& /*InUri*/, HttpPayload& /*Payload*/, IHttpResponseHandler* ResponseCallback,
                const csp::common::CancellationToken& /*CancellationToken*/)
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
        csp::web::ERequestVerb::Get, Uri("https://mock.service/api/users"), Payload, &MockHandler, csp::common::CancellationToken::Dummy());

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
        RAIIMockLogger MockLogger { };

        csp::common::LogSystem* LogSystem = csp::systems::SystemsManager::Get().GetLogSystem();
        LogSystem->SetSystemLevel(csp::common::LogLevel::VeryVerbose);

        WebClientMock* MockClient = new WebClientMock(LogSystem, true);

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
                // Capture: Verb (0), Uri (1), Payload (2), ResponseCallback (3), CancellationToken (4)
                testing::WithArgs<0, 1, 2, 3, 4>(
                    [&MockClient, &LogSystem](ERequestVerb Verb, const Uri& InUri, HttpPayload& Payload, IHttpResponseHandler* ResponseCallback,
                        csp::common::CancellationToken& CancellationToken)
                    {
                        // Mimic the logging behaviour of the WebClient SendRequest method
                        auto Request = std::make_unique<csp::web::HttpRequest>(MockClient, Verb, InUri, Payload, ResponseCallback, CancellationToken);

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

        MockClient->SendRequest(
            csp::web::ERequestVerb::Get, Uri("https://mock.service/api/users"), PayloadGet, &MockHandlerGet, csp::common::CancellationToken::Dummy());

        // PUT request
        MockHttpResponseHandler MockHandlerPut;
        HttpPayload PayloadPut;

        rapidjson::Document JsonDocPut(rapidjson::kObjectType);
        JsonDocPut.AddMember("name", "bob", JsonDocPut.GetAllocator());
        JsonDocPut.AddMember("job", "builder", JsonDocPut.GetAllocator());

        PayloadPut.SetContent(JsonDocPut);
        PayloadPut.AddHeader(CSP_TEXT("x-api-key"), CSP_TEXT("MockApiKey"));

        MockClient->SendRequest(
            csp::web::ERequestVerb::Put, Uri("https://mock.service/api/users"), PayloadPut, &MockHandlerPut, csp::common::CancellationToken::Dummy());

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
            csp::common::CancellationToken::Dummy());

        // Delete request
        MockHttpResponseHandler MockHandlerDelete;
        HttpPayload PayloadDelete;
        PayloadDelete.AddHeader(CSP_TEXT("x-api-key"), CSP_TEXT("MockApiKey"));

        MockClient->SendRequest(csp::web::ERequestVerb::Delete, Uri("https://mock.service/api/users"), PayloadDelete, &MockHandlerDelete,
            csp::common::CancellationToken::Dummy());

        delete MockClient;
    }

    csp::CSPFoundation::Shutdown();
}

CSP_PUBLIC_TEST_WITH_MOCKS(CSPEngine, WebClientMockTests, FailingStatusCodesAreRetriedTest)
{
    auto AuthContext = TestAuthContext();
    WebClientMock->WebClient::SetAuthContext(AuthContext);

    EXPECT_CALL(*WebClientMock, SendRequest)
        .WillRepeatedly([Mock = WebClientMock](auto&&... Args) { Mock->WebClient::SendRequest(std::forward<decltype(Args)>(Args)...); });

    std::atomic<size_t> LoginAttempts = 0;

    // Want to make sure there's a delay between the retry attempts
    // We should honesty be more subtle with our retry timing. Just blasting the server 5 times
    // regardless of status code or any try_after data set on the response is rude.
    // The status code could very well be saying "Stop! I'm overloaded!".
    std::vector<std::chrono::steady_clock::time_point> LoginAttemptTimes;

    EXPECT_CALL(*WebClientMock, Send)
        .WillRepeatedly(
            [&LoginAttempts, &LoginAttemptTimes](csp::web::HttpRequest& Request)
            {
                const auto Uri = csp::common::String(Request.GetUri().GetAsString());

                if (Uri.EndsWith("/users/login"))
                {
                    auto& Response = Request.GetMutableResponse();
                    // This is one of the status codes that will trigger a retry
                    Response.SetResponseCode(EResponseCodes::ResponseServiceUnavailable);
                    Response.GetMutablePayload().SetContent("");
                    ++LoginAttempts;
                    LoginAttemptTimes.push_back(std::chrono::steady_clock::now());
                }
            });

    auto* UserSystem = csp::systems::SystemsManager::Get().GetUserSystem();

    // Perform the intercepted login
    auto [LoginResult]
        = Awaitable(&csp::systems::UserSystem::Login, UserSystem, "IrrelevantEmail@woah.com", GeneratedTestAccountPassword, false, true, nullptr)
              .Await();

    EXPECT_GT(LoginAttempts.load(), 0) << "Login request was never intercepted";
    // DefaultNumRequestRetries is in HttpRequest.h, accessible from hre.
    EXPECT_EQ(LoginAttempts.load(), 1 + DefaultNumRequestRetries) << "Expected the initial request plus additional retries";
    EXPECT_EQ(LoginResult.GetResultCode(), csp::systems::EResultCode::Failed);
    EXPECT_EQ(LoginResult.GetHttpResultCode(), static_cast<uint16_t>(EResponseCodes::ResponseServiceUnavailable));

    // Each retry should wait at least the flat retry delay after the previous attempt failed.
    // DefaultRetriesDelayInMs is also in HttpRequest.h.
    for (size_t i = 1; i < LoginAttemptTimes.size(); ++i)
    {
        const auto RetryGap = std::chrono::duration_cast<std::chrono::milliseconds>(LoginAttemptTimes[i] - LoginAttemptTimes[i - 1]);
        EXPECT_GE(RetryGap.count(), static_cast<int64_t>(DefaultRetriesDelayInMs)) << "Retry " << i << " was sent sooner than the retry delay";
    }
}

CSP_PUBLIC_TEST_WITH_MOCKS(CSPEngine, WebClientMockTests, NetworkFailuresAreRetriedTest)
{
    auto AuthContext = TestAuthContext();
    WebClientMock->WebClient::SetAuthContext(AuthContext);

    EXPECT_CALL(*WebClientMock, SendRequest)
        .WillRepeatedly([Mock = WebClientMock](auto&&... Args) { Mock->WebClient::SendRequest(std::forward<decltype(Args)>(Args)...); });

    std::atomic<size_t> LoginAttempts = 0;

    // Want to make sure there's a delay between the retry attempts
    // We should honesty be more subtle with our retry timing. Just blasting the server 5 times
    // regardless of status code or any try_after data set on the response is rude.
    // The status code could very well be saying "Stop! I'm overloaded!".
    std::vector<std::chrono::steady_clock::time_point> LoginAttemptTimes;

    EXPECT_CALL(*WebClientMock, Send)
        .WillRepeatedly(
            [&LoginAttempts, &LoginAttemptTimes](csp::web::HttpRequest& Request)
            {
                const auto Uri = csp::common::String(Request.GetUri().GetAsString());

                if (Uri.EndsWith("/users/login"))
                {
                    ++LoginAttempts;
                    LoginAttemptTimes.push_back(std::chrono::steady_clock::now());
                    throw csp::web::WebClientException("Simulated network failure");
                }
            });

    auto* UserSystem = csp::systems::SystemsManager::Get().GetUserSystem();

    // Perform the intercepted login
    auto [LoginResult]
        = Awaitable(&csp::systems::UserSystem::Login, UserSystem, "IrrelevantEmail@woah.com", GeneratedTestAccountPassword, false, true, nullptr)
              .Await();

    EXPECT_GT(LoginAttempts.load(), 0) << "Login request was never intercepted";
    // DefaultNumRequestRetries is in HttpRequest.h, accessible from hre.
    EXPECT_EQ(LoginAttempts.load(), 1 + DefaultNumRequestRetries) << "Expected the initial request plus additional retries";
    EXPECT_EQ(LoginResult.GetResultCode(), csp::systems::EResultCode::Failed);
    EXPECT_EQ(LoginResult.GetHttpResultCode(),
        static_cast<uint16_t>(EResponseCodes::ResponseServiceUnavailable)); // Internally, network failure is converted to 503. I'm not sure about
                                                                            // this, I won't port this behaviour to the web retry mechanism as it they
                                                                            // care about codes much more in that domain.

    // Each retry should wait at least the flat retry delay after the previous attempt failed.
    // DefaultRetriesDelayInMs is also in HttpRequest.h.
    for (size_t i = 1; i < LoginAttemptTimes.size(); ++i)
    {
        const auto RetryGap = std::chrono::duration_cast<std::chrono::milliseconds>(LoginAttemptTimes[i] - LoginAttemptTimes[i - 1]);
        EXPECT_GE(RetryGap.count(), static_cast<int64_t>(DefaultRetriesDelayInMs)) << "Retry " << i << " was sent sooner than the retry delay";
    }
}

CSP_PUBLIC_TEST_WITH_MOCKS(CSPEngine, WebClientMockTests, SuccessfulNetworkResponsesAreNotRetried)
{
    auto AuthContext = TestAuthContext();
    WebClientMock->WebClient::SetAuthContext(AuthContext);

    EXPECT_CALL(*WebClientMock, SendRequest)
        .WillRepeatedly([Mock = WebClientMock](auto&&... Args) { Mock->WebClient::SendRequest(std::forward<decltype(Args)>(Args)...); });

    std::atomic<size_t> LoginAttempts = 0;

    EXPECT_CALL(*WebClientMock, Send)
        .WillRepeatedly(
            [&LoginAttempts](csp::web::HttpRequest& Request)
            {
                const auto Uri = csp::common::String(Request.GetUri().GetAsString());

                if (Uri.EndsWith("/users/login"))
                {
                    auto& Response = Request.GetMutableResponse();
                    Response.SetResponseCode(EResponseCodes::ResponseOK);
                    Response.GetMutablePayload().SetContent(R"({
                          "accessToken": "IrrelevantAccessToken",
                          "accessTokenExpiresAt": "2999-01-01T00:00:00.000+00:00",
                          "refreshToken": "IrrelevantRefreshToken",
                          "refreshTokenExpiresAt": "2999-01-01T00:00:00.000+00:00",
                          "userId": "IrrelevantUserId",
                          "deviceId": "IrrelevantDeviceId"
                      })");
                    ++LoginAttempts;
                }
            });

    auto* UserSystem = csp::systems::SystemsManager::Get().GetUserSystem();

    // Perform the intercepted login
    auto [LoginResult]
        = Awaitable(&csp::systems::UserSystem::Login, UserSystem, "IrrelevantEmail@woah.com", GeneratedTestAccountPassword, false, true, nullptr)
              .Await();

    EXPECT_GT(LoginAttempts.load(), 0) << "Login request was never intercepted";
    EXPECT_EQ(LoginAttempts.load(), 1) << "Expected only the initial request";
    EXPECT_EQ(LoginResult.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(LoginResult.GetHttpResultCode(), static_cast<uint16_t>(EResponseCodes::ResponseOK));
}
