﻿/*
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

#include "Services/ApiBase/ApiBase.h"
#include "Services/TrackingService/Api.h"
#include "Services/UserService/Dto.h"


namespace chs = csp::services::generated::trackingservice;

namespace csp::systems
{
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

	auto* FeatureProgressResponse		   = static_cast<chs::QuotaFeatureLimitProgressDto*>(ApiResponse->GetDto());
	const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

	if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
	{
		// Build the Dto from the response Json
		FeatureProgressResponse->FromJson(Response->GetPayload().GetContent());

		if (FeatureProgressResponse->HasLimit())
		{
			FeatureLimitInfo.Limit = FeatureProgressResponse->GetLimit();
		}

		if (FeatureProgressResponse->HasActivityCount())
		{
			FeatureLimitInfo.ActivityCount = FeatureProgressResponse->GetActivityCount();
		}

		if (FeatureProgressResponse->HasFeatureName())
		{
			FeatureLimitInfo.FeatureName = StringToTierFeatureEnum(FeatureProgressResponse->GetFeatureName());
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
			FeatureQuotaInfo.FeatureName = FeatureQuotaResponse->GetFeatureName();
		}

		if (FeatureQuotaResponse->HasTierName())
		{
			FeatureQuotaInfo.TierName = FeatureQuotaResponse->GetTierName();
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

		for (size_t i = 0; i < QuotaArray.size(); ++i)
		{
			if (QuotaArray[i].HasFeatureName())
			{
				FeaturesQuotaInfo[i].FeatureName = QuotaArray[i].GetFeatureName();
			}

			if (QuotaArray[i].HasTierName())
			{
				FeaturesQuotaInfo[i].TierName = QuotaArray[i].GetTierName();
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
const csp::common::String& TierNameEnumToString(TierNames Value)
{
	switch (Value)
	{
		case TierNames::Unlimited:
			return "Unlimited";
		case TierNames::Free:
			return "Free";
	}
}

const csp::common::String& TierFeatureEnumToString(TierFeatures Value)
{
	switch (Value)
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
	}
}

const TierNames& StringToTierNameEnum(csp::common::String Value)
{
	if (Value == "Unlimited")
	{
		return TierNames::Unlimited;
	}

	if (Value == "Free")
	{
		return TierNames::Free;
	}

	CSP_LOG_ERROR_FORMAT("TierName Not Supported %s", Value);
}

const TierFeatures& StringToTierFeatureEnum(csp::common::String Value)
{
	if (Value == "Agora")
	{
		return TierFeatures::Agora;
	}

	if (Value == "Shopify")
	{
		return TierFeatures::Shopify;
	}

	if (Value == "TicketedSpace")
	{
		return TierFeatures::TicketedSpace;
	}

	if (Value == "AudioVideoUpload")
	{
		return TierFeatures::AudioVideoUpload;
	}

	if (Value == "ObjectCaptureUpload")
	{
		return TierFeatures::ObjectCaptureUpload;
	}

	if (Value == "OpenAI")
	{
		return TierFeatures::OpenAI;
	}

	if (Value == "ScopeConcurrentUsers")
	{
		return TierFeatures::ScopeConcurrentUsers;
	}

	if (Value == "TotalUploadSizeInKilobytes")
	{
		return TierFeatures::TotalUploadSizeInKilobytes;
	}

	if (Value == "SpaceOwner")
	{
		return TierFeatures::SpaceOwner;
	}

	CSP_LOG_ERROR_FORMAT("TierFeatures Not Supported %s", Value);
}
} // namespace csp::systems