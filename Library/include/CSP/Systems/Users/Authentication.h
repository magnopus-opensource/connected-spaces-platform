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

#pragma once

#include "CSP/Common/Array.h"
#include "CSP/Common/LoginState.h"
#include "CSP/Common/String.h"
#include "CSP/Systems/SystemsResult.h"

namespace csp::common
{

class DateTime;

} // namespace csp::common

namespace csp::services
{

CSP_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
CSP_END_IGNORE

} // namespace csp::services

namespace csp::systems
{

class UserSystem;
class LoginStateResult;

/// @brief Data for access and refresh tokens, and their expiry times.
class CSP_API LoginTokenInfo
{
public:
    csp::common::String AccessToken;
    csp::common::String AccessExpiryTime;
    csp::common::String RefreshToken;
    csp::common::String RefreshExpiryTime;
};

/// @brief Result structure for a login state request.
class CSP_API LoginStateResult : public ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    friend class UserSystem;
    CSP_END_IGNORE
    /** @endcond */

public:
    [[nodiscard]] const csp::common::LoginState& GetLoginState() const;
    CSP_NO_EXPORT LoginStateResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode)
        , State(nullptr) {};

private:
    LoginStateResult();
    LoginStateResult(csp::common::LoginState* InStatePtr);

    CSP_NO_EXPORT void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    csp::common::LoginState* State;
};

/// @ingroup User System
/// @brief @brief Data class used to contain information when the login token has changed
class CSP_API LoginTokenInfoResult : public ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class UserSystem;
    /** @endcond */

public:
    [[nodiscard]] const LoginTokenInfo& GetLoginTokenInfo() const;

private:
    LoginTokenInfoResult() = default;
    LoginTokenInfoResult(void*) {};

    void FillLoginTokenInfo(const csp::common::String& AccessToken, const csp::common::String& AuthTokenExpiry,
        const csp::common::String& RefreshToken, const csp::common::String& RefreshTokenExpiry);

    LoginTokenInfo TokenInfo;
};

/// @brief Data structure for overrides to the default token options
class CSP_API TokenOptions
{
public:
    /// @brief The length of time for the access token to expire formatted as "HH:MM:SS", must be between "00:00:01" and "00:30:00"
    /// The default token expiry length is configured by MCS and defaults to 30 minutes. Value must be less than the default expiry length, or it will
    /// be ignored.
    csp::common::String AccessTokenExpiryLength;

    /// @brief The length of time for the refresh token to expire formatted as "HH:MM:SS", must be between "00:00:01" and "168:00:00" (eq. 7 days)
    /// The default token expiry length is configured by MCS and defaults to 7 days. Value must be less than the default expiry length, or it will
    /// be ignored.
    csp::common::String RefreshTokenExpiryLength;
};

typedef std::function<void(const LoginStateResult& Result)> LoginStateResultCallback;
typedef std::function<void(const NullResult& Result)> NullResultCallback;
typedef std::function<void(const LoginTokenInfoResult& Result)> LoginTokenInfoResultCallback;

} // namespace csp::systems
