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
#include "CSP/Common/StringFormat.h"
#include "CSP/Multiplayer/EventParameters.h"
#include "CSP/Systems/Users/Authentication.h"
#include "CSP/Systems/Users/Profile.h"
#include "Common/UUIDGenerator.h"
#include "Multiplayer/ErrorCodeStrings.h"
#include "Multiplayer/EventSerialisation.h"
#include "Services/AggregationService/Api.h"
#include "Services/UserService/Api.h"
#include "Systems/Users/Authentication.h"

#include <CallHelpers.h>

namespace chs_user = csp::services::generated::userservice;
namespace chs_aggregation = csp::services::generated::aggregationservice;

namespace
{

inline const char* BoolToApiString(bool Val) { return Val ? "true" : "false"; }

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
    default:
    {
        CSP_LOG_FORMAT(LogLevel::Error, "Unsupported Provider Type requested: %d, returning Google", static_cast<uint8_t>(Provider));
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

UserSystem::UserSystem()
    : SystemBase(nullptr, nullptr)
    , AuthenticationAPI(nullptr)
    , ProfileAPI(nullptr)
    , PingAPI(nullptr)
    , ExternalServiceProxyApi(nullptr)
{
}

UserSystem::UserSystem(csp::web::WebClient* InWebClient, csp::multiplayer::EventBus* InEventBus)
    : SystemBase(InWebClient, InEventBus)
    , RefreshTokenChangedCallback(nullptr)
{
    AuthenticationAPI = CSP_NEW chs_user::AuthenticationApi(InWebClient);
    ProfileAPI = CSP_NEW chs_user::ProfileApi(InWebClient);
    PingAPI = CSP_NEW chs_user::PingApi(InWebClient);
    ExternalServiceProxyApi = CSP_NEW chs_aggregation::ExternalServiceProxyApi(InWebClient);
    StripeAPI = CSP_NEW chs_user::StripeApi(InWebClient);

    RegisterSystemCallback();
}

UserSystem::~UserSystem()
{
    CSP_DELETE(PingAPI);
    CSP_DELETE(ProfileAPI);
    CSP_DELETE(AuthenticationAPI);
    CSP_DELETE(ExternalServiceProxyApi);
    CSP_DELETE(StripeAPI);

    DeregisterSystemCallback();
}

const LoginState& UserSystem::GetLoginState() const { return CurrentLoginState; }

void UserSystem::SetNewLoginTokenReceivedCallback(LoginTokenInfoResultCallback Callback) { RefreshTokenChangedCallback = Callback; }

void UserSystem::Login(const csp::common::String& UserName, const csp::common::String& Email, const csp::common::String& Password,
    const csp::common::Optional<bool>& UserHasVerifiedAge, LoginStateResultCallback Callback)
{
    if (CurrentLoginState.State == ELoginState::LoggedOut || CurrentLoginState.State == ELoginState::Error)
    {
        CurrentLoginState.State = ELoginState::LoginRequested;

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

        LoginStateResultCallback LoginStateResCallback = [=](const LoginStateResult& LoginStateRes)
        {
            if (LoginStateRes.GetResultCode() == csp::systems::EResultCode::Success)
            {
                NotifyRefreshTokenHasChanged();

                csp::multiplayer::MultiplayerConnection::ErrorCodeCallbackHandler ErrorCallback
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

                auto* MultiplayerConnection = SystemsManager::Get().GetMultiplayerConnection();
                MultiplayerConnection->Connect(ErrorCallback);
            }
            else if (LoginStateRes.GetResultCode() == csp::systems::EResultCode::Failed)
            {
                CSP_LOG_ERROR_FORMAT("Login Failed. Code: %i", LoginStateRes.GetHttpResultCode());
                Callback(LoginStateRes);
            }
        };

        csp::services::ResponseHandlerPtr ResponseHandler
            = AuthenticationAPI->CreateHandler<LoginStateResultCallback, LoginStateResult, LoginState, chs_user::AuthDto>(
                LoginStateResCallback, &CurrentLoginState);

        static_cast<chs_user::AuthenticationApi*>(AuthenticationAPI)->apiV1UsersLoginPost(Request, ResponseHandler);
    }
    else
    {
        csp::systems::LoginStateResult BadResult;
        BadResult.SetResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        Callback(BadResult);
    }
}

void UserSystem::LoginWithRefreshToken(const csp::common::String& UserId, const csp::common::String& RefreshToken, LoginStateResultCallback Callback)
{
    if (CurrentLoginState.State == ELoginState::LoggedOut || CurrentLoginState.State == ELoginState::Error)
    {
        CurrentLoginState.State = ELoginState::LoginRequested;

        auto Request = std::make_shared<chs_user::RefreshRequest>();
        Request->SetDeviceId(csp::CSPFoundation::GetDeviceId());
        Request->SetUserId(UserId);
        Request->SetRefreshToken(RefreshToken);

        LoginStateResultCallback LoginStateResCallback = [=](const LoginStateResult& LoginStateRes)
        {
            if (LoginStateRes.GetResultCode() == csp::systems::EResultCode::Success)
            {
                csp::multiplayer::MultiplayerConnection::ErrorCodeCallbackHandler ErrorCallback
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

                auto* MultiplayerConnection = SystemsManager::Get().GetMultiplayerConnection();
                MultiplayerConnection->Connect(ErrorCallback);
            }
            else
            {
                Callback(LoginStateRes);
            }
        };

        csp::services::ResponseHandlerPtr ResponseHandler
            = AuthenticationAPI->CreateHandler<LoginStateResultCallback, LoginStateResult, LoginState, chs_user::AuthDto>(
                LoginStateResCallback, &CurrentLoginState);

        static_cast<chs_user::AuthenticationApi*>(AuthenticationAPI)->apiV1UsersRefreshPost(Request, ResponseHandler);
    }
    else
    {
        csp::systems::LoginStateResult BadResult;
        BadResult.SetResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        BadResult.ResponseBody = "Already logged in!";
        Callback(BadResult);
    }
}

void UserSystem::RefreshSession(const csp::common::String& UserId, const csp::common::String& RefreshToken, NullResultCallback Callback)
{
    if (CurrentLoginState.State == ELoginState::LoggedIn)
    {
        auto Request = std::make_shared<chs_user::RefreshRequest>();
        Request->SetDeviceId(csp::CSPFoundation::GetDeviceId());
        Request->SetUserId(UserId);
        Request->SetRefreshToken(RefreshToken);

        LoginStateResultCallback LoginStateResCallback = [=](const LoginStateResult& LoginStateRes)
        {
            if (LoginStateRes.GetResultCode() == csp::systems::EResultCode::Success)
            {
                const NullResult Result(csp::systems::EResultCode::Success, 200);
                INVOKE_IF_NOT_NULL(Callback, Result);
            }
            else
            {
                const NullResult Result(LoginStateRes.GetResultCode(), LoginStateRes.GetHttpResultCode());
                INVOKE_IF_NOT_NULL(Callback, Result);
            }
        };

        csp::services::ResponseHandlerPtr ResponseHandler
            = AuthenticationAPI->CreateHandler<LoginStateResultCallback, LoginStateResult, LoginState, chs_user::AuthDto>(
                LoginStateResCallback, &CurrentLoginState);

        static_cast<chs_user::AuthenticationApi*>(AuthenticationAPI)->apiV1UsersRefreshPost(Request, ResponseHandler);
    }
}

void UserSystem::LoginAsGuest(const csp::common::Optional<bool>& UserHasVerifiedAge, LoginStateResultCallback Callback)
{
    if (CurrentLoginState.State == ELoginState::LoggedOut || CurrentLoginState.State == ELoginState::Error)
    {
        CurrentLoginState.State = ELoginState::LoginRequested;

        auto Request = std::make_shared<chs_user::LoginRequest>();
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
                csp::multiplayer::MultiplayerConnection::ErrorCodeCallbackHandler ErrorCallback
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

                auto* MultiplayerConnection = SystemsManager::Get().GetMultiplayerConnection();
                MultiplayerConnection->Connect(ErrorCallback);
            }
            else
            {
                Callback(LoginStateRes);
            }
        };

        csp::services::ResponseHandlerPtr ResponseHandler
            = AuthenticationAPI->CreateHandler<LoginStateResultCallback, LoginStateResult, LoginState, chs_user::AuthDto>(
                LoginStateResCallback, &CurrentLoginState);

        static_cast<chs_user::AuthenticationApi*>(AuthenticationAPI)->apiV1UsersLoginPost(Request, ResponseHandler);
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
    EThirdPartyAuthenticationProviders AuthProvider, const csp::common::String& RedirectURL, StringResultCallback Callback)
{
    ResetAuthenticationState();

    // Get provider_base_url and client_id
    ProviderDetailsResultCallback ThirdPartyAuthenticationDetailsCallback = [=](const ProviderDetailsResult& ProviderDetailsRes)
    {
        if (ProviderDetailsRes.GetResultCode() == csp::systems::EResultCode::Success)
        {
            const auto AuthoriseUrl = ProviderDetailsRes.GetDetails().AuthoriseURL;
            const auto ProviderClientId = ProviderDetailsRes.GetDetails().ProviderClientId;
            ThirdPartyAuthStateId = csp::GenerateUUID().c_str();
            ThirdPartyRequestedAuthProvider = AuthProvider;
            ThirdPartyAuthRedirectURL = RedirectURL;
            const auto AuthProviderFormattedScopes = FormatScopesForURL(ProviderDetailsRes.GetDetails().ProviderAuthScopes);

            auto AuthoriseURL = csp::common::StringFormat(
                "%s?client_id=%s&scope=%s&state=%s&response_type=code&redirect_uri=%s&prompt=none&response_mode=form_post", AuthoriseUrl.c_str(),
                ProviderClientId.c_str(), AuthProviderFormattedScopes.c_str(), ThirdPartyAuthStateId.c_str(), RedirectURL.c_str());

            StringResult SuccessResult(ProviderDetailsRes.GetResultCode(), ProviderDetailsRes.GetHttpResultCode());
            SuccessResult.SetValue(AuthoriseURL);
            Callback(SuccessResult);
        }
        else if (ProviderDetailsRes.GetResultCode() != csp::systems::EResultCode::InProgress)
        {
            CSP_LOG_FORMAT(LogLevel::Error, "The retrieval of third party details was not successful. ResCode: %d, HttpResCode: %d",
                static_cast<int>(ProviderDetailsRes.GetResultCode()), ProviderDetailsRes.GetHttpResultCode());

            CurrentLoginState.State = ELoginState::Error;

            StringResult ErrorResult(ProviderDetailsRes.GetResultCode(), ProviderDetailsRes.GetHttpResultCode());
            ErrorResult.SetValue("error");
            Callback(ErrorResult);
        }
    };

    const csp::services::ResponseHandlerPtr ResponseHandler
        = AuthenticationAPI->CreateHandler<ProviderDetailsResultCallback, ProviderDetailsResult, void, chs_user::SocialProviderInfo>(
            ThirdPartyAuthenticationDetailsCallback, nullptr, csp::web::EResponseCodes::ResponseOK);

    CurrentLoginState.State = ELoginState::LoginThirdPartyProviderDetailsRequested;

    static_cast<chs_user::AuthenticationApi*>(AuthenticationAPI)
        ->apiV1SocialProvidersProviderGet(ConvertExternalAuthProvidersToString(AuthProvider), csp::CSPFoundation::GetTenant(), ResponseHandler);
}

void UserSystem::LoginToThirdPartyAuthenticationProvider(const csp::common::String& ThirdPartyToken, const csp::common::String& ThirdPartyStateId,
    const csp::common::Optional<bool>& UserHasVerifiedAge, LoginStateResultCallback Callback)
{
    if (CurrentLoginState.State != ELoginState::LoginThirdPartyProviderDetailsRequested)
    {
        CSP_LOG_FORMAT(
            LogLevel::Error, "The LoginState: %d is incorrect for proceeding with the third party authentication login", CurrentLoginState.State);
        CurrentLoginState.State = ELoginState::Error;

        csp::systems::LoginStateResult ErrorResult;
        ErrorResult.SetResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseForbidden);
        Callback(ErrorResult);
    }

