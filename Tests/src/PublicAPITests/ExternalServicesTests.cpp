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
#include "CSP/Systems/ExternalServices/ExternalServiceProxySystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/SystemsResult.h"
#include "Common/Convert.h"
#include "SpaceSystemTestHelpers.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"
#include "Json/JsonSerializer.h"

#include "Services/AggregationService/AggregationServiceApiMock.h"
#include "gtest/gtest.h"
#include <future>
#include <gmock/gmock.h>

namespace chs_aggregation = csp::services::generated::aggregationservice;

namespace
{

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

CSP_PUBLIC_TEST(CSPEngine, ExternalServicesProxySystemTests, GetAgoraUserTokenTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* ExternlServiceProxySystem = SystemsManager.GetExternalServicesProxySystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";
    const char* TestAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    char UniqueAssetCollectionName[256];
    SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    csp::systems::AgoraUserTokenParams Params;
    Params.AgoraUserId = UserId;
    Params.ChannelName = Space.Id;
    Params.ReferenceId = Space.Id;
    Params.Lifespan = 10000;
    Params.ShareAudio = true;
    Params.ShareScreen = false;
    Params.ShareVideo = false;
    Params.ReadOnly = false;

    // Get token
    auto [Result] = AWAIT_PRE(ExternlServiceProxySystem, GetAgoraUserToken, RequestPredicate, Params);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_FALSE(Result.GetValue().IsEmpty()); // We assume a non-empty string means we got an Agora token back
    EXPECT_EQ(Result.GetHttpResultCode(), 200);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}

class ExternalServicesFailureMock : public PublicTestBaseWithParam<std::tuple<csp::systems::EResultCode, csp::web::EResponseCodes>>
{
};

TEST_P(ExternalServicesFailureMock, ExternalServicesFailureMockTest)
{
    const auto ExternalServiceProxyMock = std::make_unique<chs_aggregation::ExternalServiceProxyApiMock>();

    // Expected results
    const csp::systems::EResultCode ExpectedResultCode = std::get<0>(GetParam());
    const csp::web::EResponseCodes ExpectedResponseCode = std::get<1>(GetParam());

    std::promise<csp::systems::StringResult> ResultPromise;
    std::future<csp::systems::StringResult> ResultFuture = ResultPromise.get_future();

    csp::systems::ExternalServicesOperationParams ProxyParams;
    ProxyParams.OperationName = "MockOperationName";
    ProxyParams.ServiceName = "MockServiceName";
    ProxyParams.SetHelp = false;

    EXPECT_CALL(*ExternalServiceProxyMock, service_proxyPost)
        .WillOnce(
            [&ProxyParams, ExpectedResponseCode](const chs_aggregation::IExternalServiceProxyApiBase::service_proxyPostParams& /*ServiceParams*/,
                csp::services::ApiResponseHandlerBase* ResponseHandler, csp::common::CancellationToken& /*CancellationToken*/
            )
            {
                auto Response = csp::web::HttpResponse();
                Response.SetResponseCode(ExpectedResponseCode);
                ResponseHandler->OnHttpResponse(Response);
            });

    auto Callback = [&ResultPromise](const csp::systems::StringResult& Result) { ResultPromise.set_value(Result); };

    auto TokenInfo = std::make_shared<chs_aggregation::ServiceRequest>();
    TokenInfo->SetServiceName(ProxyParams.ServiceName);
    TokenInfo->SetOperationName(ProxyParams.OperationName);
    TokenInfo->SetHelp(ProxyParams.SetHelp);
    TokenInfo->SetParameters(Convert(ProxyParams.Parameters));

    auto ResponseHandler = ExternalServiceProxyMock->CreateHandler<csp::systems::StringResultCallback, csp::systems::StringResult, void,
        chs_aggregation::ServiceResponse>(Callback, nullptr);
    ExternalServiceProxyMock->service_proxyPost({ TokenInfo }, ResponseHandler, csp::common::CancellationToken::Dummy());

    auto Result = ResultFuture.get();
    EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);
    EXPECT_EQ(Result.GetHttpResultCode(), static_cast<uint16_t>(ExpectedResponseCode));
    EXPECT_EQ(Result.GetFailureReason(), csp::systems::ERequestFailureReason::None);
}

INSTANTIATE_TEST_SUITE_P(ExternalServicesProxySystemTests, ExternalServicesFailureMock,
    testing::Values(std::make_tuple(csp::systems::EResultCode::Success, csp::web::EResponseCodes::ResponseOK),
        std::make_tuple(csp::systems::EResultCode::Failed, csp::web::EResponseCodes::ResponseBadRequest)));