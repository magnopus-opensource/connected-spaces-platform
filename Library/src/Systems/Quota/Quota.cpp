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
FeatureQuotaInfo::FeatureQuotaInfo(TierFeatures FeatureNameIn, TierNames TierNameIn, int32_t LimitIn, PeriodEnum PeriodIn, bool AllowReductionsIn)
	: FeatureName(FeatureNameIn), TierName(TierNameIn), Limit(LimitIn), Period(PeriodIn), AllowReductions(AllowReductionsIn) {};

const csp::common::Array<FeatureLimitInfo>& FeaturesLimitResult::GetFeaturesLimitInfo() const
{
	return FeaturesLimitInfo;
}

void FeaturesLimitResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
	ResultBase::OnResponse(ApiResponse);

	auto* FeatureProgressResponse		   = static_cast<csp::services::DtoArray<chs::QuotaFeatureLimitProgressDto>*>(ApiResponse->GetDto());
	const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

	if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
	{
		// Build the Dto from the response Json
		FeatureProgressResponse->FromJson(Response->GetPayload().GetContent());

		// Extract data from response in our Groups array
		std::vector<chs::QuotaFeatureLimitProgressDto>& FeatureArray = FeatureProgressResponse->GetArray();
		FeaturesLimitInfo											 = csp::common::Array<FeatureLimitInfo>(FeatureArray.size());

		for (size_t i = 0; i < FeatureArray.size(); ++i)
		{
			if (FeatureArray[i].HasLimit())
			{
				FeaturesLimitInfo[i].Limit = FeatureArray[i].GetLimit();
			}

			if (FeatureArray[i].HasActivityCount())
			{
				FeaturesLimitInfo[i].ActivityCount = FeatureArray[i].GetActivityCount();
			}

			if (FeatureArray[i].HasFeatureName())
			{
				FeaturesLimitInfo[i].FeatureName = StringToTierFeatureEnum(FeatureArray[i].GetFeatureName());
			}
		}
	}
}

const FeatureLimitInfo& FeatureLimitResult::GetFeatureLimitInfo() const
{
	return FeatureLimitInfo;
}

void FeatureLimitResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
	ResultBase::OnResponse(ApiResponse);

	auto* FeatureProgressResponse		   = static_cast<csp::services::DtoArray<chs::QuotaFeatureLimitProgressDto>*>(ApiResponse->GetDto());
	const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

	if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
	{
		// Build the Dto from the response Json
		FeatureProgressResponse->FromJson(Response->GetPayload().GetContent());

		std::vector<chs::QuotaFeatureLimitProgressDto>& FeatureProgressArray = FeatureProgressResponse->GetArray();

		if (FeatureProgressArray[0].HasLimit())
		{
			FeatureLimitInfo.Limit = FeatureProgressArray[0].GetLimit();
		}

		if (FeatureProgressArray[0].HasActivityCount())
		{
			FeatureLimitInfo.ActivityCount = FeatureProgressArray[0].GetActivityCount();
		}

		if (FeatureProgressArray[0].HasFeatureName())
		{
			FeatureLimitInfo.FeatureName = StringToTierFeatureEnum(FeatureProgressArray[0].GetFeatureName());
		}
	}
}

const UserTierInfo& UserTierResult::GetUserTierInfo() const
{
	return UserTierInfo;
}

void UserTierResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
	ResultBase::OnResponse(ApiResponse);

	auto* UserTierResponse				   = static_cast<chs::QuotaTierAssignmentDto*>(ApiResponse->GetDto());
	const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

	if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
	{
		// Build the Dto from the response Json
		UserTierResponse->FromJson(Response->GetPayload().GetContent());

		if (UserTierResponse->HasAssignedToId())
		{
			UserTierInfo.AssignToId = UserTierResponse->GetAssignedToId();
		}

		if (UserTierResponse->HasAssignedToType())
		{
			UserTierInfo.AssignToType = UserTierResponse->GetAssignedToType();
		}

		if (UserTierResponse->HasTierName())
		{
			UserTierInfo.TierName = StringToTierNameEnum(UserTierResponse->GetTierName());
		}
	}
}

