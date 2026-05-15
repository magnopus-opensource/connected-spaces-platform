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
void StartMultiplayerConnection(csp::multiplayer::MultiplayerConnection& multiplayerConnection, const csp::common::String& multiplayerUri,
    csp::multiplayer::MultiplayerConnection::ErrorCodeCallbackHandler connectionCallback, const csp::systems::LoginStateResult& loginStateRes,
    csp::common::LogSystem& logSystem, bool createMultiplayerConnection)
{
    if (createMultiplayerConnection)
    {
        logSystem.LogMsg(csp::common::LogLevel::Log, "Starting Multiplayer Connection");
        multiplayerConnection.Connect(
            connectionCallback, multiplayerUri, loginStateRes.GetLoginState().AccessToken, loginStateRes.GetLoginState().DeviceId);
    }
    else
    {
        logSystem.LogMsg(csp::common::LogLevel::Log, "Not starting a Multiplayer Connection");
        connectionCallback(csp::multiplayer::ErrorCode::None);
    }
}

/* Check if the provided expiry length in token options is formatted as "HH:MM:SS" or "HHH:MM:SS"
 *
 * Return True if expiry length matches format "HH:MM:SS" or "HHH:MM:SS", false otherwise
 *
 * "HHH:MM:SS" supports durations greater than 4 days */
bool CheckExpiryLengthFormat(const csp::common::String& expiryLength)
{
    if (expiryLength.IsEmpty())
    {
        return false;
    }

    std::regex regex("^[0-9]{2,3}:[0-5][0-9]:[0-5][0-9]$");
    if (std::regex_search(expiryLength.c_str(), regex))
    {
        return true;
    }

    CSP_LOG_MSG(csp::common::LogLevel::Warning, "Expiry length token option does not match the expected format, and has been ignored.");
    return false;
}

std::optional<csp::common::String> ThirdPartyPlatformToString(const csp::common::Optional<csp::systems::EThirdPartyPlatform>& clientType)
{
    if (!clientType.HasValue())
    {
        return std::nullopt;
    }

    switch (*clientType)
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

}

