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

#include <future>

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

    // Log in
    String UserId;
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

    // Log in
    String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Ensure the MockLogger will ignore all logs except the one we care about
    EXPECT_CALL(MockLogger.MockLogCallback, Call(::testing::_, ::testing::_)).Times(::testing::AnyNumber());

    // Ensure the required fields error message is logged when we try to send an analytics event with a required field missing
    const csp::common::String AnalyticsErrorMsg
        = "ProductContextSection, Category and InteractionType are required fields for the Analytics Event and must be provided.";
    EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, AnalyticsErrorMsg)).Times(1);

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
 * Test that we can successfully send a queue of analytics events based on queue send rate
 */
CSP_PUBLIC_TEST(CSPEngine, AnalyticsSystemTests, QueueAnalyticsEventQueueSendRateTest)
{
    SetRandSeed();

    RAIIMockLogger MockLogger {};

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* AnalyticsSystem = SystemsManager.GetAnalyticsSystem();

    // The default queue send rate is too large for testing so set it to something more reasonable
    // The max queue size has been set to 3 so that we can validate that the queue is being cleared correctly
    AnalyticsSystem->__SetQueueSendRateAndMaxSize(std::chrono::seconds(3), 3);

    // Log in
    String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Ensure the MockLogger will ignore all logs except the one we care about
    EXPECT_CALL(MockLogger.MockLogCallback, Call(::testing::_, ::testing::_)).Times(::testing::AnyNumber());

    // Reset time the last queue was sent
    const std::chrono::milliseconds CurrentTime
        = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

    AnalyticsSystem->SetTimeSinceLastQueueSend(CurrentTime);

    // Analytics Data
    const auto TestProductContextSection = String("Event_ProductContextSection");
    const auto TestCategory = String("Event_Category");
    const auto TestInteractionType = String("Event_InteractionType");
    const auto TestSubCategory = String("Event_SubCategory");
    const Map<String, String> TestMetadata = { { "Key1", "Value1" }, { "Key2", "Value2" } };

    // Queue two analytics records to be sent after the queue send rate time has elapsed
    {
        std::promise<bool> QueueSentPromise1;
        std::future<bool> QueueSentFuture1 = QueueSentPromise1.get_future();

        const csp::common::String QueueSentSuccessMsg = "Successfully sent the Analytics Record queue.";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Verbose, QueueSentSuccessMsg))
            .Times(1)
            .WillOnce(::testing::Invoke([&](csp::common::LogLevel, const csp::common::String&) { QueueSentPromise1.set_value(true); }));

        // Send two analytics events to be queued for sending later as a batch
        AnalyticsSystem->QueueAnalyticsEvent(TestProductContextSection, TestCategory, TestInteractionType, TestSubCategory, TestMetadata);
        AnalyticsSystem->QueueAnalyticsEvent(TestProductContextSection, TestCategory, TestInteractionType, TestSubCategory, TestMetadata);

        csp::CSPFoundation::Tick();

        // Validate that we have two queued items
        EXPECT_TRUE(AnalyticsSystem->GetCurrentQueueSize() == 2);

        // It will not process the queue until at least 3 seconds has elapsed since the last send time
        ASSERT_NE(QueueSentFuture1.wait_for(4s), std::future_status::ready) << "Analytics queue should not yet have been sent.";
        // Calling tick will check if the queue send rate time has elapsed and send the queued events if it has
        csp::CSPFoundation::Tick();

        // Check that the queue has been flushed correctly
        EXPECT_TRUE(AnalyticsSystem->GetCurrentQueueSize() == 0);

        // Wait for the callback to be received
        QueueSentFuture1.wait();
        EXPECT_TRUE(QueueSentFuture1.get() == true);
    }

    // Queue two more analytics records to be sent after the queue send rate has elapsed
    // Validate that the queue was cleared after the first batch was sent and that it is not being sent again prematurely on Tick due to batch size
    {
        std::promise<bool> QueueSentPromise2;
        std::future<bool> QueueSentFuture2 = QueueSentPromise2.get_future();

        const csp::common::String QueueSentSuccessMsg = "Successfully sent the Analytics Record queue.";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Verbose, QueueSentSuccessMsg))
            .Times(1)
            .WillOnce(::testing::Invoke([&](csp::common::LogLevel, const csp::common::String&) { QueueSentPromise2.set_value(true); }));

        // Send two more analytics events to be queued for sending later as a batch
        AnalyticsSystem->QueueAnalyticsEvent(TestProductContextSection, TestCategory, TestInteractionType, TestSubCategory, TestMetadata);
        AnalyticsSystem->QueueAnalyticsEvent(TestProductContextSection, TestCategory, TestInteractionType, TestSubCategory, TestMetadata);

        csp::CSPFoundation::Tick();

        // Check that the queue size is correct.
        EXPECT_TRUE(AnalyticsSystem->GetCurrentQueueSize() == 2);
        EXPECT_TRUE(AnalyticsSystem->GetCurrentQueueSize() < AnalyticsSystem->GetMaxQueueSize());

        // It will not process the queue until at least 3 seconds has elapsed since the last send time
        ASSERT_NE(QueueSentFuture2.wait_for(4s), std::future_status::ready) << "Analytics queue should not yet have been sent.";
        // Calling tick will check if the queue send rate time has elapsed and send the queued events if it has
        csp::CSPFoundation::Tick();

        // Check that the queue has been flushed correctly
        EXPECT_TRUE(AnalyticsSystem->GetCurrentQueueSize() == 0);

        // Wait for the callback to be received
        QueueSentFuture2.wait();
        EXPECT_TRUE(QueueSentFuture2.get() == true);
    }

    // Log out
    LogOut(UserSystem);

    //
}

