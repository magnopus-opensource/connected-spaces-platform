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

#include "CSP/Systems/Users/UserSystem.h"

#include "CSP/CSPFoundation.h"
#include "CSP/Common/LoginState.h"
#include "CSP/Common/NetworkEventData.h"
#include "CSP/Common/SharedEnums.h"
#include "CSP/Common/StringFormat.h"
#include "CSP/Systems/Users/Authentication.h"
#include "CSP/Systems/Users/Profile.h"
#include "Common/Convert.h"
#include "Common/LoginStateData.h"
#include "Common/UUIDGenerator.h"
#include "Events/EventSystem.h"
#include "Multiplayer/NetworkEventSerialisation.h"
#include "Services/UserService/Api.h"
#include "Systems/ResultHelpers.h"
#include "Systems/Users/Authentication.h"

#include "CallHelpers.h"
#include <regex>

namespace chs_user = csp::services::generated::userservice;

namespace
{

/* Connect our main network connection, serving both out-of-space messaging, as well as in space messages, via SignalR method
 * bindings. All methods are (or at least should be) bound here, including the NetworkEventBus. It may surprise you that the methods are
 * never unbound until logout, when the MultiplayerConnection is destroyed. We may bind the methods for in-space networking,
 * but we only do anything with the data we receive if an IRealtimeEngine is set, which is set/unset on space entry.
 *
 * This does a fair amount more than binding and starting the connection, which it perhaps shouldn't.
 * Does a lot of state management resetting entities and such, as well as registering callbacks.
 *
 * CreateMultiplayerConnection supports offline flows, if false this function does merely calls the ConnectionCallback.
 *
 * This dependency needs to be broken prior to formal modularization, I suspect by injecting the MultiplayerConnection much like we inject
 * the RealtimeEngine. */
void StartMultiplayerConnection(csp::multiplayer::MultiplayerConnection& MultiplayerConnection, const csp::common::String& MultiplayerURI,
    csp::multiplayer::MultiplayerConnection::ErrorCodeCallbackHandler ConnectionCallback, const csp::systems::LoginStateResult& LoginStateRes,
    csp::common::LogSystem& LogSystem, bool CreateMultiplayerConnection)
{
    if (CreateMultiplayerConnection)
    {
        LogSystem.LogMsg(csp::common::LogLevel::Log, "Starting Multiplayer Connection");

        const auto Data = LoginStateRes.GetLoginState().GetSnapshotThreadSafe();
        MultiplayerConnection.Connect(ConnectionCallback, MultiplayerURI, Data->AccessToken, Data->DeviceId);
    }
    else
    {
        LogSystem.LogMsg(csp::common::LogLevel::Log, "Not starting a Multiplayer Connection");
        ConnectionCallback(csp::multiplayer::ErrorCode::None);
    }
}

/* Check if the provided expiry length in token options is formatted as "HH:MM:SS" or "HHH:MM:SS"
 *
 * Return True if expiry length matches format "HH:MM:SS" or "HHH:MM:SS", false otherwise
 *
 * "HHH:MM:SS" supports durations greater than 4 days */
bool CheckExpiryLengthFormat(const csp::common::String& ExpiryLength)
{
    if (ExpiryLength.IsEmpty())
    {
        return false;
    }

    std::regex Regex("^[0-9]{2,3}:[0-5][0-9]:[0-5][0-9]$");
    if (std::regex_search(ExpiryLength.c_str(), Regex))
    {
        return true;
    }

    CSP_LOG_MSG(csp::common::LogLevel::Warning, "Expiry length token option does not match the expected format, and has been ignored.");
    return false;
}

std::optional<csp::common::String> ThirdPartyPlatformToString(const csp::common::Optional<csp::systems::EThirdPartyPlatform>& ClientType)
{
    if (!ClientType.HasValue())
    {
        return std::nullopt;
    }

    switch (*ClientType)
    {
    case csp::systems::EThirdPartyPlatform::Unreal:
    {
        return "Unreal";
    }
    case csp::systems::EThirdPartyPlatform::Unity:
    {
        return "Unity";
    }
    case csp::systems::EThirdPartyPlatform::Web:
    {
        return "Web";
    }
    case csp::systems::EThirdPartyPlatform::None:
    {
        return std::nullopt;
    }
    }

    return std::nullopt;
}

// Contains shared logic for logging in with a 3rd party provider - used by both LoginToThirdPartyAuthenticationProvider and
// LoginToThirdPartyAuthenticationProviderWithToken.
void LoginSocial(csp::services::ApiBase* AuthenticationAPI, std::shared_ptr<csp::common::LoginState> CurrentLoginState,
    csp::common::LogSystem* LogSystem, const std::shared_ptr<chs_user::LoginSocialRequest>& Request, bool CreateMultiplayerConnection,
    csp::systems::LoginStateResultCallback Callback)
{
    // CurrentLoginState captured by value in callback below. This ensures the shared_ptr stays alive until the callback is executed via the
    // ResponseHandler.
    csp::systems::LoginStateResultCallback LoginStateResCallback
        = [LogSystem, Callback, CreateMultiplayerConnection, LoginStateRef = CurrentLoginState](const csp::systems::LoginStateResult& LoginStateRes)
    {
        if (LoginStateRes.GetResultCode() == csp::systems::EResultCode::Success)
        {
            csp::multiplayer::MultiplayerConnection::ErrorCodeCallbackHandler ConnectionCallback
                = [Callback, LoginStateRes](csp::multiplayer::ErrorCode ErrCode)
            {
                if (ErrCode != csp::multiplayer::ErrorCode::None)
                {
                    CSP_LOG_ERROR_FORMAT("Error connecting MultiplayerConnection: %s", csp::multiplayer::ErrorCodeToString(ErrCode).c_str());
                }

                Callback(LoginStateRes);
            };

            StartMultiplayerConnection(*csp::systems::SystemsManager::Get().GetMultiplayerConnection(),
                csp::CSPFoundation::GetEndpoints().MultiplayerConnection.GetURI(), ConnectionCallback, LoginStateRes, *LogSystem,
                CreateMultiplayerConnection);
        }
        else if (LoginStateRes.GetResultCode() == csp::systems::EResultCode::Failed)
        {
            CSP_LOG_ERROR_FORMAT("Login Failed. Code: %i", LoginStateRes.GetHttpResultCode());
            Callback(LoginStateRes);
        }
    };

    csp::services::ResponseHandlerPtr ResponseHandler = AuthenticationAPI->CreateHandler<csp::systems::LoginStateResultCallback,
        csp::systems::LoginStateResult, csp::common::LoginState, chs_user::AuthDto>(LoginStateResCallback, CurrentLoginState.get());

    static_cast<chs_user::AuthenticationApi*>(AuthenticationAPI)->usersLogin_socialPost({ Request }, ResponseHandler);
}

void SetTokenOptions(const csp::common::Optional<csp::systems::TokenOptions>& TokenOptions, std::shared_ptr<chs_user::TokenOptions> Options,
    std::shared_ptr<csp::common::LoginState> LoginState)
{
    if (TokenOptions.HasValue() && CheckExpiryLengthFormat(TokenOptions->AccessTokenExpiryLength))
    {
        Options->SetExpiryLength(TokenOptions->AccessTokenExpiryLength);
        LoginState->UpdateAccessTokenExpiry(TokenOptions->AccessTokenExpiryLength);
    }

    if (TokenOptions.HasValue() && CheckExpiryLengthFormat(TokenOptions->RefreshTokenExpiryLength))
    {
        Options->SetRefreshTokenExpiryLength(TokenOptions->RefreshTokenExpiryLength);
        LoginState->UpdateRefreshTokenExpiry(TokenOptions->RefreshTokenExpiryLength);
    }
}

} // namespace