const FeatureQuotaInfo& FeatureQuotaResult::GetFeatureQuotaInfo() const
{
	return FeatureQuotaInfo;
}

void FeatureQuotaResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
	ResultBase::OnResponse(ApiResponse);

	auto* FeatureQuotaResponse			   = static_cast<chs::QuotaFeatureTierDto*>(ApiResponse->GetDto());
	const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

	if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
	{
		// Build the Dto from the response Json
		FeatureQuotaResponse->FromJson(Response->GetPayload().GetContent());

		if (FeatureQuotaResponse->HasFeatureName())
		{
			FeatureQuotaInfo.FeatureName = StringToTierFeatureEnum(FeatureQuotaResponse->GetFeatureName());
		}

		if (FeatureQuotaResponse->HasTierName())
		{
			FeatureQuotaInfo.TierName = StringToTierNameEnum(FeatureQuotaResponse->GetTierName());
		}

		if (FeatureQuotaResponse->HasLimit())
		{
			FeatureQuotaInfo.Limit = FeatureQuotaResponse->GetLimit();
		}

		if (FeatureQuotaResponse->HasPeriod())
		{
			FeatureQuotaInfo.Period = static_cast<PeriodEnum>(FeatureQuotaResponse->GetPeriod()->GetValue());
		}

		if (FeatureQuotaResponse->HasAllowReductions())
		{
			FeatureQuotaInfo.AllowReductions = FeatureQuotaResponse->GetAllowReductions();
		}
	}
}

const csp::common::Array<FeatureQuotaInfo>& FeaturesQuotaResult::GetFeaturesQuotaInfo() const
{
	return FeaturesQuotaInfo;
}

void FeaturesQuotaResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
	ResultBase::OnResponse(ApiResponse);

	auto* FeaturesQuotaResponse			   = static_cast<csp::services::DtoArray<chs::QuotaFeatureTierDto>*>(ApiResponse->GetDto());
	const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

	if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
	{
		// Build the Dto from the response Json
		FeaturesQuotaResponse->FromJson(Response->GetPayload().GetContent());
		std::vector<chs::QuotaFeatureTierDto>& QuotaArray = FeaturesQuotaResponse->GetArray();
		FeaturesQuotaInfo								  = csp::common::Array<FeatureQuotaInfo>(QuotaArray.size());
		for (size_t i = 0; i < QuotaArray.size(); ++i)
		{
			if (QuotaArray[i].HasFeatureName())
			{
				FeaturesQuotaInfo[i].FeatureName = StringToTierFeatureEnum(QuotaArray[i].GetFeatureName());
			}

			if (QuotaArray[i].HasTierName())
			{
				FeaturesQuotaInfo[i].TierName = StringToTierNameEnum(QuotaArray[i].GetTierName());
			}

			if (QuotaArray[i].HasLimit())
			{
				FeaturesQuotaInfo[i].Limit = QuotaArray[i].GetLimit();
			}

			if (QuotaArray[i].HasPeriod())
			{
				FeaturesQuotaInfo[i].Period = static_cast<PeriodEnum>(QuotaArray[i].GetPeriod()->GetValue());
			}

			if (QuotaArray[i].HasAllowReductions())
			{
				FeaturesQuotaInfo[i].AllowReductions = QuotaArray[i].GetAllowReductions();
			}
		}
	}
}
const csp::common::String TierNameEnumToString(const TierNames& Value)
{
	csp::common::String TierString;
	switch (Value)
	{
		case TierNames::Basic:
		{
			TierString = "basic";
			break;
		}
		case TierNames::Premium:
		{
			TierString = "premium";
			break;
		}
		case TierNames::Pro:
		{
			TierString = "pro";
			break;
		}
		case TierNames::Enterprise:
		{
			TierString = "enterprise";
			break;
		}
		default:
		{
			CSP_LOG_ERROR_FORMAT("Unknown quota tier encountered whilst converting the enum to string. Value passed in was %i. To fix this, add a "
								 "case to the enum switch statement.",
								 static_cast<int>(Value));
			break;
		}
	}
	return TierString;
}

