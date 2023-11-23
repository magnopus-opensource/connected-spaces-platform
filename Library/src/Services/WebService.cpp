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
	{"join_onbehalfnotallowed", csp::services::ERequestFailureReason::AddUserToSpaceDenied},
	{"join_guestnotallowed", csp::services::ERequestFailureReason::UserSpaceAccessDenied},
	{"join_userbanned", csp::services::ERequestFailureReason::UserSpaceBannedAccessDenied},
	{"join_groupfull", csp::services::ERequestFailureReason::UserSpaceFullAccessDenied},
	{"join_groupinviteexpired", csp::services::ERequestFailureReason::UserSpaceInviteExpired},
	{"group_duplicatename", csp::services::ERequestFailureReason::SpacePublicNameDuplicate},
	{"group_spaceownerquota", csp::services::ERequestFailureReason::UserMaxSpaceLimitReached},
	{"user_accountlocked", csp::services::ERequestFailureReason::UserAccountLocked},
	{"user_emptypassword", csp::services::ERequestFailureReason::UserMissingPassword},
	{"user_emailnotconfirmed", csp::services::ERequestFailureReason::UserUnverifiedEmail},
	{"user_bannedfromgroup", csp::services::ERequestFailureReason::UserBannedFromSpace},
	{"user_emaildomainnotallowed", csp::services::ERequestFailureReason::UserInvalidEmailDomain},
	{"user_sociallogininvalid", csp::services::ERequestFailureReason::UserInvalidThirdPartyAuth},
	{"user_agenotverified", csp::services::ERequestFailureReason::UserAgeNotVerified},
	{"user_guestlogindisallowed", csp::services::ERequestFailureReason::UserGuestLoginDisallowed},
	{"prototype_reservedkeysnotallowed", csp::services::ERequestFailureReason::PrototypeReservedKeysNotAllowed},
	{"assetdetail_invalidfilecontents", csp::services::ERequestFailureReason::AssetInvalidFileContents},
	{"assetdetail_invalidfiletype", csp::services::ERequestFailureReason::AssetInvalidFileType},
	{"assetdetail_audiovideoquota", csp::services::ERequestFailureReason::AssetAudioVideoLimitReached},
	{"assetdetail_objectcapturequota", csp::services::ERequestFailureReason::AssetObjectCaptureLimitReached},
	{"assetdetail_totaluploadsizeinkilobytes", csp::services::ERequestFailureReason::AssetTotalUploadSizeLimitReached},
	{"applyticket_unknownticketnumber", csp::services::ERequestFailureReason::TicketUnknownNumber},
	{"applyticket_emaildoesntmatch", csp::services::ERequestFailureReason::TicketEmailMismatch},
	{"vendoroauthexchange_failuretoexchangecode", csp::services::ERequestFailureReason::TicketVendorOAuthFailure},
	{"applyticket_invalidauthtoken", csp::services::ERequestFailureReason::TicketOAuthTokenInvalid},
	{"applyticket_alreadyapplied", csp::services::ERequestFailureReason::TicketAlreadyApplied},
	{"shopify_vendorconnectionbroken", csp::services::ERequestFailureReason::ShopifyConnectionBroken},
	{"shopify_invalidstorename", csp::services::ERequestFailureReason::ShopifyInvalidStoreName},
	{"agoraoperation_groupownerquota", csp::services::ERequestFailureReason::UserAgoraLimitReached},
	{"openaioperation_userquota", csp::services::ERequestFailureReason::UserOpenAILimitReached},
	{"ticketedspaces_userquota", csp::services::ERequestFailureReason::UserTicketedSpacesLimitReached},
	{"shopify_userquota", csp::services::ERequestFailureReason::UserShopifyLimitReached},
	{"scopes_concurrentusersquota", csp::services::ERequestFailureReason::UserSpaceConcurrentUsersLimitReached},
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
