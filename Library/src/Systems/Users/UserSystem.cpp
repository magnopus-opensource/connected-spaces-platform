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
#include "CSP/Common/NetworkEventData.h"
#include "CSP/Common/SharedEnums.h"
#include "CSP/Common/StringFormat.h"
#include "CSP/Systems/Users/Authentication.h"
#include "CSP/Systems/Users/Profile.h"
#include "Common/Convert.h"
#include "Common/UUIDGenerator.h"
#include "Multiplayer/NetworkEventSerialisation.h"
#include "Services/UserService/Api.h"
#include "Systems/ResultHelpers.h"
#include "Systems/Users/Authentication.h"

#include "CallHelpers.h"
#include <regex>

namespace chs_user = csp::services::generated::userservice;

namespace
{

} // namespace

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
        MultiplayerConnection.Connect(
            ConnectionCallback, MultiplayerURI, LoginStateRes.GetLoginState().AccessToken, LoginStateRes.GetLoginState().DeviceId);
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

/*
 * Construct a Third Party Authentication URL to be used for authentication with a 3rd party provider.
 */
bool ConstructThirdPartyAuthURL(const csp::common::String& AuthoriseURL, const csp::common::String& ProviderClientId,
    const csp::common::String& AuthProviderFormattedScopes, const csp::common::String& ThirdPartyAuthStateId,
    const csp::common::String& ThirdPartyAuthRedirectURL, csp::common::String& ThirdPartyAuthURL)
{
    if (AuthoriseURL.IsEmpty() || ProviderClientId.IsEmpty() || AuthProviderFormattedScopes.IsEmpty())
    {
        return false;
    }

    ThirdPartyAuthURL = csp::common::StringFormat(
        "%s?client_id=%s&scope=%s&state=%s&response_type=code&redirect_uri=%s&prompt=select_account&response_mode=form_post", AuthoriseURL.c_str(),
        ProviderClientId.c_str(), AuthProviderFormattedScopes.c_str(), ThirdPartyAuthStateId.c_str(), ThirdPartyAuthRedirectURL.c_str());

    return true;
}

}

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
    default:
    {
        CSP_LOG_FORMAT(common::LogLevel::Error, "Unsupported Provider Type requested: %d, returning Invalid", static_cast<uint8_t>(Provider));
        return "Invalid";
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

AuthContext::AuthContext(csp::services::ApiBase* AuthenticationAPI, csp::common::LoginState& LoginState)
    : AuthenticationAPI { AuthenticationAPI }
    , LoginState { &LoginState }
{
}

const csp::common::LoginState& AuthContext::GetLoginState() const { return *LoginState; }

void AuthContext::RefreshToken(std::function<void(bool)> Callback)
{
    if (LoginState->State == csp::common::ELoginState::LoggedIn)
    {
        auto Request = std::make_shared<chs_user::RefreshRequest>();
        Request->SetDeviceId(csp::CSPFoundation::GetDeviceId());
        Request->SetUserId(LoginState->UserId);
        Request->SetRefreshToken(LoginState->RefreshToken);

        auto Options = std::make_shared<chs_user::TokenOptions>();
        Options->SetExpiryLength(LoginState->AccessTokenExpiryLength);
        Options->SetRefreshTokenExpiryLength(LoginState->RefreshTokenExpiryLength);
        Request->SetTokenOptions(Options);

        LoginStateResultCallback LoginStateResCallback = [=](const LoginStateResult& LoginStateRes)
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
                LoginStateResCallback, LoginState);

        static_cast<chs_user::AuthenticationApi*>(AuthenticationAPI)->usersRefreshPost({ Request }, ResponseHandler);
    }
}

UserSystem::UserSystem()
    : SystemBase(nullptr, nullptr, nullptr)
    , AuthenticationAPI(nullptr)
    , ProfileAPI(nullptr)
    , PingAPI(nullptr)
    , StripeAPI { nullptr }
    , Auth { AuthenticationAPI, CurrentLoginState }
{
}

UserSystem::UserSystem(csp::web::WebClient* InWebClient, csp::multiplayer::NetworkEventBus* InEventBus, csp::common::LogSystem& LogSystem)
    : SystemBase(InWebClient, InEventBus, &LogSystem)
    , AuthenticationAPI { new chs_user::AuthenticationApi(InWebClient) }
    , ProfileAPI { new chs_user::ProfileApi(InWebClient) }
    , PingAPI { new chs_user::PingApi(InWebClient) }
    , StripeAPI { new chs_user::StripeApi(InWebClient) }
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

