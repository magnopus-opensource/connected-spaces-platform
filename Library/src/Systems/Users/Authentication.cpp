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
#include "CSP/Systems/Users/Authentication.h"

#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "Common/DateTime.h"
#include "Debug/Logging.h"
#include "Events/EventSystem.h"
#include "Services/AggregationService/Api.h"
#include "Services/ApiBase/ApiBase.h"
#include "Services/UserService/Dto.h"


using namespace std::chrono;
namespace chs			  = csp::services::generated::userservice;
namespace chs_aggregation = csp::services::generated::aggregationservice;


namespace csp::systems
{

LoginState::LoginState() : State(ELoginState::LoggedOut), AccessTokenRefreshTime(CSP_NEW csp::common::DateTime())
{
}

LoginState::LoginState(const LoginState& OtherState)
{
	CopyStateFrom(OtherState);
}

LoginState& LoginState::operator=(const LoginState& OtherState)
{
	CopyStateFrom(OtherState);

	return *this;
}

void LoginState::CopyStateFrom(const LoginState& OtherState)
{
	State		 = OtherState.State;
	AccessToken	 = OtherState.AccessToken;
	RefreshToken = OtherState.RefreshToken;
	UserId		 = OtherState.UserId;
	DeviceId	 = OtherState.DeviceId;

	// Must reallocate the access token when copying otherwise destructor of
	// copied state will delete the original memory pointer potentially causing corruption
	AccessTokenRefreshTime = CSP_NEW csp::common::DateTime(OtherState.AccessTokenRefreshTime->GetTimePoint());
}


LoginState::~LoginState()
{
	CSP_DELETE(AccessTokenRefreshTime);
}

bool LoginState::RefreshNeeded() const
{
	if (AccessTokenRefreshTime->IsEpoch())
	{
		return false;
	}

	const auto CurrentTime = csp::common::DateTime::UtcTimeNow();

	return CurrentTime >= (*AccessTokenRefreshTime);
}


LoginStateResult::LoginStateResult() : State(nullptr)
{
}

LoginStateResult::LoginStateResult(LoginState* InStatePtr) : State(InStatePtr)
{
}

LoginState& LoginStateResult::GetLoginState()
{
	return *State;
}

const LoginState& LoginStateResult::GetLoginState() const
{
	return *State;
}

void LoginStateResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
	ResultBase::OnResponse(ApiResponse);

	auto AuthResponse					   = static_cast<chs::AuthDto*>(ApiResponse->GetDto());
	const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

	if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
	{
		// Build the Dto from the response Json
		AuthResponse->FromJson(Response->GetPayload().GetContent());

		if (State)
		{
			State->State		= ELoginState::LoggedIn;
			State->AccessToken	= AuthResponse->GetAccessToken();
			State->RefreshToken = AuthResponse->GetRefreshToken();
			State->UserId		= AuthResponse->GetUserId();
			State->DeviceId		= AuthResponse->GetDeviceId();

			const csp::common::DateTime Expiry(AuthResponse->GetAccessTokenExpiresAt());
			const csp::common::DateTime CurrentTime(csp::common::DateTime::UtcTimeNow());

			if (CurrentTime >= Expiry)
			{
				CSP_LOG_FORMAT(csp::systems::LogLevel::Error,
							   "AccessToken Expired: %s %s",
							   AuthResponse->GetAccessToken().c_str(),
							   AuthResponse->GetAccessTokenExpiresAt().c_str());

				return;
			}

			csp::web::HttpAuth::SetAccessToken(AuthResponse->GetAccessToken(),
											   AuthResponse->GetAccessTokenExpiresAt(),
											   AuthResponse->GetRefreshToken(),
											   AuthResponse->GetRefreshTokenExpiresAt());

			// Schedule a Refresh of the Token 5 minutes before it expires
			system_clock::time_point RefreshTimepoint = Expiry.GetTimePoint() - system_clock::duration(5min);
			csp::common::DateTime RefreshTime(RefreshTimepoint);

			if (RefreshTime >= Expiry)
			{
				CSP_LOG_FORMAT(csp::systems::LogLevel::Error,
							   "RefreshToken Expired: %s %s",
							   AuthResponse->GetRefreshToken().c_str(),
							   AuthResponse->GetRefreshTokenExpiresAt().c_str());

				return;
			}

			*(State->AccessTokenRefreshTime) = RefreshTime;

			// Signal login to anyone interested
			csp::events::Event* LoginEvent = csp::events::EventSystem::Get().AllocateEvent(csp::events::USERSERVICE_LOGIN_EVENT_ID);
			LoginEvent->AddString("UserId", AuthResponse->GetUserId());
			csp::events::EventSystem::Get().EnqueueEvent(LoginEvent);
		}
	}
	else
	{
		if (State)
		{
			csp::web::HttpAuth::SetAccessToken("", "", "", "");

			State->State		= ELoginState::Error;
			State->AccessToken	= "InvalidAccessToken";
			State->RefreshToken = "InvalidRefreshToken";
			State->UserId		= "InvalidUserId";
			State->DeviceId		= "InvalidDeviceId";
		}
	}
}


