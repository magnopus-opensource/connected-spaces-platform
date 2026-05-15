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
#include "CSP/Systems/WebService.h"

#include "Services/ApiBase/ApiBase.h"

namespace csp::systems
{
systems::ResultBase::ResultBase()
    : m_failureReason(ERequestFailureReason::None)
{
}

ResultBase::ResultBase(csp::systems::EResultCode resCode, uint16_t httpResCode)
    : m_result(resCode)
    , m_httpResponseCode(httpResCode)
    , m_failureReason(ERequestFailureReason::None)
{
}

ResultBase::ResultBase(csp::systems::EResultCode resCode, uint16_t httpResCode, csp::systems::ERequestFailureReason reason)
    : m_result(resCode)
    , m_httpResponseCode(httpResCode)
    , m_failureReason(reason)
{
}

bool ResultBase::operator==(const ResultBase& other) const
{
    return GetResultCode() == other.GetResultCode() && GetHttpResultCode() == other.GetHttpResultCode()
        && GetRequestProgress() == other.GetRequestProgress() && GetResponseProgress() == other.GetResponseProgress()
        && GetFailureReason() == other.GetFailureReason() && GetResponseBody() == other.GetResponseBody();
}

bool ResultBase::operator!=(const ResultBase& other) const { return !(*this == other); }

void ResultBase::OnProgress(const services::ApiResponseBase* apiResponse)
{
    if (apiResponse)
    {
        const csp::web::HttpRequest* request = apiResponse->GetResponse()->GetRequest();

        m_result = EResultCode::InProgress;

        m_requestProgress = request->GetRequestProgressPercentage();
        m_responseProgress = request->GetResponseProgressPercentage();
    }
}

/// @brief Standard response handler
/// @param ApiResponse
void ResultBase::OnResponse(const services::ApiResponseBase* apiResponse)
{
    if (apiResponse->GetResponseCode() == services::EResponseCode::ResponseSuccess)
    {
        m_result = EResultCode::Success;
    }
    else
    {
        m_result = EResultCode::Failed;
    }

    m_httpResponseCode = (uint16_t)apiResponse->GetResponse()->GetResponseCode();

    const auto* httpResponse = apiResponse->GetResponse();
    const auto& responsePayload = httpResponse->GetPayload();
    m_responseBody = responsePayload.GetContent();

    const auto& headers = responsePayload.GetHeaders();

    if (m_result == EResultCode::Failed && headers.count("x-errorcode") > 0 && !headers.at("x-errorcode").empty())
    {
        m_failureReason = ParseErrorCode(headers.at("x-errorcode").c_str());
    }
}

EResultCode ResultBase::GetResultCode() const { return m_result; }

uint16_t ResultBase::GetHttpResultCode() const { return m_httpResponseCode; }

const csp::common::String& ResultBase::GetResponseBody() const { return m_responseBody; }

float ResultBase::GetRequestProgress() const { return m_requestProgress; }

float ResultBase::GetResponseProgress() const { return m_responseProgress; }

ERequestFailureReason ResultBase::GetFailureReason() const { return m_failureReason; }

void ResultBase::SetResult(csp::systems::EResultCode resCode, uint16_t httpResCode)
{
    m_result = resCode;
    m_httpResponseCode = httpResCode;
}

ERequestFailureReason ResultBase::ParseErrorCode(const csp::common::String& value)
{
    static const std::map<csp::common::String, csp::systems::ERequestFailureReason> xErrorCodeToFailureReason = {
        { "join_onbehalfnotallowed", csp::systems::ERequestFailureReason::AddUserToSpaceDenied },
        { "join_guestnotallowed", csp::systems::ERequestFailureReason::UserSpaceAccessDenied },
        { "join_userbanned", csp::systems::ERequestFailureReason::UserSpaceBannedAccessDenied },
        { "join_groupfull", csp::systems::ERequestFailureReason::UserSpaceFullAccessDenied },
        { "join_groupinviteexpired", csp::systems::ERequestFailureReason::UserSpaceInviteExpired },
        { "group_duplicatename", csp::systems::ERequestFailureReason::SpacePublicNameDuplicate },
        { "group_spaceownerquota", csp::systems::ERequestFailureReason::UserMaxSpaceLimitReached },
        { "user_accountlocked", csp::systems::ERequestFailureReason::UserAccountLocked },
        { "user_emptypassword", csp::systems::ERequestFailureReason::UserMissingPassword },
        { "user_emailnotconfirmed", csp::systems::ERequestFailureReason::UserUnverifiedEmail },
        { "user_bannedfromgroup", csp::systems::ERequestFailureReason::UserBannedFromSpace },
        { "user_emaildomainnotallowed", csp::systems::ERequestFailureReason::UserInvalidEmailDomain },
        { "user_sociallogininvalid", csp::systems::ERequestFailureReason::UserInvalidThirdPartyAuth },
        { "user_agenotverified", csp::systems::ERequestFailureReason::UserAgeNotVerified },
        { "user_guestlogindisallowed", csp::systems::ERequestFailureReason::UserGuestLoginDisallowed },
        { "user_tokenrefreshfailed", csp::systems::ERequestFailureReason::UserTokenRefreshFailed },
        { "prototype_reservedkeysnotallowed", csp::systems::ERequestFailureReason::PrototypeReservedKeysNotAllowed },
        { "assetdetail_invalidfilecontents", csp::systems::ERequestFailureReason::AssetInvalidFileContents },
        { "assetdetail_invalidfiletype", csp::systems::ERequestFailureReason::AssetInvalidFileType },
        { "assetdetail_audiovideoquota", csp::systems::ERequestFailureReason::AssetAudioVideoLimitReached },
        { "assetdetail_objectcapturequota", csp::systems::ERequestFailureReason::AssetObjectCaptureLimitReached },
        { "assetdetail_totaluploadsizeinkilobytes", csp::systems::ERequestFailureReason::AssetTotalUploadSizeLimitReached },
        { "applyticket_unknownticketnumber", csp::systems::ERequestFailureReason::TicketUnknownNumber },
        { "applyticket_emaildoesntmatch", csp::systems::ERequestFailureReason::TicketEmailMismatch },
        { "vendoroauthexchange_failuretoexchangecode", csp::systems::ERequestFailureReason::TicketVendorOAuthFailure },
        { "applyticket_invalidauthtoken", csp::systems::ERequestFailureReason::TicketOAuthTokenInvalid },
        { "applyticket_alreadyapplied", csp::systems::ERequestFailureReason::TicketAlreadyApplied },
        { "shopify_vendorconnectionbroken", csp::systems::ERequestFailureReason::ShopifyConnectionBroken },
        { "shopify_invalidstorename", csp::systems::ERequestFailureReason::ShopifyInvalidStoreName },
        { "agoraoperation_groupownerquota", csp::systems::ERequestFailureReason::UserAgoraLimitReached },
        { "openaioperation_userquota", csp::systems::ERequestFailureReason::UserOpenAILimitReached },
        { "ticketedspaces_userquota", csp::systems::ERequestFailureReason::UserTicketedSpacesLimitReached },
        { "shopify_userquota", csp::systems::ERequestFailureReason::UserShopifyLimitReached },
        { "scopes_concurrentusersquota", csp::systems::ERequestFailureReason::UserSpaceConcurrentUsersLimitReached },
    };

    const auto reason = xErrorCodeToFailureReason.find(value);
    if (reason != xErrorCodeToFailureReason.end())
    {
        return reason->second;
    }

    CSP_LOG_ERROR_FORMAT(
        "Unknown XErrorCode string encountered whilst converting the string to ERequestFailureReason enum value. Value passed in was %s.",
        value.c_str());
    return ERequestFailureReason::Unknown;
}

} // namespace csp::systems