const csp::common::LoginState& UserSystem::GetLoginState() const { return CurrentLoginState; }

void UserSystem::SetNewLoginTokenReceivedCallback(LoginTokenInfoResultCallback Callback) { RefreshTokenChangedCallback = Callback; }

void UserSystem::Login(const csp::common::String& UserName, const csp::common::String& Email, const csp::common::String& Password,
    bool CreateMultiplayerConnection, const csp::common::Optional<bool>& UserHasVerifiedAge, const csp::common::Optional<TokenOptions>& TokenOptions,
    LoginStateResultCallback Callback)
{
    if (UserName.IsEmpty() && Email.IsEmpty())
    {
        CSP_LOG_ERROR_MSG("UserSystem::Login, One of either Username or Email must not be empty.");
        Callback(MakeInvalid<LoginStateResult>());
        return;
    }
    if (Password.IsEmpty())
    {
        CSP_LOG_ERROR_MSG("UserSystem::Login, Password must not be empty.");
        Callback(MakeInvalid<LoginStateResult>());
        return;
    }

    if (CurrentLoginState.State == csp::common::ELoginState::LoggedOut || CurrentLoginState.State == csp::common::ELoginState::Error)
    {
        CurrentLoginState.State = csp::common::ELoginState::LoginRequested;

        auto Request = std::make_shared<chs_user::LoginRequest>();
        Request->SetDeviceId(csp::CSPFoundation::GetDeviceId());
        Request->SetUserName(UserName);
        Request->SetEmail(Email);
        Request->SetPassword(Password);
        Request->SetTenant(csp::CSPFoundation::GetTenant());

        if (UserHasVerifiedAge.HasValue())
        {
            Request->SetVerifiedAgeEighteen(*UserHasVerifiedAge);
        }

        auto Options = std::make_shared<chs_user::TokenOptions>();

        if (TokenOptions.HasValue() && CheckExpiryLengthFormat(TokenOptions->AccessTokenExpiryLength))
        {
            Options->SetExpiryLength(TokenOptions->AccessTokenExpiryLength);
            CurrentLoginState.AccessTokenExpiryLength = TokenOptions->AccessTokenExpiryLength;
        }

        if (TokenOptions.HasValue() && CheckExpiryLengthFormat(TokenOptions->RefreshTokenExpiryLength))
        {
            Options->SetRefreshTokenExpiryLength(TokenOptions->RefreshTokenExpiryLength);
            CurrentLoginState.RefreshTokenExpiryLength = TokenOptions->RefreshTokenExpiryLength;
        }

        Request->SetTokenOptions(Options);

        LoginStateResultCallback LoginStateResCallback = [=](const LoginStateResult& LoginStateRes)
        {
            if (LoginStateRes.GetResultCode() == csp::systems::EResultCode::Success)
            {
                NotifyRefreshTokenHasChanged();

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
                LoginStateResCallback, &CurrentLoginState);

        static_cast<chs_user::AuthenticationApi*>(AuthenticationAPI)->usersLoginPost({ Request }, ResponseHandler);
    }
    else
    {
        csp::systems::LoginStateResult BadResult;
        BadResult.SetResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        Callback(BadResult);
    }
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

    if (CurrentLoginState.State == csp::common::ELoginState::LoggedOut || CurrentLoginState.State == csp::common::ELoginState::Error)
    {
        CurrentLoginState.State = csp::common::ELoginState::LoginRequested;

        auto Request = std::make_shared<chs_user::RefreshRequest>();
        Request->SetDeviceId(csp::CSPFoundation::GetDeviceId());
        Request->SetUserId(UserId);
        Request->SetRefreshToken(RefreshToken);

        auto Options = std::make_shared<chs_user::TokenOptions>();

        if (TokenOptions.HasValue() && CheckExpiryLengthFormat(TokenOptions->AccessTokenExpiryLength))
        {
            Options->SetExpiryLength(TokenOptions->AccessTokenExpiryLength);
            CurrentLoginState.AccessTokenExpiryLength = TokenOptions->AccessTokenExpiryLength;
        }

        if (TokenOptions.HasValue() && CheckExpiryLengthFormat(TokenOptions->RefreshTokenExpiryLength))
        {
            Options->SetRefreshTokenExpiryLength(TokenOptions->RefreshTokenExpiryLength);
            CurrentLoginState.RefreshTokenExpiryLength = TokenOptions->RefreshTokenExpiryLength;
        }

        Request->SetTokenOptions(Options);

        LoginStateResultCallback LoginStateResCallback = [=](const LoginStateResult& LoginStateRes)
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
                LoginStateResCallback, &CurrentLoginState);

        static_cast<chs_user::AuthenticationApi*>(AuthenticationAPI)->usersRefreshPost({ Request }, ResponseHandler);
    }
    else
    {
        csp::systems::LoginStateResult BadResult;
        BadResult.SetResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        BadResult.ResponseBody = "Already logged in!";
        Callback(BadResult);
    }
}

