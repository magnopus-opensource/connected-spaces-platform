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
#include "CSP/Services/WebService.h"

#include "Services/ApiBase/ApiBase.h"

namespace
{
std::map<csp::common::String, csp::services::ERequestFailureReason> XErrorCodeToFailureReason = {
	{"Join_OnBehalfNotAllowed", csp::services::ERequestFailureReason::AddUserToSpaceDenied},
	{"Join_GuestNotAllowed", csp::services::ERequestFailureReason::UserSpaceAccessDenied},
	{"Join_UserBanned", csp::services::ERequestFailureReason::UserSpaceBannedAccessDenied},
	{"Join_GroupFull", csp::services::ERequestFailureReason::UserSpaceFullAccessDenied},
	{"Join_GroupInviteExpired", csp::services::ERequestFailureReason::UserSpaceInviteExpired},
	{"Group_DuplicateName", csp::services::ERequestFailureReason::SpacePublicNameDuplicate},
	{"Group_SpaceOwnerQuota", csp::services::ERequestFailureReason::UserMaxSpaceLimitReached},
	{"User_AccountLocked", csp::services::ERequestFailureReason::UserAccountLocked},
	{"User_EmptyPassword", csp::services::ERequestFailureReason::UserMissingPassword},
	{"User_EmailNotConfirmed", csp::services::ERequestFailureReason::UserUnverifiedEmail},
	{"User_BannedFromGroup", csp::services::ERequestFailureReason::UserBannedFromSpace},
	{"User_EmailDomainNotAllowed", csp::services::ERequestFailureReason::UserInvalidEmailDomain},
	{"User_SocialLoginInvalid", csp::services::ERequestFailureReason::UserInvalidThirdPartyAuth},
	{"User_AgeNotVerified", csp::services::ERequestFailureReason::UserAgeNotVerified},
	{"User_GuestLoginDisallowed", csp::services::ERequestFailureReason::UserGuestLoginDisallowed},
	{"Prototype_ReservedKeysNotAllowed", csp::services::ERequestFailureReason::PrototypeReservedKeysNotAllowed},
	{"AssetDetail_InvalidFileContents", csp::services::ERequestFailureReason::AssetInvalidFileContents},
	{"AssetDetail_InvalidFileType", csp::services::ERequestFailureReason::AssetInvalidFileType},
	{"AssetDetail_AudioVideoQuota", csp::services::ERequestFailureReason::AssetAudioVideoLimitReached},
	{"AssetDetail_ObjectCaptureQuota", csp::services::ERequestFailureReason::AssetObjectCaptureLimitReached},
	{"AssetDetail_TotalUploadSizeInKilobytes", csp::services::ERequestFailureReason::AssetTotalUploadSizeLimitReached},
	{"ApplyTicket_UnknownTicketNumber", csp::services::ERequestFailureReason::TicketUnknownNumber},
	{"ApplyTicket_EmailDoesntMatch", csp::services::ERequestFailureReason::TicketEmailMismatch},
	{"VendorOAuthExchange_FailureToExchangeCode", csp::services::ERequestFailureReason::TicketVendorOAuthFailure},
	{"ApplyTicket_InvalidAuthToken", csp::services::ERequestFailureReason::TicketOAuthTokenInvalid},
	{"ApplyTicket_AlreadyApplied", csp::services::ERequestFailureReason::TicketAlreadyApplied},
	{"Shopify_VendorConnectionBroken", csp::services::ERequestFailureReason::ShopifyConnectionBroken},
	{"Shopify_InvalidStoreName", csp::services::ERequestFailureReason::ShopifyInvalidStoreName},
	{"AgoraOperation_GroupOwnerQuota", csp::services::ERequestFailureReason::UserAgoraLimitReached},
	{"OpenAIOperation_UserQuota", csp::services::ERequestFailureReason::UserOpenAILimitReached},
	{"TicketedSpaces_UserQuota", csp::services::ERequestFailureReason::UserTicketedSpacesLimitReached},
	{"Shopify_UserQuota", csp::services::ERequestFailureReason::UserShopifyLimitReached},
	{"Scopes_ConcurrentUsersQuota", csp::services::ERequestFailureReason::UserSpaceConcurrentUsersLimitReached},
};
}

namespace csp::services
{

ResultBase::ResultBase() : FailureReason(ERequestFailureReason::None)
{
}

ResultBase::ResultBase(csp::services::EResultCode ResCode, uint16_t HttpResCode)
	: Result(ResCode), HttpResponseCode(HttpResCode), FailureReason(ERequestFailureReason::None)
{
}

void ResultBase::OnProgress(const ApiResponseBase* ApiResponse)
{
	if (ApiResponse)
	{
		const csp::web::HttpRequest* Request = ApiResponse->GetResponse()->GetRequest();

		Result = EResultCode::InProgress;

		RequestProgress	 = Request->GetRequestProgressPercentage();
		ResponseProgress = Request->GetResponseProgressPercentage();
	}
}

/// @brief Standard response handler
/// @param ApiResponse
void ResultBase::OnResponse(const ApiResponseBase* ApiResponse)
{
	if (ApiResponse->GetResponseCode() == EResponseCode::ResponseSuccess)
	{
		Result = EResultCode::Success;
	}
	else
	{
		Result = EResultCode::Failed;
	}

	HttpResponseCode = (uint16_t) ApiResponse->GetResponse()->GetResponseCode();

	const auto* HttpResponse	= ApiResponse->GetResponse();
	const auto& ResponsePayload = HttpResponse->GetPayload();
	ResponseBody				= ResponsePayload.GetContent();

	const auto& Headers = ResponsePayload.GetHeaders();

	if (Result == EResultCode::Failed && Headers.count("x-errorcode") > 0 && !Headers.at("x-errorcode").empty())
	{
		FailureReason = ParseErrorCode(Headers.at("x-errorcode").c_str());
	}
}

const EResultCode ResultBase::GetResultCode() const
{
	return Result;
}

const uint16_t ResultBase::GetHttpResultCode() const
{
	return HttpResponseCode;
}

const csp::common::String& ResultBase::GetResponseBody() const
{
	return ResponseBody;
}

float ResultBase::GetRequestProgress() const
{
	return RequestProgress;
}

float ResultBase::GetResponseProgress() const
{
	return ResponseProgress;
}

ERequestFailureReason ResultBase::GetFailureReason() const
{
	return FailureReason;
}

void ResultBase::SetResult(csp::services::EResultCode ResCode, uint16_t HttpResCode)
{
	Result			 = ResCode;
	HttpResponseCode = HttpResCode;
}

ERequestFailureReason ResultBase::ParseErrorCode(const csp::common::String& Value)
{
	if (XErrorCodeToFailureReason.find(Value) != XErrorCodeToFailureReason.end())
	{
		return XErrorCodeToFailureReason[Value];
	}

	CSP_LOG_ERROR_FORMAT(
		"Unknown XErrorCode string encountered whilst converting the string to ERequestFailureReason enum value. Value passed in was %s.",
		Value.c_str());
	return ERequestFailureReason::Unknown;
}

} // namespace csp::services
