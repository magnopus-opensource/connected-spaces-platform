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

/*
 * Test that we can successfully send an analytics event
 */
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

/*
 * Test that we get an error when sending an analytics event with a required field missing
 */
CSP_PUBLIC_TEST(CSPEngine, AnalyticsSystemTests, SendAnalyticsEventMissingFieldsTest)
{
    SetRandSeed();

    RAIIMockLogger MockLogger {};

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* AnalyticsSystem = SystemsManager.GetAnalyticsSystem();

    String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Ensure the MockLogger will ignore all logs except the one we care about
    EXPECT_CALL(MockLogger.MockLogCallback, Call(::testing::_)).Times(::testing::AnyNumber());

    // Ensure the required fields error message is logged when we try to send an analytics event with a required field missing
    const csp::common::String AnalyticsErrorMsg = "Missing the required fields for the Analytics Event.";
    EXPECT_CALL(MockLogger.MockLogCallback, Call(AnalyticsErrorMsg)).Times(1);

    // Analytics Data
    // Passing an empty string for a required field
    const auto TestProductContextSection = String("");
    const auto TestCategory = String("Event_Category");
    const auto TestInteractionType = String("Event_InteractionType");
    const auto TestSubCategory = String("Event_SubCategory");
    const Map<String, String> TestMetadata = { { "Key1", "Value1" }, { "Key2", "Value2" } };

    auto [Result] = AWAIT_PRE(AnalyticsSystem, SendAnalyticsEvent, RequestPredicate, TestProductContextSection, TestCategory, TestInteractionType,
        TestSubCategory, TestMetadata);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
    EXPECT_EQ(Result.GetHttpResultCode(), 0);

    // Log out
    LogOut(UserSystem);
}

/*
 * Test that we can successfully send a batch of analytics events based on batch rate
 * The batch size has been set high enough that it won't trigger the batch to be sent
 */
CSP_PUBLIC_TEST(CSPEngine, AnalyticsSystemTests, BatchSendAnalyticsEventBatchRateTest)
{
    SetRandSeed();

    RAIIMockLogger MockLogger {};

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* AnalyticsSystem = SystemsManager.GetAnalyticsSystem();

    const std::chrono::milliseconds CurrentTime
        = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

    // Reset time of last batch otherwise the analytics event batch will immediately be sent
    AnalyticsSystem->SetTimeOfLastBatch(CurrentTime);
    // The default batch rate and size are too large for testing so set them to something more reasonable
    AnalyticsSystem->SetBatchRateAndSize(std::chrono::seconds(3), 5);

    String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Ensure the MockLogger will ignore all logs except the one we care about
    EXPECT_CALL(MockLogger.MockLogCallback, Call(::testing::_)).Times(::testing::AnyNumber());

    // We can confirm that the batch has been successfully sent by checking for this success log message
    const csp::common::String BatchAnalyticsSuccessMsg = "Batched Analytics Events sent.";
    EXPECT_CALL(MockLogger.MockLogCallback, Call(BatchAnalyticsSuccessMsg)).Times(1);

    // Analytics Data
    const auto TestProductContextSection = String("Event_ProductContextSection");
    const auto TestCategory = String("Event_Category");
    const auto TestInteractionType = String("Event_InteractionType");
    const auto TestSubCategory = String("Event_SubCategory");
    const Map<String, String> TestMetadata = { { "Key1", "Value1" }, { "Key2", "Value2" } };

    // Send two analytics events to be queued for sending later as a batch
    AnalyticsSystem->BatchAnalyticsEvent(TestProductContextSection, TestCategory, TestInteractionType, TestSubCategory, TestMetadata);
    AnalyticsSystem->BatchAnalyticsEvent(TestProductContextSection, TestCategory, TestInteractionType, TestSubCategory, TestMetadata);

    // The batch rate has been set to 3 seconds so we need to tick foundation for at least that long to allow the batch to be sent
    auto Start = std::chrono::steady_clock::now();
    auto Current = std::chrono::steady_clock::now();
    long long TestTime = 0;

    while (TestTime < 4)
    {
        std::this_thread::sleep_for(1s);

        Current = std::chrono::steady_clock::now();
        TestTime = std::chrono::duration_cast<std::chrono::seconds>(Current - Start).count();

        csp::CSPFoundation::Tick();
    }

    // Log out
    LogOut(UserSystem);
}

