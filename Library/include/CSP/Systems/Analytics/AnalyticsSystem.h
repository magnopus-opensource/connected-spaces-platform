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

#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Common/CancellationToken.h"
#include "CSP/Common/Optional.h"
#include "CSP/Common/String.h"
#include "CSP/Systems/SystemBase.h"

#include <chrono>
#include <mutex>

CSP_START_IGNORE
#ifdef CSP_TESTS
class CSPEngine_AnalyticsSystemTests_QueueAnalyticsEventQueueSendRateTest_Test;
class CSPEngine_AnalyticsSystemTests_QueueAnalyticsEventQueueSizeTest_Test;
class CSPEngine_AnalyticsSystemTests_FlushAnalyticsEventsQueueTest_Test;
#endif
CSP_END_IGNORE

namespace csp
{
class ClientUserAgent;
} // namespace csp

namespace csp::services
{
class ApiBase;
} // namespace csp::services

namespace csp::web
{
class WebClient;
} // namespace csp::web

namespace csp::services::generated::userservice
{
class AnalyticsRecord;
}

namespace csp::systems
{

/// @ingroup Analytics System
/// @brief Public facing system that allows AnalyticsRecords to be sent to the backend services.
/// @invariant Users must be logged in to send AnalyticsRecords to the backend services.
class CSP_API AnalyticsSystem : public SystemBase
{
    CSP_START_IGNORE
    /** @cond DO_NOT_DOCUMENT */
    friend class SystemsManager;

#ifdef CSP_TESTS
    friend class ::CSPEngine_AnalyticsSystemTests_QueueAnalyticsEventQueueSendRateTest_Test;
    friend class ::CSPEngine_AnalyticsSystemTests_QueueAnalyticsEventQueueSizeTest_Test;
    friend class ::CSPEngine_AnalyticsSystemTests_FlushAnalyticsEventsQueueTest_Test;
#endif
    /** @endcond */
    CSP_END_IGNORE

public:
    /**
     * @brief Constructs an Analytics Record which is added to a queue to be sent to the backend services in a single batch.
     * @details The queue will be sent when one of the following conditions are met:
     * 1. The time since the last batch was sent reaches the AnalyticsQueueSendRate (default 60 seconds).
     * 2. The number of events in the queue reaches the MaxQueueSize threshhold (default 25 events).
     * 3. The client application calls FlushAnalyticsEventsQueue(). Clients should call this as part of their log out or shut down procedure to force
     * the queue to be sent. For more information about flushing events see the method documentation @ref
     * AnalyticsSystem::FlushAnalyticsEventsQueue().
     *
     * Example: Consider the following user action that is to be captured as an analytics event:
     * - A [web client] user [clicks] on a [menu] item in the [UI].
     *
     * In this example:
     * - [web client] is captured internally.
     * - [clicks] is the InteractionType.
     * - [menu] is the Category.
     * - [UI] is the ProductContextSection.
     *
     * @note The following data is captured internally and included in the analytics record:
     * - tenant name, client sku, client version and client build environment.
     *
     * @pre The user must be logged in to send Analytics Records to the backend services.
     * @param ProductContextSection const csp::common::String& : The specific, high-level functional area or context within the product where the
     * event occurred. This field acts as a primary identifier for the part of the application or system the user is interacting with.
     * @param Category const csp::common::String& : Categorization field which acts as a namespace for the InteractionType. It provides a means of
     * grouping similar events, which makes it easier to analyze and filter analytics data.
     * @param InteractionType const csp::common::String& : Describes the precise and specific interaction that is being tracked. This field identifies
     * what the user did or what happened within the product at a specific moment in time.
     * @param SubCategory const csp::common::Optional<csp::common::String>& : Optional sub-category field to provide additional context if required.
     * @param Metadata const csp::common::Optional<csp::common::Map<csp::common::String, csp::common::String>>& : Optional analytics event metadata.
     * Metadata is the event payload. It may be used to store such information as the space the user is in, their geographical region, as well as
     * relevant device specs.
     */
    void QueueAnalyticsEvent(const csp::common::String& ProductContextSection, const csp::common::String& Category,
        const csp::common::String& InteractionType, const csp::common::Optional<csp::common::String>& SubCategory,
        const csp::common::Optional<csp::common::Map<csp::common::String, csp::common::String>>& Metadata);