/*
 * Test that we can successfully send a queue of analytics events based on queue size
 * The queue send rate has been set high enough that it won't trigger the queue to be sent
 */
CSP_PUBLIC_TEST(CSPEngine, AnalyticsSystemTests, QueueAnalyticsEventQueueSizeTest)
{
    SetRandSeed();

    RAIIMockLogger MockLogger {};

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* AnalyticsSystem = SystemsManager.GetAnalyticsSystem();

    // The default queue size has been reduced to something more reasonable for testing
    AnalyticsSystem->__SetQueueSendRateAndMaxSize(std::chrono::seconds(60), 3);

    // Log in
    String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Ensure the MockLogger will ignore all logs except the one we care about
    EXPECT_CALL(MockLogger.MockLogCallback, Call(::testing::_, ::testing::_)).Times(::testing::AnyNumber());

    // Analytics Data
    const auto TestProductContextSection = String("Event_ProductContextSection");
    const auto TestCategory = String("Event_Category");
    const auto TestInteractionType = String("Event_InteractionType");
    const auto TestSubCategory = String("Event_SubCategory");
    const Map<String, String> TestMetadata = { { "Key1", "Value1" }, { "Key2", "Value2" } };

    // Reset time the last queue was sent
    const std::chrono::milliseconds CurrentTime
        = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

    AnalyticsSystem->SetTimeSinceLastQueueSend(CurrentTime);

    // Queue three analytics records to be sent only when the queue reaches the max size
    {
        std::promise<bool> QueueSentPromise1;
        std::future<bool> QueueSentFuture1 = QueueSentPromise1.get_future();

        const csp::common::String QueueSentSuccessMsg = "Successfully sent the Analytics Record queue.";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Verbose, QueueSentSuccessMsg))
            .Times(1)
            .WillOnce(::testing::Invoke([&](csp::common::LogLevel, const csp::common::String&) { QueueSentPromise1.set_value(true); }));

        // Send two analytics events to be queued for sending later as a batch
        AnalyticsSystem->QueueAnalyticsEvent(TestProductContextSection, TestCategory, TestInteractionType, TestSubCategory, TestMetadata);
        AnalyticsSystem->QueueAnalyticsEvent(TestProductContextSection, TestCategory, TestInteractionType, TestSubCategory, TestMetadata);

        // Validate that we have two queued items
        EXPECT_TRUE(AnalyticsSystem->GetCurrentQueueSize() == 2);

        // Ensure that the queue is not sent before the max size is reached
        csp::CSPFoundation::Tick();

        // Validate that we still have two queued items
        EXPECT_TRUE(AnalyticsSystem->GetCurrentQueueSize() == 2);

        // Add a third analytics record to the queue which will trigger the queue to be sent
        AnalyticsSystem->QueueAnalyticsEvent(TestProductContextSection, TestCategory, TestInteractionType, TestSubCategory, TestMetadata);

        // Validate that we have queued enough analytics records to trigger the queue to be sent on next tick
        EXPECT_TRUE(AnalyticsSystem->GetCurrentQueueSize() == AnalyticsSystem->GetMaxQueueSize());

        // The queue should now be sent
        csp::CSPFoundation::Tick();

        // Check that the queue has been flushed correctly
        EXPECT_TRUE(AnalyticsSystem->GetCurrentQueueSize() == 0);

        // Wait for the callback to be received
        QueueSentFuture1.wait();
        EXPECT_TRUE(QueueSentFuture1.get() == true);
    }

    // Check that the queue is being cleared correctly and queue three analytics records to be sent only when the queue reaches the max size
    {
        std::promise<bool> QueueSentPromise2;
        std::future<bool> QueueSentFuture2 = QueueSentPromise2.get_future();

        const csp::common::String QueueSentSuccessMsg = "Successfully sent the Analytics Record queue.";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Verbose, QueueSentSuccessMsg))
            .Times(1)
            .WillOnce(::testing::Invoke([&](csp::common::LogLevel, const csp::common::String&) { QueueSentPromise2.set_value(true); }));

        // Send two analytics events to be queued for sending later as a batch
        AnalyticsSystem->QueueAnalyticsEvent(TestProductContextSection, TestCategory, TestInteractionType, TestSubCategory, TestMetadata);
        AnalyticsSystem->QueueAnalyticsEvent(TestProductContextSection, TestCategory, TestInteractionType, TestSubCategory, TestMetadata);

        // Validate that we only have the two newly queued items
        EXPECT_TRUE(AnalyticsSystem->GetCurrentQueueSize() == 2);

        // Ensure that the queue is not sent before the max size is reached
        csp::CSPFoundation::Tick();

        // Validate that we still have two queued items
        EXPECT_TRUE(AnalyticsSystem->GetCurrentQueueSize() == 2);

        // Add a third analytics record to the queue which will trigger the queue to be sent
        AnalyticsSystem->QueueAnalyticsEvent(TestProductContextSection, TestCategory, TestInteractionType, TestSubCategory, TestMetadata);

        // Validate that we have queued enough analytics records to trigger the queue to be sent on next tick
        EXPECT_TRUE(AnalyticsSystem->GetCurrentQueueSize() == AnalyticsSystem->GetMaxQueueSize());

        // The queue will now be sent
        csp::CSPFoundation::Tick();

        // Check that the queue has been flushed correctly
        EXPECT_TRUE(AnalyticsSystem->GetCurrentQueueSize() == 0);

        // Wait for the callback to be received
        QueueSentFuture2.wait();
        EXPECT_TRUE(QueueSentFuture2.get() == true);
    }

    // Log out
    LogOut(UserSystem);
}

