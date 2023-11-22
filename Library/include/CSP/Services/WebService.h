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
#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Common/Map.h"
#include "CSP/Common/String.h"

#include <cstdint>
#include <functional>

namespace csp::services
{
class ApiBase;
class ApiResponseBase;
} // namespace csp::services

namespace csp::services
{


/// @namespace csp::services
/// This namespace wraps abstraction layers around Magnopus Services

/// @brief Abstract base class for all CHS web services.
class CSP_API WebService
{
public:
	// @brief Constructs a web service.
	WebService()
	{
	}

	// @brief virtual destructor.
	virtual ~WebService() = default;
};


/// @brief Code to indicate the result of a request.
/// Request results should be checked for a success by clients before using any other accessors.
enum class EResultCode : uint8_t
{
	Init,
	InProgress,
	Success,
	Failed
};


enum class EResultBaseFailureReason
{
	Unknown = -1,
	None	= 0,
	AddUserToSpaceDenied,
	UserSpaceAccessDenied,
	UserSpaceBannedAccessDenied,
	UserSpaceFullAccessDenied,
	UserSpaceInviteExpired,
	SpacePublicNameDuplicate,
	UserMaxSpaceLimitReached,
	UserAccountLocked,
	UserMissingPassword,
	UserUnverifiedEmail,
	UserBannedFromSpace,
	UserInvalidEmailDomain,
	UserInvalidThirdPartyAuth,
	UserAgeNotVerified,
	UserGuestLoginDisallowed,
	UserAgoraLimitReached,
	UserOpenAILimitReached,
	UserTicketedSpacesLimitReached,
	UserSpaceConcurrentUsersLimitReached,
	PrototypeReservedKeysNotAllowed,
	AssetInvalidFileContents,
	AssetInvalidFileType,
	AssetAudioVideoLimitReached,
	AssetObjectCaptureLimitReached,
	AssetTotalUploadSizeLimitReached,
	TicketUnknownNumber,
	TicketEmailMismatch,
	TicketVendorOAuthFailure,
	TicketOAuthTokenInvalid,
	TicketAlreadyApplied,
	ShopifyConnectionBroken,
	ShopifyInvalidStoreName,
	UserShopifyLimitReached
};


/// @brief Base class for a HTTP request result.
class CSP_API ResultBase
{
public:
	/// @brief Constructs an empty result.
	ResultBase();

	/// @brief Virtual destructor.
	virtual ~ResultBase() = default;

	/// @brief Called when progress has been updated.
	/// @param ApiResponse const ApiResponseBase* : Response received from the request
	CSP_NO_EXPORT virtual void OnProgress(const ApiResponseBase* ApiResponse);

	/// @brief Called when a response has been received.
	/// @param ApiResponse const ApiResponseBase* : Response received from the request
	CSP_NO_EXPORT virtual void OnResponse(const ApiResponseBase* ApiResponse);

	/// @brief Status of this response.
	/// @return EResultCode
	const EResultCode GetResultCode() const;

	/// @brief Result of http request.
	/// @return uint16_t
	const uint16_t GetHttpResultCode() const;

	/// @brief Body of the response.
	const csp::common::String& GetResponseBody() const;

	/// @brief Percentage of POST/PUT request completion.
	/// @return float
	float GetRequestProgress() const;

	/// @brief Percentage of GET/HEAD response completion.
	/// @return float
	float GetResponseProgress() const;

	/// @brief Get a code representing the failure reason, if relevant.
	/// @return int
	EResultBaseFailureReason GetFailureReason() const;

protected:
	ResultBase(csp::services::EResultCode ResCode, uint16_t HttpResCode);

	void SetResult(csp::services::EResultCode ResCode, uint16_t HttpResCode);

	EResultBaseFailureReason ParseErrorCode(const csp::common::String& Value);

	EResultCode Result		  = EResultCode::Init;
	uint16_t HttpResponseCode = 0;

	float RequestProgress  = 0.0f;
	float ResponseProgress = 0.0f;

