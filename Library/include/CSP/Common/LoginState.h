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

#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Common/SharedEnums.h"
#include "CSP/Common/String.h"

namespace csp::common
{

class DateTime;

/// @brief Data structure representing the user login state, including detection of access token expiry
class CSP_API LoginState
{
public:
    LoginState();
    ~LoginState();

    LoginState(const LoginState& OtherState);
    LoginState& operator=(const LoginState& OtherState);

    /// @brief Check if the access token for the login is expired.
    /// @return Is the token expired.
    [[nodiscard]] bool RefreshNeeded() const;

    ELoginState State;
    csp::common::String AccessToken;
    csp::common::String RefreshToken;
    csp::common::String UserId;
    csp::common::String DeviceId;

    CSP_NO_EXPORT void SetAccessTokenRefreshTime(const csp::common::DateTime& NewDateTime);

private:
    void CopyStateFrom(const LoginState& OtherState);

    csp::common::DateTime* AccessTokenRefreshTime;
};
} // namespace csp::common