void UserSystem::LoginAsGuest(bool CreateMultiplayerConnection, const csp::common::Optional<bool>& UserHasVerifiedAge,
    const csp::common::Optional<TokenOptions>& TokenOptions, LoginStateResultCallback Callback)
{
    if (CurrentLoginState.State == csp::common::ELoginState::LoggedOut || CurrentLoginState.State == csp::common::ELoginState::Error)
    {
        CurrentLoginState.State = csp::common::ELoginState::LoginRequested;

        auto Request = std::make_shared<chs_user::LoginRequest>();
        Request->SetDeviceId(csp::CSPFoundation::GetDeviceId());
        Request->SetTenant(csp::CSPFoundation::GetTenant());

        if (UserHasVerifiedAge.HasValue())
        {
            Request->SetVerifiedAgeEighteen(*UserHasVerifiedAge);
        }

        auto Options = std::make_shared<chs_user::TokenOptions>();

        if (TokenOptions.HasValue() && CheckExpiryLengthFormat(TokenOptions->AccessTokenExpiryLength))
        {
            Options->SetExpiryLength(TokenOptions->AccessTokenExpiryLength);
            CurrentLoginState.AccessTokenExpiryLength = TokenOptions->AccessTokenExpiryLength;
        }

        if (TokenOptions.HasValue() && CheckExpiryLengthFormat(TokenOptions->RefreshTokenExpiryLength))
        {
            Options->SetRefreshTokenExpiryLength(TokenOptions->RefreshTokenExpiryLength);
            CurrentLoginState.RefreshTokenExpiryLength = TokenOptions->RefreshTokenExpiryLength;
        }

        Request->SetTokenOptions(Options);

        LoginStateResultCallback LoginStateResCallback = [=](const LoginStateResult& LoginStateRes)
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
                LoginStateResCallback, &CurrentLoginState);

        static_cast<chs_user::AuthenticationApi*>(AuthenticationAPI)->usersLoginPost({ Request }, ResponseHandler);
    }
    else
    {
        csp::systems::LoginStateResult BadResult;
        BadResult.SetResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        Callback(BadResult);
    }
}

void UserSystem::LoginAsGuestWithDeferredProfileCreation(const csp::common::Optional<bool>& UserHasVerifiedAge, LoginStateResultCallback Callback)
{
    if (CurrentLoginState.State == csp::common::ELoginState::LoggedOut || CurrentLoginState.State == csp::common::ELoginState::Error)
    {
        CurrentLoginState.State = csp::common::ELoginState::LoginRequested;

        auto Request = std::make_shared<chs_user::LoginGuestRequest>();
        Request->SetDeviceId(csp::CSPFoundation::GetDeviceId());
        Request->SetTenant(csp::CSPFoundation::GetTenant());

        if (UserHasVerifiedAge.HasValue())
        {
            Request->SetVerifiedAgeEighteen(*UserHasVerifiedAge);
        }

        LoginStateResultCallback LoginStateResCallback = [=](const LoginStateResult& LoginStateRes)
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
                LoginStateResCallback, &CurrentLoginState);

        // Despite the naming, "login-guest" is the deferred, optimized, non-standard guest login.
        // The regular login endpoint that "loginAsGuest" uses is the "real" one.
        static_cast<chs_user::AuthenticationApi*>(AuthenticationAPI)->usersLogin_guestPost({ Request }, ResponseHandler);
    }
    else
    {
        csp::systems::LoginStateResult BadResult;
        BadResult.SetResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        Callback(BadResult);
    }
}

csp::common::Array<EThirdPartyAuthenticationProviders> UserSystem::GetSupportedThirdPartyAuthenticationProviders() const
{
    csp::common::Array<EThirdPartyAuthenticationProviders> Providers((EThirdPartyAuthenticationProviders::Num));
    for (uint8_t idx = 0; idx < EThirdPartyAuthenticationProviders::Num; ++idx)
        Providers[idx] = static_cast<EThirdPartyAuthenticationProviders>(idx);

    return Providers;
}