/*
 * Test that we get an error when attempting to send a queue of analytics events with a required field missing
 */
CSP_PUBLIC_TEST(CSPEngine, AnalyticsSystemTests, QueueAnalyticsEventMissingFieldsTest)
{
    SetRandSeed();

    RAIIMockLogger MockLogger {};

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* AnalyticsSystem = SystemsManager.GetAnalyticsSystem();

    // Log in
    String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Ensure the MockLogger will ignore all logs except the one we care about
    EXPECT_CALL(MockLogger.MockLogCallback, Call(::testing::_, ::testing::_)).Times(::testing::AnyNumber());

    // Ensure the required fields error message is logged when we try to queue an analytics event with a required field missing
    const csp::common::String AnalyticsErrorMsg
        = "ProductContextSection, Category and InteractionType are required fields for the Analytics Event and must be provided.";
    EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, AnalyticsErrorMsg)).Times(1);

    // Analytics Data
    // Passing an empty string for a required field
    const auto TestProductContextSection = String("");
    const auto TestCategory = String("Event_Category");
    const auto TestInteractionType = String("Event_InteractionType");
    const auto TestSubCategory = String("Event_SubCategory");
    const Map<String, String> TestMetadata = { { "Key1", "Value1" }, { "Key2", "Value2" } };

    AnalyticsSystem->QueueAnalyticsEvent(TestProductContextSection, TestCategory, TestInteractionType, TestSubCategory, TestMetadata);

    // Log out
    LogOut(UserSystem);
}

