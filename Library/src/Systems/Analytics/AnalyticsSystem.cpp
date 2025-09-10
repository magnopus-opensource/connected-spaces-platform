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
#include "Services/UserService/Api.h"
#include "Services/UserService/Dto.h"
#include "Systems/ResultHelpers.h"

#include <fmt/format.h>

using namespace csp;
using namespace csp::common;

namespace chs = csp::services::generated::userservice;

namespace csp::systems
{

AnalyticsSystem::AnalyticsSystem()
    : SystemBase(nullptr, nullptr, nullptr)
    , AnalyticsApi(nullptr)
    , UserAgentInfo(nullptr)
{
}

AnalyticsSystem::AnalyticsSystem(csp::web::WebClient* InWebClient, const csp::ClientUserAgent* AgentInfo, csp::common::LogSystem& LogSystem)
    : SystemBase(InWebClient, nullptr, &LogSystem)
{
    AnalyticsApi = std::unique_ptr<csp::services::ApiBase>(new chs::AnalyticsApi(InWebClient));
    UserAgentInfo = AgentInfo;
}

AnalyticsSystem::~AnalyticsSystem() { }

void AnalyticsSystem::SendAnalyticsEvent(const String& ProductContextSection, const String& Category, const String& InteractionType,
    const Optional<String>& SubCategory, const Optional<Map<String, String>>& Metadata, NullResultCallback Callback)
{
    if (ProductContextSection.IsEmpty() || Category.IsEmpty() || InteractionType.IsEmpty())
    {
        CSP_LOG_MSG(common::LogLevel::Error, "Missing the required fields for the Analytics Event.");
        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<NullResult>());

        return;
    }

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

    std::vector<std::shared_ptr<chs::AnalyticsRecord>> Records;
    Records.push_back(Record);

    NullResultCallback SendAnalyticsCallback = [Callback](const NullResult& Result)
    {
        if (Result.GetResultCode() == csp::systems::EResultCode::InProgress)
        {
            return;
        }

        if (Result.GetResultCode() == csp::systems::EResultCode::Failed)
        {
            CSP_LOG_FORMAT(common::LogLevel::Error, "Failed to send Analytics Event. ResCode: %d, HttpResCode: %d",
                static_cast<int>(Result.GetResultCode()), Result.GetHttpResultCode());

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

} // namespace csp::systems
