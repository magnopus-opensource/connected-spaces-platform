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


enum class ELoginState : uint8_t
{
	LoginThirdPartyProviderDetailsRequested,
	LoginRequested,
	LoggedIn,
	LogoutRequested,
	LoggedOut,
	Error,
};


/// @brief Data structure representing the user login state, including detection of access token expiry
class CSP_API LoginState
{
	friend class LoginStateResult;

public:
	LoginState();
	~LoginState();

	LoginState(const LoginState& OtherState);
	LoginState& operator=(const LoginState& OtherState);

	/// @brief Check if the access token for the login is expired.
	/// @return Is the token expired.
	bool RefreshNeeded() const;

	ELoginState State;
	csp::common::String AccessToken;
	csp::common::String RefreshToken;
	csp::common::String UserId;
	csp::common::String DeviceId;

private:
	void CopyStateFrom(const LoginState& OtherState);

	csp::common::DateTime* AccessTokenRefreshTime;
};


/// @brief Data for access and refresh tokens, and their expiry times.
class CSP_API LoginTokenInfo
{
public:
	csp::common::String AccessToken;
	csp::common::String AccessExpiryTime;
	csp::common::String RefreshToken;
	csp::common::String RefreshExpiryTime;
};


enum class ELoginStateResultFailureReason
{
	Unknown = -1,
	None	= 0,
	AgeNotVerified,
	EmailNotConfirmed
};


/// @brief Result structure for a login state request.
class CSP_API LoginStateResult : public csp::services::ResultBase
{
	/** @cond DO_NOT_DOCUMENT */
	CSP_START_IGNORE
	template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
	friend class UserSystem;
	CSP_END_IGNORE
	/** @endcond */

public:
	LoginState& GetLoginState();
	const LoginState& GetLoginState() const;

protected:
	int ParseErrorCode(const csp::common::String& Value) override;

private:
	LoginStateResult();
	LoginStateResult(LoginState* InStatePtr);

	void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

	LoginState* State;
};


/// @brief Result structure for a logout state request.
class CSP_API LogoutResult : public NullResult
{
	/** @cond DO_NOT_DOCUMENT */
	CSP_START_IGNORE
	template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
	friend class UserSystem;
	CSP_END_IGNORE
	/** @endcond */

private:
	LogoutResult();
	LogoutResult(LoginState* InStatePtr);

	void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

	LoginState* State;
};


/// @ingroup User System
/// @brief @brief Data class used to contain information when the login token has changed
class CSP_API LoginTokenReceived : public csp::services::ResultBase
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

	void FillLoginTokenInfo(const csp::common::String& AccessToken,
							const csp::common::String& AuthTokenExpiry,
							const csp::common::String& RefreshToken,
							const csp::common::String& RefreshTokenExpiry);

	LoginTokenInfo LoginTokenInfo;
};


/// @ingroup User System
/// @brief @brief Data class used to contain information when a ping response is received
class CSP_API PingResponseReceived : public csp::services::ResultBase
{
	/** @cond DO_NOT_DOCUMENT */
	CSP_START_IGNORE
	template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
	friend class UserSystem;
	CSP_END_IGNORE
	/** @endcond */

public:
private:
	PingResponseReceived(void*) {};
	PingResponseReceived() {};
};


/// @brief Data structure for an Agora user token, giving userID, channel name and settings regarding sharing of audio/video/screenshare.
class CSP_API AgoraUserTokenParams
{
public:
	csp::common::String AgoraUserId;
	int Lifespan;
	csp::common::String ChannelName;
	bool ReadOnly;
	bool ShareAudio;
	bool ShareVideo;
	bool ShareScreen;
};


/// @ingroup User System
/// @brief @brief Data class used to contain information requesting a user token
class CSP_API AgoraUserTokenResult : public csp::services::ResultBase
{
	/** @cond DO_NOT_DOCUMENT */
	CSP_START_IGNORE
	template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
	friend class UserSystem;
	CSP_END_IGNORE
	/** @endcond */

public:
	const csp::common::String& GetUserToken() const;
	const csp::common::String& GetUserToken();

private:
	AgoraUserTokenResult(void*) {};
	AgoraUserTokenResult() = default;

	void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

	csp::common::String UserToken;
};


typedef std::function<void(LoginStateResult& Result)> LoginStateResultCallback;
typedef std::function<void(LogoutResult& Result)> LogoutResultCallback;

typedef std::function<void(LoginTokenReceived& Result)> NewLoginTokenReceivedCallback;

typedef std::function<void(PingResponseReceived& Result)> PingResponseReceivedCallback;

typedef std::function<void(AgoraUserTokenResult& Result)> UserTokenResultCallback;



} // namespace csp::systems