void UserSystem::GetThirdPartyProviderAuthoriseURL(
    EThirdPartyAuthenticationProviders AuthProvider, const csp::common::String& RedirectURL, ThirdPartyAuthDetailsResultCallback Callback)
{
    if (AuthProvider == EThirdPartyAuthenticationProviders::Invalid || RedirectURL.IsEmpty())
    {
        CSP_LOG_ERROR_FORMAT("Invalid parameters passed to GetThirdPartyProviderAuthoriseURL. AuthProvider: %s, RedirectURL: %s",
            ConvertExternalAuthProvidersToString(AuthProvider).c_str(), RedirectURL.c_str());

        ThirdPartyAuthDetailsResult ErrorResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        Callback(ErrorResult);

        return;
    }

    ProviderDetailsResultCallback ThirdPartyAuthenticationDetailsCallback
        = [AuthProvider, RedirectURL, Callback](const ProviderDetailsResult& ProviderDetailsRes)
    {
        if (ProviderDetailsRes.GetResultCode() == csp::systems::EResultCode::Success)
        {
            const csp::common::String AuthoriseURL = ProviderDetailsRes.GetDetails().AuthoriseURL;
            const csp::common::String ProviderClientId = ProviderDetailsRes.GetDetails().ProviderClientId;
            const csp::common::String AuthProviderFormattedScopes = FormatScopesForURL(ProviderDetailsRes.GetDetails().ProviderAuthScopes);
            const csp::common::String ThirdPartyAuthStateId = csp::GenerateUUID().c_str();

            csp::common::String ThirdPartyAuthURL;
            if (ConstructThirdPartyAuthURL(
                    AuthoriseURL, ProviderClientId, AuthProviderFormattedScopes, ThirdPartyAuthStateId, RedirectURL, ThirdPartyAuthURL))
            {
                ThirdPartyAuthDetailsResult SuccessResult(ThirdPartyAuthStateId, AuthProvider, RedirectURL, ThirdPartyAuthURL);
                Callback(SuccessResult);

                return;
            }

            CSP_LOG_ERROR_FORMAT(
                "The returned provider details were invalid. AuthoriseURL: %s, ProviderClientId: %s, AuthProviderFormattedScopes: %s",
                AuthoriseURL.c_str(), ProviderClientId.c_str(), AuthProviderFormattedScopes.c_str());

            ThirdPartyAuthDetailsResult ErrorResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
            Callback(ErrorResult);
        }
        else if (ProviderDetailsRes.GetResultCode() != csp::systems::EResultCode::InProgress)
        {
            CSP_LOG_FORMAT(common::LogLevel::Error, "The retrieval of third party details was not successful. ResCode: %d, HttpResCode: %d",
                static_cast<int>(ProviderDetailsRes.GetResultCode()), ProviderDetailsRes.GetHttpResultCode());

            ThirdPartyAuthDetailsResult ErrorResult(ProviderDetailsRes.GetResultCode(), ProviderDetailsRes.GetHttpResultCode());
            Callback(ErrorResult);
        }
    };

    const csp::services::ResponseHandlerPtr ResponseHandler
        = AuthenticationAPI->CreateHandler<ProviderDetailsResultCallback, ProviderDetailsResult, void, chs_user::SocialProviderInfo>(
            ThirdPartyAuthenticationDetailsCallback, nullptr, csp::web::EResponseCodes::ResponseOK);

    static_cast<chs_user::AuthenticationApi*>(AuthenticationAPI)
        ->social_providersProviderGet({ ConvertExternalAuthProvidersToString(AuthProvider), csp::CSPFoundation::GetTenant() }, ResponseHandler);
}

