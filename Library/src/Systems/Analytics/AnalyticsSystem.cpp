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

#include "CSP/Systems/Analytics/AnalyticsSystem.h"

#include "CSP/CSPFoundation.h"
#include "CallHelpers.h"
#include "Events/EventListener.h"
#include "Events/EventSystem.h"
#include "Services/UserService/Api.h"
#include "Services/UserService/Dto.h"
#include "Systems/ResultHelpers.h"

#include <fmt/format.h>

using namespace csp;
using namespace csp::common;

namespace chs = csp::services::generated::userservice;

namespace
{

std::shared_ptr<chs::AnalyticsRecord> CreateAnalyticsRecord(const csp::ClientUserAgent* userAgentInfo, const String& productContextSection,
    const String& category, const String& interactionType, const Optional<String>& subCategory, const Optional<Map<String, String>>& metadata)
{
    // Construct an AnalyticsRecord object
    auto record = std::make_shared<chs::AnalyticsRecord>();
    record->SetProductIdentifier("csp");
    record->SetProductContext(userAgentInfo->ClientSKU);
    record->SetTenantName(CSPFoundation::GetTenant());
    record->SetProductContextSection(productContextSection);
    record->SetCategory(category);
    record->SetInteractionType(interactionType);

    if (subCategory.HasValue())
    {
        record->SetSubCategory(*subCategory);
    }

    String clientVersion
        = fmt::format("{}/{}({})", userAgentInfo->ClientSKU.c_str(), userAgentInfo->ClientVersion.c_str(), userAgentInfo->ClientEnvironment.c_str())
              .c_str();
    String cspVersion = fmt::format("CSP/{}({})", CSPFoundation::GetVersion().c_str(), CSPFoundation::GetBuildType().c_str()).c_str();

    std::vector<String> versionsMatrix { clientVersion, cspVersion };
    record->SetVersionMatrix(versionsMatrix);

    if (metadata.HasValue())
    {
        std::map<String, String> analyticsRecordMetadata = metadata->GetUnderlying();
        record->SetMetadata(analyticsRecordMetadata);
    }

    record->SetTimestamp(DateTime::TimeNow().GetUtcString());

    return record;
}

}

namespace csp::systems
{

class AnalyticsQueueEventHandler : public csp::events::EventListener
{
public:
    AnalyticsQueueEventHandler(AnalyticsSystem* analyticsSystem);

    void OnEvent(const csp::events::Event& inEvent) override;

private:
    AnalyticsSystem* m_analyticsSystem;
};

AnalyticsQueueEventHandler::AnalyticsQueueEventHandler(AnalyticsSystem* analyticsSystem)
    : m_analyticsSystem(analyticsSystem)
{
}

void AnalyticsQueueEventHandler::OnEvent(const csp::events::Event& inEvent)
{
    if (inEvent.GetId() == csp::events::FOUNDATION_TICK_EVENT_ID && m_analyticsSystem != nullptr)
    {
        const std::chrono::milliseconds currentTime
            = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

        if (currentTime - m_analyticsSystem->GetTimeSinceLastQueueSend() >= m_analyticsSystem->GetQueueSendRate()
            || m_analyticsSystem->GetCurrentQueueSize() >= m_analyticsSystem->GetMaxQueueSize())
        {
            m_analyticsSystem->FlushAnalyticsEventsQueue([](const NullResult& /*Result*/) {});
        }
    }
}

AnalyticsSystem::AnalyticsSystem()
    : SystemBase(nullptr, nullptr, nullptr)
    , m_analyticsApi(nullptr)
    , m_eventHandler(nullptr)
    , m_analyticsQueueLock()
    , m_analyticsRecordQueue()
    , m_userAgentInfo(nullptr)
    , m_analyticsQueueSendRate(std::chrono::seconds(60))
    , m_timeSinceLastQueueSend(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()))
    , m_maxQueueSize(25)
{
}

AnalyticsSystem::AnalyticsSystem(csp::web::WebClient* inWebClient, const csp::ClientUserAgent* agentInfo, csp::common::LogSystem& logSystem)
    : SystemBase(inWebClient, nullptr, &logSystem)
    , m_analyticsApi(std::make_unique<chs::AnalyticsApi>(inWebClient))
    , m_eventHandler(std::make_unique<AnalyticsQueueEventHandler>(this))
    , m_analyticsQueueLock()
    , m_analyticsRecordQueue()
    , m_userAgentInfo(agentInfo)
    , m_analyticsQueueSendRate(std::chrono::seconds(60))
    , m_timeSinceLastQueueSend(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()))
    , m_maxQueueSize(25)
{
    csp::events::EventSystem::Get().RegisterListener(csp::events::FOUNDATION_TICK_EVENT_ID, m_eventHandler.get());
}

AnalyticsSystem::~AnalyticsSystem() { csp::events::EventSystem::Get().UnRegisterListener(csp::events::FOUNDATION_TICK_EVENT_ID, m_eventHandler.get()); }

void AnalyticsSystem::SetTimeSinceLastQueueSend(std::chrono::milliseconds newTimeSinceLastQueueSend)
{
    m_timeSinceLastQueueSend = newTimeSinceLastQueueSend;
}

