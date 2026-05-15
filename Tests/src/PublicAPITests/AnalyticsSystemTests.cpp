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

bool RequestPredicate(const csp::systems::ResultBase& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

/*
 * Test that we can successfully send an analytics event
 */
CSP_PUBLIC_TEST(CSPEngine, AnalyticsSystemTests, SendAnalyticsEventTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* analyticsSystem = systemsManager.GetAnalyticsSystem();

    // Log in
    String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Analytics Data
    const auto testProductContextSection = String("Event_ProductContextSection");
    const auto testCategory = String("Event_Category");
    const auto testInteractionType = String("Event_InteractionType");
    const auto testSubCategory = String("Event_SubCategory");
    const Map<String, String> testMetadata = { { "Key1", "Value1" }, { "Key2", "Value2" } };

    auto [Result] = AWAIT_PRE(analyticsSystem, SendAnalyticsEvent, RequestPredicate, testProductContextSection, testCategory, testInteractionType,
        testSubCategory, testMetadata);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(Result.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseCreated));

    // Log out
    LogOut(userSystem);
}

/*
 * Test that we get an error when sending an analytics event with a required field missing
 */
CSP_PUBLIC_TEST(CSPEngine, AnalyticsSystemTests, SendAnalyticsEventMissingFieldsTest)
{
    SetRandSeed();

    RAIIMockLogger mockLogger {};

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* analyticsSystem = systemsManager.GetAnalyticsSystem();

    // Log in
    String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Ensure the MockLogger will ignore all logs except the one we care about
    EXPECT_CALL(mockLogger.MockLogCallback, Call(::testing::_, ::testing::_)).Times(::testing::AnyNumber());

    // Ensure the required fields error message is logged when we try to send an analytics event with a required field missing
    const csp::common::String analyticsErrorMsg
        = "ProductContextSection, Category and InteractionType are required fields for the Analytics Event and must be provided.";
    EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, analyticsErrorMsg)).Times(1);

    // Analytics Data
    // Passing an empty string for a required field
    const auto testProductContextSection = String("");
    const auto testCategory = String("Event_Category");
    const auto testInteractionType = String("Event_InteractionType");
    const auto testSubCategory = String("Event_SubCategory");
    const Map<String, String> testMetadata = { { "Key1", "Value1" }, { "Key2", "Value2" } };

    auto [Result] = AWAIT_PRE(analyticsSystem, SendAnalyticsEvent, RequestPredicate, testProductContextSection, testCategory, testInteractionType,
        testSubCategory, testMetadata);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
    EXPECT_EQ(Result.GetHttpResultCode(), 0);

    // Log out
    LogOut(userSystem);
}

/*
 * Test that we can successfully send a queue of analytics events based on queue send rate
 */
