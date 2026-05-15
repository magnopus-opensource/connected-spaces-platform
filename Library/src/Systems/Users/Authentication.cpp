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

#include "Common/Convert.h"

using namespace csp;
using namespace csp::common;
using namespace std::chrono;

namespace chs = csp::services::generated::userservice;
namespace chs_aggregation = csp::services::generated::aggregationservice;

namespace csp::systems
{

LoginStateResult::LoginStateResult()
    : m_state(nullptr)
{
}

LoginStateResult::LoginStateResult(csp::common::LoginState* inStatePtr)
    : m_state(inStatePtr)
{
}

const csp::common::LoginState& LoginStateResult::GetLoginState() const { return *m_state; }

namespace
{
    csp::common::ApplicationSettings MakeApplicationSetting(const csp::services::generated::userservice::ApplicationSettingsDto& setting)
    {
        csp::common::ApplicationSettings applicationSetting;
        applicationSetting.ApplicationName = setting.HasApplicationName() ? setting.GetApplicationName() : services::utility::string_t { "" };
        applicationSetting.Context = setting.HasContext() ? setting.GetContext() : services::utility::string_t { "" };
        applicationSetting.AllowAnonymous = setting.HasAllowAnonymous() ? setting.GetAllowAnonymous() : false;
        applicationSetting.Settings = setting.HasSettings() ? csp::common::Convert(setting.GetSettings()) : decltype(applicationSetting.Settings) {};
        return applicationSetting;
    }

    // Otherwise known as a UserSetting
    csp::common::SettingsCollection MakeSettingsCollection(const csp::services::generated::userservice::SettingsDto& setting)
    {
        csp::common::SettingsCollection settingsCollection;
        settingsCollection.UserId = setting.HasUserId() ? setting.GetUserId() : services::utility::string_t { "" };
        settingsCollection.Context = setting.HasContext() ? setting.GetContext() : services::utility::string_t { "" };
        settingsCollection.Settings = setting.HasSettings() ? csp::common::Convert(setting.GetSettings()) : decltype(settingsCollection.Settings) {};
        return settingsCollection;
    }
}

void LoginStateResult::OnResponse(const services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto authResponse = static_cast<chs::AuthDto*>(apiResponse->GetDto());
    const web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        authResponse->FromJson(response->GetPayload().GetContent());

        if (m_state)
        {
            m_state->State = ELoginState::LoggedIn;
            m_state->AccessToken = authResponse->GetAccessToken();
            m_state->RefreshToken = authResponse->GetRefreshToken();
            m_state->UserId = authResponse->GetUserId();
            m_state->DeviceId = authResponse->GetDeviceId();

            if (authResponse->HasDefaultSettings())
            {
                const auto& defaultSettings = authResponse->GetDefaultSettings();
                if (defaultSettings->HasDefaultUserSettings())
                {
                    const auto& userSettingsDto = defaultSettings->GetDefaultUserSettings();
                    for (const auto& settingDto : userSettingsDto)
                    {
                        m_state->DefaultSettings.Append(MakeSettingsCollection(*settingDto));
                    }
                }
                if (defaultSettings->HasDefaultApplicationSettings())
                {
                    const auto& applicationSettingsDto = defaultSettings->GetDefaultApplicationSettings();
                    for (const auto& settingDto : applicationSettingsDto)
                    {
                        m_state->DefaultApplicationSettings.Append(MakeApplicationSetting(*settingDto));
                    }
                }
            }

            const DateTime expiry(authResponse->GetAccessTokenExpiresAt());
            const DateTime currentTime(DateTime::UtcTimeNow());

            if (currentTime >= expiry)
            {
                CSP_LOG_FORMAT(LogLevel::Error, "AccessToken Expired: %s %s", authResponse->GetAccessToken().c_str(),
                    authResponse->GetAccessTokenExpiresAt().c_str());

                return;
            }

            web::HttpAuth::SetAccessToken(authResponse->GetAccessToken(), authResponse->GetAccessTokenExpiresAt(), authResponse->GetRefreshToken(),
                authResponse->GetRefreshTokenExpiresAt());

            // Schedule a Refresh of the Token 5 minutes before it expires
            system_clock::time_point refreshTimepoint = expiry.GetTimePoint() - system_clock::duration(5min);
            DateTime refreshTime(refreshTimepoint);

            if (refreshTime >= expiry)
            {
                CSP_LOG_FORMAT(LogLevel::Error, "RefreshToken Expired: %s %s", authResponse->GetRefreshToken().c_str(),
                    authResponse->GetRefreshTokenExpiresAt().c_str());

                return;
            }

            m_state->SetAccessTokenRefreshTime(refreshTime);

            // Signal login to anyone interested
            events::Event* loginEvent = events::EventSystem::Get().AllocateEvent(events::USERSERVICE_LOGIN_EVENT_ID);
            loginEvent->AddString("UserId", authResponse->GetUserId());
            events::EventSystem::Get().EnqueueEvent(loginEvent);

            SystemsManager::Get().GetUserSystem()->NotifyRefreshTokenHasChanged();
        }
    }
    else
    {
        if (m_state)
        {
            web::HttpAuth::SetAccessToken("", "", "", "");

            m_state->State = ELoginState::Error;
            m_state->AccessToken = "InvalidAccessToken";
            m_state->RefreshToken = "InvalidRefreshToken";
            m_state->UserId = "InvalidUserId";
            m_state->DeviceId = "InvalidDeviceId";
        }
    }
}

