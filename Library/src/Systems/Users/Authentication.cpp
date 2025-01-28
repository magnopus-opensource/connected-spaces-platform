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

#include "CSP/Systems/Users/Authentication.h"

#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "Common/DateTime.h"
#include "Debug/Logging.h"
#include "Events/EventSystem.h"
#include "Services/AggregationService/Api.h"
#include "Services/ApiBase/ApiBase.h"
#include "Services/UserService/Dto.h"
#include "Systems/Users/Authentication.h"

#include "Common/Convert.h"

using namespace csp;
using namespace csp::common;
using namespace std::chrono;

namespace chs = csp::services::generated::userservice;
namespace chs_aggregation = csp::services::generated::aggregationservice;

namespace csp::systems
{

LoginState::LoginState()
    : State(ELoginState::LoggedOut)
    , AccessTokenRefreshTime(CSP_NEW DateTime())
{
}

LoginState::LoginState(const LoginState& OtherState) { CopyStateFrom(OtherState); }

LoginState& LoginState::operator=(const LoginState& OtherState)
{
    CopyStateFrom(OtherState);

    return *this;
}

void LoginState::CopyStateFrom(const LoginState& OtherState)
{
    State = OtherState.State;
    AccessToken = OtherState.AccessToken;
    RefreshToken = OtherState.RefreshToken;
    UserId = OtherState.UserId;
    DeviceId = OtherState.DeviceId;

    // Must reallocate the access token when copying otherwise destructor of
    // copied state will delete the original memory pointer potentially causing corruption
    AccessTokenRefreshTime = CSP_NEW DateTime(OtherState.AccessTokenRefreshTime->GetTimePoint());
}

LoginState::~LoginState() { CSP_DELETE(AccessTokenRefreshTime); }

bool LoginState::RefreshNeeded() const
{
    if (AccessTokenRefreshTime->IsEpoch())
    {
        return false;
    }

    const auto CurrentTime = DateTime::UtcTimeNow();

    return CurrentTime >= (*AccessTokenRefreshTime);
}

LoginStateResult::LoginStateResult()
    : State(nullptr)
{
}

LoginStateResult::LoginStateResult(LoginState* InStatePtr)
    : State(InStatePtr)
{
}

const LoginState& LoginStateResult::GetLoginState() const { return *State; }

void LoginStateResult::OnResponse(const services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto AuthResponse = static_cast<chs::AuthDto*>(ApiResponse->GetDto());
    const web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        AuthResponse->FromJson(Response->GetPayload().GetContent());

        if (State)
        {
            State->State = ELoginState::LoggedIn;
            State->AccessToken = AuthResponse->GetAccessToken();
            State->RefreshToken = AuthResponse->GetRefreshToken();
            State->UserId = AuthResponse->GetUserId();
            State->DeviceId = AuthResponse->GetDeviceId();
            State->OrganizationIds = csp::common::Convert(AuthResponse->GetOrganizationIds());

            const DateTime Expiry(AuthResponse->GetAccessTokenExpiresAt());
            const DateTime CurrentTime(DateTime::UtcTimeNow());

            if (CurrentTime >= Expiry)
            {
                CSP_LOG_FORMAT(LogLevel::Error, "AccessToken Expired: %s %s", AuthResponse->GetAccessToken().c_str(),
                    AuthResponse->GetAccessTokenExpiresAt().c_str());

                return;
            }

            web::HttpAuth::SetAccessToken(AuthResponse->GetAccessToken(), AuthResponse->GetAccessTokenExpiresAt(), AuthResponse->GetRefreshToken(),
                AuthResponse->GetRefreshTokenExpiresAt());

            // Schedule a Refresh of the Token 5 minutes before it expires
            system_clock::time_point RefreshTimepoint = Expiry.GetTimePoint() - system_clock::duration(5min);
            DateTime RefreshTime(RefreshTimepoint);

            if (RefreshTime >= Expiry)
            {
                CSP_LOG_FORMAT(LogLevel::Error, "RefreshToken Expired: %s %s", AuthResponse->GetRefreshToken().c_str(),
                    AuthResponse->GetRefreshTokenExpiresAt().c_str());

                return;
            }

            *(State->AccessTokenRefreshTime) = RefreshTime;

            // Signal login to anyone interested
            events::Event* LoginEvent = events::EventSystem::Get().AllocateEvent(events::USERSERVICE_LOGIN_EVENT_ID);
            LoginEvent->AddString("UserId", AuthResponse->GetUserId());
            events::EventSystem::Get().EnqueueEvent(LoginEvent);

            SystemsManager::Get().GetUserSystem()->NotifyRefreshTokenHasChanged();
        }
    }
    else
    {
        if (State)
        {
            web::HttpAuth::SetAccessToken("", "", "", "");

            State->State = ELoginState::Error;
            State->AccessToken = "InvalidAccessToken";
            State->RefreshToken = "InvalidRefreshToken";
            State->UserId = "InvalidUserId";
            State->DeviceId = "InvalidDeviceId";
        }
    }
}