CSP_PUBLIC_TEST(CSPEngine, AnalyticsSystemTests, QueueAnalyticsEventQueueSendRateTest)
{
    SetRandSeed();

    RAIIMockLogger mockLogger {};

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* analyticsSystem = systemsManager.GetAnalyticsSystem();

    // The default queue send rate is too large for testing so set it to something more reasonable
    // The max queue size has been set to 3 so that we can validate that the queue is being cleared correctly
    analyticsSystem->__SetQueueSendRateAndMaxSize(std::chrono::seconds(3), 3);

    // Log in
    String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Ensure the MockLogger will ignore all logs except the one we care about
    EXPECT_CALL(mockLogger.MockLogCallback, Call(::testing::_, ::testing::_)).Times(::testing::AnyNumber());

    // Reset time the last queue was sent
    const std::chrono::milliseconds currentTime
        = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

    analyticsSystem->SetTimeSinceLastQueueSend(currentTime);

    // Analytics Data
    const auto testProductContextSection = String("Event_ProductContextSection");
    const auto testCategory = String("Event_Category");
    const auto testInteractionType = String("Event_InteractionType");
    const auto testSubCategory = String("Event_SubCategory");
    const Map<String, String> testMetadata = { { "Key1", "Value1" }, { "Key2", "Value2" } };

    // Queue two analytics records to be sent after the queue send rate time has elapsed
    {
        std::promise<bool> queueSentPromise1;
        std::future<bool> queueSentFuture1 = queueSentPromise1.get_future();

        const csp::common::String queueSentSuccessMsg = "Successfully sent the Analytics Record queue.";
        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Verbose, queueSentSuccessMsg))
            .Times(1)
            .WillOnce(::testing::Invoke([&](csp::common::LogLevel, const csp::common::String&) { queueSentPromise1.set_value(true); }));

        // Send two analytics events to be queued for sending later as a batch
        analyticsSystem->QueueAnalyticsEvent(testProductContextSection, testCategory, testInteractionType, testSubCategory, testMetadata);
        analyticsSystem->QueueAnalyticsEvent(testProductContextSection, testCategory, testInteractionType, testSubCategory, testMetadata);

        csp::CSPFoundation::Tick();

        // Validate that we have two queued items
        EXPECT_TRUE(analyticsSystem->GetCurrentQueueSize() == 2);

        // It will not process the queue until at least 3 seconds has elapsed since the last send time
        ASSERT_NE(queueSentFuture1.wait_for(4s), std::future_status::ready) << "Analytics queue should not yet have been sent.";
        // Calling tick will check if the queue send rate time has elapsed and send the queued events if it has
        csp::CSPFoundation::Tick();

        // Check that the queue has been flushed correctly
        EXPECT_TRUE(analyticsSystem->GetCurrentQueueSize() == 0);

        // Wait for the callback to be received
        queueSentFuture1.wait();
        EXPECT_TRUE(queueSentFuture1.get() == true);
    }

    // Queue two more analytics records to be sent after the queue send rate has elapsed
    // Validate that the queue was cleared after the first batch was sent and that it is not being sent again prematurely on Tick due to batch size
    {
        std::promise<bool> queueSentPromise2;
        std::future<bool> queueSentFuture2 = queueSentPromise2.get_future();

        const csp::common::String queueSentSuccessMsg = "Successfully sent the Analytics Record queue.";
        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Verbose, queueSentSuccessMsg))
            .Times(1)
            .WillOnce(::testing::Invoke([&](csp::common::LogLevel, const csp::common::String&) { queueSentPromise2.set_value(true); }));

        // Send two more analytics events to be queued for sending later as a batch
        analyticsSystem->QueueAnalyticsEvent(testProductContextSection, testCategory, testInteractionType, testSubCategory, testMetadata);
        analyticsSystem->QueueAnalyticsEvent(testProductContextSection, testCategory, testInteractionType, testSubCategory, testMetadata);

        csp::CSPFoundation::Tick();

        // Check that the queue size is correct.
        EXPECT_TRUE(analyticsSystem->GetCurrentQueueSize() == 2);
        EXPECT_TRUE(analyticsSystem->GetCurrentQueueSize() < analyticsSystem->GetMaxQueueSize());

        // It will not process the queue until at least 3 seconds has elapsed since the last send time
        ASSERT_NE(queueSentFuture2.wait_for(4s), std::future_status::ready) << "Analytics queue should not yet have been sent.";
        // Calling tick will check if the queue send rate time has elapsed and send the queued events if it has
        csp::CSPFoundation::Tick();

        // Check that the queue has been flushed correctly
        EXPECT_TRUE(analyticsSystem->GetCurrentQueueSize() == 0);

        // Wait for the callback to be received
        queueSentFuture2.wait();
        EXPECT_TRUE(queueSentFuture2.get() == true);
    }

    // Log out
    LogOut(userSystem);

    //
}

/*
 * Test that we can successfully send a queue of analytics events based on queue size
 * The queue send rate has been set high enough that it won't trigger the queue to be sent
 */
