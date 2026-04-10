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

#include "gtest/gtest-param-test.h"
#include "gtest/gtest.h"
#include <gmock/gmock.h>

using namespace csp::web;

namespace CSPEngine
{

class GetFile : public PublicTestBaseInternalWithParam<std::tuple<csp::common::ELoginState, csp::common::String>>
{
};

class GetResponseHeaders : public PublicTestBaseInternalWithParam<std::tuple<csp::common::ELoginState, csp::common::String>>
{
};

TEST_P(GetFile, GetFileSendsCorrectRequest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto MockClient = WebClientMock(80, ETransferProtocol::HTTP, nullptr, true);
    auto MockContext = MockAuthContext();
    MockApiResponseHandler MockHandler;

    const csp::common::String FileUrl = "https://mock.service/assets/test-file.glb";

    // Construct a LoginState object with the correct state.
    csp::common::LoginState LoginState;
    LoginState.State = std::get<0>(GetParam());
    LoginState.AccessToken = std::get<1>(GetParam());

    EXPECT_CALL(MockClient, SendRequest)
        .WillOnce(
            [&FileUrl, &LoginState](ERequestVerb Verb, const Uri& InUri, HttpPayload& Payload, IHttpResponseHandler* /*ResponseCallback*/,
                csp::common::CancellationToken& /*CancellationToken*/, bool /*AsyncResponse*/)
            {
                EXPECT_EQ(Verb, ERequestVerb::GET);

                EXPECT_STREQ(InUri.GetAsString(), FileUrl.c_str());

                // Verify Content-Type header is set
                const auto& Headers = Payload.GetHeaders();
                auto ContentTypeIt = Headers.find("Content-Type");
                ASSERT_NE(ContentTypeIt, Headers.end());
                EXPECT_EQ(ContentTypeIt->second, "text/json");

                if (LoginState.State == csp::common::ELoginState::LoggedIn)
                {
                    // Verify that the Authorization header is present
                    auto AuthIt = Headers.find("Authorization");
                    ASSERT_NE(AuthIt, Headers.end()) << "Authorization header should be present when the user is logged in";

                    std::string BearerToken = AuthIt->second;

                    // Verify the token is not empty after "Bearer "
                    EXPECT_TRUE(BearerToken.length() > 7) << "Bearer token should not be empty";

                    const std::string Prefix("Bearer ");

                    EXPECT_TRUE(BearerToken.compare(0, Prefix.length(), Prefix) == 0)
                        << "Authorization header should start with 'Bearer ', but was: " << BearerToken;
                }
            });

    EXPECT_CALL(MockContext, GetLoginState).WillRepeatedly(::testing::ReturnRef(LoginState));

    RemoteFileManager FileManager(&MockClient, MockContext);

    FileManager.GetFile(FileUrl, &MockHandler, csp::common::CancellationToken::Dummy());

    csp::CSPFoundation::Shutdown();
}

TEST_P(GetResponseHeaders, GetResponseHeadersSendsCorrectRequest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto MockClient = WebClientMock(80, ETransferProtocol::HTTP, nullptr, true);
    auto MockContext = MockAuthContext();
    MockApiResponseHandler MockHandler;

    const csp::common::String FileUrl = "https://mock.service/assets/test-file.glb";

    // Construct a LoginState object with the correct state.
    csp::common::LoginState LoginState;
    LoginState.State = std::get<0>(GetParam());
    LoginState.AccessToken = std::get<1>(GetParam());

    EXPECT_CALL(MockClient, SendRequest)
        .WillOnce(
            [&FileUrl, &LoginState](ERequestVerb Verb, const Uri& InUri, HttpPayload& Payload, IHttpResponseHandler* /*ResponseCallback*/,
                csp::common::CancellationToken& /*CancellationToken*/, bool /*AsyncResponse*/)
            {
                EXPECT_EQ(Verb, ERequestVerb::HEAD);

                EXPECT_STREQ(InUri.GetAsString(), FileUrl.c_str());

                if (LoginState.State == csp::common::ELoginState::LoggedIn)
                {
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
                }
                else
                {
                    // The X-AssetPlatform header is added to all HTTP requests. There should be no additional Authorization header set
                    const auto& Headers = Payload.GetHeaders();
                    EXPECT_EQ(Headers.size(), 1) << "Expected exactly one header to be present in the request";
                }
            });

    EXPECT_CALL(MockContext, GetLoginState).WillRepeatedly(::testing::ReturnRef(LoginState));

    RemoteFileManager FileManager(&MockClient, MockContext);

    FileManager.GetResponseHeaders(FileUrl, &MockHandler);

    csp::CSPFoundation::Shutdown();
}

INSTANTIATE_TEST_SUITE_P(RemoteFileManagerTests, GetFile,
    testing::Values(
        std::make_tuple(csp::common::ELoginState::LoggedOut, ""), std::make_tuple(csp::common::ELoginState::LoggedIn, "MockAccessToken")));

INSTANTIATE_TEST_SUITE_P(RemoteFileManagerTests, GetResponseHeaders,
    testing::Values(
        std::make_tuple(csp::common::ELoginState::LoggedOut, ""), std::make_tuple(csp::common::ELoginState::LoggedIn, "MockAccessToken")));
        
}