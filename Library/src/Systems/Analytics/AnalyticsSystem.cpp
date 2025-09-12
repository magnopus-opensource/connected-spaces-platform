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

class AnalyticsBatchEventHandler : public csp::events::EventListener
{
public:
    AnalyticsBatchEventHandler(AnalyticsSystem* AnalyticsSystem);

    void OnEvent(const csp::events::Event& InEvent) override;

private:
    AnalyticsSystem* _AnalyticsSystem;
};

AnalyticsBatchEventHandler::AnalyticsBatchEventHandler(AnalyticsSystem* AnalyticsSystem)
    : _AnalyticsSystem(AnalyticsSystem)
{
}

void AnalyticsBatchEventHandler::OnEvent(const csp::events::Event& InEvent)
{
    if (InEvent.GetId() == csp::events::FOUNDATION_TICK_EVENT_ID && _AnalyticsSystem != nullptr)
    {
        _AnalyticsSystem->ProcessBatchedAnalyticsRecords();
    }
}

AnalyticsSystem::AnalyticsSystem()
    : SystemBase(nullptr, nullptr, nullptr)
    , EventHandler(nullptr)
    , AnalyticsBatchLock(new std::recursive_mutex)
    , AnalyticsApi(nullptr)
    , AnalyticsRecordBatch(nullptr)
    , UserAgentInfo(nullptr)
    , AnalyticsBatchRate(std::chrono::seconds(60))
    , TimeOfLastBatch(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()))
    , MaxBatchSize(25)
    , IsFlushingAnalyticsEvents(false)
{
}

AnalyticsSystem::AnalyticsSystem(csp::web::WebClient* InWebClient, const csp::ClientUserAgent* AgentInfo, csp::common::LogSystem& LogSystem)
    : SystemBase(InWebClient, nullptr, &LogSystem)
    , EventHandler(new AnalyticsBatchEventHandler(this))
    , AnalyticsBatchLock(new std::recursive_mutex)
    , AnalyticsRecordBatch(new(std::deque<std::shared_ptr<chs::AnalyticsRecord>>))
    , UserAgentInfo(AgentInfo)
    , AnalyticsBatchRate(std::chrono::seconds(60))
    , TimeOfLastBatch(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()))
    , MaxBatchSize(25)
    , IsFlushingAnalyticsEvents(false)
{
    AnalyticsApi = std::unique_ptr<csp::services::ApiBase>(new chs::AnalyticsApi(InWebClient));

    csp::events::EventSystem::Get().RegisterListener(csp::events::FOUNDATION_TICK_EVENT_ID, EventHandler);
}

AnalyticsSystem::~AnalyticsSystem()
{
    csp::events::EventSystem::Get().UnRegisterListener(csp::events::FOUNDATION_TICK_EVENT_ID, EventHandler);

    delete (EventHandler);
    delete (AnalyticsBatchLock);
    delete (AnalyticsRecordBatch);
}

std::chrono::milliseconds AnalyticsSystem::GetTimeOfLastBatch() const { return TimeOfLastBatch; }

void AnalyticsSystem::SetTimeOfLastBatch(std::chrono::milliseconds NewTimeOfLastBatch) { TimeOfLastBatch = NewTimeOfLastBatch; }

// This is a utility function to allow us to test the batching functionality in a reasonable time frame.
void AnalyticsSystem::SetBatchRateAndSize(std::chrono::milliseconds NewBatchRate, size_t NewBatchSize)
{
    std::scoped_lock AnalyticsBatchLocker(*AnalyticsBatchLock);

    AnalyticsBatchRate = NewBatchRate;
    MaxBatchSize = NewBatchSize;
}

