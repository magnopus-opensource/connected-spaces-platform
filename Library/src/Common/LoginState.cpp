/*
 * Copyright 2025 Magnopus LLC

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

#include "CSP/Common/LoginState.h"
#include "Common/LoginStateData.h"

namespace csp::common
{

LoginState::LoginState()
    : Data(std::make_unique<LoginStateData>())
{
}

LoginState::~LoginState() { }

LoginState::LoginState(const LoginState& OtherState) { CopyStateFrom(OtherState); }

LoginState& LoginState::operator=(const LoginState& OtherState)
{
    if (this != &OtherState)
    {
        CopyStateFrom(OtherState);
    }

    return *this;
}

LoginStateData LoginState::GetSnapshotThreadSafe() const
{
    std::scoped_lock lock(LoginStateMutex);

    return *Data;
}

void LoginState::SetLoginStateDataThreadSafe(LoginStateData NewData)
{
    std::scoped_lock lock(LoginStateMutex);

    *Data = std::move(NewData);
}

bool LoginState::TrySetLoginRequested()
{
    std::scoped_lock lock(LoginStateMutex);

    if (Data->State == csp::common::ELoginState::LoggedOut || Data->State == csp::common::ELoginState::Error)
    {
        Data->State = csp::common::ELoginState::LoginRequested;

        return true;
    }

    return false;
}

bool LoginState::TrySetLogoutRequested()
{
    std::scoped_lock lock(LoginStateMutex);

    if (Data->State == csp::common::ELoginState::LoggedIn)
    {
        Data->State = csp::common::ELoginState::LogoutRequested;

        return true;
    }

    return false;
}

void LoginState::ResetResponseLoginState(ELoginState LoginState)
{
    std::scoped_lock lock(LoginStateMutex);

    Data = std::make_unique<LoginStateData>();
    Data->State = LoginState;
}

void LoginState::UpdateAccessTokenExpiry(csp::common::String AccessTokenExpiryLength)
{
    std::scoped_lock lock(LoginStateMutex);

    Data->AccessTokenExpiryLength = AccessTokenExpiryLength;
}

void LoginState::UpdateRefreshTokenExpiry(csp::common::String RefreshTokenExpiryLength)
{
    std::scoped_lock lock(LoginStateMutex);

    Data->RefreshTokenExpiryLength = RefreshTokenExpiryLength;
}

bool LoginState::RefreshNeeded() const
{
    std::scoped_lock lock(LoginStateMutex);

    if (Data->GetAccessTokenRefreshTime().IsEpoch())
    {
        return false;
    }

    const auto CurrentTime = DateTime::UtcTimeNow();

    return CurrentTime >= (Data->GetAccessTokenRefreshTime());
}

csp::common::String LoginState::GetUserId() const
{
    std::scoped_lock lock(LoginStateMutex);

    return Data->UserId;
}

csp::common::String LoginState::GetDeviceId() const
{
    std::scoped_lock lock(LoginStateMutex);

    return Data->DeviceId;
}

csp::common::String LoginState::GetAccessToken() const
{
    std::scoped_lock lock(LoginStateMutex);

    return Data->AccessToken;
}

csp::common::String LoginState::GetRefreshToken() const
{
    std::scoped_lock lock(LoginStateMutex);

    return Data->RefreshToken;
}

csp::common::ELoginState LoginState::GetLoginStateValue() const
{
    std::scoped_lock lock(LoginStateMutex);

    return Data->State;
}

csp::common::List<csp::common::ApplicationSettings> LoginState::GetDefaultApplicationSettings() const
{
    std::scoped_lock lock(LoginStateMutex);

    return Data->DefaultApplicationSettings;
}

csp::common::List<csp::common::SettingsCollection> LoginState::GetDefaultSettings() const
{
    std::scoped_lock lock(LoginStateMutex);

    return Data->DefaultSettings;
}

void LoginState::CopyStateFrom(const LoginState& OtherState)
{
    std::scoped_lock lock(LoginStateMutex, OtherState.LoginStateMutex);

    Data = std::make_unique<LoginStateData>(*OtherState.Data);
}

} // namespace csp::common