CSP_PUBLIC_TEST(CSPEngine, AnalyticsSystemTests, QueueAnalyticsEventQueueSizeTest)
{
    SetRandSeed();

    RAIIMockLogger mockLogger {};

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* analyticsSystem = systemsManager.GetAnalyticsSystem();

    // The default queue size has been reduced to something more reasonable for testing
    analyticsSystem->__SetQueueSendRateAndMaxSize(std::chrono::seconds(60), 3);

    // Log in
    String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Ensure the MockLogger will ignore all logs except the one we care about
    EXPECT_CALL(mockLogger.MockLogCallback, Call(::testing::_, ::testing::_)).Times(::testing::AnyNumber());

    // Analytics Data
    const auto testProductContextSection = String("Event_ProductContextSection");
    const auto testCategory = String("Event_Category");
    const auto testInteractionType = String("Event_InteractionType");
    const auto testSubCategory = String("Event_SubCategory");
    const Map<String, String> testMetadata = { { "Key1", "Value1" }, { "Key2", "Value2" } };

    // Reset time the last queue was sent
    const std::chrono::milliseconds currentTime
        = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

    analyticsSystem->SetTimeSinceLastQueueSend(currentTime);

    // Queue three analytics records to be sent only when the queue reaches the max size
    {
        std::promise<bool> queueSentPromise1;
        std::future<bool> queueSentFuture1 = queueSentPromise1.get_future();

        const csp::common::String queueSentSuccessMsg = "Successfully sent the Analytics Record queue.";
        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Verbose, queueSentSuccessMsg))
            .Times(1)
            .WillOnce(::testing::Invoke([&](csp::common::LogLevel, const csp::common::String&) { queueSentPromise1.set_value(true); }));

        // Send two analytics events to be queued for sending later as a batch
        analyticsSystem->QueueAnalyticsEvent(testProductContextSection, testCategory, testInteractionType, testSubCategory, testMetadata);
        analyticsSystem->QueueAnalyticsEvent(testProductContextSection, testCategory, testInteractionType, testSubCategory, testMetadata);

        // Validate that we have two queued items
        EXPECT_TRUE(analyticsSystem->GetCurrentQueueSize() == 2);

        // Ensure that the queue is not sent before the max size is reached
        csp::CSPFoundation::Tick();

        // Validate that we still have two queued items
        EXPECT_TRUE(analyticsSystem->GetCurrentQueueSize() == 2);

        // Add a third analytics record to the queue which will trigger the queue to be sent
        analyticsSystem->QueueAnalyticsEvent(testProductContextSection, testCategory, testInteractionType, testSubCategory, testMetadata);

        // Validate that we have queued enough analytics records to trigger the queue to be sent on next tick
        EXPECT_TRUE(analyticsSystem->GetCurrentQueueSize() == analyticsSystem->GetMaxQueueSize());

        // The queue should now be sent
        csp::CSPFoundation::Tick();

        // Check that the queue has been flushed correctly
        EXPECT_TRUE(analyticsSystem->GetCurrentQueueSize() == 0);

        // Wait for the callback to be received
        queueSentFuture1.wait();
        EXPECT_TRUE(queueSentFuture1.get() == true);
    }

    // Check that the queue is being cleared correctly and queue three analytics records to be sent only when the queue reaches the max size
    {
        std::promise<bool> queueSentPromise2;
        std::future<bool> queueSentFuture2 = queueSentPromise2.get_future();

        const csp::common::String queueSentSuccessMsg = "Successfully sent the Analytics Record queue.";
        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Verbose, queueSentSuccessMsg))
            .Times(1)
            .WillOnce(::testing::Invoke([&](csp::common::LogLevel, const csp::common::String&) { queueSentPromise2.set_value(true); }));

        // Send two analytics events to be queued for sending later as a batch
        analyticsSystem->QueueAnalyticsEvent(testProductContextSection, testCategory, testInteractionType, testSubCategory, testMetadata);
        analyticsSystem->QueueAnalyticsEvent(testProductContextSection, testCategory, testInteractionType, testSubCategory, testMetadata);

        // Validate that we only have the two newly queued items
        EXPECT_TRUE(analyticsSystem->GetCurrentQueueSize() == 2);

        // Ensure that the queue is not sent before the max size is reached
        csp::CSPFoundation::Tick();

        // Validate that we still have two queued items
        EXPECT_TRUE(analyticsSystem->GetCurrentQueueSize() == 2);

        // Add a third analytics record to the queue which will trigger the queue to be sent
        analyticsSystem->QueueAnalyticsEvent(testProductContextSection, testCategory, testInteractionType, testSubCategory, testMetadata);

        // Validate that we have queued enough analytics records to trigger the queue to be sent on next tick
        EXPECT_TRUE(analyticsSystem->GetCurrentQueueSize() == analyticsSystem->GetMaxQueueSize());

        // The queue will now be sent
        csp::CSPFoundation::Tick();

        // Check that the queue has been flushed correctly
        EXPECT_TRUE(analyticsSystem->GetCurrentQueueSize() == 0);

        // Wait for the callback to be received
        queueSentFuture2.wait();
        EXPECT_TRUE(queueSentFuture2.get() == true);
    }

    // Log out
    LogOut(userSystem);
}

/*
 * Test that we get an error when attempting to send a queue of analytics events with a required field missing
 */
CSP_PUBLIC_TEST(CSPEngine, AnalyticsSystemTests, QueueAnalyticsEventMissingFieldsTest)
{
    SetRandSeed();

    RAIIMockLogger mockLogger {};

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* analyticsSystem = systemsManager.GetAnalyticsSystem();

    // Log in
    String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Ensure the MockLogger will ignore all logs except the one we care about
    EXPECT_CALL(mockLogger.MockLogCallback, Call(::testing::_, ::testing::_)).Times(::testing::AnyNumber());

    // Ensure the required fields error message is logged when we try to queue an analytics event with a required field missing
    const csp::common::String analyticsErrorMsg
        = "ProductContextSection, Category and InteractionType are required fields for the Analytics Event and must be provided.";
    EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, analyticsErrorMsg)).Times(1);

    // Analytics Data
    // Passing an empty string for a required field
    const auto testProductContextSection = String("");
    const auto testCategory = String("Event_Category");
    const auto testInteractionType = String("Event_InteractionType");
    const auto testSubCategory = String("Event_SubCategory");
    const Map<String, String> testMetadata = { { "Key1", "Value1" }, { "Key2", "Value2" } };

    analyticsSystem->QueueAnalyticsEvent(testProductContextSection, testCategory, testInteractionType, testSubCategory, testMetadata);

    // Log out
    LogOut(userSystem);
}

