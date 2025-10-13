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

std::shared_ptr<chs::AnalyticsRecord> CreateAnalyticsRecord(const csp::ClientUserAgent* UserAgentInfo, const String& ProductContextSection,
    const String& Category, const String& InteractionType, const Optional<String>& SubCategory, const Optional<Map<String, String>>& Metadata)
{
    // Construct an AnalyticsRecord object
    auto Record = std::make_shared<chs::AnalyticsRecord>();
    Record->SetProductIdentifier("csp");
    Record->SetProductContext(UserAgentInfo->ClientSKU);
    Record->SetTenantName(CSPFoundation::GetTenant());
    Record->SetProductContextSection(ProductContextSection);
    Record->SetCategory(Category);
    Record->SetInteractionType(InteractionType);

    if (SubCategory.HasValue())
    {
        Record->SetSubCategory(*SubCategory);
    }

    String ClientVersion
        = fmt::format("{}/{}({})", UserAgentInfo->ClientSKU.c_str(), UserAgentInfo->ClientVersion.c_str(), UserAgentInfo->ClientEnvironment.c_str())
              .c_str();
    String CSPVersion = fmt::format("CSP/{}({})", CSPFoundation::GetVersion().c_str(), CSPFoundation::GetBuildType().c_str()).c_str();

    std::vector<String> VersionsMatrix { ClientVersion, CSPVersion };
    Record->SetVersionMatrix(VersionsMatrix);

    if (Metadata.HasValue())
    {
        std::map<String, String> AnalyticsRecordMetadata = Metadata->GetUnderlying();
        Record->SetMetadata(AnalyticsRecordMetadata);
    }

    Record->SetTimestamp(DateTime::TimeNow().GetUtcString());

    return Record;
}

}

namespace csp::systems
{

class AnalyticsQueueEventHandler : public csp::events::EventListener
{
public:
    AnalyticsQueueEventHandler(AnalyticsSystem* AnalyticsSystem);

    void OnEvent(const csp::events::Event& InEvent) override;

private:
    AnalyticsSystem* _AnalyticsSystem;
};

AnalyticsQueueEventHandler::AnalyticsQueueEventHandler(AnalyticsSystem* AnalyticsSystem)
    : _AnalyticsSystem(AnalyticsSystem)
{
}

void AnalyticsQueueEventHandler::OnEvent(const csp::events::Event& InEvent)
{
    if (InEvent.GetId() == csp::events::FOUNDATION_TICK_EVENT_ID && _AnalyticsSystem != nullptr)
    {
        const std::chrono::milliseconds CurrentTime
            = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

        if (CurrentTime - _AnalyticsSystem->GetTimeSinceLastQueueSend() >= _AnalyticsSystem->GetQueueSendRate()
            || _AnalyticsSystem->GetCurrentQueueSize() >= _AnalyticsSystem->GetMaxQueueSize())
        {
            _AnalyticsSystem->FlushAnalyticsEventsQueue([](const NullResult& /*Result*/) {});
        }
    }
}

AnalyticsSystem::AnalyticsSystem()
    : SystemBase(nullptr, nullptr, nullptr)
    , AnalyticsApi(nullptr)
    , EventHandler(nullptr)
    , AnalyticsQueueLock()
    , AnalyticsRecordQueue()
    , UserAgentInfo(nullptr)
    , AnalyticsQueueSendRate(std::chrono::seconds(60))
    , TimeSinceLastQueueSend(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()))
    , MaxQueueSize(25)
{
}

AnalyticsSystem::AnalyticsSystem(csp::web::WebClient* InWebClient, const csp::ClientUserAgent* AgentInfo, csp::common::LogSystem& LogSystem)
    : SystemBase(InWebClient, nullptr, &LogSystem)
    , AnalyticsApi(std::make_unique<chs::AnalyticsApi>(InWebClient))
    , EventHandler(std::make_unique<AnalyticsQueueEventHandler>(this))
    , AnalyticsQueueLock()
    , AnalyticsRecordQueue()
    , UserAgentInfo(AgentInfo)
    , AnalyticsQueueSendRate(std::chrono::seconds(60))
    , TimeSinceLastQueueSend(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()))
    , MaxQueueSize(25)
{
    csp::events::EventSystem::Get().RegisterListener(csp::events::FOUNDATION_TICK_EVENT_ID, EventHandler.get());
}

AnalyticsSystem::~AnalyticsSystem() { csp::events::EventSystem::Get().UnRegisterListener(csp::events::FOUNDATION_TICK_EVENT_ID, EventHandler.get()); }

void AnalyticsSystem::SetTimeSinceLastQueueSend(std::chrono::milliseconds NewTimeSinceLastQueueSend)
{
    TimeSinceLastQueueSend = NewTimeSinceLastQueueSend;
}

