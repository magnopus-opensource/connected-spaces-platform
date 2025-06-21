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
    , AccessTokenRefreshTime(new DateTime())
{
}

LoginState::LoginState(const LoginState& OtherState) { CopyStateFrom(OtherState); }

LoginState& LoginState::operator=(const LoginState& OtherState)
{
    CopyStateFrom(OtherState);

    return *this;
}

void LoginState::SetAccessTokenRefreshTime(const csp::common::DateTime& NewDateTime)
{
    // Why this data member is a pointer is beyond me, but it's too bedded in to change right this second ... invoke the copy assignment operator.
    *AccessTokenRefreshTime = NewDateTime;
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
    AccessTokenRefreshTime = new DateTime(OtherState.AccessTokenRefreshTime->GetTimePoint());
}

LoginState::~LoginState() { delete (AccessTokenRefreshTime); }

bool LoginState::RefreshNeeded() const
{
    if (AccessTokenRefreshTime->IsEpoch())
    {
        return false;
    }

    const auto CurrentTime = DateTime::UtcTimeNow();

    return CurrentTime >= (*AccessTokenRefreshTime);
}
} // namespace csp::common
