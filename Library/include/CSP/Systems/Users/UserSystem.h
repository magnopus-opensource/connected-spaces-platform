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

#include "CSP/CSPCommon.h"
#include "CSP/Common/Array.h"
#include "CSP/Common/Interfaces/IAuthContext.h"
#include "CSP/Common/LoginState.h"
#include "CSP/Common/NetworkEventData.h"
#include "CSP/Common/Optional.h"
#include "CSP/Common/String.h"
#include "CSP/Systems/Quota/Quota.h"
#include "CSP/Systems/SystemBase.h"
#include "CSP/Systems/Users/Authentication.h"
#include "CSP/Systems/Users/Profile.h"
#include "CSP/Systems/Users/ThirdPartyAuthentication.h"

namespace csp::web
{

class WebClient;

} // namespace csp::web

namespace csp::systems
{
class UserSystem;
class TokenOptions;

// This class exists purely to appease the wrapper generator.
// IAuthContext was previously implemented by the UserSystem.
// However, due to limitations with function pointers as parameters, we needed to hide the interface implementation.
// Now, AuthContext, uses the functionality from the UserSystem to act as the AuthContext.
CSP_START_IGNORE
class AuthContext : public csp::common::IAuthContext
{
public:
    AuthContext(csp::services::ApiBase* AuthenticationAPI, csp::common::LoginState& LoginState);

    const csp::common::LoginState& GetLoginState() const override;

    /// @brief Refreshes the sessions RefreshToken.
    /// This is currently used internally by the web client.
    void RefreshToken(std::function<void(bool)> Callback) override;

private:
    csp::services::ApiBase* AuthenticationAPI;
    csp::common::LoginState* LoginState;
};
CSP_END_IGNORE

/// @ingroup User System
/// @brief Public facing system that allows interfacing with Magnopus Cloud Services' user service.
/// Offers methods for creating accounts, authenticating, and retrieving user profiles.
class CSP_API UserSystem : public SystemBase
{
    CSP_START_IGNORE
    /** @cond DO_NOT_DOCUMENT */
    friend class SystemsManager;
    friend class LoginStateResult;
    /** @endcond */
    CSP_END_IGNORE

public:
    // Authentication
    /// @brief Get the current login state.
    /// @return LoginState : Current login state
    const csp::common::LoginState& GetLoginState() const;

    /// @brief Sets a callback that will get fired when the login token has changed as a result of logging in with credentials or with a token or
    /// after the Connected Spaces Platform internal system has refreshed the session.
    /// In the callback result the token and it's expiration time will be provided.
    /// The expiration time is in OSI format {Year}-{Month}-{Date}T{Hour}:{Min}:{Sec}
    /// For C#: register a callback to the OnNewLoginTokenReceived event
    /// @param Callback LoginTokenInfoResultCallback : callback that gets called as described above
    CSP_EVENT void SetNewLoginTokenReceivedCallback(LoginTokenInfoResultCallback Callback);

    /// @brief Log in to Magnopus Cloud Services services using a username-password or email-password combination.
    /// @param UserName csp::common::String
    /// @param Email csp::common::String
    /// @param Password csp::common::String
    /// @param CreateMultiplayerConnection bool : Whether to create a multiplayer connection. If false, this session will not establish a SignalR
    /// connection to backend services, and thus be unable to receive messages or events. This session will also be unable to enter online spaces via
    /// a csp::multiplayer::OnlineRealtimeEngine. If true, this session will receive events, and may enter both online and offline spaces.
    /// @param UserHasVerifiedAge csp::common::Optional<bool> : An optional bool to specify whether or not the user has verified that they are over 18
    /// @param TokenOptions csp::common::Optional<TokenOptions> : Optional override for default token options.
    /// The default token expiry length is configured by MCS and defaults to 30 minutes. Value must be less than the default expiry length, or it will
    /// be ignored.
    /// @param Callback LoginStateResultCallback : callback to call when a response is received
    /// @pre One of either UserName or Email must not be empty.
    /// @pre Password must not be empty.
    CSP_ASYNC_RESULT void Login(const csp::common::String& UserName, const csp::common::String& Email, const csp::common::String& Password,
        bool CreateMultiplayerConnection, const csp::common::Optional<bool>& UserHasVerifiedAge,
        const csp::common::Optional<TokenOptions>& TokenOptions, LoginStateResultCallback Callback);