void AnalyticsSystem::SendAnalyticsEvent(const String& ProductContextSection, const String& Category, const String& InteractionType,
    const Optional<String>& SubCategory, const Optional<Map<String, String>>& Metadata, NullResultCallback Callback)
{
    if (ProductContextSection.IsEmpty() || Category.IsEmpty() || InteractionType.IsEmpty())
    {
        CSP_LOG_MSG(common::LogLevel::Error, "Missing the required fields for the Analytics Event.");
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

            NullResult ErrorResult(Result.GetResultCode(), Result.GetHttpResultCode());
            Callback(ErrorResult);

            return;
        }

        NullResult SuccessResult(Result.GetResultCode(), Result.GetHttpResultCode());
        Callback(SuccessResult);
    };

    csp::services::ResponseHandlerPtr ResponseHandler
        = AnalyticsApi->CreateHandler<NullResultCallback, NullResult, void, chs::AnalyticsRecord>(SendAnalyticsCallback, nullptr);

    static_cast<chs::AnalyticsApi*>(AnalyticsApi.get())->analyticsBulkPost(Records, ResponseHandler);
}

void AnalyticsSystem::BatchAnalyticsEvent(const String& ProductContextSection, const String& Category, const String& InteractionType,
    const Optional<String>& SubCategory, const Optional<Map<String, String>>& Metadata)
{
    if (ProductContextSection.IsEmpty() || Category.IsEmpty() || InteractionType.IsEmpty())
    {
        CSP_LOG_MSG(common::LogLevel::Error, "Missing the required fields for the Analytics Event.");

        return;
    }

    auto Record = CreateAnalyticsRecord(UserAgentInfo, ProductContextSection, Category, InteractionType, SubCategory, Metadata);

    std::scoped_lock AnalyticsBatchLocker(*AnalyticsBatchLock);
    AnalyticsRecordBatch->emplace_back(Record);
}

void AnalyticsSystem::ProcessBatchedAnalyticsRecords()
{
    std::scoped_lock AnalyticsBatchLocker(*AnalyticsBatchLock);

    if (AnalyticsRecordBatch->empty())
    {
        return;
    }

    const std::chrono::milliseconds CurrentTime
        = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

    if (CurrentTime - GetTimeOfLastBatch() >= AnalyticsBatchRate)
    {
        FlushAnalyticsEvents();
        return;
    }

    if (AnalyticsRecordBatch->size() >= MaxBatchSize)
    {
        FlushAnalyticsEvents();
        return;
    }
}

void AnalyticsSystem::FlushAnalyticsEvents()
{
    std::scoped_lock AnalyticsBatchLocker(*AnalyticsBatchLock);

    if (AnalyticsRecordBatch->empty())
    {
        return;
    }

    const std::chrono::milliseconds CurrentTime
        = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

    SetTimeOfLastBatch(CurrentTime);

    std::vector<std::shared_ptr<chs::AnalyticsRecord>> Records;
    Records.reserve(AnalyticsRecordBatch->size());

    while (AnalyticsRecordBatch->empty() == false)
    {
        auto Record = AnalyticsRecordBatch->front();

        Records.push_back(Record);

        AnalyticsRecordBatch->pop_front();
    }

    NullResultCallback SendBatchAnalyticsCallback = [LogSystem = this->LogSystem, BatchSize = Records.size()](const NullResult& Result)
    {
        if (Result.GetResultCode() == csp::systems::EResultCode::Success)
        {
            LogSystem->LogMsg(common::LogLevel::Verbose, "Batched Analytics Events sent.");
        }
        else if (Result.GetResultCode() == csp::systems::EResultCode::Failed)
        {
            LogSystem->LogMsg(common::LogLevel::Error,
                fmt::format("Failed to send Batch Analytics Event. ResCode: {}, HttpResCode: {}", static_cast<int>(Result.GetResultCode()),
                    Result.GetHttpResultCode())
                    .c_str());
        }
    };

    csp::services::ResponseHandlerPtr ResponseHandler
        = AnalyticsApi->CreateHandler<NullResultCallback, NullResult, void, chs::AnalyticsRecord>(SendBatchAnalyticsCallback, nullptr);

    static_cast<chs::AnalyticsApi*>(AnalyticsApi.get())->analyticsBulkPost(Records, ResponseHandler);
}

} // namespace csp::systems
