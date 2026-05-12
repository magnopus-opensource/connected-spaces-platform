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

#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Common/Settings.h"
#include "CSP/Common/SharedEnums.h"
#include "CSP/Common/String.h"
#include "Common/DateTime.h"

#include <memory>

namespace csp::common
{

class LoginStateData
{
public:
    LoginStateData()
        : State(ELoginState::LoggedOut)
        , AccessTokenRefreshTime(csp::common::DateTime())
    {
    }

    LoginStateData(const LoginStateData& Other)
        : State(Other.State)
        , AccessToken(Other.AccessToken)
        , AccessTokenExpiryLength(Other.AccessTokenExpiryLength)
        , RefreshToken(Other.RefreshToken)
        , RefreshTokenExpiryLength(Other.RefreshTokenExpiryLength)
        , UserId(Other.UserId)
        , DeviceId(Other.DeviceId)
        , DefaultApplicationSettings(Other.DefaultApplicationSettings)
        , DefaultSettings(Other.DefaultSettings)
        , AccessTokenRefreshTime(Other.AccessTokenRefreshTime)
    {
    }

    LoginStateData& operator=(const LoginStateData& Other)
    {
        if (this == &Other)
        {
            return *this;
        }

        State                       = Other.State;
        AccessToken                 = Other.AccessToken;
        AccessTokenExpiryLength     = Other.AccessTokenExpiryLength;
        RefreshToken                = Other.RefreshToken;
        RefreshTokenExpiryLength    = Other.RefreshTokenExpiryLength;
        UserId                      = Other.UserId;
        DeviceId                    = Other.DeviceId;
        DefaultApplicationSettings  = Other.DefaultApplicationSettings;
        DefaultSettings             = Other.DefaultSettings;

        AccessTokenRefreshTime = Other.AccessTokenRefreshTime;

        return *this;
    }

    ELoginState State = ELoginState::LoggedOut;
    csp::common::String AccessToken;
    csp::common::String AccessTokenExpiryLength;
    csp::common::String RefreshToken;
    csp::common::String RefreshTokenExpiryLength;
    csp::common::String UserId;
    csp::common::String DeviceId;

    /// @Brief Default, tenant-wide settings returned from the service, often used to store universal data such as feature flags.
    csp::common::List<csp::common::ApplicationSettings> DefaultApplicationSettings;

    /// @Brief Default settings relevant to the specific user returned from the service. Also known as "UserSettings".
    /// @see csp::systems::SettingsSystem
    csp::common::List<csp::common::SettingsCollection> DefaultSettings;

    CSP_NO_EXPORT void SetAccessTokenRefreshTime(const csp::common::DateTime& NewDateTime) { AccessTokenRefreshTime = NewDateTime; }
    CSP_NO_EXPORT const csp::common::DateTime& GetAccessTokenRefreshTime() const { return AccessTokenRefreshTime; }

private:
    csp::common::DateTime AccessTokenRefreshTime;
};

} // namespace csp::common