    /// @brief Resume a previous session for the associated user ID using a refresh token
    /// The refresh token can be obtained after registering a callback with `SetNewLoginTokenReceivedCallback` and logging in regularly.
    /// @param UserId csp::common::String : User ID for the previous session
    /// @param RefreshToken csp::common::String : Refresh token to be used for refreshing the authentication token
    /// @param CreateMultiplayerConnection bool : Whether to create a multiplayer connection. If false, this session will not establish a SignalR
    /// connection to backend services, and thus be unable to receive messages or events. This session will also be unable to enter online spaces via
    /// a csp::multiplayer::OnlineRealtimeEngine. If true, this session will receive events, and may enter both online and offline spaces.
    /// @param TokenOptions csp::common::Optional<TokenOptions> : Optional override for default token options.
    /// The default token expiry length is configured by MCS and defaults to 30 minutes. Value must be less than the default expiry length, or it will
    /// be ignored.
    /// @param Callback LoginStateResultCallback : Callback when asynchronous task finishes
    /// @pre UserId must not be empty.
    CSP_ASYNC_RESULT void LoginWithRefreshToken(const csp::common::String& UserId, const csp::common::String& RefreshToken,
        bool CreateMultiplayerConnection, const csp::common::Optional<TokenOptions>& TokenOptions, LoginStateResultCallback Callback);

    /// @brief Log in to Magnopus Cloud Services as a guest.
    /// @param CreateMultiplayerConnection bool : Whether to create a multiplayer connection. If false, this session will not establish a SignalR
    /// connection to backend services, and thus be unable to receive messages or events. This session will also be unable to enter online spaces via
    /// a csp::multiplayer::OnlineRealtimeEngine. If true, this session will receive events, and may enter both online and offline spaces.
    /// @param UserHasVerifiedAge csp::common::Optional<bool> : An optional bool to specify whether or not the user has verified that they are over 18
    /// @param TokenOptions csp::common::Optional<TokenOptions> : Optional override for default token options.
    /// The default token expiry length is configured by MCS and defaults to 30 minutes. Value must be less than the default expiry length, or it will
    /// be ignored.
    /// @param Callback LoginStateResultCallback : callback to call when a response is received
    CSP_ASYNC_RESULT void LoginAsGuest(bool CreateMultiplayerConnection, const csp::common::Optional<bool>& UserHasVerifiedAge,
        const csp::common::Optional<TokenOptions>& TokenOptions, LoginStateResultCallback Callback);

    /// @brief Log in to Magnopus Cloud Services as a guest, allowing the backend to defer profile creation and perform other optimizations.
    /// This login method is intended only for use with offline realtime engines, and as such does not start a multiplayer connection.
    /// @warning Unless you have a good reason, you should prefer the regular LoginAsGuest function. This method is designed for specific
    /// non-multiplayer cases where the backend services are expecting a huge amount of anonymous profiles and wish to be allowed to buffer profile
    /// creation. For this reason, there is an undefined delay after calling this function until the session can be thought to be conceptually "logged
    /// in". Beware if you are calling user system methods after having logged in using this method. If you find yourself doing that, you may wish to
    /// use the regular LoginAsGuest method instead.
    /// @param UserHasVerifiedAge csp::common::Optional<bool> : An optional bool to specify whether or not the user has verified that they are over 18
    /// @param Callback LoginStateResultCallback : callback to call when a response is received
    CSP_ASYNC_RESULT void LoginAsGuestWithDeferredProfileCreation(
        const csp::common::Optional<bool>& UserHasVerifiedAge, LoginStateResultCallback Callback);

    /// @ingroup Third Party Authentication
    /// @brief As a Connected Spaces Platform user the 3rd party authentication flow consists of two steps, first calling
    /// GetThirdPartyProviderAuthoriseURL followed by LoginToThirdPartyAuthenticationProvider You can see a Sequence Diagram with all the parties
    /// involved including what a Client should be calling and when here https://miro.com/app/board/uXjVPflpu98=/.

    /// @brief API to retrieve the Connected Spaces Platform supported 3rd party authentication providers
    /// @return Array of Connected Spaces Platform supported 3rd party authentication providers
    [[nodiscard]] csp::common::Array<EThirdPartyAuthenticationProviders> GetSupportedThirdPartyAuthenticationProviders() const;