/*
 * Test that we can successfully flush a queue of analytics events.
 * The queue send rate and size have been left at their defaults which means they won't trigger the queue to be sent.
 */
CSP_PUBLIC_TEST(CSPEngine, AnalyticsSystemTests, FlushAnalyticsEventsQueueTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* analyticsSystem = systemsManager.GetAnalyticsSystem();

    // Reset time the last queue was sent
    const std::chrono::milliseconds currentTime
        = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

    analyticsSystem->SetTimeSinceLastQueueSend(currentTime);

    // Log in
    String userId;
    LogInAsNewTestUser(userSystem, userId);

    std::promise<NullResult> resultPromise;
    std::future<NullResult> resultFuture = resultPromise.get_future();

    NullResultCallback flushResultCallback = [&resultPromise](const NullResult& result)
    {
        EXPECT_TRUE(result.GetResultCode() == csp::systems::EResultCode::Success);
        resultPromise.set_value(result);
    };

    // Analytics Data
    const auto testProductContextSection = String("Event_ProductContextSection");
    const auto testCategory = String("Event_Category");
    const auto testInteractionType = String("Event_InteractionType");
    const auto testSubCategory = String("Event_SubCategory");
    const Map<String, String> testMetadata = { { "Key1", "Value1" }, { "Key2", "Value2" } };

    // Send two analytics events to be queued for sending later as a batch
    analyticsSystem->QueueAnalyticsEvent(testProductContextSection, testCategory, testInteractionType, testSubCategory, testMetadata);
    analyticsSystem->QueueAnalyticsEvent(testProductContextSection, testCategory, testInteractionType, testSubCategory, testMetadata);

    // Check that the queue has two analytics records
    EXPECT_TRUE(analyticsSystem->GetCurrentQueueSize() == 2);

    // The queue send rate and max size have been left at their default of 60 seconds & 25 analytics records
    // We will tick to ensure the queue is not sent as a result of the queue send rate
    csp::CSPFoundation::Tick();
    ASSERT_NE(resultFuture.wait_for(1s), std::future_status::ready) << "Analytics queue should not yet have been sent.";
    csp::CSPFoundation::Tick();

    // Check that the queue still has two analytics records
    EXPECT_TRUE(analyticsSystem->GetCurrentQueueSize() == 2);

    analyticsSystem->FlushAnalyticsEventsQueue(flushResultCallback);

    // Check that the queue has been flushed correctly
    EXPECT_TRUE(analyticsSystem->GetCurrentQueueSize() == 0);

    // Wait for the callback to be received
    resultFuture.wait();
    EXPECT_TRUE(resultFuture.get().GetResultCode() == csp::systems::EResultCode::Success);

    // Log out
    LogOut(userSystem);
}

/*
 * Test that when we try to flush an empty queue of analytics events the callback is fired correctly.
 */
CSP_PUBLIC_TEST(CSPEngine, AnalyticsSystemTests, FlushEmptyAnalyticsEventsQueueTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* analyticsSystem = systemsManager.GetAnalyticsSystem();

    // Log in
    String userId;
    LogInAsNewTestUser(userSystem, userId);

    std::promise<NullResult> resultPromise;
    std::future<NullResult> resultFuture = resultPromise.get_future();

    NullResultCallback flushResultCallback = [&resultPromise](const NullResult& result)
    {
        resultPromise.set_value(result);
    };

    // Check that the queue is empty
    EXPECT_TRUE(analyticsSystem->GetCurrentQueueSize() == 0);

    analyticsSystem->FlushAnalyticsEventsQueue(flushResultCallback);

    // Wait for the callback to be received
    resultFuture.wait();
    auto result = resultFuture.get();
    EXPECT_TRUE(result.GetResultCode() == csp::systems::EResultCode::Success);
    EXPECT_TRUE(result.GetHttpResultCode() == static_cast<uint16_t>(csp::web::EResponseCodes::ResponseNoContent));

    // Log out
    LogOut(userSystem);
}
