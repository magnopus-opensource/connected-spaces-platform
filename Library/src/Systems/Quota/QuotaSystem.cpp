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
#include "Systems/ResultHelpers.h"

#include "CSP/Common/Interfaces/IAuthContext.h"
#include "Services/TrackingService/Api.h"

namespace chs = csp::services::generated::trackingservice;

namespace csp::systems
{
QuotaSystem::QuotaSystem()
    : SystemBase(nullptr, nullptr, nullptr)
    , m_quotaTierAssignmentApi(nullptr)
    , m_quotaManagementApi(nullptr)
    , m_quotaActivityApi(nullptr)
    , m_context { nullptr }
{
}

QuotaSystem::QuotaSystem(csp::web::WebClient* inWebClient, csp::common::LogSystem& logSystem, const csp::common::IAuthContext& context)
    : SystemBase(inWebClient, nullptr, &logSystem)
    , m_context { &context }
{
    m_quotaManagementApi = new chs::QuotaManagementApi(inWebClient);
    m_quotaTierAssignmentApi = new chs::QuotaTierAssignmentApi(inWebClient);
    m_quotaActivityApi = new chs::QuotaActivityApi(inWebClient);
}

QuotaSystem::~QuotaSystem()
{
    delete (m_quotaManagementApi);
    delete (m_quotaTierAssignmentApi);
    delete (m_quotaActivityApi);
}

void QuotaSystem::GetTotalSpacesOwnedByUser(FeatureLimitCallback callback)
{
    if (m_context->GetLoginState().State != common::ELoginState::LoggedIn)
    {
        m_logSystem->LogMsg(common::LogLevel::Warning, "QuotaSystem::GetTotalSpacesOwnedByUser: User is not logged in. Aborting operation.");
        callback(MakeInvalid<FeatureLimitResult>());
        return;
    }

    std::vector<csp::common::String> featureNamesList = { TierFeatureEnumToString(TierFeatures::SpaceOwner) };

    csp::services::ResponseHandlerPtr responseHandler = m_quotaManagementApi->CreateHandler<FeatureLimitCallback, FeatureLimitResult, void,
        csp::services::DtoArray<chs::QuotaFeatureLimitProgressDto>>(callback, nullptr);

    static_cast<chs::QuotaActivityApi*>(m_quotaManagementApi)
        ->usersUserIdQuota_progressGet({ m_context->GetLoginState().UserId, featureNamesList }, responseHandler);
}

void QuotaSystem::GetConcurrentUsersInSpace(const csp::common::String& spaceId, FeatureLimitCallback callback)
{
    std::vector<csp::common::String> featureNamesList = { TierFeatureEnumToString(TierFeatures::ScopeConcurrentUsers) };

    csp::services::ResponseHandlerPtr responseHandler = m_quotaManagementApi->CreateHandler<FeatureLimitCallback, FeatureLimitResult, void,
        csp::services::DtoArray<chs::QuotaFeatureLimitProgressDto>>(callback, nullptr);

    static_cast<chs::QuotaActivityApi*>(m_quotaManagementApi)->groupsGroupIdQuota_progressGet({ spaceId.c_str(), featureNamesList }, responseHandler);
}

void QuotaSystem::GetTotalSpaceSizeInKilobytes(const csp::common::String& spaceId, FeatureLimitCallback callback)
{
    std::vector<csp::common::String> featureNamesList = { TierFeatureEnumToString(TierFeatures::TotalUploadSizeInKilobytes) };

    csp::services::ResponseHandlerPtr responseHandler = m_quotaManagementApi->CreateHandler<FeatureLimitCallback, FeatureLimitResult, void,
        csp::services::DtoArray<chs::QuotaFeatureLimitProgressDto>>(callback, nullptr);

    static_cast<chs::QuotaActivityApi*>(m_quotaManagementApi)->groupsGroupIdQuota_progressGet({ spaceId, featureNamesList }, responseHandler);
}

void QuotaSystem::GetTierFeatureProgressForUser(const csp::common::Array<TierFeatures>& featureNames, FeaturesLimitCallback callback)
{
    if (m_context->GetLoginState().State != common::ELoginState::LoggedIn)
    {
        m_logSystem->LogMsg(common::LogLevel::Warning, "QuotaSystem::GetTierFeatureProgressForUser: User is not logged in. Aborting operation.");
        callback(MakeInvalid<FeaturesLimitResult>());
        return;
    }

    csp::services::ResponseHandlerPtr responseHandler = m_quotaManagementApi->CreateHandler<FeaturesLimitCallback, FeaturesLimitResult, void,
        csp::services::DtoArray<chs::QuotaFeatureLimitProgressDto>>(callback, nullptr);

    std::vector<csp::common::String> featureNamesList;
    featureNamesList.reserve(featureNames.Size());

    for (size_t idx = 0; idx < featureNames.Size(); ++idx)
    {
        featureNamesList.push_back(TierFeatureEnumToString(featureNames[idx]));
    }

    static_cast<chs::QuotaActivityApi*>(m_quotaManagementApi)
        ->usersUserIdQuota_progressGet({ m_context->GetLoginState().UserId, featureNamesList }, responseHandler);
}

void QuotaSystem::GetTierFeatureProgressForSpace(
    const csp::common::String& spaceId, const csp::common::Array<TierFeatures>& featureNames, FeaturesLimitCallback callback)
{
    csp::services::ResponseHandlerPtr responseHandler = m_quotaActivityApi->CreateHandler<FeaturesLimitCallback, FeaturesLimitResult, void,
        csp::services::DtoArray<chs::QuotaFeatureLimitProgressDto>>(callback, nullptr);

    std::vector<csp::common::String> featureNamesList;
    featureNamesList.reserve(featureNames.Size());

    for (size_t idx = 0; idx < featureNames.Size(); ++idx)
    {
        featureNamesList.push_back(TierFeatureEnumToString(featureNames[idx]));
    }

    static_cast<chs::QuotaActivityApi*>(m_quotaManagementApi)->groupsGroupIdQuota_progressGet({ spaceId, featureNamesList }, responseHandler);
}

void QuotaSystem::GetCurrentUserTier(UserTierCallback callback)
{
    if (m_context->GetLoginState().State != common::ELoginState::LoggedIn)
    {
        m_logSystem->LogMsg(common::LogLevel::Warning, "QuotaSystem::GetCurrentUserTier: User is not logged in. Aborting operation.");
        callback(MakeInvalid<UserTierResult>());
        return;
    }

    csp::services::ResponseHandlerPtr responseHandler
        = m_quotaTierAssignmentApi->CreateHandler<UserTierCallback, UserTierResult, void, chs::QuotaTierAssignmentDto>(callback, nullptr);

    static_cast<chs::QuotaTierAssignmentApi*>(m_quotaTierAssignmentApi)
        ->usersUserIdTier_assignmentGet({ m_context->GetLoginState().UserId }, responseHandler);
}

void QuotaSystem::SetUserTier(TierNames tierName, const csp::common::String& userId, UserTierCallback callback)
{
    csp::services::ResponseHandlerPtr responseHandler
        = m_quotaTierAssignmentApi->CreateHandler<UserTierCallback, UserTierResult, void, chs::QuotaTierAssignmentDto>(callback, nullptr);

    auto requestBody = std::make_shared<chs::QuotaTierAssignmentDto>();
    requestBody->SetTierName(TierNameEnumToString(tierName));
    requestBody->SetTenantName(CSPFoundation::GetTenant());

    // There is an expiry timestamp in the DTO, but it isn't documented, and from experimenting
    // I could not get it to return a non rejected code, so omitting it.

    static_cast<chs::QuotaTierAssignmentApi*>(m_quotaTierAssignmentApi)->usersUserIdTier_assignmentPut({ userId, requestBody }, responseHandler);
}

void QuotaSystem::GetTierFeatureQuota(TierNames tierName, TierFeatures featureName, FeatureQuotaCallback callback)
{
    csp::services::ResponseHandlerPtr responseHandler
        = m_quotaManagementApi->CreateHandler<FeatureQuotaCallback, FeatureQuotaResult, void, chs::QuotaFeatureTierDto>(callback, nullptr);

    static_cast<chs::QuotaManagementApi*>(m_quotaManagementApi)
        ->tiersTierNameFeaturesFeatureNameQuotaGet({ TierNameEnumToString(tierName), TierFeatureEnumToString(featureName) }, responseHandler);
}

void QuotaSystem::GetTierFeaturesQuota(TierNames tierName, FeaturesQuotaCallback callback)
{
    csp::services::ResponseHandlerPtr responseHandler
        = m_quotaManagementApi->CreateHandler<FeaturesQuotaCallback, FeaturesQuotaResult, void, csp::services::DtoArray<chs::QuotaFeatureTierDto>>(
            callback, nullptr);

    static_cast<chs::QuotaManagementApi*>(m_quotaManagementApi)->tiersTierNameQuotasGet({ TierNameEnumToString(tierName) }, responseHandler);
}
} // namespace csp::systems