    /// @brief First step of the 3rd party authentication flow
    /// Note: The steps are as follows:
    /// 1. This step. Call this method to retrieve a ThirdPartyAuthDetails object. This will contain the Authorise URL required for step 2.
    /// 2. The caller should navigate to the Authorise URL retrieved in step 1, where they will authenticate with the 3rd party provider and retrieve
    /// a token and state Id from the provider.
    /// 3. Call @ref UserSystem::LoginToThirdPartyAuthenticationProvider() with the token and state Id retrieved in step 2, as well as the
    /// ThirdPartyAuthDetails object returned in step 1.
    ///
    /// @param AuthProvider EThirdPartyAuthenticationProviders : The Authentication Provider you want to be authenticated with.
    /// @param RedirectURL csp::common::String : the RedirectURL you want to be used for this authentication flow.
    /// @param Callback ThirdPartyAuthDetailsResultCallback : callback that contains the following 3rd party authentication details.
    ///
    /// ThirdPartyAuthDetails will contain the following data:
    /// - ThirdPartyAuthStateId String : the state Id that will be required for step 3.
    /// - ThirdPartyRequestedAuthProvider EThirdPartyAuthenticationProviders : the Authentication Provider that will be required for step 3.
    /// - ThirdPartyAuthRedirectURL String : the Redirect URL that will be required for step 3.
    /// - ThirdPartyAuthURL String : the Authorise URL that the Client should be navigating to for step 2.
    CSP_ASYNC_RESULT void GetThirdPartyProviderAuthoriseURL(
        EThirdPartyAuthenticationProviders AuthProvider, const csp::common::String& RedirectURL, ThirdPartyAuthDetailsResultCallback Callback);

    /// @brief Second step of the 3rd party authentication flow
    /// Note: The steps are as follows:
    /// 1. Call @ref UserSystem::GetThirdPartyProviderAuthoriseURL() to retrieve a ThirdPartyAuthDetails object. This will contain the Authorise URL
    /// required for step 2.
    /// 2. The caller should navigate to the Authorise URL retrieved in step 1, where they will authenticate with the 3rd party provider and retrieve
    /// a token and state Id from the provider.
    /// 3. This step. Call this method with the token and state Id retrieved in step 2, as well as the ThirdPartyAuthDetails object
    /// returned in step 1.
    ///
    /// @param ThirdPartyToken csp::common::String : The authentication token returned by the Provider in step 2.
    /// @param ThirdPartyStateId csp::common::String : The state Id returned by the Provider in step 2.
    /// @param ThirdPartyAuthDetails ThirdPartyAuthDetails : The ThirdPartyAuthDetails object returned by @ref
    /// UserSystem::GetThirdPartyProviderAuthoriseURL() in step 1.
    /// @param CreateMultiplayerConnection bool : Whether to create a multiplayer connection. If false, this session will not establish a SignalR
    /// connection to backend services, and thus be unable to receive messages or events. This session will also be unable to enter online spaces via
    /// a csp::multiplayer::OnlineRealtimeEngine. If true, this session will receive events, and may enter both online and offline spaces.
    /// @param UserHasVerifiedAge csp::common::Optional<bool> : An optional bool to specify whether or not the user has verified that they are
    /// over 18.
    /// @param TokenOptions csp::common::Optional<TokenOptions> : Optional override for default token options.
    /// The default token expiry length is configured by MCS and defaults to 30 minutes. Value must be less than the default expiry length, or it will
    /// be ignored.
    /// @param Callback LoginStateResultCallback : callback that contains the result of the 3rd party authentication operation.
    CSP_ASYNC_RESULT void LoginToThirdPartyAuthenticationProvider(const csp::common::String& ThirdPartyToken,
        const csp::common::String& ThirdPartyStateId, const ThirdPartyAuthDetails& ThirdPartyAuthDetails, bool CreateMultiplayerConnection,
        const csp::common::Optional<bool>& UserHasVerifiedAge, const csp::common::Optional<TokenOptions>& TokenOptions,
        LoginStateResultCallback Callback);

    /// @brief Logout from Magnopus Cloud Services.
    /// @param Callback NullResultCallback : callback to call when a response is received
    CSP_ASYNC_RESULT void Logout(NullResultCallback Callback);

    // Profile