    // checking that the stored ThirdPartyAuthStateId matches the one passed by the Client as a security safety net suggested by the Auth Providers
    if (ThirdPartyAuthStateId != ThirdPartyStateId)
    {
        CSP_LOG_MSG(LogLevel::Error, "The state ID is not correct"); // intentionally not to explicit about the error for security reasons
        CurrentLoginState.State = ELoginState::Error;

        csp::systems::LoginStateResult ErrorResult;
        ErrorResult.SetResult(csp::systems::EResultCode::Failed, (uint16_t)csp::web::EResponseCodes::ResponseBadRequest);
        Callback(ErrorResult);
    }

    LoginStateResultCallback LoginStateResCallback = [=](const LoginStateResult& LoginStateRes)
    {
        if (LoginStateRes.GetResultCode() == csp::systems::EResultCode::Success)
        {
            NotifyRefreshTokenHasChanged();

            csp::multiplayer::MultiplayerConnection::ErrorCodeCallbackHandler ErrorCallback
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

            auto* MultiplayerConnection = SystemsManager::Get().GetMultiplayerConnection();
            MultiplayerConnection->Connect(ErrorCallback);
        }
        else
        {
            Callback(LoginStateRes);
        }
    };

    const auto Request = std::make_shared<chs_user::LoginSocialRequest>();
    Request->SetDeviceId(csp::CSPFoundation::GetDeviceId());
    Request->SetOAuthRedirectUri(ThirdPartyAuthRedirectURL);
    Request->SetProvider(ConvertExternalAuthProvidersToString(ThirdPartyRequestedAuthProvider));
    Request->SetToken(ThirdPartyToken);
    Request->SetTenant(csp::CSPFoundation::GetTenant());