void UserSystem::LoginToThirdPartyAuthenticationProvider(const csp::common::String& ThirdPartyToken, const csp::common::String& ThirdPartyStateId,
    const ThirdPartyAuthDetails& ThirdPartyAuthDetails, bool CreateMultiplayerConnection, const csp::common::Optional<bool>& UserHasVerifiedAge,
    const csp::common::Optional<TokenOptions>& TokenOptions, LoginStateResultCallback Callback)
{
    if (ThirdPartyToken.IsEmpty() || ThirdPartyStateId.IsEmpty())
    {
        CSP_LOG_ERROR_FORMAT("Invalid parameters passed to LoginToThirdPartyAuthenticationProvider. ThirdPartyToken: %s, ThirdPartyStateId: %s",
            ThirdPartyToken.c_str(), ThirdPartyStateId.c_str());

        CurrentLoginState.State = csp::common::ELoginState::Error;
        csp::systems::LoginStateResult ErrorResult;
        ErrorResult.SetResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        Callback(ErrorResult);

        return;
    }

    if (ThirdPartyAuthDetails.ThirdPartyRequestedAuthProvider == EThirdPartyAuthenticationProviders::Invalid
        || ThirdPartyAuthDetails.ThirdPartyAuthRedirectURL.IsEmpty())
    {
        CSP_LOG_ERROR_FORMAT(
            "The third party provider details are not valid. You must call AssetSystem::GetThirdPartyProviderAuthoriseURL() first to "
            "retrieve the provider details from MCS. AuthProvider: %s, RedirectURL: %s",
            ConvertExternalAuthProvidersToString(ThirdPartyAuthDetails.ThirdPartyRequestedAuthProvider).c_str(),
            ThirdPartyAuthDetails.ThirdPartyAuthRedirectURL.c_str());

        CurrentLoginState.State = csp::common::ELoginState::Error;
        csp::systems::LoginStateResult ErrorResult;
        ErrorResult.SetResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        Callback(ErrorResult);

        return;
    }

    // Confirm that the stored ThirdPartyAuthStateId matches the one passed by the Client.
    if (ThirdPartyAuthDetails.ThirdPartyAuthStateId != ThirdPartyStateId)
    {
        CSP_LOG_MSG(common::LogLevel::Error,
            "The state ID is not correct. If you have not already done so, please call AssetSystem::GetThirdPartyProviderAuthoriseURL() first to "
            "retrieve the provider details from MCS."); // The ThirdPartyStateId is not being logged for security reasons

        CurrentLoginState.State = csp::common::ELoginState::Error;

        csp::systems::LoginStateResult ErrorResult;
        ErrorResult.SetResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        Callback(ErrorResult);

        return;
    }

    if (CurrentLoginState.State == csp::common::ELoginState::LoggedIn)
    {
        CSP_LOG_MSG(common::LogLevel::Warning,
            "You are already logged in. Please note that this call to UserSystem::LoginToThirdPartyAuthenticationProvider will issue a new access "
            "token. Your existing token will still be valid but cannot be refreshed after it expires.");
    }

    LoginStateResultCallback LoginStateResCallback = [=](const LoginStateResult& LoginStateRes)
    {
        if (LoginStateRes.GetResultCode() == csp::systems::EResultCode::InProgress)
        {
            return;
        }

        if (LoginStateRes.GetResultCode() == csp::systems::EResultCode::Failed)
        {
            Callback(LoginStateRes);

            return;
        }

        NotifyRefreshTokenHasChanged();

        csp::multiplayer::MultiplayerConnection::ErrorCodeCallbackHandler ConnectionCallback
            = [Callback, LoginStateRes](csp::multiplayer::ErrorCode ErrCode)
        {
            if (ErrCode != csp::multiplayer::ErrorCode::None)
            {
                CSP_LOG_ERROR_FORMAT("Error connecting MultiplayerConnection: %s", csp::multiplayer::ErrorCodeToString(ErrCode).c_str());
            }

            Callback(LoginStateRes);

            return;
        };

        StartMultiplayerConnection(*SystemsManager::Get().GetMultiplayerConnection(), CSPFoundation::GetEndpoints().MultiplayerConnection.GetURI(),
            ConnectionCallback, LoginStateRes, *LogSystem, CreateMultiplayerConnection);
    };

    const auto Request = std::make_shared<chs_user::LoginSocialRequest>();
    Request->SetDeviceId(csp::CSPFoundation::GetDeviceId());
    Request->SetOAuthRedirectUri(ThirdPartyAuthDetails.ThirdPartyAuthRedirectURL);
    Request->SetProvider(ConvertExternalAuthProvidersToString(ThirdPartyAuthDetails.ThirdPartyRequestedAuthProvider));
    Request->SetToken(ThirdPartyToken);
    Request->SetTenant(csp::CSPFoundation::GetTenant());

    if (UserHasVerifiedAge.HasValue())
    {
        Request->SetVerifiedAgeEighteen(*UserHasVerifiedAge);
    }

    auto Options = std::make_shared<chs_user::TokenOptions>();

    if (TokenOptions.HasValue() && CheckExpiryLengthFormat(TokenOptions->AccessTokenExpiryLength))
    {
        Options->SetExpiryLength(TokenOptions->AccessTokenExpiryLength);
        CurrentLoginState.AccessTokenExpiryLength = TokenOptions->AccessTokenExpiryLength;
    }

    if (TokenOptions.HasValue() && CheckExpiryLengthFormat(TokenOptions->RefreshTokenExpiryLength))
    {
        Options->SetRefreshTokenExpiryLength(TokenOptions->RefreshTokenExpiryLength);
        CurrentLoginState.RefreshTokenExpiryLength = TokenOptions->RefreshTokenExpiryLength;
    }

    Request->SetTokenOptions(Options);

    CurrentLoginState.State = csp::common::ELoginState::LoginRequested;

    csp::services::ResponseHandlerPtr ResponseHandler
        = AuthenticationAPI->CreateHandler<LoginStateResultCallback, LoginStateResult, csp::common::LoginState, chs_user::AuthDto>(
            LoginStateResCallback, &CurrentLoginState);

    static_cast<chs_user::AuthenticationApi*>(AuthenticationAPI)->usersLogin_socialPost({ Request }, ResponseHandler);
}

