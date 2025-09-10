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
#include "Awaitable.h"
#include "CSP/CSPFoundation.h"
#include "CSP/Common/SharedEnums.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Systems/Analytics/AnalyticsSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "Common/Web/HttpPayload.h"
#include "RAIIMockLogger.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"

#include "gtest/gtest.h"

using namespace csp::common;
using namespace csp::systems;

namespace
{

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

CSP_PUBLIC_TEST(CSPEngine, AnalyticsSystemTests, SendAnalyticsEventTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* AnalyticsSystem = SystemsManager.GetAnalyticsSystem();

    String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Analytics Data
    const auto TestProductContextSection = String("Event_ProductContextSection");
    const auto TestCategory = String("Event_Category");
    const auto TestInteractionType = String("Event_InteractionType");
    const auto TestSubCategory = String("Event_SubCategory");
    const Map<String, String> TestMetadata = { { "Key1", "Value1" }, { "Key2", "Value2" } };

    auto [Result] = AWAIT_PRE(AnalyticsSystem, SendAnalyticsEvent, RequestPredicate, TestProductContextSection, TestCategory, TestInteractionType,
        TestSubCategory, TestMetadata);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(Result.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseCreated));

    // Log out
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, AnalyticsSystemTests, SendAnalyticsEventMissingFieldsTest)
{
    SetRandSeed();

    RAIIMockLogger MockLogger {};

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* AnalyticsSystem = SystemsManager.GetAnalyticsSystem();
    auto& LogSystem = *SystemsManager.GetLogSystem();

    // We are only concerned with error logs in this test
    LogSystem.SetSystemLevel(csp::common::LogLevel::Error);

    String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Analytics Data
    // Passing an empty string for a required field
    const auto TestProductContextSection = String("");
    const auto TestCategory = String("Event_Category");
    const auto TestInteractionType = String("Event_InteractionType");
    const auto TestSubCategory = String("Event_SubCategory");
    const Map<String, String> TestMetadata = { { "Key1", "Value1" }, { "Key2", "Value2" } };

    // Ensure the required fields error message is logged when we try to send an analytics event with a required field missing
    const csp::common::String LockErrorMsg = "Missing the required fields for the Analytics Event.";
    EXPECT_CALL(MockLogger.MockLogCallback, Call(LockErrorMsg)).Times(1);

    auto [Result] = AWAIT_PRE(AnalyticsSystem, SendAnalyticsEvent, RequestPredicate, TestProductContextSection, TestCategory, TestInteractionType,
        TestSubCategory, TestMetadata);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
    EXPECT_EQ(Result.GetHttpResultCode(), 0);

    // Log out
    LogOut(UserSystem);
}