// This is a utility function to allow us to test the batching functionality in a reasonable time frame.
void AnalyticsSystem::__SetQueueSendRateAndMaxSize(std::chrono::milliseconds newSendRate, size_t newQueueSize)
{
    std::scoped_lock analyticsQueueLocker(m_analyticsQueueLock);

    m_analyticsQueueSendRate = newSendRate;
    m_maxQueueSize = newQueueSize;
}

void AnalyticsSystem::SendAnalyticsEvent(const String& productContextSection, const String& category, const String& interactionType,
    const Optional<String>& subCategory, const Optional<Map<String, String>>& metadata, NullResultCallback callback)
{
    if (productContextSection.IsEmpty() || category.IsEmpty() || interactionType.IsEmpty())
    {
        CSP_LOG_MSG(common::LogLevel::Error,
            "ProductContextSection, Category and InteractionType are required fields for the Analytics Event and must be provided.");
        INVOKE_IF_NOT_NULL(callback, MakeInvalid<NullResult>());

        return;
    }

    auto record = CreateAnalyticsRecord(m_userAgentInfo, productContextSection, category, interactionType, subCategory, metadata);

    std::vector<std::shared_ptr<chs::AnalyticsRecord>> records;
    records.push_back(record);

    NullResultCallback sendAnalyticsCallback = [logSystem = this->m_logSystem, callback](const NullResult& result)
    {
        if (result.GetResultCode() == csp::systems::EResultCode::InProgress)
        {
            return;
        }

        if (result.GetResultCode() == csp::systems::EResultCode::Failed)
        {
            logSystem->LogMsg(common::LogLevel::Error,
                fmt::format("Failed to send Analytics Event. ResCode: {}, HttpResCode: {}", static_cast<int>(result.GetResultCode()),
                    result.GetHttpResultCode())
                    .c_str());
        }

        INVOKE_IF_NOT_NULL(callback, result);
    };

    csp::services::ResponseHandlerPtr responseHandler
        = m_analyticsApi->CreateHandler<NullResultCallback, NullResult, void, chs::AnalyticsRecord>(sendAnalyticsCallback, nullptr);

    static_cast<chs::AnalyticsApi*>(m_analyticsApi.get())->analyticsBulkPost({ records }, responseHandler);
}

void AnalyticsSystem::QueueAnalyticsEvent(const String& productContextSection, const String& category, const String& interactionType,
    const Optional<String>& subCategory, const Optional<Map<String, String>>& metadata)
{
    if (productContextSection.IsEmpty() || category.IsEmpty() || interactionType.IsEmpty())
    {
        CSP_LOG_MSG(common::LogLevel::Error,
            "ProductContextSection, Category and InteractionType are required fields for the Analytics Event and must be provided.");

        return;
    }

    auto record = CreateAnalyticsRecord(m_userAgentInfo, productContextSection, category, interactionType, subCategory, metadata);

    std::scoped_lock analyticsQueueLocker(m_analyticsQueueLock);
    m_analyticsRecordQueue.emplace_back(record);
}

void AnalyticsSystem::FlushAnalyticsEventsQueue(NullResultCallback callback)
{
    std::scoped_lock analyticsQueueLocker(m_analyticsQueueLock);

    if (m_analyticsRecordQueue.empty())
    {
        // Return Success and ResponseNoContent to indicate that the flush operation was successful but there were no records to send.
        NullResult result(csp::systems::EResultCode::Success, 204);
        INVOKE_IF_NOT_NULL(callback, result);

        return;
    }

    const std::chrono::milliseconds currentTime
        = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

    SetTimeSinceLastQueueSend(currentTime);

    NullResultCallback sendBatchAnalyticsCallback = [logSystem = this->m_logSystem, callback](const NullResult& result)
    {
        if (result.GetResultCode() == csp::systems::EResultCode::InProgress)
        {
            return;
        }

        if (result.GetResultCode() == csp::systems::EResultCode::Success)
        {
            logSystem->LogMsg(common::LogLevel::Verbose, "Successfully sent the Analytics Record queue.");
        }
        else if (result.GetResultCode() == csp::systems::EResultCode::Failed)
        {
            logSystem->LogMsg(common::LogLevel::Error,
                fmt::format("Failed to send Analytics Event. ResCode: {}, HttpResCode: {}", static_cast<int>(result.GetResultCode()),
                    result.GetHttpResultCode())
                    .c_str());
        }

        if (callback)
        {
            INVOKE_IF_NOT_NULL(callback, result);
        }
    };

    csp::services::ResponseHandlerPtr responseHandler
        = m_analyticsApi->CreateHandler<NullResultCallback, NullResult, void, chs::AnalyticsRecord>(sendBatchAnalyticsCallback, nullptr);

    static_cast<chs::AnalyticsApi*>(m_analyticsApi.get())->analyticsBulkPost({ m_analyticsRecordQueue }, responseHandler);

    // Clear the analytics record queue.
    // The async analyticsBulkPost endpoint serializes the records data to json so it is safe to clear here.
    m_analyticsRecordQueue.clear();
}

} // namespace csp::systems
