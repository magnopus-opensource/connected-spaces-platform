/*
* Copyright 2023 Magnopus LLC

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

#include "CSP/Systems/Quota/QuotaSystem.h"

#include "CSP/Systems/Users/UserSystem.h"
#include "Services/TrackingService/Api.h"

namespace chs = csp::services::generated::trackingservice;

namespace csp::systems
{
QuotaSystem::QuotaSystem()
    : SystemBase(nullptr, nullptr, nullptr)
    , QuotaTierAssignmentAPI(nullptr)
    , QuotaManagementAPI(nullptr)
    , QuotaActivityAPI(nullptr)
{
}

QuotaSystem::QuotaSystem(csp::web::WebClient* InWebClient, csp::common::LogSystem& LogSystem)
    : SystemBase(InWebClient, nullptr, &LogSystem)
{
    QuotaManagementAPI = new chs::QuotaManagementApi(InWebClient);
    QuotaTierAssignmentAPI = new chs::QuotaTierAssignmentApi(InWebClient);
    QuotaActivityAPI = new chs::QuotaActivityApi(InWebClient);
}

QuotaSystem::~QuotaSystem()
{
    delete (QuotaManagementAPI);
    delete (QuotaTierAssignmentAPI);
    delete (QuotaActivityAPI);
}

void QuotaSystem::GetTotalSpacesOwnedByUser(FeatureLimitCallback Callback)
{
    std::vector<csp::common::String> FeatureNamesList = { TierFeatureEnumToString(TierFeatures::SpaceOwner) };

    csp::services::ResponseHandlerPtr ResponseHandler = QuotaManagementAPI->CreateHandler<FeatureLimitCallback, FeatureLimitResult, void,
        csp::services::DtoArray<chs::QuotaFeatureLimitProgressDto>>(Callback, nullptr);

    static_cast<chs::QuotaActivityApi*>(QuotaManagementAPI)
        ->usersUserIdQuotaProgressGet(csp::systems::SystemsManager::Get().GetUserSystem()->GetLoginState().UserId, FeatureNamesList, ResponseHandler);
}

void QuotaSystem::GetConcurrentUsersInSpace(const csp::common::String& SpaceId, FeatureLimitCallback Callback)
{
    std::vector<csp::common::String> FeatureNamesList = { TierFeatureEnumToString(TierFeatures::ScopeConcurrentUsers) };

    csp::services::ResponseHandlerPtr ResponseHandler = QuotaManagementAPI->CreateHandler<FeatureLimitCallback, FeatureLimitResult, void,
        csp::services::DtoArray<chs::QuotaFeatureLimitProgressDto>>(Callback, nullptr);

    static_cast<chs::QuotaActivityApi*>(QuotaManagementAPI)->groupsGroupIdQuotaProgressGet(SpaceId.c_str(), FeatureNamesList, ResponseHandler);
}

void QuotaSystem::GetTotalSpaceSizeInKilobytes(const csp::common::String& SpaceId, FeatureLimitCallback Callback)
{
    std::vector<csp::common::String> FeatureNamesList = { TierFeatureEnumToString(TierFeatures::TotalUploadSizeInKilobytes) };

    csp::services::ResponseHandlerPtr ResponseHandler = QuotaManagementAPI->CreateHandler<FeatureLimitCallback, FeatureLimitResult, void,
        csp::services::DtoArray<chs::QuotaFeatureLimitProgressDto>>(Callback, nullptr);

    static_cast<chs::QuotaActivityApi*>(QuotaManagementAPI)->groupsGroupIdQuotaProgressGet(SpaceId, FeatureNamesList, ResponseHandler);
}

void QuotaSystem::GetTierFeatureProgressForUser(const csp::common::Array<TierFeatures>& FeatureNames, FeaturesLimitCallback Callback)
{
    csp::services::ResponseHandlerPtr ResponseHandler = QuotaManagementAPI->CreateHandler<FeaturesLimitCallback, FeaturesLimitResult, void,
        csp::services::DtoArray<chs::QuotaFeatureLimitProgressDto>>(Callback, nullptr);

    std::vector<csp::common::String> FeatureNamesList;
    FeatureNamesList.reserve(FeatureNames.Size());

    for (size_t idx = 0; idx < FeatureNames.Size(); ++idx)
    {
        FeatureNamesList.push_back(TierFeatureEnumToString(FeatureNames[idx]));
    }

    static_cast<chs::QuotaActivityApi*>(QuotaManagementAPI)
        ->usersUserIdQuotaProgressGet(csp::systems::SystemsManager::Get().GetUserSystem()->GetLoginState().UserId, FeatureNamesList, ResponseHandler);
}

void QuotaSystem::GetTierFeatureProgressForSpace(
    const csp::common::String& SpaceId, const csp::common::Array<TierFeatures>& FeatureNames, FeaturesLimitCallback Callback)
{
    csp::services::ResponseHandlerPtr ResponseHandler = QuotaActivityAPI->CreateHandler<FeaturesLimitCallback, FeaturesLimitResult, void,
        csp::services::DtoArray<chs::QuotaFeatureLimitProgressDto>>(Callback, nullptr);

    std::vector<csp::common::String> FeatureNamesList;
    FeatureNamesList.reserve(FeatureNames.Size());

    for (size_t idx = 0; idx < FeatureNames.Size(); ++idx)
    {
        FeatureNamesList.push_back(TierFeatureEnumToString(FeatureNames[idx]));
    }

    static_cast<chs::QuotaActivityApi*>(QuotaManagementAPI)->groupsGroupIdQuotaProgressGet(SpaceId, FeatureNamesList, ResponseHandler);
}

void QuotaSystem::GetCurrentUserTier(UserTierCallback Callback)
{
    csp::services::ResponseHandlerPtr ResponseHandler
        = QuotaTierAssignmentAPI->CreateHandler<UserTierCallback, UserTierResult, void, chs::QuotaTierAssignmentDto>(Callback, nullptr);

    static_cast<chs::QuotaTierAssignmentApi*>(QuotaTierAssignmentAPI)
        ->usersUserIdTierAssignmentGet(csp::systems::SystemsManager::Get().GetUserSystem()->GetLoginState().UserId, ResponseHandler);
}

void QuotaSystem::GetTierFeatureQuota(TierNames TierName, TierFeatures FeatureName, FeatureQuotaCallback Callback)
{
    csp::services::ResponseHandlerPtr ResponseHandler
        = QuotaManagementAPI->CreateHandler<FeatureQuotaCallback, FeatureQuotaResult, void, chs::QuotaFeatureTierDto>(Callback, nullptr);

    static_cast<chs::QuotaManagementApi*>(QuotaManagementAPI)
        ->tiersTierNameFeaturesFeatureNameQuotaGet(TierNameEnumToString(TierName), TierFeatureEnumToString(FeatureName), ResponseHandler);
}

void QuotaSystem::GetTierFeaturesQuota(TierNames TierName, FeaturesQuotaCallback Callback)
{
    csp::services::ResponseHandlerPtr ResponseHandler
        = QuotaManagementAPI->CreateHandler<FeaturesQuotaCallback, FeaturesQuotaResult, void, csp::services::DtoArray<chs::QuotaFeatureTierDto>>(
            Callback, nullptr);

    static_cast<chs::QuotaManagementApi*>(QuotaManagementAPI)->tiersTierNameQuotasGet(TierNameEnumToString(TierName), ResponseHandler);
}
} // namespace csp::systems