    /// @brief Creates a new user profile.
    /// @param UserName csp::common::Optional<csp::common::String> : user name associated with the new profile
    /// @param DisplayName csp::common::Optional<csp::common::String> : user display name associated with the new profile
    /// @param Email csp::common::String : email address associated with the new profile
    /// @param Password csp::common::String : password associated with the new profile
    /// @param ReceiveNewsletter bool : `true` if the user wants to receive the Magnopus Cloud Services newsletter
    /// @param UserHasVerifiedAge csp::common::Optional<bool> : An optional bool to specify whether or not the user has verified that they are over 18
    /// @param RedirectUrl csp::common::Optional<csp::common::String> : the URL to redirect the user to after they have registered
    /// @param InviteToken csp::common::Optional<csp::common::String> : A token provided to the user that can be used to auto-confirm their account
    /// @param Callback ProfileResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void CreateUser(const csp::common::Optional<csp::common::String>& UserName,
        const csp::common::Optional<csp::common::String>& DisplayName, const csp::common::String& Email, const csp::common::String& Password,
        bool ReceiveNewsletter, bool UserHasVerifiedAge, const csp::common::Optional<csp::common::String>& RedirectUrl,
        const csp::common::Optional<csp::common::String>& InviteToken, ProfileResultCallback Callback);