namespace csp::systems
{

const char* EMPTY_SPACE_STRING = " ";

csp::common::String ConvertExternalAuthProvidersToString(EThirdPartyAuthenticationProviders provider)
{
    switch (provider)
    {
    case EThirdPartyAuthenticationProviders::Google:
        return "Google";
    case EThirdPartyAuthenticationProviders::Discord:
        return "Discord";
    case EThirdPartyAuthenticationProviders::Apple:
        return "Apple";
    default:
    {
        CSP_LOG_FORMAT(common::LogLevel::Error, "Unsupported Provider Type requested: %d, returning Google", static_cast<uint8_t>(provider));
        return "Google";
    }
    }
}

csp::common::String FormatScopesForURL(csp::common::Array<csp::common::String> scopes)
{
    csp::common::String formattedScopes;
    for (size_t idx = 0; idx < scopes.Size(); ++idx)
    {
        formattedScopes.Append(scopes[idx]);
        if (idx != scopes.Size() - 1)
        {
            formattedScopes.Append(EMPTY_SPACE_STRING);
        }
    }

    return formattedScopes;
}

AuthContext::AuthContext(csp::services::ApiBase* authenticationApi, csp::common::LoginState& loginState)
    : m_authenticationApi { authenticationApi }
    , m_loginState { &loginState }
{
}

const csp::common::LoginState& AuthContext::GetLoginState() const { return *m_loginState; }

void AuthContext::RefreshToken(std::function<void(bool)> callback)
{
    if (m_loginState->State == csp::common::ELoginState::LoggedIn)
    {
        auto request = std::make_shared<chs_user::RefreshRequest>();
        request->SetDeviceId(csp::CSPFoundation::GetDeviceId());
        request->SetUserId(m_loginState->UserId);
        request->SetRefreshToken(m_loginState->RefreshToken);

        auto options = std::make_shared<chs_user::TokenOptions>();
        options->SetExpiryLength(m_loginState->AccessTokenExpiryLength);
        options->SetRefreshTokenExpiryLength(m_loginState->RefreshTokenExpiryLength);
        request->SetTokenOptions(options);

        LoginStateResultCallback loginStateResCallback = [=](const LoginStateResult& loginStateRes)
        {
            if (loginStateRes.GetResultCode() == csp::systems::EResultCode::InProgress)
            {
                return;
            }
            else if (loginStateRes.GetResultCode() == csp::systems::EResultCode::Success)
            {
                const NullResult result(csp::systems::EResultCode::Success, 200);
                INVOKE_IF_NOT_NULL(callback, true);
            }
            else
            {
                const NullResult result(loginStateRes.GetResultCode(), loginStateRes.GetHttpResultCode());
                INVOKE_IF_NOT_NULL(callback, false);
            }
        };

        csp::services::ResponseHandlerPtr responseHandler
            = m_authenticationApi->CreateHandler<LoginStateResultCallback, LoginStateResult, csp::common::LoginState, chs_user::AuthDto>(
                loginStateResCallback, m_loginState);

        static_cast<chs_user::AuthenticationApi*>(m_authenticationApi)->usersRefreshPost({ request }, responseHandler);
    }
}

UserSystem::UserSystem()
    : SystemBase(nullptr, nullptr, nullptr)
    , m_authenticationApi(nullptr)
    , m_profileApi(nullptr)
    , m_pingApi(nullptr)
    , m_stripeApi { nullptr }
    , m_auth { m_authenticationApi, m_currentLoginState }
{
}

UserSystem::UserSystem(csp::web::WebClient* inWebClient, csp::multiplayer::NetworkEventBus* inEventBus, csp::common::LogSystem& logSystem)
    : SystemBase(inWebClient, inEventBus, &logSystem)
    , m_authenticationApi { new chs_user::AuthenticationApi(inWebClient) }
    , m_profileApi { new chs_user::ProfileApi(inWebClient) }
    , m_pingApi { new chs_user::PingApi(inWebClient) }
    , m_stripeApi { new chs_user::StripeApi(inWebClient) }
    , m_refreshTokenChangedCallback(nullptr)
    , m_auth { m_authenticationApi, m_currentLoginState }
{
}

UserSystem::~UserSystem()
{
    delete (m_pingApi);
    delete (m_profileApi);
    delete (m_authenticationApi);
    delete (m_stripeApi);
}

void UserSystem::SetNetworkEventBus(csp::multiplayer::NetworkEventBus& eventBus)
{
    m_eventBusPtr = &eventBus;

    RegisterSystemCallback();
}

const csp::common::LoginState& UserSystem::GetLoginState() const { return m_currentLoginState; }

void UserSystem::SetNewLoginTokenReceivedCallback(LoginTokenInfoResultCallback callback) { m_refreshTokenChangedCallback = callback; }

void UserSystem::Login(const csp::common::String& email, const csp::common::String& password, bool createMultiplayerConnection,
    const csp::common::Optional<bool>& userHasVerifiedAge, const csp::common::Optional<TokenOptions>& tokenOptions, LoginStateResultCallback callback)
{
    if (email.IsEmpty())
    {
        CSP_LOG_ERROR_MSG("UserSystem::Login, Email must not be empty.");
        callback(MakeInvalid<LoginStateResult>());
        return;
    }
    if (password.IsEmpty())
    {
        CSP_LOG_ERROR_MSG("UserSystem::Login, Password must not be empty.");
        callback(MakeInvalid<LoginStateResult>());
        return;
    }

    if (m_currentLoginState.State == csp::common::ELoginState::LoggedOut || m_currentLoginState.State == csp::common::ELoginState::Error)
    {
        m_currentLoginState.State = csp::common::ELoginState::LoginRequested;

        auto request = std::make_shared<chs_user::LoginRequest>();
        request->SetDeviceId(csp::CSPFoundation::GetDeviceId());
        request->SetEmail(email);
        request->SetPassword(password);
        request->SetTenant(csp::CSPFoundation::GetTenant());

        if (userHasVerifiedAge.HasValue())
        {
            request->SetVerifiedAgeEighteen(*userHasVerifiedAge);
        }

        auto options = std::make_shared<chs_user::TokenOptions>();

        if (tokenOptions.HasValue() && CheckExpiryLengthFormat(tokenOptions->AccessTokenExpiryLength))
        {
            options->SetExpiryLength(tokenOptions->AccessTokenExpiryLength);
            m_currentLoginState.AccessTokenExpiryLength = tokenOptions->AccessTokenExpiryLength;
        }

        if (tokenOptions.HasValue() && CheckExpiryLengthFormat(tokenOptions->RefreshTokenExpiryLength))
        {
            options->SetRefreshTokenExpiryLength(tokenOptions->RefreshTokenExpiryLength);
            m_currentLoginState.RefreshTokenExpiryLength = tokenOptions->RefreshTokenExpiryLength;
        }

        request->SetTokenOptions(options);

        LoginStateResultCallback loginStateResCallback = [=](const LoginStateResult& loginStateRes)
        {
            if (loginStateRes.GetResultCode() == csp::systems::EResultCode::Success)
            {
                csp::multiplayer::MultiplayerConnection::ErrorCodeCallbackHandler connectionCallback
                    = [callback, loginStateRes](csp::multiplayer::ErrorCode errCode)
                {
                    if (errCode != csp::multiplayer::ErrorCode::None)
                    {
                        CSP_LOG_ERROR_FORMAT("Error connecting MultiplayerConnection: %s", csp::multiplayer::ErrorCodeToString(errCode).c_str());

                        callback(loginStateRes);
                        return;
                    }

                    callback(loginStateRes);
                };

                StartMultiplayerConnection(*SystemsManager::Get().GetMultiplayerConnection(),
                    CSPFoundation::GetEndpoints().MultiplayerConnection.GetURI(), connectionCallback, loginStateRes, *m_logSystem,
                    createMultiplayerConnection);
            }
            else if (loginStateRes.GetResultCode() == csp::systems::EResultCode::Failed)
            {
                CSP_LOG_ERROR_FORMAT("Login Failed. Code: %i", loginStateRes.GetHttpResultCode());
                callback(loginStateRes);
            }
        };

        csp::services::ResponseHandlerPtr responseHandler
            = m_authenticationApi->CreateHandler<LoginStateResultCallback, LoginStateResult, csp::common::LoginState, chs_user::AuthDto>(
                loginStateResCallback, &m_currentLoginState);

        static_cast<chs_user::AuthenticationApi*>(m_authenticationApi)->usersLoginPost({ request }, responseHandler);
    }
    else
    {
        csp::systems::LoginStateResult badResult;
        badResult.SetResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        callback(badResult);
    }
}

void UserSystem::LoginWithRefreshToken(const csp::common::String& userId, const csp::common::String& refreshToken, bool createMultiplayerConnection,
    const csp::common::Optional<TokenOptions>& tokenOptions, LoginStateResultCallback callback)
{
    if (userId.IsEmpty())
    {
        CSP_LOG_ERROR_MSG("UserSystem::LoginWithRefreshToken, UserId must not be empty.");
        callback(MakeInvalid<LoginStateResult>());
        return;
    }

    if (m_currentLoginState.State == csp::common::ELoginState::LoggedOut || m_currentLoginState.State == csp::common::ELoginState::Error)
    {
        m_currentLoginState.State = csp::common::ELoginState::LoginRequested;

        auto request = std::make_shared<chs_user::RefreshRequest>();
        request->SetDeviceId(csp::CSPFoundation::GetDeviceId());
        request->SetUserId(userId);
        request->SetRefreshToken(refreshToken);

        auto options = std::make_shared<chs_user::TokenOptions>();

        if (tokenOptions.HasValue() && CheckExpiryLengthFormat(tokenOptions->AccessTokenExpiryLength))
        {
            options->SetExpiryLength(tokenOptions->AccessTokenExpiryLength);
            m_currentLoginState.AccessTokenExpiryLength = tokenOptions->AccessTokenExpiryLength;
        }

        if (tokenOptions.HasValue() && CheckExpiryLengthFormat(tokenOptions->RefreshTokenExpiryLength))
        {
            options->SetRefreshTokenExpiryLength(tokenOptions->RefreshTokenExpiryLength);
            m_currentLoginState.RefreshTokenExpiryLength = tokenOptions->RefreshTokenExpiryLength;
        }

        request->SetTokenOptions(options);

        LoginStateResultCallback loginStateResCallback = [=](const LoginStateResult& loginStateRes)
        {
            if (loginStateRes.GetResultCode() == csp::systems::EResultCode::Success)
            {
                csp::multiplayer::MultiplayerConnection::ErrorCodeCallbackHandler connectionCallback
                    = [callback, loginStateRes](csp::multiplayer::ErrorCode errCode)
                {
                    if (errCode != csp::multiplayer::ErrorCode::None)
                    {
                        CSP_LOG_ERROR_FORMAT("Error connecting MultiplayerConnection: %s", csp::multiplayer::ErrorCodeToString(errCode).c_str());

                        callback(loginStateRes);
                        return;
                    }

                    callback(loginStateRes);
                };

                StartMultiplayerConnection(*SystemsManager::Get().GetMultiplayerConnection(),
                    CSPFoundation::GetEndpoints().MultiplayerConnection.GetURI(), connectionCallback, loginStateRes, *m_logSystem,
                    createMultiplayerConnection);
            }
            else
            {
                callback(loginStateRes);
            }
        };

        csp::services::ResponseHandlerPtr responseHandler
            = m_authenticationApi->CreateHandler<LoginStateResultCallback, LoginStateResult, csp::common::LoginState, chs_user::AuthDto>(
                loginStateResCallback, &m_currentLoginState);

        static_cast<chs_user::AuthenticationApi*>(m_authenticationApi)->usersRefreshPost({ request }, responseHandler);
    }
    else
    {
        csp::systems::LoginStateResult badResult;
        badResult.SetResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        badResult.m_responseBody = "Already logged in!";
        callback(badResult);
    }
}

void UserSystem::LoginAsGuest(bool createMultiplayerConnection, const csp::common::Optional<bool>& userHasVerifiedAge,
    const csp::common::Optional<TokenOptions>& tokenOptions, LoginStateResultCallback callback)
{
    if (m_currentLoginState.State == csp::common::ELoginState::LoggedOut || m_currentLoginState.State == csp::common::ELoginState::Error)
    {
        m_currentLoginState.State = csp::common::ELoginState::LoginRequested;

        auto request = std::make_shared<chs_user::LoginRequest>();
        request->SetDeviceId(csp::CSPFoundation::GetDeviceId());
        request->SetTenant(csp::CSPFoundation::GetTenant());

        if (userHasVerifiedAge.HasValue())
        {
            request->SetVerifiedAgeEighteen(*userHasVerifiedAge);
        }

        auto options = std::make_shared<chs_user::TokenOptions>();

        if (tokenOptions.HasValue() && CheckExpiryLengthFormat(tokenOptions->AccessTokenExpiryLength))
        {
            options->SetExpiryLength(tokenOptions->AccessTokenExpiryLength);
            m_currentLoginState.AccessTokenExpiryLength = tokenOptions->AccessTokenExpiryLength;
        }

        if (tokenOptions.HasValue() && CheckExpiryLengthFormat(tokenOptions->RefreshTokenExpiryLength))
        {
            options->SetRefreshTokenExpiryLength(tokenOptions->RefreshTokenExpiryLength);
            m_currentLoginState.RefreshTokenExpiryLength = tokenOptions->RefreshTokenExpiryLength;
        }

        request->SetTokenOptions(options);

        LoginStateResultCallback loginStateResCallback = [=](const LoginStateResult& loginStateRes)
        {
            if (loginStateRes.GetResultCode() == csp::systems::EResultCode::Success)
            {
                csp::multiplayer::MultiplayerConnection::ErrorCodeCallbackHandler connectionCallback
                    = [callback, loginStateRes](csp::multiplayer::ErrorCode errCode)
                {
                    if (errCode != csp::multiplayer::ErrorCode::None)
                    {
                        CSP_LOG_ERROR_FORMAT("Error connecting MultiplayerConnection: %s", csp::multiplayer::ErrorCodeToString(errCode).c_str());

                        callback(loginStateRes);
                        return;
                    }

                    callback(loginStateRes);
                };

                StartMultiplayerConnection(*SystemsManager::Get().GetMultiplayerConnection(),
                    CSPFoundation::GetEndpoints().MultiplayerConnection.GetURI(), connectionCallback, loginStateRes, *m_logSystem,
                    createMultiplayerConnection);
            }
            else
            {
                callback(loginStateRes);
            }
        };

        csp::services::ResponseHandlerPtr responseHandler
            = m_authenticationApi->CreateHandler<LoginStateResultCallback, LoginStateResult, csp::common::LoginState, chs_user::AuthDto>(
                loginStateResCallback, &m_currentLoginState);

        static_cast<chs_user::AuthenticationApi*>(m_authenticationApi)->usersLoginPost({ request }, responseHandler);
    }
    else
    {
        csp::systems::LoginStateResult badResult;
        badResult.SetResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        callback(badResult);
    }
}

void UserSystem::LoginAsGuestWithDeferredProfileCreation(const csp::common::Optional<bool>& userHasVerifiedAge, LoginStateResultCallback callback)
{
    if (m_currentLoginState.State == csp::common::ELoginState::LoggedOut || m_currentLoginState.State == csp::common::ELoginState::Error)
    {
        m_currentLoginState.State = csp::common::ELoginState::LoginRequested;

        auto request = std::make_shared<chs_user::LoginGuestRequest>();
        request->SetDeviceId(csp::CSPFoundation::GetDeviceId());
        request->SetTenant(csp::CSPFoundation::GetTenant());

        if (userHasVerifiedAge.HasValue())
        {
            request->SetVerifiedAgeEighteen(*userHasVerifiedAge);
        }

        LoginStateResultCallback loginStateResCallback = [=](const LoginStateResult& loginStateRes)
        {
            if (loginStateRes.GetResultCode() == csp::systems::EResultCode::Success)
            {
                csp::multiplayer::MultiplayerConnection::ErrorCodeCallbackHandler connectionCallback
                    = [callback, loginStateRes](csp::multiplayer::ErrorCode errCode)
                {
                    if (errCode != csp::multiplayer::ErrorCode::None)
                    {
                        // It would be extremely strange to hit this branch, but it remains here just in case.
                        CSP_LOG_ERROR_FORMAT("Unexpected error connecting MultiplayerConnection. This is strange! : %s",
                            csp::multiplayer::ErrorCodeToString(errCode).c_str());
                        callback(loginStateRes);
                        return;
                    }

                    callback(loginStateRes);
                };

                // Do not start a multiplayer connection, need to call through this to trigger all the callbacks though.
                StartMultiplayerConnection(*SystemsManager::Get().GetMultiplayerConnection(),
                    CSPFoundation::GetEndpoints().MultiplayerConnection.GetURI(), connectionCallback, loginStateRes, *m_logSystem, false);
            }
            else
            {
                callback(loginStateRes);
            }
        };

        csp::services::ResponseHandlerPtr responseHandler
            = m_authenticationApi->CreateHandler<LoginStateResultCallback, LoginStateResult, csp::common::LoginState, chs_user::AuthDto>(
                loginStateResCallback, &m_currentLoginState);

        // Despite the naming, "login-guest" is the deferred, optimized, non-standard guest login.
        // The regular login endpoint that "loginAsGuest" uses is the "real" one.
        static_cast<chs_user::AuthenticationApi*>(m_authenticationApi)->usersLogin_guestPost({ request }, responseHandler);
    }
    else
    {
        csp::systems::LoginStateResult badResult;
        badResult.SetResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        callback(badResult);
    }
}

csp::common::Array<EThirdPartyAuthenticationProviders> UserSystem::GetSupportedThirdPartyAuthenticationProviders() const
{
    csp::common::Array<EThirdPartyAuthenticationProviders> providers((EThirdPartyAuthenticationProviders::Num));
    for (uint8_t idx = 0; idx < EThirdPartyAuthenticationProviders::Num; ++idx)
        providers[idx] = static_cast<EThirdPartyAuthenticationProviders>(idx);

    return providers;
}

void UserSystem::ResetThirdPartyAuthState()
{
    m_thirdPartyAuthStateId = "";
    m_thirdPartyClientType = "";
    m_thirdPartyAuthRedirectUrl = "";
    m_thirdPartyRequestedAuthProvider = EThirdPartyAuthenticationProviders::Invalid;
}

void UserSystem::GetThirdPartyProviderAuthorizeURL(EThirdPartyAuthenticationProviders authProvider, const csp::common::String& redirectUrl,
    const csp::common::Optional<EThirdPartyPlatform>& clientType, StringResultCallback callback)
{
    if (authProvider == EThirdPartyAuthenticationProviders::Invalid)
    {
        CSP_LOG_ERROR_MSG(
            "UserSystem::GetThirdPartyProviderAuthorizeURL() - EThirdPartyAuthenticationProviders::Invalid was passed as an AuthProvider.");

        StringResult errorResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        callback(errorResult);

        return;
    }

    if (redirectUrl.IsEmpty())
    {
        CSP_LOG_ERROR_MSG("UserSystem::GetThirdPartyProviderAuthorizeURL() - An empty RedirectURL was passed.");

        StringResult errorResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        callback(errorResult);

        return;
    }

    ProviderDetailsResultCallback thirdPartyAuthenticationDetailsCallback = [=](const ProviderDetailsResult& providerDetailsRes)
    {
        if (providerDetailsRes.GetResultCode() == csp::systems::EResultCode::Success)
        {
            const auto providerRedirectUrl = providerDetailsRes.GetDetails().ProviderRedirectURL;
            m_thirdPartyAuthStateId = providerDetailsRes.GetDetails().ThirdPartyAuthStateId;

            m_thirdPartyRequestedAuthProvider = authProvider;
            m_thirdPartyAuthRedirectUrl = redirectUrl;

            StringResult successResult(providerDetailsRes.GetResultCode(), providerDetailsRes.GetHttpResultCode());
            successResult.SetValue(providerRedirectUrl);
            callback(successResult);
        }
        else if (providerDetailsRes.GetResultCode() != csp::systems::EResultCode::InProgress)
        {
            CSP_LOG_FORMAT(common::LogLevel::Error, "The retrieval of third party details was not successful. ResCode: %d, HttpResCode: %d",
                static_cast<int>(providerDetailsRes.GetResultCode()), providerDetailsRes.GetHttpResultCode());

            StringResult errorResult(providerDetailsRes.GetResultCode(), providerDetailsRes.GetHttpResultCode());
            errorResult.SetValue("error");
            callback(errorResult);
        }
    };

    // Reset the state related to third party authentication. This ensures that if the client calls GetThirdPartyProviderAuthorizeURL and the calls
    // fails they will not be able to call LoginToThirdPartyAuthenticationProvider() with invalid state.
    ResetThirdPartyAuthState();

    std::optional<csp::common::String> client = ThirdPartyPlatformToString(clientType);
    if (client.has_value())
    {
        m_thirdPartyClientType = client.value();
    }

    const csp::services::ResponseHandlerPtr responseHandler
        = m_authenticationApi->CreateHandler<ProviderDetailsResultCallback, ProviderDetailsResult, void, chs_user::SocialProviderInfo>(
            thirdPartyAuthenticationDetailsCallback, nullptr, csp::web::EResponseCodes::ResponseOK);

    static_cast<chs_user::AuthenticationApi*>(m_authenticationApi)
        ->social_providersProviderGet(
            { ConvertExternalAuthProvidersToString(authProvider), redirectUrl, csp::CSPFoundation::GetTenant(), client }, responseHandler);
}

void UserSystem::LoginToThirdPartyAuthenticationProvider(const csp::common::String& thirdPartyToken, const csp::common::String& thirdPartyStateId,
    bool createMultiplayerConnection, const csp::common::Optional<bool>& userHasVerifiedAge, const csp::common::Optional<TokenOptions>& tokenOptions,
    LoginStateResultCallback callback)
{
    if (thirdPartyToken.IsEmpty() || thirdPartyStateId.IsEmpty())
    {
        CSP_LOG_ERROR_MSG(
            "UserSystem::LoginToThirdPartyAuthenticationProvider() - both ThirdPartyToken and ThirdPartyStateId must be provided. Please call "
            "AssetSystem::GetThirdPartyProviderAuthorizeURL() first to get an Authorization URL, which can then be used to retrieve them.");

        csp::systems::LoginStateResult errorResult;
        errorResult.SetResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        callback(errorResult);

        return;
    }

    // Check that the stored ThirdPartyAuthStateId matches the one passed by the Client.
    // This is a security precaution suggested by the Auth Providers.
    if (m_thirdPartyAuthStateId != thirdPartyStateId)
    {
        CSP_LOG_ERROR_MSG("The provided ThirdPartyStateId is not correct"); // intentionally not too explicit about the error for security reasons

        csp::systems::LoginStateResult errorResult;
        errorResult.SetResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        callback(errorResult);
    }

    if (m_currentLoginState.State != csp::common::ELoginState::LoggedOut && m_currentLoginState.State != csp::common::ELoginState::Error)
    {
        CSP_LOG_MSG(csp::common::LogLevel::Warning, "You are not in the correct login state to proceed with third-party authentication.");

        csp::systems::LoginStateResult badResult;
        badResult.SetResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        callback(badResult);
    }

    m_currentLoginState.State = csp::common::ELoginState::LoginRequested;

    LoginStateResultCallback loginStateResCallback = [=](const LoginStateResult& loginStateRes)
    {
        if (loginStateRes.GetResultCode() == csp::systems::EResultCode::Success)
        {
            csp::multiplayer::MultiplayerConnection::ErrorCodeCallbackHandler connectionCallback
                = [callback, loginStateRes](csp::multiplayer::ErrorCode errCode)
            {
                if (errCode != csp::multiplayer::ErrorCode::None)
                {
                    CSP_LOG_ERROR_FORMAT("Error connecting MultiplayerConnection: %s", csp::multiplayer::ErrorCodeToString(errCode).c_str());
                }

                callback(loginStateRes);
            };

            StartMultiplayerConnection(*SystemsManager::Get().GetMultiplayerConnection(),
                CSPFoundation::GetEndpoints().MultiplayerConnection.GetURI(), connectionCallback, loginStateRes, *m_logSystem,
                createMultiplayerConnection);
        }
        else if (loginStateRes.GetResultCode() == csp::systems::EResultCode::Failed)
        {
            CSP_LOG_ERROR_FORMAT("Login Failed. Code: %i", loginStateRes.GetHttpResultCode());
            callback(loginStateRes);
        }
    };

    const auto request = std::make_shared<chs_user::LoginSocialRequest>();
    request->SetDeviceId(csp::CSPFoundation::GetDeviceId());
    request->SetOAuthRedirectUri(m_thirdPartyAuthRedirectUrl);
    request->SetProvider(ConvertExternalAuthProvidersToString(m_thirdPartyRequestedAuthProvider));
    request->SetClient(m_thirdPartyClientType);
    request->SetToken(thirdPartyToken);
    request->SetTenant(csp::CSPFoundation::GetTenant());

    if (userHasVerifiedAge.HasValue())
    {
        request->SetVerifiedAgeEighteen(*userHasVerifiedAge);
    }

    auto options = std::make_shared<chs_user::TokenOptions>();

    if (tokenOptions.HasValue() && CheckExpiryLengthFormat(tokenOptions->AccessTokenExpiryLength))
    {
        options->SetExpiryLength(tokenOptions->AccessTokenExpiryLength);
        m_currentLoginState.AccessTokenExpiryLength = tokenOptions->AccessTokenExpiryLength;
    }

    if (tokenOptions.HasValue() && CheckExpiryLengthFormat(tokenOptions->RefreshTokenExpiryLength))
    {
        options->SetRefreshTokenExpiryLength(tokenOptions->RefreshTokenExpiryLength);
        m_currentLoginState.RefreshTokenExpiryLength = tokenOptions->RefreshTokenExpiryLength;
    }

    request->SetTokenOptions(options);

    m_currentLoginState.State = csp::common::ELoginState::LoginRequested;

    csp::services::ResponseHandlerPtr responseHandler
        = m_authenticationApi->CreateHandler<LoginStateResultCallback, LoginStateResult, csp::common::LoginState, chs_user::AuthDto>(
            loginStateResCallback, &m_currentLoginState);

    static_cast<chs_user::AuthenticationApi*>(m_authenticationApi)->usersLogin_socialPost({ request }, responseHandler);
}

void UserSystem::Logout(NullResultCallback callback)
{
    if (m_currentLoginState.State == csp::common::ELoginState::LoggedIn)
    {
        m_currentLoginState.State = csp::common::ELoginState::LogoutRequested;

        // Disconnect MultiplayerConnection before logging out
        csp::multiplayer::MultiplayerConnection::ErrorCodeCallbackHandler errorCallback = [callback, this](csp::multiplayer::ErrorCode errCode)
        {
            if (errCode != csp::multiplayer::ErrorCode::None)
            {
                CSP_LOG_ERROR_FORMAT("Error disconnecting MultiplayerConnection: %s", csp::multiplayer::ErrorCodeToString(errCode).c_str());
            }

            auto request = std::make_shared<chs_user::LogoutRequest>();
            request->SetUserId(m_currentLoginState.UserId);
            request->SetDeviceId(m_currentLoginState.DeviceId);

            csp::services::ResponseHandlerPtr responseHandler
                = m_authenticationApi->CreateHandler<NullResultCallback, LogoutResult, csp::common::LoginState, csp::services::NullDto>(
                    callback, &m_currentLoginState, csp::web::EResponseCodes::ResponseNoContent);

            static_cast<chs_user::AuthenticationApi*>(m_authenticationApi)->usersLogoutPost({ request }, responseHandler);
        };

        auto* multiplayerConnection = SystemsManager::Get().GetMultiplayerConnection();
        multiplayerConnection->Disconnect(errorCallback);
    }
    else
    {
        csp::systems::LogoutResult badResult;
        badResult.SetResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        callback(badResult);
    }
}

void UserSystem::CreateUser(const csp::common::Optional<csp::common::String>& displayName, const csp::common::String& email,
    const csp::common::String& password, bool receiveNewsletter, bool hasVerifiedAge, const csp::common::Optional<csp::common::String>& redirectUrl,
    const csp::common::Optional<csp::common::String>& inviteToken, ProfileResultCallback callback)
{
    auto request = std::make_shared<chs_user::CreateUserRequest>();

    if (displayName.HasValue())
    {
        request->SetDisplayName(*displayName);
    }

    request->SetEmail(email);
    request->SetPassword(password);

    auto initialSettings = std::make_shared<chs_user::InitialSettingsDto>();
    initialSettings->SetContext("UserSettings");
    initialSettings->SetSettings({ { "Newsletter", receiveNewsletter ? "true" : "false" } });
    request->SetInitialSettings({ initialSettings });
    request->SetTenant(csp::CSPFoundation::GetTenant());

    request->SetVerifiedAgeEighteen(hasVerifiedAge);

    if (redirectUrl.HasValue())
    {
        request->SetRedirectUrl(redirectUrl->c_str());
    }

    if (inviteToken.HasValue())
    {
        request->SetInviteToken(*inviteToken);
    }
    const csp::services::ResponseHandlerPtr responseHandler
        = m_profileApi->CreateHandler<ProfileResultCallback, ProfileResult, void, chs_user::ProfileDto>(
            callback, nullptr, csp::web::EResponseCodes::ResponseCreated);

    static_cast<chs_user::ProfileApi*>(m_profileApi)->usersPost({ request }, responseHandler);
}

void UserSystem::UpgradeGuestAccount(
    const csp::common::String& displayName, const csp::common::String& email, const csp::common::String& password, ProfileResultCallback callback)
{
    const csp::common::String userId = m_currentLoginState.UserId;

    auto request = std::make_shared<chs_user::UpgradeGuestRequest>();

    request->SetDisplayName(displayName);
    request->SetEmail(email);
    request->SetPassword(password);
    request->SetGuestDeviceId(csp::CSPFoundation::GetDeviceId());

    const csp::services::ResponseHandlerPtr responseHandler
        = m_profileApi->CreateHandler<ProfileResultCallback, ProfileResult, void, chs_user::ProfileDto>(callback, nullptr);

    static_cast<chs_user::ProfileApi*>(m_profileApi)->usersUserIdUpgrade_guestPost({ userId, request }, responseHandler);
}

void UserSystem::ConfirmUserEmail(NullResultCallback callback)
{
    const csp::common::String userId = m_currentLoginState.UserId;

    csp::services::ResponseHandlerPtr responseHandler = m_profileApi->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(
        callback, nullptr, csp::web::EResponseCodes::ResponseNoContent);

    static_cast<chs_user::ProfileApi*>(m_profileApi)->usersUserIdConfirm_emailPost({ userId, nullptr }, responseHandler);
}

void UserSystem::ResetUserPassword(
    const csp::common::String& token, const csp::common::String& userId, const csp::common::String& newPassword, NullResultCallback callback)
{

    auto request = std::make_shared<chs_user::TokenResetPasswordRequest>();

    request->SetToken(token);
    request->SetNewPassword(newPassword);

    csp::services::ResponseHandlerPtr responseHandler = m_profileApi->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(
        callback, nullptr, csp::web::EResponseCodes::ResponseNoContent);

    static_cast<chs_user::ProfileApi*>(m_profileApi)->usersUserIdToken_change_passwordPost({ userId, request }, responseHandler);
}

void UserSystem::UpdateUserDisplayName(const csp::common::String& userId, const csp::common::String& newUserDisplayName, NullResultCallback callback)
{
    const csp::services::ResponseHandlerPtr responseHandler
        = m_profileApi->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(callback, nullptr);

    static_cast<chs_user::ProfileApi*>(m_profileApi)->usersUserIdDisplay_namePut({ userId, newUserDisplayName }, responseHandler);
}

void UserSystem::DeleteUser(const csp::common::String& userId, NullResultCallback callback)
{
    csp::services::ResponseHandlerPtr responseHandler = m_profileApi->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(
        callback, nullptr, csp::web::EResponseCodes::ResponseNoContent);

    static_cast<chs_user::ProfileApi*>(m_profileApi)->usersUserIdDelete({ userId }, responseHandler);
}

bool UserSystem::EmailCheck(const std::string& email) const { return email.find("@") != std::string::npos; }

void UserSystem::ForgotPassword(const csp::common::String& email, const csp::common::Optional<csp::common::String>& redirectUrl,
    const csp::common::Optional<csp::common::String>& emailLinkUrl, bool useTokenChangePasswordUrl, NullResultCallback callback)
{
    if (EmailCheck(email.c_str()))
    {
        auto request = std::make_shared<chs_user::ForgotPasswordRequest>();
        request->SetEmail(email);
        request->SetTenant(csp::CSPFoundation::GetTenant());

        std::optional<csp::common::String> redirectUrlValue;
        std::optional<csp::common::String> emailLinkUrlValue;

        if (redirectUrl.HasValue())
        {
            redirectUrlValue = *redirectUrl;
        }

        if (emailLinkUrl.HasValue())
        {
            emailLinkUrlValue = *emailLinkUrl;
        }

        csp::services::ResponseHandlerPtr responseHandler = m_profileApi->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(
            callback, nullptr, csp::web::EResponseCodes::ResponseNoContent);

        static_cast<chs_user::ProfileApi*>(m_profileApi)
            ->usersForgot_passwordPost({ redirectUrlValue, useTokenChangePasswordUrl, emailLinkUrlValue, request }, responseHandler);
    }
    else
    {
        callback(csp::systems::NullResult(csp::systems::EResultCode::Failed, static_cast<uint16_t>(csp::web::EResponseCodes::ResponseBadRequest)));
    }
}

void UserSystem::GetProfileByUserId(const csp::common::String& inUserId, ProfileResultCallback callback)
{
    const csp::common::String userId = inUserId;

    csp::services::ResponseHandlerPtr responseHandler
        = m_profileApi->CreateHandler<ProfileResultCallback, ProfileResult, void, chs_user::ProfileDto>(callback, nullptr);

    static_cast<chs_user::ProfileApi*>(m_profileApi)->usersUserIdGet({ userId }, responseHandler);
}

void UserSystem::GetProfilesByUserId(const csp::common::Array<csp::common::String>& inUserIds, BasicProfilesResultCallback callback)
{
    const std::vector<csp::common::String> userIds(inUserIds.Data(), inUserIds.Data() + inUserIds.Size());

    csp::services::ResponseHandlerPtr responseHandler
        = m_profileApi->CreateHandler<BasicProfilesResultCallback, BasicProfilesResult, void, csp::services::DtoArray<chs_user::ProfileLiteDto>>(
            callback, nullptr);

    static_cast<chs_user::ProfileApi*>(m_profileApi)->usersLiteGet({ userIds }, responseHandler);
}

void UserSystem::GetBasicProfilesByUserId(const csp::common::Array<csp::common::String>& inUserIds, BasicProfilesResultCallback callback)
{
    const std::vector<csp::common::String> userIds(inUserIds.Data(), inUserIds.Data() + inUserIds.Size());

    csp::services::ResponseHandlerPtr responseHandler
        = m_profileApi->CreateHandler<BasicProfilesResultCallback, BasicProfilesResult, void, csp::services::DtoArray<chs_user::ProfileLiteDto>>(
            callback, nullptr);

    static_cast<chs_user::ProfileApi*>(m_profileApi)->usersLiteGet({ userIds }, responseHandler);
}

void UserSystem::Ping(NullResultCallback callback)
{
    csp::services::ResponseHandlerPtr pingResponseHandler
        = m_pingApi->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(callback, nullptr);
    static_cast<chs_user::PingApi*>(m_pingApi)->pingGet({}, pingResponseHandler);
}

void UserSystem::ResendVerificationEmail(
    const csp::common::String& inEmail, const csp::common::Optional<csp::common::String>& inRedirectUrl, NullResultCallback callback)
{
    const csp::common::String& tenant = CSPFoundation::GetTenant();
    std::optional<csp::common::String> redirectUrl;

    if (inRedirectUrl.HasValue())
    {
        redirectUrl = *inRedirectUrl;
    }

    csp::services::ResponseHandlerPtr responseHandler
        = m_profileApi->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(callback, nullptr);

    static_cast<chs_user::ProfileApi*>(m_profileApi)->usersEmailsEmailConfirm_emailRe_sendPost({ inEmail, tenant, redirectUrl }, responseHandler);
}

void UserSystem::GetCustomerPortalUrl(const csp::common::String& userId, StringResultCallback callback)
{
    csp::services::ResponseHandlerPtr responseHandler
        = m_stripeApi->CreateHandler<StringResultCallback, CustomerPortalUrlResult, void, chs_user::StripeCustomerPortalDto>(callback, nullptr);

    static_cast<chs_user::StripeApi*>(m_stripeApi)->vendorsStripeCustomer_portalsUserIdGet({ userId }, responseHandler);
};

void UserSystem::GetCheckoutSessionUrl(TierNames tier, StringResultCallback callback)
{
    auto checkoutSessionInfo = std::make_shared<chs_user::StripeCheckoutRequest>();

    checkoutSessionInfo->SetLookupKey(TierNameEnumToString(tier));

    csp::services::ResponseHandlerPtr responseHandler
        = m_stripeApi->CreateHandler<StringResultCallback, CheckoutSessionUrlResult, void, chs_user::StripeCheckoutSessionDto>(callback, nullptr);

    static_cast<chs_user::StripeApi*>(m_stripeApi)->vendorsStripeCheckout_sessionsPost({ checkoutSessionInfo }, responseHandler);
};

void UserSystem::NotifyRefreshTokenHasChanged()
{
    if (m_refreshTokenChangedCallback)
    {
        LoginTokenInfoResult internalResult;
        internalResult.FillLoginTokenInfo(csp::web::HttpAuth::GetAccessToken(), csp::web::HttpAuth::GetTokenExpiry(),
            csp::web::HttpAuth::GetRefreshToken(), csp::web::HttpAuth::GetRefreshTokenExpiry());

        m_refreshTokenChangedCallback(internalResult);
    }
}

void UserSystem::SetUserPermissionsChangedCallback(UserPermissionsChangedCallbackHandler callback)
{
    m_userPermissionsChangedCallback = callback;
    RegisterSystemCallback();
}

void UserSystem::RegisterSystemCallback()
{
    if (!m_eventBusPtr)
    {
        CSP_LOG_ERROR_MSG("Error: Failed to register UserSystem. NetworkEventBus must be instantiated in the MultiplayerConnection first.");
        return;
    }

    if (!m_userPermissionsChangedCallback)
    {
        return;
    }

    m_eventBusPtr->ListenNetworkEvent(
        csp::multiplayer::NetworkEventRegistration("CSPInternal::UserSystem",
            csp::multiplayer::NetworkEventBus::StringFromNetworkEvent(csp::multiplayer::NetworkEventBus::NetworkEvent::AccessControlChanged)),
        [this](const csp::common::NetworkEventData& networkEventData) { this->OnAccessControlChangedEvent(networkEventData); });
}

void UserSystem::OnAccessControlChangedEvent(const csp::common::NetworkEventData& networkEventData)
{
    if (!m_userPermissionsChangedCallback)
    {
        return;
    }

    const csp::common::AccessControlChangedNetworkEventData& accessControlChangedNetworkEventData
        = static_cast<const csp::common::AccessControlChangedNetworkEventData&>(networkEventData);

    m_userPermissionsChangedCallback(accessControlChangedNetworkEventData);
}

csp::common::IAuthContext& UserSystem::GetAuthContext() { return m_auth; }
} // namespace csp::systems
