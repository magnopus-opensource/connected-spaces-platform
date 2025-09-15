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
#include <deque>
#include <mutex>

CSP_START_IGNORE
#ifdef CSP_TESTS
class CSPEngine_AnalyticsSystemTests_BatchSendAnalyticsEventBatchRateTest_Test;
class CSPEngine_AnalyticsSystemTests_BatchSendAnalyticsEventBatchSizeTest_Test;
class CSPEngine_AnalyticsSystemTests_FlushAnalyticsEventsBatchTest_Test;
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
/// @brief Public facing system that allows AnalyticsRecords to be sent to MCS.
/// @invariant Users must be logged in to send AnalyticsRecords to MCS.
class CSP_API AnalyticsSystem : public SystemBase
{
    CSP_START_IGNORE
    /** @cond DO_NOT_DOCUMENT */
    friend class SystemsManager;
    friend class AnalyticsBatchEventHandler;

#ifdef CSP_TESTS
    friend class ::CSPEngine_AnalyticsSystemTests_BatchSendAnalyticsEventBatchRateTest_Test;
    friend class ::CSPEngine_AnalyticsSystemTests_BatchSendAnalyticsEventBatchSizeTest_Test;
    friend class ::CSPEngine_AnalyticsSystemTests_FlushAnalyticsEventsBatchTest_Test;
#endif
    /** @endcond */
    CSP_END_IGNORE

public:
    /**
     * @brief Constructs an Analytics Record which is added to a queue to be sent to MCS in a single batch.
     * @details The batch will be sent when one of the following conditions are met:
     * 1. The time since the last batch was sent reaches the batch rate (default 60 seconds).
     * 2. The number of events in the queue reaches the maximum batch size (default 25 events).
     * 3. The client application calls FlushAnalyticsEvents() as part of their log out or shut down procedure to force the batch to be sent.
     * For more information about flushing events see the method documentation @ref AnalyticsSystem::FlushAnalyticsEvents().
     *
     * Example: Consider the following user action that is to be captured as an analytics event:
     * A [web client] user [clicks] on a [menu] item in the [UI].
     *
     * In this example:
     * - [web client] is captured internally.
     * - [clicks] is the InteractionType.
     * - [menu] is the Category.
     * - [UI] is the ProductContextSection.
     *
     * @note The following data is captured internally and included in the analytics record: tenant name, client sku, client version and client build
     * environment.
     *
     * @pre The user must be logged in to send Analytics Records to MCS.
     * @param ProductContextSection const csp::common::String& : Section of the client or runtime-context.
     * @param Category const csp::common::String& : Categorization field.
     * @param InteractionType const csp::common::String& : The interaction that occurred.
     * @param SubCategory const csp::common::Optional<csp::common::String>& : Optional sub-category field.
     * @param Metadata const csp::common::Optional<csp::common::Map<csp::common::String, csp::common::String>>& : Optional analytics event metadata.
     * Metadata is the event payload. It may be used to store such information as the space the user is in, their geographical region as well as
     * relevant device specs.
     */
    void BatchAnalyticsEvent(const csp::common::String& ProductContextSection, const csp::common::String& Category,
        const csp::common::String& InteractionType, const csp::common::Optional<csp::common::String>& SubCategory,
        const csp::common::Optional<csp::common::Map<csp::common::String, csp::common::String>>& Metadata);

    /**
     * @brief Constructs an Analytics Record which is immediately sent to MCS.
     * @note The BatchAnalyticsEvent method should be used by default as it will batch events before sending them.
     * This method will immediately send the analytics event and should therefore only be used when this behaviour is required.
     *
     * For more information about how the Analytics Record is constructed, see the documentation for @ref AnalyticsSystem::BatchAnalyticsEvent().
     *
     * @pre The user must be logged in to send an Analytics Record to MCS.
     * @param ProductContextSection const csp::common::String& : Section of the client or runtime-context.
     * @param Category const csp::common::String& : Categorization field.
     * @param InteractionType const csp::common::String& : The interaction that occurred.
     * @param SubCategory const csp::common::Optional<csp::common::String>& : Optional sub-category field.
     * @param Metadata const csp::common::Optional<csp::common::Map<csp::common::String, csp::common::String>>& : Optional analytics event metadata.
     * Metadata is the event payload. It may be used to store such information as the space the user is in, their geographical region as well as
     * relevant device specs.
     */
    void SendAnalyticsEvent(const csp::common::String& ProductContextSection, const csp::common::String& Category,
        const csp::common::String& InteractionType, const csp::common::Optional<csp::common::String>& SubCategory,
        const csp::common::Optional<csp::common::Map<csp::common::String, csp::common::String>>& Metadata, NullResultCallback Callback);

    /**
     * @brief Trigger immediate dispatch of the Analytics Records queue to MCS.
     * @note This method is designed to be called as part of a client applications log out and shutdown procedures to ensure that any queued analytics
     * records are flushed and sent to MCS before the user is logged out or the application is shut down.
     * @pre The user must be logged in to send an Analytics Record to MCS.
     */
    void FlushAnalyticsEvents();

private:
    AnalyticsSystem(); // This constructor is only provided to appease the wrapper generator and should not be used
    CSP_NO_EXPORT AnalyticsSystem(csp::web::WebClient* InWebClient, const csp::ClientUserAgent* AgentInfo, common::LogSystem& LogSystem);
    ~AnalyticsSystem();

    // Method called on Tick. This will send batched Analytics Records if the batch rate or size have been met.
    void ProcessBatchedAnalyticsRecords();

    std::chrono::milliseconds GetTimeOfLastBatch() const;
    void SetTimeOfLastBatch(std::chrono::milliseconds NewTimeOfLastBatch);

    // This is a utility function to allow us to test the batching functionality in a reasonable time frame.
    void SetBatchRateAndSize(std::chrono::milliseconds NewBatchRate, size_t NewBatchSize);

    class AnalyticsBatchEventHandler* EventHandler;
    std::recursive_mutex* AnalyticsBatchLock;

    std::unique_ptr<csp::services::ApiBase> AnalyticsApi;

    std::deque<std::shared_ptr<csp::services::generated::userservice::AnalyticsRecord>>* AnalyticsRecordBatch;
    const csp::ClientUserAgent* UserAgentInfo;
    std::chrono::milliseconds AnalyticsBatchRate;
    std::chrono::milliseconds TimeOfLastBatch;
    size_t MaxBatchSize;
    bool IsFlushingAnalyticsEvents;
};

} // namespace csp::systems