    /// @brief Upgrade guest user to full user profile.
    /// @param UserName csp::common::String : user name associated with the new profile
    /// @param DisplayName csp::common::String : user display name associated with the new profile
    /// @param Email csp::common::String : email address associated with the new profile
    /// @param Password csp::common::String : password associated with the new profile
    /// @param Callback ProfileResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void UpgradeGuestAccount(const csp::common::String& UserName, const csp::common::String& DisplayName,
        const csp::common::String& Email, const csp::common::String& Password, ProfileResultCallback Callback);

    /// @brief Send a confirmation email.
    /// @param Callback NullResultCallback : callback to call when a response is received
    CSP_ASYNC_RESULT void ConfirmUserEmail(NullResultCallback Callback);

    /// @brief Reset the users password.
    /// @param Token csp::common::String : Token received through email by user
    /// @param UserId csp::common::String : The id of the user resetting their password
    /// @param NewPassword csp::common::String : The new password for the associated account
    /// @param Callback NullResultCallback : callback to call when a response is received
    CSP_ASYNC_RESULT void ResetUserPassword(
        const csp::common::String& Token, const csp::common::String& UserId, const csp::common::String& NewPassword, NullResultCallback Callback);

    /// @brief Updates the user display name information.
    /// @param UserId csp::common::String : id of the user that will be updated
    /// @param NewUserDisplayName csp::common::String : new display name that will replace the previous value
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void UpdateUserDisplayName(
        const csp::common::String& UserId, const csp::common::String& NewUserDisplayName, NullResultCallback Callback);

    /// @brief Delete the user. Note that you need permission to be able to delete the user (You can delete the user you are logged in as).
    /// @param UserId csp::common::String : id of the user that will be deleted
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void DeleteUser(const csp::common::String& UserId, NullResultCallback Callback);

    /// @brief Allow a user to reset their password if forgotten by providing an email address.
    /// @param Email csp::common::String : account to recover password for
    /// @param RedirectUrl csp::common::Optional<csp::common::String> : the URL to redirect the user to after they have registered
    /// @param EmailLinkUrl csp::common::Optional<csp::common::String> : the URL inside the reset email sent to the user
    /// @Param UseTokenChangePasswordUrl bool : if true the link in the email will direct the user to the Token Change URL
    /// @param Callback NullResultCallback : callback to call when a response is received
    CSP_ASYNC_RESULT void ForgotPassword(const csp::common::String& Email, const csp::common::Optional<csp::common::String>& RedirectUrl,
        const csp::common::Optional<csp::common::String>& EmailLinkUrl, bool UseTokenChangePasswordUrl, NullResultCallback Callback);

    /// @brief Get a user profile by user ID.
    /// @param InUserId csp::common::String : the ID of the user to get
    /// @param Callback ProfileResultCallback : callback to call when a response is received
    CSP_ASYNC_RESULT void GetProfileByUserId(const csp::common::String& InUserId, ProfileResultCallback Callback);

    [[deprecated("Deprecated in favour of GetBasicProfilesByUserId")]] CSP_ASYNC_RESULT void GetProfilesByUserId(
        const csp::common::Array<csp::common::String>& InUserIds, BasicProfilesResultCallback Callback);

    /// @brief Get a list of minimal profiles (avatarId, personalityType, userName, and platform) by user IDs.
    /// @param InUserIds csp::common::Array<csp::common::String> : an array of user IDs to search for users by
    /// @param Callback BasicProfilesResultCallback : callback to call when a response is received
    CSP_ASYNC_RESULT void GetBasicProfilesByUserId(const csp::common::Array<csp::common::String>& InUserIds, BasicProfilesResultCallback Callback);

    /// @brief Ping Magnopus Cloud Services
    /// @param Callback NullResultCallback : callback to call when a response is received
    CSP_ASYNC_RESULT void Ping(NullResultCallback Callback);

    /// @brief Re-send user verification email
    /// @param InEmail csp::common::String : User's email address
    /// @param InRedirectUrl csp::common::Optional<csp::common::String> : URL to redirect user to after they have registered
    /// @param Callback NullResultCallback : Callback to call when response is received
    CSP_ASYNC_RESULT void ResendVerificationEmail(
        const csp::common::String& InEmail, const csp::common::Optional<csp::common::String>& InRedirectUrl, NullResultCallback Callback);

    /// @brief Get the Customer Portal Url for a user from Stripe
    /// @param UserId csp::common::String : the id of the user associated with the customer portal
    /// @param Callback StringResultCallback : callback that contains the customer portal URL of the User
    CSP_ASYNC_RESULT void GetCustomerPortalUrl(const csp::common::String& UserId, StringResultCallback Callback);

    /// @brief Get the checkout session Url for a user from Stripe
    /// @param Tier csp::systems::TierNames : the tier of the checkout session needed
    /// @param Callback StringResultCallback : callback that contains the checkout session URL of the tier
    CSP_ASYNC_RESULT void GetCheckoutSessionUrl(TierNames Tier, StringResultCallback Callback);

    // Callback to receive access permission changes Data when a message is sent.
    typedef std::function<void(const csp::common::AccessControlChangedNetworkEventData&)> UserPermissionsChangedCallbackHandler;

    /// @brief Sets a callback for an access control changed event.
    ///
    /// Occurs when a user's permissions are altered, impacting their ability to interact with specific spaces.
    /// Clients can use this event to reflect access levels in real time.
    ///
    /// @param Callback UserPermissionsChangedCallbackHandler: Callback to receive data for the user permissions that has been changed.
    CSP_EVENT void SetUserPermissionsChangedCallback(UserPermissionsChangedCallbackHandler Callback);

    /// @brief Registers the system to listen for the named event.
    void RegisterSystemCallback() override;
    /// @brief Deserialises the event values of the system.
    /// @param EventValues std::vector<signalr::value> : event values to deserialise
    CSP_NO_EXPORT void OnAccessControlChangedEvent(const csp::common::NetworkEventData& NetworkEventData);

    /// The IAuthContext object is owned by the UserSystem, and will be destroyed when the UserSystem is destroyed.
    CSP_NO_EXPORT csp::common::IAuthContext& GetAuthContext();

private:
    UserSystem(); // This constructor is only provided to appease the wrapper generator and should not be used
    UserSystem(csp::web::WebClient* InWebClient, csp::multiplayer::NetworkEventBus* InEventBus, csp::common::LogSystem& LogSystem);
    ~UserSystem();

    // Emergency Fix: We have a circular dependency issue due to SignalR requiring the AuthContext for construction. To get around this
    // we pass nullptr to the UserSystem ctor for the NetworkEventBus, and then call this method to set it after the NetworkEventBus has been
    // constructed.
    void SetNetworkEventBus(csp::multiplayer::NetworkEventBus& EventBus);

    [[nodiscard]] bool EmailCheck(const std::string& Email) const;

    void NotifyRefreshTokenHasChanged();

    csp::services::ApiBase* AuthenticationAPI;
    csp::services::ApiBase* ProfileAPI;
    csp::services::ApiBase* PingAPI;
    csp::services::ApiBase* StripeAPI;

    csp::common::LoginState CurrentLoginState;

    LoginTokenInfoResultCallback RefreshTokenChangedCallback;

    UserPermissionsChangedCallbackHandler UserPermissionsChangedCallback;

    AuthContext Auth;
};

} // namespace csp::systems
