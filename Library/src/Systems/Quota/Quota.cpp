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
#include "CSP/Systems/Quota/Quota.h"

#include "Debug/Logging.h"
#include "Services/ApiBase/ApiBase.h"
#include "Services/TrackingService/Api.h"
#include "Services/UserService/Dto.h"

namespace chs = csp::services::generated::trackingservice;

namespace csp::systems
{
FeatureQuotaInfo::FeatureQuotaInfo(TierFeatures featureNameIn, TierNames tierNameIn, int32_t limitIn, PeriodEnum periodIn, bool /*AllowReductionsIn*/)
    : FeatureName(featureNameIn)
    , TierName(tierNameIn)
    , Limit(limitIn)
    , Period(periodIn) {};

bool FeatureLimitInfo::operator==(const FeatureLimitInfo& other) const
{
    return FeatureName == other.FeatureName && ActivityCount == other.ActivityCount && Limit == other.Limit;
}

bool UserTierInfo::operator==(const UserTierInfo& other) const
{
    return AssignToType == other.AssignToType && AssignToId == other.AssignToId && TierName == other.TierName;
}

bool FeatureQuotaInfo::operator==(const FeatureQuotaInfo& other) const
{
    return FeatureName == other.FeatureName && TierName == other.TierName && Limit == other.Limit && Period == other.Period;
}

bool FeatureLimitInfo::operator!=(const FeatureLimitInfo& other) const { return !(*this == other); }
bool UserTierInfo::operator!=(const UserTierInfo& other) const { return !(*this == other); }
bool FeatureQuotaInfo::operator!=(const FeatureQuotaInfo& other) const { return !(*this == other); }

const csp::common::Array<FeatureLimitInfo>& FeaturesLimitResult::GetFeaturesLimitInfo() const { return m_featuresLimitInfo; }

void FeaturesLimitResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* featureProgressResponse = static_cast<csp::services::DtoArray<chs::QuotaFeatureLimitProgressDto>*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        featureProgressResponse->FromJson(response->GetPayload().GetContent());

        // Extract data from response in our Groups array
        std::vector<chs::QuotaFeatureLimitProgressDto>& featureArray = featureProgressResponse->GetArray();
        m_featuresLimitInfo = csp::common::Array<FeatureLimitInfo>(featureArray.size());

        for (size_t i = 0; i < featureArray.size(); ++i)
        {
            if (featureArray[i].HasLimit())
            {
                m_featuresLimitInfo[i].Limit = featureArray[i].GetLimit();
            }

            if (featureArray[i].HasActivityCount())
            {
                m_featuresLimitInfo[i].ActivityCount = featureArray[i].GetActivityCount();
            }

            if (featureArray[i].HasFeatureName())
            {
                m_featuresLimitInfo[i].FeatureName = StringToTierFeatureEnum(featureArray[i].GetFeatureName());
            }
        }
    }
}

const FeatureLimitInfo& FeatureLimitResult::GetFeatureLimitInfo() const { return m_featureLimitInfo; }

void FeatureLimitResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* featureProgressResponse = static_cast<csp::services::DtoArray<chs::QuotaFeatureLimitProgressDto>*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        featureProgressResponse->FromJson(response->GetPayload().GetContent());

        std::vector<chs::QuotaFeatureLimitProgressDto>& featureProgressArray = featureProgressResponse->GetArray();

        if (featureProgressArray[0].HasLimit())
        {
            m_featureLimitInfo.Limit = featureProgressArray[0].GetLimit();
        }

        if (featureProgressArray[0].HasActivityCount())
        {
            m_featureLimitInfo.ActivityCount = featureProgressArray[0].GetActivityCount();
        }

        if (featureProgressArray[0].HasFeatureName())
        {
            m_featureLimitInfo.FeatureName = StringToTierFeatureEnum(featureProgressArray[0].GetFeatureName());
        }
    }
}

const UserTierInfo& UserTierResult::GetUserTierInfo() const { return m_userTierInfo; }

void UserTierResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* userTierResponse = static_cast<chs::QuotaTierAssignmentDto*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        userTierResponse->FromJson(response->GetPayload().GetContent());

        if (userTierResponse->HasAssignedToId())
        {
            m_userTierInfo.AssignToId = userTierResponse->GetAssignedToId();
        }

        if (userTierResponse->HasAssignedToType())
        {
            m_userTierInfo.AssignToType = userTierResponse->GetAssignedToType();
        }

        if (userTierResponse->HasTierName())
        {
            m_userTierInfo.TierName = StringToTierNameEnum(userTierResponse->GetTierName());
        }
    }
}

const FeatureQuotaInfo& FeatureQuotaResult::GetFeatureQuotaInfo() const { return m_featureQuotaInfo; }

void FeatureQuotaResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* featureQuotaResponse = static_cast<chs::QuotaFeatureTierDto*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        featureQuotaResponse->FromJson(response->GetPayload().GetContent());

        if (featureQuotaResponse->HasFeatureName())
        {
            m_featureQuotaInfo.FeatureName = StringToTierFeatureEnum(featureQuotaResponse->GetFeatureName());
        }

        if (featureQuotaResponse->HasTierName())
        {
            m_featureQuotaInfo.TierName = StringToTierNameEnum(featureQuotaResponse->GetTierName());
        }

        if (featureQuotaResponse->HasLimit())
        {
            m_featureQuotaInfo.Limit = featureQuotaResponse->GetLimit();
        }

        if (featureQuotaResponse->HasPeriod())
        {
            m_featureQuotaInfo.Period = static_cast<PeriodEnum>(featureQuotaResponse->GetPeriod()->GetValue());
        }
    }
}

