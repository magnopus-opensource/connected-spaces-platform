/*
 * Copyright 2026 Magnopus LLC

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
#include "Mocks/AuthContextMock.h"
#include "Mocks/WebClientMock.h"
#include "PlatformTestUtils.h"
#include "TestHelpers.h"
#include "Web/RemoteFileManager.h"

#include "gtest/gtest.h"
#include <gmock/gmock.h>

using namespace csp::web;

/*
 * Tests that GetFile sends a GET request with the correct URI and Content-Type header.
 * Confirm that it does not include an Authorization header when the user is not logged in.
 */
CSP_INTERNAL_TEST(CSPEngine, RemoteFileManagerTests, GetFileSendsCorrectRequestWhenNotLoggedIn)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto MockClient = WebClientMock(80, ETransferProtocol::HTTP, nullptr, true);
    auto MockContext = MockAuthContext();
    MockApiResponseHandler MockHandler;

    const csp::common::String FileUrl = "https://mock.service/assets/test-file.glb";

    EXPECT_CALL(MockClient, SendRequest)
        .WillOnce(
            [&FileUrl](ERequestVerb Verb, const Uri& InUri, HttpPayload& Payload, IHttpResponseHandler* /*ResponseCallback*/,
                csp::common::CancellationToken& /*CancellationToken*/, bool /*AsyncResponse*/)
            {
                EXPECT_EQ(Verb, ERequestVerb::GET);

                EXPECT_STREQ(InUri.GetAsString(), FileUrl.c_str());

                // Verify Content-Type header is set
                const auto& Headers = Payload.GetHeaders();
                auto ContentTypeIt = Headers.find("Content-Type");
                ASSERT_NE(ContentTypeIt, Headers.end());
                EXPECT_EQ(ContentTypeIt->second, "text/json");

                // Confirm that the Authorization header is not present when we are not logged in
                EXPECT_FALSE(Headers.count("Authorization")) << "Expected no Authorization header to be present in the request";
            });

    // The LoginState ctor default initialises the state to csp::common::ELoginState::LoggedOut, which is what we want for this test.
    csp::common::LoginState LoginState;

    EXPECT_CALL(MockContext, GetLoginState).WillOnce(::testing::ReturnRef(LoginState));

    RemoteFileManager FileManager(&MockClient, &MockContext);

    FileManager.GetFile(FileUrl, &MockHandler, csp::common::CancellationToken::Dummy());

    csp::CSPFoundation::Shutdown();
}

/*
 * Tests that GetResponseHeaders sends a GET request with the correct URI.
 * Confirm that it does not include an Authorization header when the user is not logged in.
 */
CSP_INTERNAL_TEST(CSPEngine, RemoteFileManagerTests, GetResponseHeadersSendsCorrectRequestWhenNotLoggedIn)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto MockClient = WebClientMock(80, ETransferProtocol::HTTP, nullptr, true);
    auto MockContext = MockAuthContext();
    MockApiResponseHandler MockHandler;

    const csp::common::String FileUrl = "https://mock.service/assets/test-file.glb";

    EXPECT_CALL(MockClient, SendRequest)
        .WillOnce(
            [&FileUrl](ERequestVerb Verb, const Uri& InUri, HttpPayload& Payload, IHttpResponseHandler* /*ResponseCallback*/,
                csp::common::CancellationToken& /*CancellationToken*/, bool /*AsyncResponse*/)
            {
                EXPECT_EQ(Verb, ERequestVerb::HEAD);

                EXPECT_STREQ(InUri.GetAsString(), FileUrl.c_str());

                // The X-AssetPlatform header is added to all HTTP requests. Verify that no additional headers have been set
                const auto& Headers = Payload.GetHeaders();
                EXPECT_EQ(Headers.size(), 1) << "Expected exactly one header to be present in the request";
            });

    // The LoginState ctor default initialises the state to csp::common::ELoginState::LoggedOut, which is what we want for this test.
    csp::common::LoginState LoginState;

    EXPECT_CALL(MockContext, GetLoginState).WillOnce(::testing::ReturnRef(LoginState));

    RemoteFileManager FileManager(&MockClient, &MockContext);

    FileManager.GetResponseHeaders(FileUrl, &MockHandler);

    csp::CSPFoundation::Shutdown();
}

/*
 * Test that GetFile includes the Authorization header with a Bearer token when the user is logged in.
 */
