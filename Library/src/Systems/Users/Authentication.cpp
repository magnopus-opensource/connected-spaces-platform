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

#include "CSP/Common/LoginState.h"
#include "Common/LoginStateData.h"
#include "CSP/Common/Settings.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "Common/DateTime.h"
#include "Debug/Logging.h"
#include "Events/EventSystem.h"
#include "Services/AggregationService/Api.h"
#include "Services/ApiBase/ApiBase.h"
#include "Services/UserService/Dto.h"
#include "Systems/Users/Authentication.h"

using namespace csp;
using namespace csp::common;

namespace chs = csp::services::generated::userservice;
namespace chs_aggregation = csp::services::generated::aggregationservice;

namespace csp::systems
{

LoginStateResult::LoginStateResult()
    : State(nullptr)
{
}

LoginStateResult::LoginStateResult(csp::common::LoginState* LoginState)
    : State(LoginState)
{
}

const csp::common::LoginState& LoginStateResult::GetLoginState() const { return *State; }

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
            auto DataOpt = csp::common::AuthDtoToLoginStateData(AuthResponse);

            if (DataOpt.has_value() == false)
            {
                return;
            }

            auto Data = *DataOpt;

            State->SetLoginStateDataThreadSafe(Data);

            web::HttpAuth::SetAccessToken(AuthResponse->GetAccessToken(), AuthResponse->GetAccessTokenExpiresAt(), AuthResponse->GetRefreshToken(),
                AuthResponse->GetRefreshTokenExpiresAt());

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

            State->ReinitializeResponseLoginState(ELoginState::Error);
        }
    }
}

LogoutResult::LogoutResult()
    : State(nullptr)
{
}

LogoutResult::LogoutResult(csp::common::LoginState* LoginState)
    : NullResult()
    , State(LoginState)
{
}

void LogoutResult::OnResponse(const services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    if (ApiResponse->GetResponseCode() == services::EResponseCode::ResponseSuccess)
    {
        if (State)
        {
            State->ReinitializeResponseLoginState(ELoginState::LoggedOut);

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
            State->ReinitializeResponseLoginState(ELoginState::Error);

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