/*
 * Test that we can successfully send a batch of analytics events based on batch size
 * The batch rate has been set high enough that it won't trigger the batch to be sent
 */
CSP_PUBLIC_TEST(CSPEngine, AnalyticsSystemTests, BatchSendAnalyticsEventBatchSizeTest)
{
    SetRandSeed();

    RAIIMockLogger MockLogger {};

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* AnalyticsSystem = SystemsManager.GetAnalyticsSystem();

    const std::chrono::milliseconds CurrentTime
        = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

    // Reset time of last batch otherwise the analytics event batch will immediately be sent
    AnalyticsSystem->SetTimeOfLastBatch(CurrentTime);
    // The default batch rate and size are too large for testing so set them to something more reasonable
    AnalyticsSystem->SetBatchRateAndSize(std::chrono::seconds(20), 3);

    String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Ensure the MockLogger will ignore all logs except the one we care about
    EXPECT_CALL(MockLogger.MockLogCallback, Call(::testing::_)).Times(::testing::AnyNumber());

    auto SendBatchStart = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point LogTime;
    bool BatchSent = false;
    // We can confirm that the batch has been successfully sent by checking for this success log message
    // We also check that the batch has been sent as a result of batch size rather than batch rate by checking
    // the elapsed test time is less than the batch rate of 20 seconds
    const csp::common::String BatchAnalyticsSuccessMsg = "Batched Analytics Events sent.";
    EXPECT_CALL(MockLogger.MockLogCallback, Call(BatchAnalyticsSuccessMsg))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const csp::common::String&)
            {
                LogTime = std::chrono::steady_clock::now();
                long long ElapsedTestTime = std::chrono::duration_cast<std::chrono::seconds>(LogTime - SendBatchStart).count();
                EXPECT_TRUE(ElapsedTestTime < 20);
                BatchSent = true;
            }));

    // Analytics Data
    const auto TestProductContextSection = String("Event_ProductContextSection");
    const auto TestCategory = String("Event_Category");
    const auto TestInteractionType = String("Event_InteractionType");
    const auto TestSubCategory = String("Event_SubCategory");
    const Map<String, String> TestMetadata = { { "Key1", "Value1" }, { "Key2", "Value2" } };

    // Send three analytics events to be queued for sending later as a batch
    AnalyticsSystem->BatchAnalyticsEvent(TestProductContextSection, TestCategory, TestInteractionType, TestSubCategory, TestMetadata);
    AnalyticsSystem->BatchAnalyticsEvent(TestProductContextSection, TestCategory, TestInteractionType, TestSubCategory, TestMetadata);
    AnalyticsSystem->BatchAnalyticsEvent(TestProductContextSection, TestCategory, TestInteractionType, TestSubCategory, TestMetadata);

    // The batch rate has been set to 20 seconds so we need to tick foundation for at least that long
    // We check that the batch is sent as a result of batch size rather than batch rate in the EXPECT_CALL above
    auto Start = std::chrono::steady_clock::now();
    auto Current = std::chrono::steady_clock::now();
    long long TestTime = 0;

    // Once the batch has been set as a result of the BatchSize having been reached, BatchSent will be set to true
    // in the EXPECT_CALL above and the while loop will exit before the 20 second batch rate is reached. This is
    // also validated in the EXPECT_CALL above.
    while (!BatchSent && TestTime < 21)
    {
        std::this_thread::sleep_for(1s);

        Current = std::chrono::steady_clock::now();
        TestTime = std::chrono::duration_cast<std::chrono::seconds>(Current - Start).count();

        csp::CSPFoundation::Tick();
    }

    // Log out
    LogOut(UserSystem);
}

/*
 * Test that we get an error when attempting to batch send an analytics event with a required field missing
 */
