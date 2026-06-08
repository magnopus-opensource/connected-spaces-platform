/*
 * Copyright 2026 Magnopus LLC

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

#include "LoginStateData.h"

#include "CSP/Common/Settings.h"
#include "Common/Convert.h"
#include "Common/DateTime.h"
#include "Debug/Logging.h"
#include "Services/ApiBase/ApiBase.h"
#include "Services/UserService/Dto.h"

namespace
{
csp::common::ApplicationSettings MakeApplicationSetting(const csp::services::generated::userservice::ApplicationSettingsDto& Setting)
{
    csp::common::ApplicationSettings ApplicationSetting;
    ApplicationSetting.ApplicationName = Setting.HasApplicationName() ? Setting.GetApplicationName() : csp::services::utility::string_t { "" };
    ApplicationSetting.Context = Setting.HasContext() ? Setting.GetContext() : csp::services::utility::string_t { "" };
    ApplicationSetting.AllowAnonymous = Setting.HasAllowAnonymous() ? Setting.GetAllowAnonymous() : false;
    ApplicationSetting.Settings = Setting.HasSettings() ? csp::common::Convert(Setting.GetSettings()) : decltype(ApplicationSetting.Settings) {};
    return ApplicationSetting;
}

// Otherwise known as a UserSetting
csp::common::SettingsCollection MakeSettingsCollection(const csp::services::generated::userservice::SettingsDto& Setting)
{
    csp::common::SettingsCollection SettingsCollection;
    SettingsCollection.UserId = Setting.HasUserId() ? Setting.GetUserId() : csp::services::utility::string_t { "" };
    SettingsCollection.Context = Setting.HasContext() ? Setting.GetContext() : csp::services::utility::string_t { "" };
    SettingsCollection.Settings = Setting.HasSettings() ? csp::common::Convert(Setting.GetSettings()) : decltype(SettingsCollection.Settings) {};
    return SettingsCollection;
}
} // namespace

namespace csp::common
{

LoginStateData AuthDtoToLoginStateData(const csp::services::generated::userservice::AuthDto* AuthResponse)
{
    auto Data = LoginStateData();

    Data.State = ELoginState::LoggedIn;
    Data.AccessToken = AuthResponse->GetAccessToken();
    Data.RefreshToken = AuthResponse->GetRefreshToken();
    Data.UserId = AuthResponse->GetUserId();
    Data.DeviceId = AuthResponse->GetDeviceId();

    if (AuthResponse->HasDefaultSettings())
    {
        const auto& DefaultSettings = AuthResponse->GetDefaultSettings();
        if (DefaultSettings->HasDefaultUserSettings())
        {
            const auto& UserSettingsDto = DefaultSettings->GetDefaultUserSettings();
            for (const auto& SettingDto : UserSettingsDto)
            {
                Data.DefaultSettings.Append(MakeSettingsCollection(*SettingDto));
            }
        }
        if (DefaultSettings->HasDefaultApplicationSettings())
        {
            const auto& ApplicationSettingsDto = DefaultSettings->GetDefaultApplicationSettings();
            for (const auto& SettingDto : ApplicationSettingsDto)
            {
                Data.DefaultApplicationSettings.Append(MakeApplicationSetting(*SettingDto));
            }
        }
    }

    const DateTime Expiry(AuthResponse->GetAccessTokenExpiresAt());
    const DateTime CurrentTime(DateTime::UtcTimeNow());

    if (CurrentTime >= Expiry)
    {
        CSP_LOG_FORMAT(
            LogLevel::Error, "AccessToken Expired: %s %s", AuthResponse->GetAccessToken().c_str(), AuthResponse->GetAccessTokenExpiresAt().c_str());

        return Data;
    }

    // Schedule a Refresh of the Token 5 minutes before it expires
    std::chrono::system_clock::time_point RefreshTimepoint = Expiry.GetTimePoint() - std::chrono::system_clock::duration(std::chrono::minutes(5));
    DateTime RefreshTime(RefreshTimepoint);

    if (RefreshTime >= Expiry)
    {
        CSP_LOG_FORMAT(LogLevel::Error, "RefreshToken Expired: %s %s", AuthResponse->GetRefreshToken().c_str(),
            AuthResponse->GetRefreshTokenExpiresAt().c_str());

        return Data;
    }

    Data.SetAccessTokenRefreshTime(RefreshTime);

    return Data;
}

} // namespace csp::common
