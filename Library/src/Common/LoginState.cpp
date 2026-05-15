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

#include "Common/DateTime.h"

namespace csp::common
{

LoginState::LoginState()
    : State(ELoginState::LoggedOut)
    , m_accessTokenRefreshTime(new DateTime())
{
}

LoginState::LoginState(const LoginState& otherState) { CopyStateFrom(otherState); }

LoginState& LoginState::operator=(const LoginState& otherState)
{
    CopyStateFrom(otherState);

    return *this;
}

void LoginState::SetAccessTokenRefreshTime(const csp::common::DateTime& newDateTime)
{
    // Why this data member is a pointer is beyond me, but it's too bedded in to change right this second ... invoke the copy assignment operator.
    *m_accessTokenRefreshTime = newDateTime;
}

void LoginState::CopyStateFrom(const LoginState& otherState)
{
    State = otherState.State;
    AccessToken = otherState.AccessToken;
    AccessTokenExpiryLength = otherState.AccessTokenExpiryLength;
    RefreshToken = otherState.RefreshToken;
    RefreshTokenExpiryLength = otherState.RefreshTokenExpiryLength;
    UserId = otherState.UserId;
    DeviceId = otherState.DeviceId;
    DefaultApplicationSettings = otherState.DefaultApplicationSettings;
    DefaultSettings = otherState.DefaultSettings;

    // Must reallocate the access token when copying otherwise destructor of
    // copied state will delete the original memory pointer potentially causing corruption
    m_accessTokenRefreshTime = new DateTime(otherState.m_accessTokenRefreshTime->GetTimePoint());
}

LoginState::~LoginState() { delete (m_accessTokenRefreshTime); }

bool LoginState::RefreshNeeded() const
{
    if (m_accessTokenRefreshTime->IsEpoch())
    {
        return false;
    }

    const auto currentTime = DateTime::UtcTimeNow();

    return currentTime >= (*m_accessTokenRefreshTime);
}
} // namespace csp::common