LogoutResult::LogoutResult()
    : m_state(nullptr)
{
}

LogoutResult::LogoutResult(csp::common::LoginState* inStatePtr)
    : NullResult(inStatePtr)
    , m_state(inStatePtr)
{
}

void LogoutResult::OnResponse(const services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    if (apiResponse->GetResponseCode() == services::EResponseCode::ResponseSuccess)
    {
        if (m_state)
        {
            m_state->State = ELoginState::LoggedOut;
            m_state->AccessToken = "InvalidAccessToken";
            m_state->RefreshToken = "InvalidRefreshToken";
            m_state->UserId = "InvalidUserId";
            m_state->DeviceId = "InvalidDeviceId";

            web::HttpAuth::SetAccessToken("", "", "", "");

            // Send logout event
            events::Event* logoutEvent = events::EventSystem::Get().AllocateEvent(events::USERSERVICE_LOGOUT_EVENT_ID);
            events::EventSystem::Get().EnqueueEvent(logoutEvent);
        }
    }
    else
    {
        if (m_state)
        {
            m_state->State = ELoginState::Error;
            m_state->AccessToken = "InvalidAccessToken";
            m_state->RefreshToken = "InvalidRefreshToken";
            m_state->UserId = "InvalidUserId";
            m_state->DeviceId = "InvalidDeviceId";

            web::HttpAuth::SetAccessToken("", "", "", "");
        }
    }
}

const LoginTokenInfo& LoginTokenInfoResult::GetLoginTokenInfo() const { return m_tokenInfo; }

void LoginTokenInfoResult::FillLoginTokenInfo(
    const String& accessToken, const String& accessTokenExpiry, const String& refreshToken, const String& refreshTokenExpiry)
{
    SetResult(EResultCode::Success, static_cast<uint16_t>(web::EResponseCodes::ResponseOK));

    m_tokenInfo.AccessToken = accessToken;
    m_tokenInfo.AccessExpiryTime = accessTokenExpiry;
    m_tokenInfo.RefreshToken = refreshToken;
    m_tokenInfo.RefreshExpiryTime = refreshTokenExpiry;
}

void CheckoutSessionUrlResult::OnResponse(const services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto checkoutSessionResponse = static_cast<chs::StripeCheckoutSessionDto*>(apiResponse->GetDto());
    const web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == services::EResponseCode::ResponseSuccess)
    {
        checkoutSessionResponse->FromJson(response->GetPayload().GetContent());

        if (checkoutSessionResponse->HasCheckoutUrl())
        {
            SetValue(checkoutSessionResponse->GetCheckoutUrl());
        }
    }
}

void CustomerPortalUrlResult::OnResponse(const services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto customerPortalResponse = static_cast<chs::StripeCustomerPortalDto*>(apiResponse->GetDto());
    const web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == services::EResponseCode::ResponseSuccess)
    {
        customerPortalResponse->FromJson(response->GetPayload().GetContent());

        if (customerPortalResponse->HasCustomerPortalUrl())
        {
            SetValue(customerPortalResponse->GetCustomerPortalUrl());
        }
    }
}

} // namespace csp::systems
