#pragma once

#include "Olympus/Common/Array.h"
#include "Olympus/Common/String.h"
#include "Olympus/Systems/SystemsResult.h"

namespace oly_common
{
class DateTime;
}

namespace oly_services
{

OLY_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
OLY_END_IGNORE

} // namespace oly_services

namespace oly_systems
{

class UserSystem;
class LoginStateResult;

enum class ELoginState : uint8_t
{
    LoginThirdPartyProviderDetailsRequested,
    LoginRequested,
    LoggedIn,
    LogoutRequested,
    LoggedOut,
    Error,
};

class OLY_API LoginState
{
    friend class LoginStateResult;

public:
    LoginState();
    ~LoginState();

    LoginState(const LoginState& OtherState);
    LoginState& operator=(const LoginState& OtherState);

    bool RefreshNeeded() const;

    ELoginState State;
    oly_common::String AccessToken;
    oly_common::String RefreshToken;
    oly_common::String UserId;
    oly_common::String DeviceId;

private:
    void CopyStateFrom(const LoginState& OtherState);

    oly_common::DateTime* AccessTokenRefreshTime;
};

class OLY_API LoginTokenInfo
{
public:
    oly_common::String AccessToken;
    oly_common::String AccessExpiryTime;
    oly_common::String RefreshToken;
    oly_common::String RefreshExpiryTime;
};

class OLY_API LoginStateResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    friend class UserSystem;
    OLY_END_IGNORE
    /** @endcond */

public:
    LoginState& GetLoginState();
    const LoginState& GetLoginState() const;

private:
    LoginStateResult();
    LoginStateResult(LoginState* InStatePtr);

    void OnResponse(const oly_services::ApiResponseBase* ApiResponse) override;

private:
    LoginState* State;
};

class OLY_API LogoutResult : public NullResult
{
    /** @cond DO_NOT_DOCUMENT */
    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    friend class UserSystem;
    OLY_END_IGNORE
    /** @endcond */

private:
    LogoutResult();
    LogoutResult(LoginState* InStatePtr);

    void OnResponse(const oly_services::ApiResponseBase* ApiResponse) override;

    LoginState* State;
};

/// @ingroup User System
/// @brief @brief Data class used to contain information when the login token has changed
class OLY_API LoginTokenReceived : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class UserSystem;
    /** @endcond */

public:
    LoginTokenInfo& GetLoginTokenInfo();
    const LoginTokenInfo& GetLoginTokenInfo() const;

private:
    LoginTokenReceived(void*) {};
    LoginTokenReceived() {};

    void FillLoginTokenInfo(const oly_common::String& AccessToken, const oly_common::String& AuthTokenExpiry, const oly_common::String& RefreshToken,
        const oly_common::String& RefreshTokenExpiry);

    LoginTokenInfo LoginTokenInfo;
};

/// @ingroup User System
/// @brief @brief Data class used to contain information when a ping response is received
class OLY_API PingResponseReceived : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    friend class UserSystem;
    OLY_END_IGNORE
    /** @endcond */

public:
private:
    PingResponseReceived(void*) {};
    PingResponseReceived() {};
};

class OLY_API AgoraUserTokenParams
{
public:
    oly_common::String AgoraUserId;
    int Lifespan;
    oly_common::String ChannelName;
    bool ReadOnly;
    bool ShareAudio;
    bool ShareVideo;
    bool ShareScreen;
};

/// @ingroup User System
/// @brief @brief Data class used to contain information requesting a user token
class OLY_API AgoraUserTokenResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    friend class UserSystem;
    OLY_END_IGNORE
    /** @endcond */

public:
    const oly_common::String& GetUserToken() const;
    const oly_common::String& GetUserToken();

private:
    AgoraUserTokenResult(void*) {};
    AgoraUserTokenResult() = default;

    void OnResponse(const oly_services::ApiResponseBase* ApiResponse) override;

    oly_common::String UserToken;
};

typedef std::function<void(LoginStateResult& Result)> LoginStateResultCallback;
typedef std::function<void(LogoutResult& Result)> LogoutResultCallback;

typedef std::function<void(LoginTokenReceived& Result)> NewLoginTokenReceivedCallback;

typedef std::function<void(PingResponseReceived& Result)> PingResponseReceivedCallback;

typedef std::function<void(AgoraUserTokenResult& Result)> UserTokenResultCallback;

} // namespace oly_systems