void UserSystem::Logout(NullResultCallback Callback)
{
    if (CurrentLoginState.State == csp::common::ELoginState::LoggedIn)
    {
        CurrentLoginState.State = csp::common::ELoginState::LogoutRequested;

        // Disconnect MultiplayerConnection before logging out
        csp::multiplayer::MultiplayerConnection::ErrorCodeCallbackHandler ErrorCallback = [Callback, this](csp::multiplayer::ErrorCode ErrCode)
        {
            if (ErrCode != csp::multiplayer::ErrorCode::None)
            {
                CSP_LOG_ERROR_FORMAT("Error disconnecting MultiplayerConnection: %s", csp::multiplayer::ErrorCodeToString(ErrCode).c_str());
            }

            auto Request = std::make_shared<chs_user::LogoutRequest>();
            Request->SetUserId(CurrentLoginState.UserId);
            Request->SetDeviceId(CurrentLoginState.DeviceId);

            csp::services::ResponseHandlerPtr ResponseHandler
                = AuthenticationAPI->CreateHandler<NullResultCallback, LogoutResult, csp::common::LoginState, csp::services::NullDto>(
                    Callback, &CurrentLoginState, csp::web::EResponseCodes::ResponseNoContent);

            static_cast<chs_user::AuthenticationApi*>(AuthenticationAPI)->usersLogoutPost({ Request }, ResponseHandler);
        };

        auto* MultiplayerConnection = SystemsManager::Get().GetMultiplayerConnection();
        MultiplayerConnection->Disconnect(ErrorCallback);
    }
    else
    {
        csp::systems::LogoutResult BadResult;
        BadResult.SetResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        Callback(BadResult);
    }
}

void UserSystem::CreateUser(const csp::common::Optional<csp::common::String>& UserName, const csp::common::Optional<csp::common::String>& DisplayName,
    const csp::common::String& Email, const csp::common::String& Password, bool ReceiveNewsletter, bool HasVerifiedAge,
    const csp::common::Optional<csp::common::String>& RedirectUrl, const csp::common::Optional<csp::common::String>& InviteToken,
    ProfileResultCallback Callback)
{
    auto Request = std::make_shared<chs_user::CreateUserRequest>();

    if (UserName.HasValue())
    {
        Request->SetUserName(*UserName);
    }

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

void UserSystem::UpgradeGuestAccount(const csp::common::String& UserName, const csp::common::String& DisplayName, const csp::common::String& Email,
    const csp::common::String& Password, ProfileResultCallback Callback)
{
    const csp::common::String UserId = CurrentLoginState.UserId;

    auto Request = std::make_shared<chs_user::UpgradeGuestRequest>();

    Request->SetUserName(UserName);
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
    const csp::common::String UserId = CurrentLoginState.UserId;

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

// void UserSystem::ResetAuthenticationState()
//{
//     CurrentLoginState.State = csp::common::ELoginState::LoggedOut;
//     ThirdPartyAuthStateId = "";
//     ThirdPartyRequestedAuthProvider = EThirdPartyAuthenticationProviders::Invalid;
// }

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
