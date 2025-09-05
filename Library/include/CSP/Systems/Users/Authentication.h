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

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

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

/// @brief Data structure for an Agora user token, giving userID, referenceID, channel name and settings regarding sharing of audio/video/screenshare.
class CSP_API AgoraUserTokenParams
{
public:
    /// @brief The unique identifer for the user requesting the token.
    csp::common::String AgoraUserId;

    /// @brief The unique name for the Agora channel being joined. It can be set to any string combination. For group calls all users must reference
    /// the same channelName.
    csp::common::String ChannelName;

    /// @brief The unique identfier for the space being joined. Only needs to be set if the channelName is not set to the space ID, so the appropriate
    /// permissions can be requested. It can be set to an empty string if not required.
    csp::common::String ReferenceId;

    /// @brief The amount of time the token is valid for in milliseconds.
    int Lifespan;

    /// @brief If the token is ready only.
    bool ReadOnly;

    /// @brief If the token is configured for sharing of audio.
    bool ShareAudio;

    /// @brief If the token is configured for sharing of video.
    bool ShareVideo;

    /// @brief If the token is configured for sharing of the user's screen.
    bool ShareScreen;
};

/// @brief Data structure for a custom service proxy posting, giving service name, operation name, set help and parameters
class CSP_API TokenInfoParams
{
public:
    /// @brief The service name for the requested token.
    csp::common::String ServiceName;

    /// @brief The operation name for the requested token.
    csp::common::String OperationName;

    /// @brief Whether to set help.
    bool SetHelp;

    /// @brief Map of parameters required for the operation on the service
    csp::common::Map<csp::common::String, csp::common::String> Parameters;
};

/// @brief Data structure for overrides to the default token token options
class CSP_API TokenOptions
{
public:
    /// @brief The length of time for a token to expire formatted as "HH:MM:SS", must be between "00:00:01" and "00:30:00"
    csp::common::String ExpiryLength;
};

typedef std::function<void(const LoginStateResult& Result)> LoginStateResultCallback;
typedef std::function<void(const NullResult& Result)> NullResultCallback;
typedef std::function<void(const LoginTokenInfoResult& Result)> LoginTokenInfoResultCallback;

} // namespace csp::systems