/*
 * Test that we can successfully flush a queue of analytics events.
 * The queue send rate and size have been left at their defaults which means they won't trigger the queue to be sent.
 */
CSP_PUBLIC_TEST(CSPEngine, AnalyticsSystemTests, FlushAnalyticsEventsQueueTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* AnalyticsSystem = SystemsManager.GetAnalyticsSystem();

    // Reset time the last queue was sent
    const std::chrono::milliseconds CurrentTime
        = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

    AnalyticsSystem->SetTimeSinceLastQueueSend(CurrentTime);

    // Log in
    String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    std::promise<NullResult> ResultPromise;
    std::future<NullResult> ResultFuture = ResultPromise.get_future();

    NullResultCallback FlushResultCallback = [&ResultPromise](const NullResult& Result)
    {
        EXPECT_TRUE(Result.GetResultCode() == csp::systems::EResultCode::Success);
        ResultPromise.set_value(Result);
    };

    // Analytics Data
    const auto TestProductContextSection = String("Event_ProductContextSection");
    const auto TestCategory = String("Event_Category");
    const auto TestInteractionType = String("Event_InteractionType");
    const auto TestSubCategory = String("Event_SubCategory");
    const Map<String, String> TestMetadata = { { "Key1", "Value1" }, { "Key2", "Value2" } };

    // Send two analytics events to be queued for sending later as a batch
    AnalyticsSystem->QueueAnalyticsEvent(TestProductContextSection, TestCategory, TestInteractionType, TestSubCategory, TestMetadata);
    AnalyticsSystem->QueueAnalyticsEvent(TestProductContextSection, TestCategory, TestInteractionType, TestSubCategory, TestMetadata);

    // Check that the queue has two analytics records
    EXPECT_TRUE(AnalyticsSystem->GetCurrentQueueSize() == 2);

    // The queue send rate and max size have been left at their default of 60 seconds & 25 analytics records
    // We will tick to ensure the queue is not sent as a result of the queue send rate
    csp::CSPFoundation::Tick();
    ASSERT_NE(ResultFuture.wait_for(1s), std::future_status::ready) << "Analytics queue should not yet have been sent.";
    csp::CSPFoundation::Tick();

    // Check that the queue still has two analytics records
    EXPECT_TRUE(AnalyticsSystem->GetCurrentQueueSize() == 2);

    AnalyticsSystem->FlushAnalyticsEventsQueue(FlushResultCallback);

    // Check that the queue has been flushed correctly
    EXPECT_TRUE(AnalyticsSystem->GetCurrentQueueSize() == 0);

    // Wait for the callback to be received
    ResultFuture.wait();
    EXPECT_TRUE(ResultFuture.get().GetResultCode() == csp::systems::EResultCode::Success);

    // Log out
    LogOut(UserSystem);
}

/*
 * Test that when we try to flush an empty queue of analytics events the callback is fired correctly.
 */
CSP_PUBLIC_TEST(CSPEngine, AnalyticsSystemTests, FlushEmptyAnalyticsEventsQueueTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* AnalyticsSystem = SystemsManager.GetAnalyticsSystem();

    // Log in
    String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    std::promise<NullResult> ResultPromise;
    std::future<NullResult> ResultFuture = ResultPromise.get_future();

    NullResultCallback FlushResultCallback = [&ResultPromise](const NullResult& Result)
    {
        ResultPromise.set_value(Result);
    };

    // Check that the queue is empty
    EXPECT_TRUE(AnalyticsSystem->GetCurrentQueueSize() == 0);

    AnalyticsSystem->FlushAnalyticsEventsQueue(FlushResultCallback);

    // Wait for the callback to be received
    ResultFuture.wait();
    auto Result = ResultFuture.get();
    EXPECT_TRUE(Result.GetResultCode() == csp::systems::EResultCode::Success);
    EXPECT_TRUE(Result.GetHttpResultCode() == static_cast<uint16_t>(csp::web::EResponseCodes::ResponseNoContent));

    // Log out
    LogOut(UserSystem);
}