const csp::common::String TierFeatureEnumToString(const TierFeatures& Value)
{
	csp::common::String TierString;

	switch (Value)
	{
		case TierFeatures::Agora:
		{
			TierString = "Agora";
			break;
		}
		case TierFeatures::Shopify:
		{
			TierString = "Shopify";
			break;
		}
		case TierFeatures::TicketedSpace:
		{
			TierString = "TicketedSpace";
			break;
		}
		case TierFeatures::AudioVideoUpload:
		{
			TierString = "AudioVideoUpload";
			break;
		}
		case TierFeatures::ObjectCaptureUpload:
		{
			TierString = "ObjectCaptureUpload";
			break;
		}
		case TierFeatures::OpenAI:
		{
			TierString = "OpenAI";
			break;
		}
		case TierFeatures::ScopeConcurrentUsers:
		{
			TierString = "ScopeConcurrentUsers";
			break;
		}
		case TierFeatures::TotalUploadSizeInKilobytes:
		{
			TierString = "TotalUploadSizeInKilobytes";
			break;
		}
		case TierFeatures::SpaceOwner:
		{
			TierString = "SpaceOwner";
			break;
		}
		default:
		{
			CSP_LOG_ERROR_FORMAT("Unknown quota feature encountered whilst converting the enum to string. Value passed in was %i. To fix this, add a "
								 "case to the enum switch statement.",
								 static_cast<int>(Value));
			break;
		}
	}

	return TierString;
}

const TierNames StringToTierNameEnum(const csp::common::String& Value)
{
	TierNames TierName = TierNames::Basic;

	if (Value == "basic")
	{
		TierName = TierNames::Basic;
	}
	else if (Value == "premium")
	{
		TierName = TierNames::Premium;
	}
	else if (Value == "pro")
	{
		TierName = TierNames::Pro;
	}
	else if (Value == "enterprise")
	{
		TierName = TierNames::Enterprise;
	}
	else
	{
		CSP_LOG_ERROR_FORMAT("QuotaSystem TierName not recognized: %s. Defaulting to basic tier.", Value.c_str());
	}

	return TierName;
}

const TierFeatures StringToTierFeatureEnum(const csp::common::String& Value)
{
	TierFeatures TierFeature = TierFeatures::SpaceOwner;

	if (Value == "Agora")
	{
		TierFeature = TierFeatures::Agora;
	}
	else if (Value == "Shopify")
	{
		TierFeature = TierFeatures::Shopify;
	}
	else if (Value == "TicketedSpace")
	{
		TierFeature = TierFeatures::TicketedSpace;
	}
	else if (Value == "AudioVideoUpload")
	{
		TierFeature = TierFeatures::AudioVideoUpload;
	}
	else if (Value == "ObjectCaptureUpload")
	{
		TierFeature = TierFeatures::ObjectCaptureUpload;
	}
	else if (Value == "OpenAI")
	{
		TierFeature = TierFeatures::OpenAI;
	}
	else if (Value == "ScopeConcurrentUsers")
	{
		TierFeature = TierFeatures::ScopeConcurrentUsers;
	}
	else if (Value == "TotalUploadSizeInKilobytes")
	{
		TierFeature = TierFeatures::TotalUploadSizeInKilobytes;
	}
	else if (Value == "SpaceOwner")
	{
		TierFeature = TierFeatures::SpaceOwner;
	}
	else
	{
		CSP_LOG_ERROR_FORMAT("QuotaSystem TierFeature not recognized: %s. Defaulting to SpaceOwner", Value.c_str());
	}

	return TierFeature;
}

} // namespace csp::systems