    if (UserHasVerifiedAge.HasValue())
    {
        Request->SetVerifiedAgeEighteen(*UserHasVerifiedAge);
    }

    CurrentLoginState.State = ELoginState::LoginRequested;

    csp::services::ResponseHandlerPtr ResponseHandler
        = AuthenticationAPI->CreateHandler<LoginStateResultCallback, LoginStateResult, LoginState, chs_user::AuthDto>(
            LoginStateResCallback, &CurrentLoginState);

    static_cast<chs_user::AuthenticationApi*>(AuthenticationAPI)->apiV1UsersLoginSocialPost(Request, ResponseHandler);
}

void UserSystem::Logout(NullResultCallback Callback)
{
    if (CurrentLoginState.State == ELoginState::LoggedIn)
    {
        CurrentLoginState.State = ELoginState::LogoutRequested;

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
                = AuthenticationAPI->CreateHandler<NullResultCallback, LogoutResult, LoginState, csp::services::NullDto>(
                    Callback, &CurrentLoginState, csp::web::EResponseCodes::ResponseNoContent);

            static_cast<chs_user::AuthenticationApi*>(AuthenticationAPI)->apiV1UsersLogoutPost(Request, ResponseHandler);
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

    static_cast<chs_user::ProfileApi*>(ProfileAPI)->apiV1UsersPost(Request, ResponseHandler);
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

    static_cast<chs_user::ProfileApi*>(ProfileAPI)->apiV1UsersUserIdUpgradeGuestPost(UserId, Request, ResponseHandler);
}

void UserSystem::ConfirmUserEmail(NullResultCallback Callback)
{
    const csp::common::String UserId = CurrentLoginState.UserId;

    csp::services::ResponseHandlerPtr ResponseHandler = ProfileAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(
        Callback, nullptr, csp::web::EResponseCodes::ResponseNoContent);

    static_cast<chs_user::ProfileApi*>(ProfileAPI)->apiV1UsersUserIdConfirmEmailPost(UserId, nullptr, ResponseHandler);
}

void UserSystem::ResetUserPassword(
    const csp::common::String& Token, const csp::common::String& UserId, const csp::common::String& NewPassword, NullResultCallback Callback)
{

    auto Request = std::make_shared<chs_user::TokenResetPasswordRequest>();

    Request->SetToken(Token);
    Request->SetNewPassword(NewPassword);

    csp::services::ResponseHandlerPtr ResponseHandler = ProfileAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(
        Callback, nullptr, csp::web::EResponseCodes::ResponseNoContent);

    static_cast<chs_user::ProfileApi*>(ProfileAPI)->apiV1UsersUserIdTokenChangePasswordPost(UserId, Request, ResponseHandler);
}

void UserSystem::UpdateUserDisplayName(const csp::common::String& UserId, const csp::common::String& NewUserDisplayName, NullResultCallback Callback)
{
    const csp::services::ResponseHandlerPtr ResponseHandler
        = ProfileAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(Callback, nullptr);

    static_cast<chs_user::ProfileApi*>(ProfileAPI)->apiV1UsersUserIdDisplayNamePut(UserId, NewUserDisplayName, ResponseHandler);
}

void UserSystem::DeleteUser(const csp::common::String& UserId, NullResultCallback Callback)
{
    csp::services::ResponseHandlerPtr ResponseHandler = ProfileAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(
        Callback, nullptr, csp::web::EResponseCodes::ResponseNoContent);

    static_cast<chs_user::ProfileApi*>(ProfileAPI)->apiV1UsersUserIdDelete(UserId, ResponseHandler);
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
            ->apiV1UsersForgotPasswordPost(RedirectUrlValue, UseTokenChangePasswordUrl, EmailLinkUrlValue, Request, ResponseHandler);
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

    static_cast<chs_user::ProfileApi*>(ProfileAPI)->apiV1UsersUserIdGet(UserId, ResponseHandler);
}

void UserSystem::GetProfilesByUserId(const csp::common::Array<csp::common::String>& InUserIds, BasicProfilesResultCallback Callback)
{
    const std::vector<csp::common::String> UserIds(InUserIds.Data(), InUserIds.Data() + InUserIds.Size());

    csp::services::ResponseHandlerPtr ResponseHandler
        = ProfileAPI->CreateHandler<BasicProfilesResultCallback, BasicProfilesResult, void, csp::services::DtoArray<chs_user::ProfileLiteDto>>(
            Callback, nullptr);

    static_cast<chs_user::ProfileApi*>(ProfileAPI)->apiV1UsersLiteGet(UserIds, ResponseHandler);
}

void UserSystem::GetBasicProfilesByUserId(const csp::common::Array<csp::common::String>& InUserIds, BasicProfilesResultCallback Callback)
{
    const std::vector<csp::common::String> UserIds(InUserIds.Data(), InUserIds.Data() + InUserIds.Size());

    csp::services::ResponseHandlerPtr ResponseHandler
        = ProfileAPI->CreateHandler<BasicProfilesResultCallback, BasicProfilesResult, void, csp::services::DtoArray<chs_user::ProfileLiteDto>>(
            Callback, nullptr);

    static_cast<chs_user::ProfileApi*>(ProfileAPI)->apiV1UsersLiteGet(UserIds, ResponseHandler);
}

void UserSystem::Ping(NullResultCallback Callback)
{
    csp::services::ResponseHandlerPtr PingResponseHandler
        = PingAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(Callback, nullptr);
    static_cast<chs_user::PingApi*>(PingAPI)->pingGet(PingResponseHandler);
}

void UserSystem::GetAgoraUserToken(const AgoraUserTokenParams& Params, StringResultCallback Callback)
{
    auto TokenInfo = std::make_shared<chs_aggregation::ServiceRequest>();
    TokenInfo->SetServiceName("Agora");
    TokenInfo->SetOperationName("getUserToken");
    TokenInfo->SetHelp(false);

    std::map<csp::common::String, csp::common::String> Parameters;
    Parameters["userId"] = Params.AgoraUserId;
    Parameters["channelName"] = Params.ChannelName;
    Parameters["referenceId"] = Params.ReferenceId;
    Parameters["lifespan"] = std::to_string(Params.Lifespan).c_str();
    Parameters["readOnly"] = BoolToApiString(Params.ReadOnly);
    Parameters["shareAudio"] = BoolToApiString(Params.ShareAudio);
    Parameters["shareVideo"] = BoolToApiString(Params.ShareVideo);
    Parameters["shareScreen"] = BoolToApiString(Params.ShareScreen);

    TokenInfo->SetParameters(Parameters);

    csp::services::ResponseHandlerPtr ResponseHandler
        = ExternalServiceProxyApi->CreateHandler<StringResultCallback, AgoraUserTokenResult, void, chs_aggregation::ServiceResponse>(
            Callback, nullptr);
    static_cast<chs_aggregation::ExternalServiceProxyApi*>(ExternalServiceProxyApi)->serviceProxyPost(TokenInfo, ResponseHandler);
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

    static_cast<chs_user::ProfileApi*>(ProfileAPI)->apiV1UsersEmailsEmailConfirmEmailReSendPost(InEmail, Tenant, RedirectUrl, ResponseHandler);
}

void UserSystem::GetCustomerPortalUrl(const csp::common::String& UserId, StringResultCallback Callback)
{
    csp::services::ResponseHandlerPtr ResponseHandler
        = StripeAPI->CreateHandler<StringResultCallback, CustomerPortalUrlResult, void, chs_user::StripeCustomerPortalDto>(Callback, nullptr);

    static_cast<chs_user::StripeApi*>(StripeAPI)->apiV1VendorsStripeCustomerPortalsUserIdGet(UserId, ResponseHandler);
};

void UserSystem::GetCheckoutSessionUrl(TierNames Tier, StringResultCallback Callback)
{
    auto CheckoutSessionInfo = std::make_shared<chs_user::StripeCheckoutRequest>();

    CheckoutSessionInfo->SetLookupKey(TierNameEnumToString(Tier));

    csp::services::ResponseHandlerPtr ResponseHandler
        = StripeAPI->CreateHandler<StringResultCallback, CheckoutSessionUrlResult, void, chs_user::StripeCheckoutSessionDto>(Callback, nullptr);

    static_cast<chs_user::StripeApi*>(StripeAPI)->apiV1VendorsStripeCheckoutSessionsPost(CheckoutSessionInfo, ResponseHandler);
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

void UserSystem::ResetAuthenticationState()
{
    CurrentLoginState.State = ELoginState::LoggedOut;
    ThirdPartyAuthStateId = "";
    ThirdPartyRequestedAuthProvider = EThirdPartyAuthenticationProviders::Invalid;
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
        CSP_LOG_ERROR_MSG("Error: Failed to register UserSystem. EventBus must be instantiated in the MultiplayerConnection first.");
        return;
    }

    if (!UserPermissionsChangedCallback)
    {
        return;
    }

    EventBusPtr->ListenNetworkEvent("AccessControlChanged", this);
}

void UserSystem::DeregisterSystemCallback()
{
    if (EventBusPtr)
    {
        EventBusPtr->StopListenNetworkEvent("AccessControlChanged");
    }
}

void UserSystem::OnEvent(const std::vector<signalr::value>& EventValues)
{
    if (!UserPermissionsChangedCallback)
    {
        return;
    }

    csp::multiplayer::UserPermissionsChangedEventDeserialiser Deserialiser;
    Deserialiser.Parse(EventValues);
    UserPermissionsChangedCallback(Deserialiser.GetEventParams());
}

} // namespace csp::systems