CSP_INTERNAL_TEST(CSPEngine, RemoteFileManagerTests, GetFileIncludesAuthorizationHeaderWhenLoggedIn)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto MockClient = WebClientMock(80, ETransferProtocol::HTTP, nullptr, true);
    auto MockContext = MockAuthContext();
    MockApiResponseHandler MockHandler;

    const csp::common::String FileUrl = "https://mock.service/assets/test-file.glb";

    EXPECT_CALL(MockClient, SendRequest)
        .WillOnce(
            [&FileUrl](ERequestVerb Verb, const Uri& InUri, HttpPayload& Payload, IHttpResponseHandler* /*ResponseCallback*/,
                csp::common::CancellationToken& /*CancellationToken*/, bool /*AsyncResponse*/)
            {
                EXPECT_EQ(Verb, ERequestVerb::GET);

                EXPECT_STREQ(InUri.GetAsString(), FileUrl.c_str());

                // Verify Content-Type header is set
                const auto& Headers = Payload.GetHeaders();
                auto ContentTypeIt = Headers.find("Content-Type");
                ASSERT_NE(ContentTypeIt, Headers.end());
                EXPECT_EQ(ContentTypeIt->second, "text/json");

                // Verify that the Authorization header is present
                auto AuthIt = Headers.find("Authorization");
                ASSERT_NE(AuthIt, Headers.end()) << "Authorization header should be present when the user is logged in";

                std::string BearerToken = AuthIt->second;

                // Verify the token is not empty after "Bearer "
                EXPECT_TRUE(BearerToken.length() > 7) << "Bearer token should not be empty";

                const std::string Prefix("Bearer ");

                EXPECT_TRUE(BearerToken.compare(0, Prefix.length(), Prefix) == 0)
                    << "Authorization header should start with 'Bearer ', but was: " << BearerToken;
            });

    // Construct a LoginState object with the correct state.
    csp::common::LoginState LoginState;
    LoginState.State = csp::common::ELoginState::LoggedIn;
    LoginState.AccessToken = "MockAccessToken";

    EXPECT_CALL(MockContext, GetLoginState).WillRepeatedly(::testing::ReturnRef(LoginState));

    RemoteFileManager FileManager(&MockClient, &MockContext);

    FileManager.GetFile(FileUrl, &MockHandler, csp::common::CancellationToken::Dummy());

    csp::CSPFoundation::Shutdown();
}

/*
 * Test that GetResponseHeaders includes the Authorization header with a Bearer token when the user is logged in.
 */
CSP_INTERNAL_TEST(CSPEngine, RemoteFileManagerTests, GetResponseHeadersIncludesAuthorizationHeaderWhenLoggedIn)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto MockClient = WebClientMock(80, ETransferProtocol::HTTP, nullptr, true);
    auto MockContext = MockAuthContext();
    MockApiResponseHandler MockHandler;

    const csp::common::String FileUrl = "https://mock.service/assets/test-file.glb";

    EXPECT_CALL(MockClient, SendRequest)
        .WillOnce(
            [&FileUrl](ERequestVerb Verb, const Uri& InUri, HttpPayload& Payload, IHttpResponseHandler* /*ResponseCallback*/,
                csp::common::CancellationToken& /*CancellationToken*/, bool /*AsyncResponse*/)
            {
                EXPECT_EQ(Verb, ERequestVerb::HEAD);

                EXPECT_STREQ(InUri.GetAsString(), FileUrl.c_str());

                // The X-AssetPlatform header is added to all HTTP requests. In addition the Authorization header should have been set
                const auto& Headers = Payload.GetHeaders();
                EXPECT_EQ(Headers.size(), 2) << "Expected exactly two headers to be present in the request";

                // Verify that the Authorization header is present
                auto AuthIt = Headers.find("Authorization");
                ASSERT_NE(AuthIt, Headers.end()) << "Authorization header should be present when the user is logged in";

                std::string BearerToken = AuthIt->second;

                // Verify the token is not empty after "Bearer "
                EXPECT_TRUE(BearerToken.length() > 7) << "Bearer token should not be empty";

                const std::string Prefix("Bearer ");

                EXPECT_TRUE(BearerToken.compare(0, Prefix.length(), Prefix) == 0)
                    << "Authorization header should start with 'Bearer ', but was: " << BearerToken;
            });

    // Construct a LoginState object with the correct state.
    csp::common::LoginState LoginState;
    LoginState.State = csp::common::ELoginState::LoggedIn;
    LoginState.AccessToken = "MockAccessToken";

    EXPECT_CALL(MockContext, GetLoginState).WillRepeatedly(::testing::ReturnRef(LoginState));

    RemoteFileManager FileManager(&MockClient, &MockContext);

    FileManager.GetResponseHeaders(FileUrl, &MockHandler);

    csp::CSPFoundation::Shutdown();
}