namespace csp::systems
{

const char* EMPTY_SPACE_STRING = " ";

csp::common::String ConvertExternalAuthProvidersToString(EThirdPartyAuthenticationProviders Provider)
{
    switch (Provider)
    {
    case EThirdPartyAuthenticationProviders::Google:
        return "Google";
    case EThirdPartyAuthenticationProviders::Discord:
        return "Discord";
    case EThirdPartyAuthenticationProviders::Apple:
        return "Apple";
    case EThirdPartyAuthenticationProviders::Netflix:
        return "Netflix";
    default:
    {
        CSP_LOG_FORMAT(common::LogLevel::Error, "Unsupported Provider Type requested: %d, returning Google", static_cast<uint8_t>(Provider));
        return "Google";
    }
    }
}

csp::common::String FormatScopesForURL(csp::common::Array<csp::common::String> Scopes)
{
    csp::common::String FormattedScopes;
    for (size_t idx = 0; idx < Scopes.Size(); ++idx)
    {
        FormattedScopes.Append(Scopes[idx]);
        if (idx != Scopes.Size() - 1)
        {
            FormattedScopes.Append(EMPTY_SPACE_STRING);
        }
    }

    return FormattedScopes;
}

AuthContext::AuthContext(csp::services::ApiBase* AuthenticationAPI, std::shared_ptr<csp::common::LoginState> LoginState)
    : AuthenticationAPI { AuthenticationAPI }
    , LoginState { LoginState }
{
}

const csp::common::LoginState& AuthContext::GetLoginState() const { return *LoginState; }

void AuthContext::RefreshToken(std::function<void(bool)> Callback)
{
    const auto Data = LoginState->GetSnapshotThreadSafe();
    if (Data->State != csp::common::ELoginState::LoggedIn)
    {
        return;
    }

    auto Request = std::make_shared<chs_user::RefreshRequest>();
    Request->SetDeviceId(csp::CSPFoundation::GetDeviceId());
    Request->SetUserId(Data->UserId);
    Request->SetRefreshToken(Data->RefreshToken);

    auto Options = std::make_shared<chs_user::TokenOptions>();
    Options->SetExpiryLength(Data->AccessTokenExpiryLength);
    Options->SetRefreshTokenExpiryLength(Data->RefreshTokenExpiryLength);
    Request->SetTokenOptions(Options);

    // LoginState captured by value in callback below. This ensures the shared_ptr stays alive until the callback is executed via the
    // ResponseHandler.
    LoginStateResultCallback LoginStateResCallback = [Callback, LoginStateRef = LoginState](const LoginStateResult& LoginStateRes)
    {
        if (LoginStateRes.GetResultCode() == csp::systems::EResultCode::InProgress)
        {
            return;
        }
        else if (LoginStateRes.GetResultCode() == csp::systems::EResultCode::Success)
        {
            const NullResult Result(csp::systems::EResultCode::Success, 200);
            INVOKE_IF_NOT_NULL(Callback, true);
        }
        else
        {
            const NullResult Result(LoginStateRes.GetResultCode(), LoginStateRes.GetHttpResultCode());
            INVOKE_IF_NOT_NULL(Callback, false);
        }
    };

    csp::services::ResponseHandlerPtr ResponseHandler
        = AuthenticationAPI->CreateHandler<LoginStateResultCallback, LoginStateResult, csp::common::LoginState, chs_user::AuthDto>(
            LoginStateResCallback, LoginState.get());

    static_cast<chs_user::AuthenticationApi*>(AuthenticationAPI)->usersRefreshPost({ Request }, ResponseHandler);
}

UserSystem::UserSystem()
    : SystemBase(nullptr, nullptr, nullptr)
    , AuthenticationAPI(nullptr)
    , ProfileAPI(nullptr)
    , PingAPI(nullptr)
    , StripeAPI { nullptr }
    , CurrentLoginState(std::make_shared<csp::common::LoginState>())
    , RefreshTokenChangedCallback(nullptr)
    , Auth { AuthenticationAPI, CurrentLoginState }
{
}

UserSystem::UserSystem(csp::web::WebClient* InWebClient, csp::multiplayer::NetworkEventBus* InEventBus, csp::common::LogSystem& LogSystem)
    : SystemBase(InWebClient, InEventBus, &LogSystem)
    , AuthenticationAPI { new chs_user::AuthenticationApi(InWebClient) }
    , ProfileAPI { new chs_user::ProfileApi(InWebClient) }
    , PingAPI { new chs_user::PingApi(InWebClient) }
    , StripeAPI { new chs_user::StripeApi(InWebClient) }
    , CurrentLoginState(std::make_shared<csp::common::LoginState>())
    , RefreshTokenChangedCallback(nullptr)
    , Auth { AuthenticationAPI, CurrentLoginState }
{
}

UserSystem::~UserSystem()
{
    delete (PingAPI);
    delete (ProfileAPI);
    delete (AuthenticationAPI);
    delete (StripeAPI);
}

void UserSystem::SetNetworkEventBus(csp::multiplayer::NetworkEventBus& EventBus)
{
    EventBusPtr = &EventBus;

    RegisterSystemCallback();
}

const csp::common::LoginState& UserSystem::GetLoginState() const { return *CurrentLoginState; }

void UserSystem::SetNewLoginTokenReceivedCallback(LoginTokenInfoResultCallback Callback) { RefreshTokenChangedCallback = Callback; }

void UserSystem::Login(const csp::common::String& Email, const csp::common::String& Password, bool CreateMultiplayerConnection,
    const csp::common::Optional<bool>& UserHasVerifiedAge, const csp::common::Optional<TokenOptions>& TokenOptions, LoginStateResultCallback Callback)
{
    if (Email.IsEmpty())
    {
        CSP_LOG_ERROR_MSG("UserSystem::Login, Email must not be empty.");
        Callback(MakeInvalid<LoginStateResult>());
        return;
    }
    if (Password.IsEmpty())
    {
        CSP_LOG_ERROR_MSG("UserSystem::Login, Password must not be empty.");
        Callback(MakeInvalid<LoginStateResult>());
        return;
    }

    auto Options = std::make_shared<chs_user::TokenOptions>();

    if (!CurrentLoginState->TrySetLoginRequested())
    {
        csp::systems::LoginStateResult BadResult;
        BadResult.SetResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        Callback(BadResult);
        return;
    }

    SetTokenOptions(TokenOptions, Options, CurrentLoginState);

    auto Request = std::make_shared<chs_user::LoginRequest>();
    Request->SetDeviceId(csp::CSPFoundation::GetDeviceId());
    Request->SetEmail(Email);
    Request->SetPassword(Password);
    Request->SetTenant(csp::CSPFoundation::GetTenant());

    if (UserHasVerifiedAge.HasValue())
    {
        Request->SetVerifiedAgeEighteen(*UserHasVerifiedAge);
    }

    Request->SetTokenOptions(Options);

    // CurrentLoginState captured by value in callback below. This ensures the shared_ptr stays alive until the callback is executed via the
    // ResponseHandler.
    LoginStateResultCallback LoginStateResCallback
        = [this, Callback, CreateMultiplayerConnection, LoginStateRef = CurrentLoginState](const LoginStateResult& LoginStateRes)
    {
        if (LoginStateRes.GetResultCode() == csp::systems::EResultCode::Success)
        {
            csp::multiplayer::MultiplayerConnection::ErrorCodeCallbackHandler ConnectionCallback
                = [Callback, LoginStateRes](csp::multiplayer::ErrorCode ErrCode)
            {
                if (ErrCode != csp::multiplayer::ErrorCode::None)
                {
                    CSP_LOG_ERROR_FORMAT("Error connecting MultiplayerConnection: %s", csp::multiplayer::ErrorCodeToString(ErrCode).c_str());

                    Callback(LoginStateRes);
                    return;
                }

                Callback(LoginStateRes);
            };

            StartMultiplayerConnection(*SystemsManager::Get().GetMultiplayerConnection(),
                CSPFoundation::GetEndpoints().MultiplayerConnection.GetURI(), ConnectionCallback, LoginStateRes, *LogSystem,
                CreateMultiplayerConnection);
        }
        else if (LoginStateRes.GetResultCode() == csp::systems::EResultCode::Failed)
        {
            CSP_LOG_ERROR_FORMAT("Login Failed. Code: %i", LoginStateRes.GetHttpResultCode());
            Callback(LoginStateRes);
        }
    };

    csp::services::ResponseHandlerPtr ResponseHandler
        = AuthenticationAPI->CreateHandler<LoginStateResultCallback, LoginStateResult, csp::common::LoginState, chs_user::AuthDto>(
            LoginStateResCallback, CurrentLoginState.get());

    static_cast<chs_user::AuthenticationApi*>(AuthenticationAPI)->usersLoginPost({ Request }, ResponseHandler);
}

void UserSystem::LoginWithRefreshToken(const csp::common::String& UserId, const csp::common::String& RefreshToken, bool CreateMultiplayerConnection,
    const csp::common::Optional<TokenOptions>& TokenOptions, LoginStateResultCallback Callback)
{
    if (UserId.IsEmpty())
    {
        CSP_LOG_ERROR_MSG("UserSystem::LoginWithRefreshToken, UserId must not be empty.");
        Callback(MakeInvalid<LoginStateResult>());
        return;
    }

    auto Options = std::make_shared<chs_user::TokenOptions>();

    if (!CurrentLoginState->TrySetLoginRequested())
    {
        csp::systems::LoginStateResult BadResult;
        BadResult.SetResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        Callback(BadResult);
        return;
    }

    SetTokenOptions(TokenOptions, Options, CurrentLoginState);

    auto Request = std::make_shared<chs_user::RefreshRequest>();
    Request->SetDeviceId(csp::CSPFoundation::GetDeviceId());
    Request->SetUserId(UserId);
    Request->SetRefreshToken(RefreshToken);

    Request->SetTokenOptions(Options);

    // CurrentLoginState captured by value in callback below. This ensures the shared_ptr stays alive until the callback is executed via the
    // ResponseHandler.
    LoginStateResultCallback LoginStateResCallback
        = [this, Callback, CreateMultiplayerConnection, LoginStateRef = CurrentLoginState](const LoginStateResult& LoginStateRes)
    {
        if (LoginStateRes.GetResultCode() == csp::systems::EResultCode::Success)
        {
            csp::multiplayer::MultiplayerConnection::ErrorCodeCallbackHandler ConnectionCallback
                = [Callback, LoginStateRes](csp::multiplayer::ErrorCode ErrCode)
            {
                if (ErrCode != csp::multiplayer::ErrorCode::None)
                {
                    CSP_LOG_ERROR_FORMAT("Error connecting MultiplayerConnection: %s", csp::multiplayer::ErrorCodeToString(ErrCode).c_str());

                    Callback(LoginStateRes);
                    return;
                }

                Callback(LoginStateRes);
            };

            StartMultiplayerConnection(*SystemsManager::Get().GetMultiplayerConnection(),
                CSPFoundation::GetEndpoints().MultiplayerConnection.GetURI(), ConnectionCallback, LoginStateRes, *LogSystem,
                CreateMultiplayerConnection);
        }
        else
        {
            Callback(LoginStateRes);
        }
    };

    csp::services::ResponseHandlerPtr ResponseHandler
        = AuthenticationAPI->CreateHandler<LoginStateResultCallback, LoginStateResult, csp::common::LoginState, chs_user::AuthDto>(
            LoginStateResCallback, CurrentLoginState.get());

    static_cast<chs_user::AuthenticationApi*>(AuthenticationAPI)->usersRefreshPost({ Request }, ResponseHandler);
}

void UserSystem::LoginAsGuest(bool CreateMultiplayerConnection, const csp::common::Optional<bool>& UserHasVerifiedAge,
    const csp::common::Optional<TokenOptions>& TokenOptions, LoginStateResultCallback Callback)
{
    auto Options = std::make_shared<chs_user::TokenOptions>();

    if (!CurrentLoginState->TrySetLoginRequested())
    {
        csp::systems::LoginStateResult BadResult;
        BadResult.SetResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        Callback(BadResult);
        return;
    }

    SetTokenOptions(TokenOptions, Options, CurrentLoginState);

    auto Request = std::make_shared<chs_user::LoginRequest>();
    Request->SetDeviceId(csp::CSPFoundation::GetDeviceId());
    Request->SetTenant(csp::CSPFoundation::GetTenant());

    if (UserHasVerifiedAge.HasValue())
    {
        Request->SetVerifiedAgeEighteen(*UserHasVerifiedAge);
    }

    Request->SetTokenOptions(Options);

    // CurrentLoginState captured by value in callback below. This ensures the shared_ptr stays alive until the callback is executed via the
    // ResponseHandler.
    LoginStateResultCallback LoginStateResCallback
        = [this, Callback, CreateMultiplayerConnection, LoginStateRef = CurrentLoginState](const LoginStateResult& LoginStateRes)
    {
        if (LoginStateRes.GetResultCode() == csp::systems::EResultCode::Success)
        {
            csp::multiplayer::MultiplayerConnection::ErrorCodeCallbackHandler ConnectionCallback
                = [Callback, LoginStateRes](csp::multiplayer::ErrorCode ErrCode)
            {
                if (ErrCode != csp::multiplayer::ErrorCode::None)
                {
                    CSP_LOG_ERROR_FORMAT("Error connecting MultiplayerConnection: %s", csp::multiplayer::ErrorCodeToString(ErrCode).c_str());

                    Callback(LoginStateRes);
                    return;
                }

                Callback(LoginStateRes);
            };

            StartMultiplayerConnection(*SystemsManager::Get().GetMultiplayerConnection(),
                CSPFoundation::GetEndpoints().MultiplayerConnection.GetURI(), ConnectionCallback, LoginStateRes, *LogSystem,
                CreateMultiplayerConnection);
        }
        else
        {
            Callback(LoginStateRes);
        }
    };

    csp::services::ResponseHandlerPtr ResponseHandler
        = AuthenticationAPI->CreateHandler<LoginStateResultCallback, LoginStateResult, csp::common::LoginState, chs_user::AuthDto>(
            LoginStateResCallback, CurrentLoginState.get());

    static_cast<chs_user::AuthenticationApi*>(AuthenticationAPI)->usersLoginPost({ Request }, ResponseHandler);
}

void UserSystem::LoginAsGuestWithDeferredProfileCreation(const csp::common::Optional<bool>& UserHasVerifiedAge, LoginStateResultCallback Callback)
{
    if (!CurrentLoginState->TrySetLoginRequested())
    {
        csp::systems::LoginStateResult BadResult;
        BadResult.SetResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        Callback(BadResult);
        return;
    }

    auto Request = std::make_shared<chs_user::LoginGuestRequest>();
    Request->SetDeviceId(csp::CSPFoundation::GetDeviceId());
    Request->SetTenant(csp::CSPFoundation::GetTenant());

    if (UserHasVerifiedAge.HasValue())
    {
        Request->SetVerifiedAgeEighteen(*UserHasVerifiedAge);
    }

    // CurrentLoginState captured by value in callback below. This ensures the shared_ptr stays alive until the callback is executed via the
    // ResponseHandler.
    LoginStateResultCallback LoginStateResCallback = [this, Callback, LoginStateRef = CurrentLoginState](const LoginStateResult& LoginStateRes)
    {
        if (LoginStateRes.GetResultCode() == csp::systems::EResultCode::Success)
        {
            csp::multiplayer::MultiplayerConnection::ErrorCodeCallbackHandler ConnectionCallback
                = [Callback, LoginStateRes](csp::multiplayer::ErrorCode ErrCode)
            {
                if (ErrCode != csp::multiplayer::ErrorCode::None)
                {
                    // It would be extremely strange to hit this branch, but it remains here just in case.
                    CSP_LOG_ERROR_FORMAT("Unexpected error connecting MultiplayerConnection. This is strange! : %s",
                        csp::multiplayer::ErrorCodeToString(ErrCode).c_str());
                    Callback(LoginStateRes);
                    return;
                }

                Callback(LoginStateRes);
            };

            // Do not start a multiplayer connection, need to call through this to trigger all the callbacks though.
            StartMultiplayerConnection(*SystemsManager::Get().GetMultiplayerConnection(),
                CSPFoundation::GetEndpoints().MultiplayerConnection.GetURI(), ConnectionCallback, LoginStateRes, *LogSystem, false);
        }
        else
        {
            Callback(LoginStateRes);
        }
    };

    csp::services::ResponseHandlerPtr ResponseHandler
        = AuthenticationAPI->CreateHandler<LoginStateResultCallback, LoginStateResult, csp::common::LoginState, chs_user::AuthDto>(
            LoginStateResCallback, CurrentLoginState.get());

    // Despite the naming, "login-guest" is the deferred, optimized, non-standard guest login.
    // The regular login endpoint that "loginAsGuest" uses is the "real" one.
    static_cast<chs_user::AuthenticationApi*>(AuthenticationAPI)->usersLogin_guestPost({ Request }, ResponseHandler);
}

csp::common::Array<EThirdPartyAuthenticationProviders> UserSystem::GetSupportedThirdPartyAuthenticationProviders() const
{
    csp::common::Array<EThirdPartyAuthenticationProviders> Providers((EThirdPartyAuthenticationProviders::Num));
    for (uint8_t idx = 0; idx < EThirdPartyAuthenticationProviders::Num; ++idx)
        Providers[idx] = static_cast<EThirdPartyAuthenticationProviders>(idx);

    return Providers;
}

void UserSystem::ResetThirdPartyAuthState()
{
    ThirdPartyAuthStateId = "";
    ThirdPartyClientType = "";
    ThirdPartyAuthRedirectURL = "";
    ThirdPartyRequestedAuthProvider = EThirdPartyAuthenticationProviders::Invalid;
}

void UserSystem::GetThirdPartyProviderAuthorizeURL(EThirdPartyAuthenticationProviders AuthProvider, const csp::common::String& RedirectURL,
    const csp::common::Optional<EThirdPartyPlatform>& ClientType, StringResultCallback Callback)
{
    if (AuthProvider == EThirdPartyAuthenticationProviders::Invalid)
    {
        CSP_LOG_ERROR_MSG(
            "UserSystem::GetThirdPartyProviderAuthorizeURL() - EThirdPartyAuthenticationProviders::Invalid was passed as an AuthProvider.");

        StringResult ErrorResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        Callback(ErrorResult);

        return;
    }

    if (RedirectURL.IsEmpty())
    {
        CSP_LOG_ERROR_MSG("UserSystem::GetThirdPartyProviderAuthorizeURL() - An empty RedirectURL was passed.");

        StringResult ErrorResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        Callback(ErrorResult);

        return;
    }

    ProviderDetailsResultCallback ThirdPartyAuthenticationDetailsCallback = [=](const ProviderDetailsResult& ProviderDetailsRes)
    {
        if (ProviderDetailsRes.GetResultCode() == csp::systems::EResultCode::Success)
        {
            const auto ProviderRedirectUrl = ProviderDetailsRes.GetDetails().ProviderRedirectURL;
            ThirdPartyAuthStateId = ProviderDetailsRes.GetDetails().ThirdPartyAuthStateId;

            ThirdPartyRequestedAuthProvider = AuthProvider;
            ThirdPartyAuthRedirectURL = RedirectURL;

            StringResult SuccessResult(ProviderDetailsRes.GetResultCode(), ProviderDetailsRes.GetHttpResultCode());
            SuccessResult.SetValue(ProviderRedirectUrl);
            Callback(SuccessResult);
        }
        else if (ProviderDetailsRes.GetResultCode() != csp::systems::EResultCode::InProgress)
        {
            CSP_LOG_FORMAT(common::LogLevel::Error, "The retrieval of third party details was not successful. ResCode: %d, HttpResCode: %d",
                static_cast<int>(ProviderDetailsRes.GetResultCode()), ProviderDetailsRes.GetHttpResultCode());

            StringResult ErrorResult(ProviderDetailsRes.GetResultCode(), ProviderDetailsRes.GetHttpResultCode());
            ErrorResult.SetValue("error");
            Callback(ErrorResult);
        }
    };

    // Reset the state related to third party authentication. This ensures that if the client calls GetThirdPartyProviderAuthorizeURL and the calls
    // fails they will not be able to call LoginToThirdPartyAuthenticationProvider() with invalid state.
    ResetThirdPartyAuthState();

    std::optional<csp::common::String> Client = ThirdPartyPlatformToString(ClientType);
    if (Client.has_value())
    {
        ThirdPartyClientType = Client.value();
    }

    const csp::services::ResponseHandlerPtr ResponseHandler
        = AuthenticationAPI->CreateHandler<ProviderDetailsResultCallback, ProviderDetailsResult, void, chs_user::SocialProviderInfo>(
            ThirdPartyAuthenticationDetailsCallback, nullptr, csp::web::EResponseCodes::ResponseOK);

    static_cast<chs_user::AuthenticationApi*>(AuthenticationAPI)
        ->social_providersProviderGet(
            { ConvertExternalAuthProvidersToString(AuthProvider), RedirectURL, csp::CSPFoundation::GetTenant(), Client }, ResponseHandler);
}

void UserSystem::LoginToThirdPartyAuthenticationProvider(const csp::common::String& ThirdPartyToken, const csp::common::String& ThirdPartyStateId,
    bool CreateMultiplayerConnection, const csp::common::Optional<bool>& UserHasVerifiedAge, const csp::common::Optional<TokenOptions>& TokenOptions,
    LoginStateResultCallback Callback)
{
    if (ThirdPartyToken.IsEmpty() || ThirdPartyStateId.IsEmpty())
    {
        CSP_LOG_ERROR_MSG(
            "UserSystem::LoginToThirdPartyAuthenticationProvider() - both ThirdPartyToken and ThirdPartyStateId must be provided. Please call "
            "AssetSystem::GetThirdPartyProviderAuthorizeURL() first to get an Authorization URL, which can then be used to retrieve them.");

        csp::systems::LoginStateResult ErrorResult;
        ErrorResult.SetResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        Callback(ErrorResult);

        return;
    }

    // Check that the stored ThirdPartyAuthStateId matches the one passed by the Client.
    // This is a security precaution suggested by the Auth Providers.
    if (ThirdPartyAuthStateId != ThirdPartyStateId)
    {
        CSP_LOG_ERROR_MSG("The provided ThirdPartyStateId is not correct"); // intentionally not too explicit about the error for security reasons

        csp::systems::LoginStateResult ErrorResult;
        ErrorResult.SetResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        Callback(ErrorResult);

        return;
    }

    auto Options = std::make_shared<chs_user::TokenOptions>();

    if (!CurrentLoginState->TrySetLoginRequested())
    {
        csp::systems::LoginStateResult BadResult;
        BadResult.SetResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        Callback(BadResult);

        return;
    }

    SetTokenOptions(TokenOptions, Options, CurrentLoginState);

    const auto Request = std::make_shared<chs_user::LoginSocialRequest>();
    Request->SetDeviceId(csp::CSPFoundation::GetDeviceId());
    Request->SetOAuthRedirectUri(ThirdPartyAuthRedirectURL);
    Request->SetProvider(ConvertExternalAuthProvidersToString(ThirdPartyRequestedAuthProvider));
    Request->SetClient(ThirdPartyClientType);
    Request->SetToken(ThirdPartyToken);
    Request->SetTenant(csp::CSPFoundation::GetTenant());

    if (UserHasVerifiedAge.HasValue())
    {
        Request->SetVerifiedAgeEighteen(*UserHasVerifiedAge);
    }

    Request->SetTokenOptions(Options);

    LoginSocial(AuthenticationAPI, CurrentLoginState, LogSystem, Request, CreateMultiplayerConnection, Callback);
}

void UserSystem::LoginToThirdPartyAuthenticationProviderWithToken(EThirdPartyAuthenticationProviders AuthProvider,
    const csp::common::String& ThirdPartyToken, const csp::common::Optional<EThirdPartyPlatform>& ClientType, bool CreateMultiplayerConnection,
    const csp::common::Optional<bool>& UserHasVerifiedAge, LoginStateResultCallback Callback)
{
    if (AuthProvider == EThirdPartyAuthenticationProviders::Invalid)
    {
        CSP_LOG_ERROR_MSG("UserSystem::LoginToThirdPartyAuthenticationProviderWithToken() - EThirdPartyAuthenticationProviders::Invalid was passed "
                          "as an AuthProvider.");

        csp::systems::LoginStateResult ErrorResult;
        ErrorResult.SetResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        Callback(ErrorResult);

        return;
    }

    if (ThirdPartyToken.IsEmpty())
    {
        CSP_LOG_ERROR_MSG("UserSystem::LoginToThirdPartyAuthenticationProviderWithToken() - a ThirdPartyToken must be provided. Please acquire a "
                          "token from the third party provider directly before making this call.");

        csp::systems::LoginStateResult ErrorResult;
        ErrorResult.SetResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        Callback(ErrorResult);

        return;
    }

    if (!CurrentLoginState->TrySetLoginRequested())
    {
        csp::systems::LoginStateResult BadResult;
        BadResult.SetResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        Callback(BadResult);

        return;
    }

    const auto Request = std::make_shared<chs_user::LoginSocialRequest>();
    Request->SetDeviceId(csp::CSPFoundation::GetDeviceId());
    Request->SetProvider(ConvertExternalAuthProvidersToString(AuthProvider));
    Request->SetToken(ThirdPartyToken);
    Request->SetTenant(csp::CSPFoundation::GetTenant());

    std::optional<csp::common::String> Client = ThirdPartyPlatformToString(ClientType);
    if (Client.has_value())
    {
        Request->SetClient(Client.value());
    }

    if (UserHasVerifiedAge.HasValue())
    {
        Request->SetVerifiedAgeEighteen(*UserHasVerifiedAge);
    }

    LoginSocial(AuthenticationAPI, CurrentLoginState, LogSystem, Request, CreateMultiplayerConnection, Callback);
}

void UserSystem::FederatedLogin(
    const csp::common::String& FederatedLoginDetailsJson, bool CreateMultiplayerConnection, LoginStateResultCallback Callback)
{
    chs_user::AuthDto AuthDetails;

    AuthDetails.FromJson(FederatedLoginDetailsJson);

    if (AuthDetails.GetAccessToken().IsEmpty())
    {
        CSP_LOG_ERROR_MSG("UserSystem::SetLoginDetails() - Parsing error: AccessToken was not provided.");

        csp::systems::LoginStateResult BadResult;
        BadResult.SetResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        Callback(BadResult);

        return;
    }

    if (AuthDetails.GetRefreshToken().IsEmpty())
    {
        CSP_LOG_ERROR_MSG("UserSystem::SetLoginDetails() - Parsing error: RefreshToken was not provided.");

        csp::systems::LoginStateResult BadResult;
        BadResult.SetResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        Callback(BadResult);

        return;
    }

    if (AuthDetails.GetUserId().IsEmpty())
    {
        CSP_LOG_ERROR_MSG("UserSystem::SetLoginDetails() - Parsing error: UserId was not provided.");

        csp::systems::LoginStateResult BadResult;
        BadResult.SetResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        Callback(BadResult);

        return;
    }

    if (AuthDetails.GetDeviceId().IsEmpty())
    {
        CSP_LOG_ERROR_MSG("UserSystem::SetLoginDetails() - Parsing error: DeviceId was not provided.");

        csp::systems::LoginStateResult BadResult;
        BadResult.SetResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        Callback(BadResult);

        return;
    }

    auto DataOpt = csp::common::AuthDtoToLoginStateData(&AuthDetails);

    if (DataOpt.has_value() == false)
    {
        csp::systems::LoginStateResult BadResult;
        BadResult.SetResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        Callback(BadResult);

        return;
    }

    auto Data = *DataOpt;

    CurrentLoginState->SetLoginStateDataThreadSafe(Data);

    web::HttpAuth::SetAccessToken(
        AuthDetails.GetAccessToken(), AuthDetails.GetAccessTokenExpiresAt(), AuthDetails.GetRefreshToken(), AuthDetails.GetRefreshTokenExpiresAt());

    // Signal login to anyone interested
    events::Event* LoginEvent = events::EventSystem::Get().AllocateEvent(events::USERSERVICE_LOGIN_EVENT_ID);
    LoginEvent->AddString("UserId", AuthDetails.GetUserId());
    events::EventSystem::Get().EnqueueEvent(LoginEvent);

    SystemsManager::Get().GetUserSystem()->NotifyRefreshTokenHasChanged();

    LoginStateResult Result { CurrentLoginState.get() };
    Result.SetResult(EResultCode::Success, static_cast<uint16_t>(csp::web::EResponseCodes::ResponseOK));

    csp::multiplayer::MultiplayerConnection::ErrorCodeCallbackHandler ConnectionCallback = [Callback, Result](csp::multiplayer::ErrorCode ErrCode)
    {
        if (ErrCode != csp::multiplayer::ErrorCode::None)
        {
            CSP_LOG_ERROR_FORMAT("Error connecting MultiplayerConnection: %s", csp::multiplayer::ErrorCodeToString(ErrCode).c_str());

            Callback(Result);
            return;
        }

        Callback(Result);
    };

    StartMultiplayerConnection(*SystemsManager::Get().GetMultiplayerConnection(), CSPFoundation::GetEndpoints().MultiplayerConnection.GetURI(),
        ConnectionCallback, Result, *LogSystem, CreateMultiplayerConnection);
}

void UserSystem::Logout(NullResultCallback Callback)
{
    // Confirm we are in the correct state to proceed with logout.
    if (!CurrentLoginState->TrySetLogoutRequested())
    {
        csp::systems::LogoutResult BadResult;
        BadResult.SetResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        Callback(BadResult);

        return;
    }

    // Disconnect MultiplayerConnection before logging out
    csp::multiplayer::MultiplayerConnection::ErrorCodeCallbackHandler ErrorCallback = [Callback, this](csp::multiplayer::ErrorCode ErrCode)
    {
        if (ErrCode != csp::multiplayer::ErrorCode::None)
        {
            CSP_LOG_ERROR_FORMAT("Error disconnecting MultiplayerConnection: %s", csp::multiplayer::ErrorCodeToString(ErrCode).c_str());
        }

        const auto Data = CurrentLoginState->GetSnapshotThreadSafe();

        auto Request = std::make_shared<chs_user::LogoutRequest>();
        Request->SetUserId(Data->UserId);
        Request->SetDeviceId(Data->DeviceId);

        // CurrentLoginState captured by value in the wrapped callback below. This ensures the shared_ptr stays alive until the callback is executed
        // via the ResponseHandler.
        NullResultCallback WrappedCallback = [Callback, LoginStateRef = CurrentLoginState](const NullResult& Result) { Callback(Result); };

        csp::services::ResponseHandlerPtr ResponseHandler
            = AuthenticationAPI->CreateHandler<NullResultCallback, LogoutResult, csp::common::LoginState, csp::services::NullDto>(
                WrappedCallback, CurrentLoginState.get(), csp::web::EResponseCodes::ResponseNoContent);

        static_cast<chs_user::AuthenticationApi*>(AuthenticationAPI)->usersLogoutPost({ Request }, ResponseHandler);
    };

    auto* MultiplayerConnection = SystemsManager::Get().GetMultiplayerConnection();
    MultiplayerConnection->Disconnect(ErrorCallback);
}

void UserSystem::CreateUser(const csp::common::Optional<csp::common::String>& DisplayName, const csp::common::String& Email,
    const csp::common::String& Password, bool ReceiveNewsletter, bool HasVerifiedAge, const csp::common::Optional<csp::common::String>& RedirectUrl,
    const csp::common::Optional<csp::common::String>& InviteToken, ProfileResultCallback Callback)
{
    auto Request = std::make_shared<chs_user::CreateUserRequest>();

    if (DisplayName.HasValue())
    {
        Request->SetDisplayName(*DisplayName);
    }

    Request->SetEmail(Email);
    Request->SetPassword(Password);

    auto InitialSettings = std::make_shared<chs_user::InitialSettingsDto>();
    InitialSettings->SetContext("UserSettings");
    InitialSettings->SetSettings({ { "Newsletter", ReceiveNewsletter ? "true" : "false" } });
    Request->SetInitialSettings({ InitialSettings });
    Request->SetTenant(csp::CSPFoundation::GetTenant());

    Request->SetVerifiedAgeEighteen(HasVerifiedAge);

    if (RedirectUrl.HasValue())
    {
        Request->SetRedirectUrl(RedirectUrl->c_str());
    }

    if (InviteToken.HasValue())
    {
        Request->SetInviteToken(*InviteToken);
    }
    const csp::services::ResponseHandlerPtr ResponseHandler
        = ProfileAPI->CreateHandler<ProfileResultCallback, ProfileResult, void, chs_user::ProfileDto>(
            Callback, nullptr, csp::web::EResponseCodes::ResponseCreated);

    static_cast<chs_user::ProfileApi*>(ProfileAPI)->usersPost({ Request }, ResponseHandler);
}

void UserSystem::UpgradeGuestAccount(
    const csp::common::String& DisplayName, const csp::common::String& Email, const csp::common::String& Password, ProfileResultCallback Callback)
{
    auto UserId = CurrentLoginState->GetUserId();

    auto Request = std::make_shared<chs_user::UpgradeGuestRequest>();

    Request->SetDisplayName(DisplayName);
    Request->SetEmail(Email);
    Request->SetPassword(Password);
    Request->SetGuestDeviceId(csp::CSPFoundation::GetDeviceId());

    const csp::services::ResponseHandlerPtr ResponseHandler
        = ProfileAPI->CreateHandler<ProfileResultCallback, ProfileResult, void, chs_user::ProfileDto>(Callback, nullptr);

    static_cast<chs_user::ProfileApi*>(ProfileAPI)->usersUserIdUpgrade_guestPost({ UserId, Request }, ResponseHandler);
}

void UserSystem::ConfirmUserEmail(NullResultCallback Callback)
{
    auto UserId = CurrentLoginState->GetUserId();

    csp::services::ResponseHandlerPtr ResponseHandler = ProfileAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(
        Callback, nullptr, csp::web::EResponseCodes::ResponseNoContent);

    static_cast<chs_user::ProfileApi*>(ProfileAPI)->usersUserIdConfirm_emailPost({ UserId, nullptr }, ResponseHandler);
}

void UserSystem::ResetUserPassword(
    const csp::common::String& Token, const csp::common::String& UserId, const csp::common::String& NewPassword, NullResultCallback Callback)
{
    auto Request = std::make_shared<chs_user::TokenResetPasswordRequest>();

    Request->SetToken(Token);
    Request->SetNewPassword(NewPassword);

    csp::services::ResponseHandlerPtr ResponseHandler = ProfileAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(
        Callback, nullptr, csp::web::EResponseCodes::ResponseNoContent);

    static_cast<chs_user::ProfileApi*>(ProfileAPI)->usersUserIdToken_change_passwordPost({ UserId, Request }, ResponseHandler);
}

void UserSystem::UpdateUserDisplayName(const csp::common::String& UserId, const csp::common::String& NewUserDisplayName, NullResultCallback Callback)
{
    const csp::services::ResponseHandlerPtr ResponseHandler
        = ProfileAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(Callback, nullptr);

    static_cast<chs_user::ProfileApi*>(ProfileAPI)->usersUserIdDisplay_namePut({ UserId, NewUserDisplayName }, ResponseHandler);
}

void UserSystem::DeleteUser(const csp::common::String& UserId, NullResultCallback Callback)
{
    csp::services::ResponseHandlerPtr ResponseHandler = ProfileAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(
        Callback, nullptr, csp::web::EResponseCodes::ResponseNoContent);

    static_cast<chs_user::ProfileApi*>(ProfileAPI)->usersUserIdDelete({ UserId }, ResponseHandler);
}

bool UserSystem::EmailCheck(const std::string& Email) const { return Email.find("@") != std::string::npos; }

void UserSystem::ForgotPassword(const csp::common::String& Email, const csp::common::Optional<csp::common::String>& RedirectUrl,
    const csp::common::Optional<csp::common::String>& EmailLinkUrl, bool UseTokenChangePasswordUrl, NullResultCallback Callback)
{
    if (EmailCheck(Email.c_str()))
    {
        auto Request = std::make_shared<chs_user::ForgotPasswordRequest>();
        Request->SetEmail(Email);
        Request->SetTenant(csp::CSPFoundation::GetTenant());

        std::optional<csp::common::String> RedirectUrlValue;
        std::optional<csp::common::String> EmailLinkUrlValue;

        if (RedirectUrl.HasValue())
        {
            RedirectUrlValue = *RedirectUrl;
        }

        if (EmailLinkUrl.HasValue())
        {
            EmailLinkUrlValue = *EmailLinkUrl;
        }

        csp::services::ResponseHandlerPtr ResponseHandler = ProfileAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(
            Callback, nullptr, csp::web::EResponseCodes::ResponseNoContent);

        static_cast<chs_user::ProfileApi*>(ProfileAPI)
            ->usersForgot_passwordPost({ RedirectUrlValue, UseTokenChangePasswordUrl, EmailLinkUrlValue, Request }, ResponseHandler);
    }
    else
    {
        Callback(csp::systems::NullResult(csp::systems::EResultCode::Failed, static_cast<uint16_t>(csp::web::EResponseCodes::ResponseBadRequest)));
    }
}

void UserSystem::GetProfileByUserId(const csp::common::String& InUserId, ProfileResultCallback Callback)
{
    const csp::common::String UserId = InUserId;

    csp::services::ResponseHandlerPtr ResponseHandler
        = ProfileAPI->CreateHandler<ProfileResultCallback, ProfileResult, void, chs_user::ProfileDto>(Callback, nullptr);

    static_cast<chs_user::ProfileApi*>(ProfileAPI)->usersUserIdGet({ UserId }, ResponseHandler);
}

void UserSystem::GetProfilesByUserId(const csp::common::Array<csp::common::String>& InUserIds, BasicProfilesResultCallback Callback)
{
    const std::vector<csp::common::String> UserIds(InUserIds.Data(), InUserIds.Data() + InUserIds.Size());

    csp::services::ResponseHandlerPtr ResponseHandler
        = ProfileAPI->CreateHandler<BasicProfilesResultCallback, BasicProfilesResult, void, csp::services::DtoArray<chs_user::ProfileLiteDto>>(
            Callback, nullptr);

    static_cast<chs_user::ProfileApi*>(ProfileAPI)->usersLiteGet({ UserIds }, ResponseHandler);
}

void UserSystem::GetBasicProfilesByUserId(const csp::common::Array<csp::common::String>& InUserIds, BasicProfilesResultCallback Callback)
{
    const std::vector<csp::common::String> UserIds(InUserIds.Data(), InUserIds.Data() + InUserIds.Size());

    csp::services::ResponseHandlerPtr ResponseHandler
        = ProfileAPI->CreateHandler<BasicProfilesResultCallback, BasicProfilesResult, void, csp::services::DtoArray<chs_user::ProfileLiteDto>>(
            Callback, nullptr);

    static_cast<chs_user::ProfileApi*>(ProfileAPI)->usersLiteGet({ UserIds }, ResponseHandler);
}

void UserSystem::Ping(NullResultCallback Callback)
{
    csp::services::ResponseHandlerPtr PingResponseHandler
        = PingAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(Callback, nullptr);
    static_cast<chs_user::PingApi*>(PingAPI)->pingGet({}, PingResponseHandler);
}

void UserSystem::ResendVerificationEmail(
    const csp::common::String& InEmail, const csp::common::Optional<csp::common::String>& InRedirectUrl, NullResultCallback Callback)
{
    const csp::common::String& Tenant = CSPFoundation::GetTenant();
    std::optional<csp::common::String> RedirectUrl;

    if (InRedirectUrl.HasValue())
    {
        RedirectUrl = *InRedirectUrl;
    }

    csp::services::ResponseHandlerPtr ResponseHandler
        = ProfileAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(Callback, nullptr);

    static_cast<chs_user::ProfileApi*>(ProfileAPI)->usersEmailsEmailConfirm_emailRe_sendPost({ InEmail, Tenant, RedirectUrl }, ResponseHandler);
}

void UserSystem::GetCustomerPortalUrl(const csp::common::String& UserId, StringResultCallback Callback)
{
    csp::services::ResponseHandlerPtr ResponseHandler
        = StripeAPI->CreateHandler<StringResultCallback, CustomerPortalUrlResult, void, chs_user::StripeCustomerPortalDto>(Callback, nullptr);

    static_cast<chs_user::StripeApi*>(StripeAPI)->vendorsStripeCustomer_portalsUserIdGet({ UserId }, ResponseHandler);
};

void UserSystem::GetCheckoutSessionUrl(TierNames Tier, StringResultCallback Callback)
{
    auto CheckoutSessionInfo = std::make_shared<chs_user::StripeCheckoutRequest>();

    CheckoutSessionInfo->SetLookupKey(TierNameEnumToString(Tier));

    csp::services::ResponseHandlerPtr ResponseHandler
        = StripeAPI->CreateHandler<StringResultCallback, CheckoutSessionUrlResult, void, chs_user::StripeCheckoutSessionDto>(Callback, nullptr);

    static_cast<chs_user::StripeApi*>(StripeAPI)->vendorsStripeCheckout_sessionsPost({ CheckoutSessionInfo }, ResponseHandler);
};

void UserSystem::NotifyRefreshTokenHasChanged()
{
    if (RefreshTokenChangedCallback)
    {
        LoginTokenInfoResult InternalResult;
        InternalResult.FillLoginTokenInfo(csp::web::HttpAuth::GetAccessToken(), csp::web::HttpAuth::GetTokenExpiry(),
            csp::web::HttpAuth::GetRefreshToken(), csp::web::HttpAuth::GetRefreshTokenExpiry());

        RefreshTokenChangedCallback(InternalResult);
    }
}

void UserSystem::SetUserPermissionsChangedCallback(UserPermissionsChangedCallbackHandler Callback)
{
    UserPermissionsChangedCallback = Callback;
    RegisterSystemCallback();
}

void UserSystem::RegisterSystemCallback()
{
    if (!EventBusPtr)
    {
        CSP_LOG_ERROR_MSG("Error: Failed to register UserSystem. NetworkEventBus must be instantiated in the MultiplayerConnection first.");
        return;
    }

    if (!UserPermissionsChangedCallback)
    {
        return;
    }

    EventBusPtr->ListenNetworkEvent(
        csp::multiplayer::NetworkEventRegistration("CSPInternal::UserSystem",
            csp::multiplayer::NetworkEventBus::StringFromNetworkEvent(csp::multiplayer::NetworkEventBus::NetworkEvent::AccessControlChanged)),
        [this](const csp::common::NetworkEventData& NetworkEventData) { this->OnAccessControlChangedEvent(NetworkEventData); });
}

void UserSystem::OnAccessControlChangedEvent(const csp::common::NetworkEventData& NetworkEventData)
{
    if (!UserPermissionsChangedCallback)
    {
        return;
    }

    const csp::common::AccessControlChangedNetworkEventData& AccessControlChangedNetworkEventData
        = static_cast<const csp::common::AccessControlChangedNetworkEventData&>(NetworkEventData);

    UserPermissionsChangedCallback(AccessControlChangedNetworkEventData);
}

csp::common::IAuthContext& UserSystem::GetAuthContext() { return Auth; }

} // namespace csp::systems