CSP_PUBLIC_TEST(CSPEngine, AnalyticsSystemTests, BatchSendAnalyticsEventMissingFieldsTest)
{
    SetRandSeed();

    RAIIMockLogger MockLogger {};

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* AnalyticsSystem = SystemsManager.GetAnalyticsSystem();

    String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Ensure the MockLogger will ignore all logs except the one we care about
    EXPECT_CALL(MockLogger.MockLogCallback, Call(::testing::_)).Times(::testing::AnyNumber());

    // Ensure the required fields error message is logged when we try to batch send an analytics event with a required field missing
    const csp::common::String AnalyticsErrorMsg = "Missing the required fields for the Analytics Event.";
    EXPECT_CALL(MockLogger.MockLogCallback, Call(AnalyticsErrorMsg)).Times(1);

    // Analytics Data
    // Passing an empty string for a required field
    const auto TestProductContextSection = String("");
    const auto TestCategory = String("Event_Category");
    const auto TestInteractionType = String("Event_InteractionType");
    const auto TestSubCategory = String("Event_SubCategory");
    const Map<String, String> TestMetadata = { { "Key1", "Value1" }, { "Key2", "Value2" } };

    AnalyticsSystem->BatchAnalyticsEvent(TestProductContextSection, TestCategory, TestInteractionType, TestSubCategory, TestMetadata);

    // Log out
    LogOut(UserSystem);
}

/*
 * Test that we can successfully flush a batch of analytics events.
 * The batch rate and size has been set high enough that they won't trigger the batch to be sent.
 */
CSP_PUBLIC_TEST(CSPEngine, AnalyticsSystemTests, FlushAnalyticsEventsBatchTest)
{
    SetRandSeed();

    RAIIMockLogger MockLogger {};

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* AnalyticsSystem = SystemsManager.GetAnalyticsSystem();

    const std::chrono::milliseconds CurrentTime
        = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

    // Reset time of last batch otherwise the analytics event batch will immediately be sent
    AnalyticsSystem->SetTimeOfLastBatch(CurrentTime);
    // The default batch rate and size are too large for testing so set them to something more reasonable
    AnalyticsSystem->SetBatchRateAndSize(std::chrono::seconds(20), 5);

    String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Ensure the MockLogger will ignore all logs except the one we care about
    EXPECT_CALL(MockLogger.MockLogCallback, Call(::testing::_)).Times(::testing::AnyNumber());

    auto SendBatchStart = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point LogTime;
    bool BatchFlushed = false;
    // We can confirm that the batch has been successfully flushed by checking for this success log message
    // We also check that the batch has been sent as a result of flush rather than batch rate by checking that
    // the elapsed test time is less than the batch rate of 20 seconds
    const csp::common::String BatchAnalyticsSuccessMsg = "Batched Analytics Events sent.";
    EXPECT_CALL(MockLogger.MockLogCallback, Call(BatchAnalyticsSuccessMsg))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const csp::common::String&)
            {
                LogTime = std::chrono::steady_clock::now();
                long long ElapsedTestTime = std::chrono::duration_cast<std::chrono::seconds>(LogTime - SendBatchStart).count();
                EXPECT_TRUE(ElapsedTestTime < 20);
                BatchFlushed = true;
            }));

    // Analytics Data
    const auto TestProductContextSection = String("Event_ProductContextSection");
    const auto TestCategory = String("Event_Category");
    const auto TestInteractionType = String("Event_InteractionType");
    const auto TestSubCategory = String("Event_SubCategory");
    const Map<String, String> TestMetadata = { { "Key1", "Value1" }, { "Key2", "Value2" } };

    // Send two analytics events to be queued for sending later as a batch
    AnalyticsSystem->BatchAnalyticsEvent(TestProductContextSection, TestCategory, TestInteractionType, TestSubCategory, TestMetadata);
    AnalyticsSystem->BatchAnalyticsEvent(TestProductContextSection, TestCategory, TestInteractionType, TestSubCategory, TestMetadata);

    // The batch rate has been set to 20 seconds so we need to tick foundation for at least that long to allow the batch to be sent
    auto Start = std::chrono::steady_clock::now();
    auto Current = std::chrono::steady_clock::now();
    long long TestTime = 0;

    // We will make the call to flush the batch after 3 seconds. If successful BatchFlushed will be set to true
    // in the EXPECT_CALL above and the while loop will exit before the 20 second batch rate is reached. This is
    // also validated in the EXPECT_CALL above.
    while (!BatchFlushed && TestTime < 21)
    {
        std::this_thread::sleep_for(1s);

        Current = std::chrono::steady_clock::now();
        TestTime = std::chrono::duration_cast<std::chrono::seconds>(Current - Start).count();

        if (TestTime == 3)
        {
            // Flush the batch of events after 3 seconds
            AnalyticsSystem->FlushAnalyticsEvents();
        }

        csp::CSPFoundation::Tick();
    }

    // Log out
    LogOut(UserSystem);
}