	csp::common::String ResponseBody;
	EResultBaseFailureReason FailureReason;

	csp::common::Map<csp::common::String, EResultBaseFailureReason> XErrorCodeToFailureReason = {
		{"Join_OnBehalfNotAllowed", EResultBaseFailureReason::AddUserToSpaceDenied},
		{"Join_GuestNotAllowed", EResultBaseFailureReason::UserSpaceAccessDenied},
		{"Join_UserBanned", EResultBaseFailureReason::UserSpaceBannedAccessDenied},
		{"Join_GroupFull", EResultBaseFailureReason::UserSpaceFullAccessDenied},
		{"Join_GroupInviteExpired", EResultBaseFailureReason::UserSpaceInviteExpired},
		{"Group_DuplicateName", EResultBaseFailureReason::SpacePublicNameDuplicate},
		{"Group_SpaceOwnerQuota", EResultBaseFailureReason::UserMaxSpaceLimitReached},
		{"User_AccountLocked", EResultBaseFailureReason::UserAccountLocked},
		{"User_EmptyPassword", EResultBaseFailureReason::UserMissingPassword},
		{"User_EmailNotConfirmed", EResultBaseFailureReason::UserUnverifiedEmail},
		{"User_BannedFromGroup", EResultBaseFailureReason::UserBannedFromSpace},
		{"User_EmailDomainNotAllowed", EResultBaseFailureReason::UserInvalidEmailDomain},
		{"User_SocialLoginInvalid", EResultBaseFailureReason::UserInvalidThirdPartyAuth},
		{"User_AgeNotVerified", EResultBaseFailureReason::UserAgeNotVerified},
		{"User_GuestLoginDisallowed", EResultBaseFailureReason::UserGuestLoginDisallowed},
		{"Prototype_ReservedKeysNotAllowed", EResultBaseFailureReason::PrototypeReservedKeysNotAllowed},
		{"AssetDetail_InvalidFileContents", EResultBaseFailureReason::AssetInvalidFileContents},
		{"AssetDetail_InvalidFileType", EResultBaseFailureReason::AssetInvalidFileType},
		{"AssetDetail_AudioVideoQuota", EResultBaseFailureReason::AssetAudioVideoLimitReached},
		{"AssetDetail_ObjectCaptureQuota", EResultBaseFailureReason::AssetObjectCaptureLimitReached},
		{"AssetDetail_TotalUploadSizeInKilobytes", EResultBaseFailureReason::AssetTotalUploadSizeLimitReached},
		{"ApplyTicket_UnknownTicketNumber", EResultBaseFailureReason::TicketUnknownNumber},
		{"ApplyTicket_EmailDoesntMatch", EResultBaseFailureReason::TicketEmailMismatch},
		{"VendorOAuthExchange_FailureToExchangeCode", EResultBaseFailureReason::TicketVendorOAuthFailure},
		{"ApplyTicket_InvalidAuthToken", EResultBaseFailureReason::TicketOAuthTokenInvalid},
		{"ApplyTicket_AlreadyApplied", EResultBaseFailureReason::TicketAlreadyApplied},
		{"Shopify_VendorConnectionBroken", EResultBaseFailureReason::ShopifyConnectionBroken},
		{"Shopify_InvalidStoreName", EResultBaseFailureReason::ShopifyInvalidStoreName},
		{"AgoraOperation_GroupOwnerQuota", EResultBaseFailureReason::UserAgoraLimitReached},
		{"OpenAIOperation_UserQuota", EResultBaseFailureReason::UserOpenAILimitReached},
		{"TicketedSpaces_UserQuota", EResultBaseFailureReason::UserTicketedSpacesLimitReached},
		{"Shopify_UserQuota", EResultBaseFailureReason::UserShopifyLimitReached},
		{"Scopes_ConcurrentUsersQuota", EResultBaseFailureReason::UserSpaceConcurrentUsersLimitReached},
	};
};

} // namespace csp::services