// This is a utility function to allow us to test the batching functionality in a reasonable time frame.
void AnalyticsSystem::__SetQueueSendRateAndMaxSize(std::chrono::milliseconds NewSendRate, size_t NewQueueSize)
{
    std::scoped_lock AnalyticsQueueLocker(AnalyticsQueueLock);

    AnalyticsQueueSendRate = NewSendRate;
    MaxQueueSize = NewQueueSize;
}

void AnalyticsSystem::SendAnalyticsEvent(const String& ProductContextSection, const String& Category, const String& InteractionType,
    const Optional<String>& SubCategory, const Optional<Map<String, String>>& Metadata, NullResultCallback Callback)
{
    if (ProductContextSection.IsEmpty() || Category.IsEmpty() || InteractionType.IsEmpty())
    {
        CSP_LOG_MSG(common::LogLevel::Error,
            "ProductContextSection, Category and InteractionType are required fields for the Analytics Event and must be provided.");
        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<NullResult>());

        return;
    }

    auto Record = CreateAnalyticsRecord(UserAgentInfo, ProductContextSection, Category, InteractionType, SubCategory, Metadata);

    std::vector<std::shared_ptr<chs::AnalyticsRecord>> Records;
    Records.push_back(Record);

    NullResultCallback SendAnalyticsCallback = [LogSystem = this->LogSystem, Callback](const NullResult& Result)
    {
        if (Result.GetResultCode() == csp::systems::EResultCode::InProgress)
        {
            return;
        }

        if (Result.GetResultCode() == csp::systems::EResultCode::Failed)
        {
            LogSystem->LogMsg(common::LogLevel::Error,
                fmt::format("Failed to send Analytics Event. ResCode: {}, HttpResCode: {}", static_cast<int>(Result.GetResultCode()),
                    Result.GetHttpResultCode())
                    .c_str());
        }

        INVOKE_IF_NOT_NULL(Callback, Result);
    };

    csp::services::ResponseHandlerPtr ResponseHandler
        = AnalyticsApi->CreateHandler<NullResultCallback, NullResult, void, chs::AnalyticsRecord>(SendAnalyticsCallback, nullptr);

    static_cast<chs::AnalyticsApi*>(AnalyticsApi.get())->analyticsBulkPost({ Records }, ResponseHandler);
}

void AnalyticsSystem::QueueAnalyticsEvent(const String& ProductContextSection, const String& Category, const String& InteractionType,
    const Optional<String>& SubCategory, const Optional<Map<String, String>>& Metadata)
{
    if (ProductContextSection.IsEmpty() || Category.IsEmpty() || InteractionType.IsEmpty())
    {
        CSP_LOG_MSG(common::LogLevel::Error,
            "ProductContextSection, Category and InteractionType are required fields for the Analytics Event and must be provided.");

        return;
    }

    auto Record = CreateAnalyticsRecord(UserAgentInfo, ProductContextSection, Category, InteractionType, SubCategory, Metadata);

    std::scoped_lock AnalyticsQueueLocker(AnalyticsQueueLock);
    AnalyticsRecordQueue.emplace_back(Record);
}

void AnalyticsSystem::FlushAnalyticsEventsQueue(NullResultCallback Callback)
{
    std::scoped_lock AnalyticsQueueLocker(AnalyticsQueueLock);

    if (AnalyticsRecordQueue.empty())
    {
        return;
    }

    const std::chrono::milliseconds CurrentTime
        = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

    SetTimeSinceLastQueueSend(CurrentTime);

    NullResultCallback SendBatchAnalyticsCallback = [this, Callback](const NullResult& Result)
    {
        if (Result.GetResultCode() == csp::systems::EResultCode::InProgress)
        {
            return;
        }

        if (Result.GetResultCode() == csp::systems::EResultCode::Success)
        {
            this->LogSystem->LogMsg(common::LogLevel::Verbose, "Successfully sent the Analytics Record queue.");
        }
        else if (Result.GetResultCode() == csp::systems::EResultCode::Failed)
        {
            this->LogSystem->LogMsg(common::LogLevel::Error,
                fmt::format("Failed to send Analytics Event. ResCode: {}, HttpResCode: {}", static_cast<int>(Result.GetResultCode()),
                    Result.GetHttpResultCode())
                    .c_str());
        }

        // Note: We may in the future wish to consider a retry mechanism when there is a failure to send the AnalyticsRecordQueue.
        // For now we are just clearing the queue in either case.
        this->AnalyticsRecordQueue.clear();

        if (Callback)
        {
            INVOKE_IF_NOT_NULL(Callback, Result);
        }
    };

    csp::services::ResponseHandlerPtr ResponseHandler
        = AnalyticsApi->CreateHandler<NullResultCallback, NullResult, void, chs::AnalyticsRecord>(SendBatchAnalyticsCallback, nullptr);

    static_cast<chs::AnalyticsApi*>(AnalyticsApi.get())->analyticsBulkPost({ AnalyticsRecordQueue }, ResponseHandler);
}

} // namespace csp::systems