LogoutResult::LogoutResult()
    : State(nullptr)
{
}

LogoutResult::LogoutResult(LoginState* InStatePtr)
    : NullResult(InStatePtr)
    , State(InStatePtr)
{
}

void LogoutResult::OnResponse(const services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    if (ApiResponse->GetResponseCode() == services::EResponseCode::ResponseSuccess)
    {
        if (State)
        {
            State->State = ELoginState::LoggedOut;
            State->AccessToken = "InvalidAccessToken";
            State->RefreshToken = "InvalidRefreshToken";
            State->UserId = "InvalidUserId";
            State->DeviceId = "InvalidDeviceId";

            web::HttpAuth::SetAccessToken("", "", "", "");

            // Send logout event
            events::Event* LogoutEvent = events::EventSystem::Get().AllocateEvent(events::USERSERVICE_LOGOUT_EVENT_ID);
            events::EventSystem::Get().EnqueueEvent(LogoutEvent);
        }
    }
    else
    {
        if (State)
        {
            State->State = ELoginState::Error;
            State->AccessToken = "InvalidAccessToken";
            State->RefreshToken = "InvalidRefreshToken";
            State->UserId = "InvalidUserId";
            State->DeviceId = "InvalidDeviceId";

            web::HttpAuth::SetAccessToken("", "", "", "");
        }
    }
}

const LoginTokenInfo& LoginTokenInfoResult::GetLoginTokenInfo() const { return TokenInfo; }

void LoginTokenInfoResult::FillLoginTokenInfo(
    const String& AccessToken, const String& AccessTokenExpiry, const String& RefreshToken, const String& RefreshTokenExpiry)
{
    SetResult(EResultCode::Success, static_cast<uint16_t>(web::EResponseCodes::ResponseOK));

    TokenInfo.AccessToken = AccessToken;
    TokenInfo.AccessExpiryTime = AccessTokenExpiry;
    TokenInfo.RefreshToken = RefreshToken;
    TokenInfo.RefreshExpiryTime = RefreshTokenExpiry;
}

void AgoraUserTokenResult::OnResponse(const services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto AuthResponse = static_cast<chs_aggregation::ServiceResponse*>(ApiResponse->GetDto());
    const web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == services::EResponseCode::ResponseSuccess)
    {
        AuthResponse->FromJson(Response->GetPayload().GetContent());
        std::shared_ptr<rapidjson::Document> Result = AuthResponse->GetOperationResult();

        if (!Result)
        {
            CSP_LOG_MSG(LogLevel::Error, "AgoraUserTokenResult invalid");

            return;
        }

        if (!Result->HasMember("token"))
        {
            CSP_LOG_MSG(LogLevel::Error, "AgoraUserTokenResult doesn't contain expected member: token");

            return;
        }

        SetValue(Result->operator[]("token").GetString());
    }
}

void CheckoutSessionUrlResult::OnResponse(const services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto CheckoutSessionResponse = static_cast<chs::StripeCheckoutSessionDto*>(ApiResponse->GetDto());
    const web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == services::EResponseCode::ResponseSuccess)
    {
        CheckoutSessionResponse->FromJson(Response->GetPayload().GetContent());

        if (CheckoutSessionResponse->HasCheckoutUrl())
        {
            SetValue(CheckoutSessionResponse->GetCheckoutUrl());
        }
    }
}

void CustomerPortalUrlResult::OnResponse(const services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto CustomerPortalResponse = static_cast<chs::StripeCustomerPortalDto*>(ApiResponse->GetDto());
    const web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == services::EResponseCode::ResponseSuccess)
    {
        CustomerPortalResponse->FromJson(Response->GetPayload().GetContent());

        if (CustomerPortalResponse->HasCustomerPortalUrl())
        {
            SetValue(CustomerPortalResponse->GetCustomerPortalUrl());
        }
    }
}

} // namespace csp::systems