LogoutResult::LogoutResult() : State(nullptr)
{
}

LogoutResult::LogoutResult(LoginState* InStatePtr) : NullResult(InStatePtr), State(InStatePtr)
{
}

void LogoutResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
	ResultBase::OnResponse(ApiResponse);

	if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
	{
		if (State)
		{
			State->State		= ELoginState::LoggedOut;
			State->AccessToken	= "InvalidAccessToken";
			State->RefreshToken = "InvalidRefreshToken";
			State->UserId		= "InvalidUserId";
			State->DeviceId		= "InvalidDeviceId";

			csp::web::HttpAuth::SetAccessToken("", "", "", "");

			// Send logout event
			csp::events::Event* LogoutEvent = csp::events::EventSystem::Get().AllocateEvent(csp::events::USERSERVICE_LOGOUT_EVENT_ID);
			csp::events::EventSystem::Get().EnqueueEvent(LogoutEvent);
		}
	}
	else
	{
		if (State)
		{
			State->State		= ELoginState::Error;
			State->AccessToken	= "InvalidAccessToken";
			State->RefreshToken = "InvalidRefreshToken";
			State->UserId		= "InvalidUserId";
			State->DeviceId		= "InvalidDeviceId";

			csp::web::HttpAuth::SetAccessToken("", "", "", "");
		}
	}
}


LoginTokenInfo& LoginTokenReceived::GetLoginTokenInfo()
{
	return LoginTokenInfo;
}

const LoginTokenInfo& LoginTokenReceived::GetLoginTokenInfo() const
{
	return LoginTokenInfo;
}

void LoginTokenReceived::FillLoginTokenInfo(const csp::common::String& AccessToken,
											const csp::common::String& AccessTokenExpiry,
											const csp::common::String& RefreshToken,
											const csp::common::String& RefreshTokenExpiry)
{
	SetResult(csp::systems::EResultCode::Success, static_cast<uint16_t>(csp::web::EResponseCodes::ResponseOK));

	LoginTokenInfo.AccessToken		 = AccessToken;
	LoginTokenInfo.AccessExpiryTime	 = AccessTokenExpiry;
	LoginTokenInfo.RefreshToken		 = RefreshToken;
	LoginTokenInfo.RefreshExpiryTime = RefreshTokenExpiry;
}

const csp::common::String& AgoraUserTokenResult::GetUserToken() const
{
	return UserToken;
}

const csp::common::String& AgoraUserTokenResult::GetUserToken()
{
	return UserToken;
}

void AgoraUserTokenResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
	csp::systems::ResultBase::OnResponse(ApiResponse);

	auto AuthResponse					   = static_cast<chs_aggregation::ServiceResponse*>(ApiResponse->GetDto());
	const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

	if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
	{
		AuthResponse->FromJson(Response->GetPayload().GetContent());

		std::shared_ptr<rapidjson::Document> Result = AuthResponse->GetOperationResult();

		if (!Result)
		{
			CSP_LOG_MSG(csp::systems::LogLevel::Error, "AgoraUserTokenResult invalid");
			return;
		}

		if (!Result->HasMember("token"))
		{
			CSP_LOG_MSG(csp::systems::LogLevel::Error, "AgoraUserTokenResult doesn't contain expected member: token");
			return;
		}

		UserToken = Result->operator[]("token").GetString();
	}
}

} // namespace csp::systems
