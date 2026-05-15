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
#include "CSP/Common/CancellationToken.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Systems/ExternalServices/ExternalServiceProxySystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/SystemsResult.h"
#include "Common/Convert.h"
#include "SpaceSystemTestHelpers.h"
#include "Services/AggregationService/AggregationServiceApiMock.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"
#include "Json/JsonSerializer.h"

#include "gtest/gtest.h"
#include <future>
#include <gmock/gmock.h>

namespace chs_aggregation = csp::services::generated::aggregationservice;

namespace
{

bool RequestPredicate(const csp::systems::ResultBase& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

CSP_PUBLIC_TEST(CSPEngine, ExternalServicesProxySystemTests, GetAgoraUserTokenTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* externalServiceProxySystem = systemsManager.GetExternalServicesProxySystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    csp::systems::AgoraUserTokenParams params;
    params.AgoraUserId = userId;
    params.ChannelName = space.Id;
    params.ReferenceId = space.Id;
    params.Lifespan = 10000;
    params.ShareAudio = true;
    params.ShareScreen = false;
    params.ShareVideo = false;
    params.ReadOnly = false;

    // Get token
    auto [Result] = AWAIT_PRE(externalServiceProxySystem, GetAgoraUserToken, RequestPredicate, params);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_FALSE(Result.GetValue().IsEmpty()); // We assume a non-empty string means we got an Agora token back
    EXPECT_EQ(Result.GetHttpResultCode(), 200);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

class ExternalServicesMock
    : public PublicTestBaseWithParam<std::tuple<csp::systems::EResultCode, csp::web::EResponseCodes, csp::common::String, bool>>
{
};

TEST_P(ExternalServicesMock, ExternalServicesMockTest)
{
    const auto externalServiceProxyMock = std::make_unique<chs_aggregation::ExternalServiceProxyApiMock>();

    csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(csp::common::LogLevel::Error);

    // Expected results
    const csp::systems::EResultCode expectedResultCode = std::get<0>(GetParam());
    const csp::web::EResponseCodes expectedResponseCode = std::get<1>(GetParam());
    const csp::common::String expectedOperationResultJsonString = std::get<2>(GetParam());
    const bool jsonIsValid = std::get<3>(GetParam());

    // The promise
    std::promise<csp::systems::StringResult> resultPromise;
    std::future<csp::systems::StringResult> resultFuture = resultPromise.get_future();

    // Spoofed parameters
    csp::systems::ExternalServicesOperationParams proxyParams;
    proxyParams.OperationName = "MockOperationName";
    proxyParams.ServiceName = "MockServiceName";
    proxyParams.SetHelp = false;

    EXPECT_CALL(*externalServiceProxyMock, service_proxyPost)
        .WillOnce(
            [expectedResponseCode, expectedOperationResultJsonString, jsonIsValid](
                const chs_aggregation::IExternalServiceProxyApiBase::service_proxyPostParams& /*ServiceParams*/,
                csp::services::ApiResponseHandlerBase* responseHandler, csp::common::CancellationToken& /*CancellationToken*/
            )
            {
                auto response = csp::web::HttpResponse();
                response.SetResponseCode(expectedResponseCode);

                csp::web::HttpPayload payload;

                const csp::common::String requestBody = R"(
                {
	                "operationResult":)"
                    + expectedOperationResultJsonString + R"(
                })";

                payload.AddHeader(CSP_TEXT("Content-Type"), CSP_TEXT("application/json"));
                payload.SetContent(requestBody);

                response.GetMutablePayload() = payload;
                responseHandler->OnHttpResponse(response);
            });

    auto callback = [&resultPromise](const csp::systems::StringResult& result) { resultPromise.set_value(result); };

    auto tokenInfo = std::make_shared<chs_aggregation::ServiceRequest>();
    tokenInfo->SetServiceName(proxyParams.ServiceName);
    tokenInfo->SetOperationName(proxyParams.OperationName);
    tokenInfo->SetHelp(proxyParams.SetHelp);
    tokenInfo->SetParameters(Convert(proxyParams.Parameters));

    auto responseHandler = externalServiceProxyMock->CreateHandler<csp::systems::StringResultCallback, csp::systems::ExternalServiceInvocationResult,
        void, chs_aggregation::ServiceResponse>(callback, nullptr);
    externalServiceProxyMock->service_proxyPost({ tokenInfo }, responseHandler, csp::common::CancellationToken::Dummy());

    auto result = resultFuture.get();
    EXPECT_EQ(result.GetResultCode(), expectedResultCode);
    EXPECT_EQ(result.GetHttpResultCode(), static_cast<uint16_t>(expectedResponseCode));
    EXPECT_EQ(result.GetFailureReason(), csp::systems::ERequestFailureReason::None);

    if (jsonIsValid)
    {
        EXPECT_EQ(result.GetValue(), expectedOperationResultJsonString);
    }
    else
    {
        // If Json is malformed, we expect the string to be empty
        EXPECT_TRUE(result.GetValue().IsEmpty());
    }
}

INSTANTIATE_TEST_SUITE_P(ExternalServicesProxySystemTests, ExternalServicesMock,
    testing::Values(
        std::make_tuple(csp::systems::EResultCode::Success, csp::web::EResponseCodes::ResponseOK, R"({"validJsonProperty":"validJsonValue"})", true),
        std::make_tuple(csp::systems::EResultCode::Success, csp::web::EResponseCodes::ResponseOK, "ThisIsNotValidJson", false),
        std::make_tuple(csp::systems::EResultCode::Failed, csp::web::EResponseCodes::ResponseBadRequest, "", true)));