    /**
     * @brief Constructs an Analytics Record which is immediately sent to the backend services.
     * @note The QueueAnalyticsEvent method should be used by default as it will queue events before sending them. This method will immediately send
     * the analytics event and should therefore only be used when this behaviour is required.
     *
     * For more information about how the Analytics Record is constructed, see the documentation for @ref AnalyticsSystem::QueueAnalyticsEvent().
     *
     * @pre The user must be logged in to send Analytics Records to the backend services.
     * @param ProductContextSection const csp::common::String& : The specific, high-level functional area or context within the product where the
     * event occurred. This field acts as a primary identifier for the part of the application or system the user is interacting with.
     * @param Category const csp::common::String& : Categorization field which acts as a namespace for the InteractionType. It provides a means of
     * grouping similar events, which makes it easier to analyze and filter analytics data.
     * @param InteractionType const csp::common::String& : Describes the precise and specific interaction that is being tracked. This field identifies
     * what the user did or what happened within the product at a specific moment in time.
     * @param SubCategory const csp::common::Optional<csp::common::String>& : Optional sub-category field to provide additional context if required.
     * @param Metadata const csp::common::Optional<csp::common::Map<csp::common::String, csp::common::String>>& : Optional analytics event metadata.
     * Metadata is the event payload. It may be used to store such information as the space the user is in, their geographical region, as well as
     * relevant device specs.
     * @param Callback NullResultCallback : the callback to execute on completion of the send operation.
     */
    void SendAnalyticsEvent(const csp::common::String& ProductContextSection, const csp::common::String& Category,
        const csp::common::String& InteractionType, const csp::common::Optional<csp::common::String>& SubCategory,
        const csp::common::Optional<csp::common::Map<csp::common::String, csp::common::String>>& Metadata, NullResultCallback Callback);

    /**
     * @brief Trigger immediate dispatch of the Analytics Records queue to the backend services.
     * @note This method should be called as part of client log out or shut down procedure to ensure that any queued analytics records are flushed and
     * sent to the backend services before the user is logged out or the application is shut down.
     * @param Callback NullResultCallback : the callback to execute on completion of the flush operation.
     * @pre The user must be logged in to send an Analytics Record to the backend services.
     */
    CSP_EVENT void FlushAnalyticsEventsQueue(NullResultCallback Callback);

    /**
     * @brief Retrieves the time since the queue was last sent.
     * @return std::chrono::milliseconds : time since epoch in milliseconds.
     */
    CSP_NO_EXPORT std::chrono::milliseconds GetTimeSinceLastQueueSend() const { return TimeSinceLastQueueSend; }

    /**
     * @brief Retrieves the rate at which the queued Analytics Records are sent.
     * @return std::chrono::milliseconds : send rate in milliseconds.
     */
    CSP_NO_EXPORT std::chrono::milliseconds GetQueueSendRate() const { return AnalyticsQueueSendRate; }

    /**
     * @brief Retrieves the current size of the Analytics Records queue.
     * @return size_t : the queue size.
     */
    CSP_NO_EXPORT size_t GetCurrentQueueSize() const { return AnalyticsRecordQueue.size(); }

    /**
     * @brief Retrieves the max permitted size of the Analytics Records queue.
     * If the queue size reaches this value, the queue will be sent as a single batch to the backend services.
     * @return size_t : the queue size at which a batch will be sent.
     */
    CSP_NO_EXPORT size_t GetMaxQueueSize() const { return MaxQueueSize; }

private:
    AnalyticsSystem(); // This constructor is only provided to appease the wrapper generator and should not be used
    CSP_NO_EXPORT AnalyticsSystem(csp::web::WebClient* InWebClient, const csp::ClientUserAgent* AgentInfo, common::LogSystem& LogSystem);
    ~AnalyticsSystem();

    void SetTimeSinceLastQueueSend(std::chrono::milliseconds NewTimeSinceLastQueueSend);

    // This is a utility function to allow us to test the queueing functionality in a reasonable time frame.
    void __SetQueueSendRateAndMaxSize(std::chrono::milliseconds NewSendRate, size_t NewQueueSize);

    std::unique_ptr<csp::services::ApiBase> AnalyticsApi;

    std::unique_ptr<class AnalyticsQueueEventHandler> EventHandler;
    std::mutex AnalyticsQueueLock;
    std::vector<std::shared_ptr<csp::services::generated::userservice::AnalyticsRecord>> AnalyticsRecordQueue;

    const csp::ClientUserAgent* UserAgentInfo;
    std::chrono::milliseconds AnalyticsQueueSendRate;
    std::chrono::milliseconds TimeSinceLastQueueSend;
    size_t MaxQueueSize;
};

} // namespace csp::systems