const csp::common::Array<FeatureQuotaInfo>& FeaturesQuotaResult::GetFeaturesQuotaInfo() const { return m_featuresQuotaInfo; }

void FeaturesQuotaResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* featuresQuotaResponse = static_cast<csp::services::DtoArray<chs::QuotaFeatureTierDto>*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        featuresQuotaResponse->FromJson(response->GetPayload().GetContent());
        std::vector<chs::QuotaFeatureTierDto>& quotaArray = featuresQuotaResponse->GetArray();
        m_featuresQuotaInfo = csp::common::Array<FeatureQuotaInfo>(quotaArray.size());
        for (size_t i = 0; i < quotaArray.size(); ++i)
        {
            if (quotaArray[i].HasFeatureName())
            {
                m_featuresQuotaInfo[i].FeatureName = StringToTierFeatureEnum(quotaArray[i].GetFeatureName());
            }

            if (quotaArray[i].HasTierName())
            {
                m_featuresQuotaInfo[i].TierName = StringToTierNameEnum(quotaArray[i].GetTierName());
            }

            if (quotaArray[i].HasLimit())
            {
                m_featuresQuotaInfo[i].Limit = quotaArray[i].GetLimit();
            }

            if (quotaArray[i].HasPeriod())
            {
                m_featuresQuotaInfo[i].Period = static_cast<PeriodEnum>(quotaArray[i].GetPeriod()->GetValue());
            }
        }
    }
}
const csp::common::String TierNameEnumToString(const TierNames& value)
{
    switch (value)
    {
    case TierNames::Basic:
        return "basic";
    case TierNames::Premium:
        return "premium";
    case TierNames::Pro:
        return "pro";
    case TierNames::Enterprise:
        return "enterprise";
    case TierNames::Invalid:
        return "Invalid";
    default:
    {
        CSP_LOG_ERROR_FORMAT("Unknown quota tier encountered whilst converting the enum to string. Value passed in was %i. To fix this, add a "
                             "case to the enum switch statement.",
            static_cast<int>(value));
        break;
    }
    }

    // return a default/invalid value if no matching tier was found
    return "Invalid";
}

const csp::common::String TierFeatureEnumToString(const TierFeatures& value)
{
    switch (value)
    {
    case TierFeatures::Agora:
        return "Agora";
    case TierFeatures::Shopify:
        return "Shopify";
    case TierFeatures::TicketedSpace:
        return "TicketedSpace";
    case TierFeatures::AudioVideoUpload:
        return "AudioVideoUpload";
    case TierFeatures::ObjectCaptureUpload:
        return "ObjectCaptureUpload";
    case TierFeatures::OpenAI:
        return "OpenAI";
    case TierFeatures::ScopeConcurrentUsers:
        return "ScopeConcurrentUsers";
    case TierFeatures::TotalUploadSizeInKilobytes:
        return "TotalUploadSizeInKilobytes";
    case TierFeatures::SpaceOwner:
        return "SpaceOwner";
    case TierFeatures::GoogleGenAI:
        return "GoogleGenAI";
    case TierFeatures::Invalid:
        return "Invalid";
    default:
    {
        CSP_LOG_ERROR_FORMAT("Unknown quota feature encountered whilst converting the enum to string. Value passed in was %i. To fix this, add a "
                             "case to the enum switch statement.",
            static_cast<int>(value));
        break;
    }
    }

    // return a default/invalid value if no matching feature was found
    return "Invalid";
}
TierNames StringToTierNameEnum(const csp::common::String& value)
{
    if (value == "basic")
    {
        return TierNames::Basic;
    }

    if (value == "premium")
    {
        return TierNames::Premium;
    }

    if (value == "pro")
    {
        return TierNames::Pro;
    }

    if (value == "enterprise")
    {
        return TierNames::Enterprise;
    }

    CSP_LOG_ERROR_FORMAT("QuotaSystem TierName not recognized: %s. Defaulting to Invalid.", value.c_str());
    return TierNames::Invalid;
}

TierFeatures StringToTierFeatureEnum(const csp::common::String& value)
{
    if (value == "Agora")
    {
        return TierFeatures::Agora;
    }

    if (value == "Shopify")
    {
        return TierFeatures::Shopify;
    }

    if (value == "TicketedSpace")
    {
        return TierFeatures::TicketedSpace;
    }

    if (value == "AudioVideoUpload")
    {
        return TierFeatures::AudioVideoUpload;
    }

    if (value == "ObjectCaptureUpload")
    {
        return TierFeatures::ObjectCaptureUpload;
    }

    if (value == "OpenAI")
    {
        return TierFeatures::OpenAI;
    }

    if (value == "ScopeConcurrentUsers")
    {
        return TierFeatures::ScopeConcurrentUsers;
    }

    if (value == "TotalUploadSizeInKilobytes")
    {
        return TierFeatures::TotalUploadSizeInKilobytes;
    }

    if (value == "SpaceOwner")
    {
        return TierFeatures::SpaceOwner;
    }

    if (value == "GoogleGenAI")
    {
        return TierFeatures::GoogleGenAI;
    }

    CSP_LOG_ERROR_FORMAT("QuotaSystem TierFeature not recognized: %s. Defaulting to Invalid", value.c_str());
    return TierFeatures::Invalid;
}

} // namespace csp::systems
