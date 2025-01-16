#pragma once

#include "Olympus/Common/Array.h"
#include "Olympus/Common/Optional.h"
#include "Olympus/Common/String.h"
#include "Olympus/OlympusCommon.h"
#include "Olympus/Systems/SystemBase.h"
#include "Olympus/Systems/Users/Authentication.h"
#include "Olympus/Systems/Users/Profile.h"
#include "Olympus/Systems/Users/ThirdPartyAuthentication.h"

namespace oly_web
{

class WebClient;

}

namespace oly_systems
{

/// @ingroup User System
/// @brief Public facing system that allows interfacing with CHS's user service.
/// Offers methods for creating accounts, authenticating, and retrieving user profiles.
class OLY_API OLY_NO_DISPOSE UserSystem : public SystemBase
{
public:
    ~UserSystem();

    // Authentication

    /// @brief Get the current login state.
    /// @return LoginState : Current login state
    const LoginState& GetLoginState() const;

    /// @brief Sets a callback that will get fired when the login token has changed as a result of logging in with credentials or with a token or
    /// after the Foundation internal system has refreshed the session.
    /// In the callback result the token and it's expiration time will be provided.
    /// The expiration time is in OSI format {Year}-{Month}-{Date}T{Hour}:{Min}:{Sec}
    /// For C#: register a callback to the OnNewLoginTokenReceived event
    /// @param Callback NewLoginTokenReceivedCallback : callback that gets called as described above
    OLY_EVENT void SetNewLoginTokenReceivedCallback(NewLoginTokenReceivedCallback Callback);

    /// @brief Log in to Magnopus Cloud Hosted services using a username-password or email-password combination.
    /// @param UserName oly_common::String
    /// @param Email oly_common::String
    /// @param Password oly_common::String
    /// @param Callback LoginStateResultCallback : callback to call when a response is received
    OLY_ASYNC_RESULT void Login(
        const oly_common::String& UserName, const oly_common::String& Email, const oly_common::String& Password, LoginStateResultCallback Callback);

    /// @brief Log in to CHS using a login token
    /// The login token can be obtained after using the Login API with credentials and having registered a callback through
    /// SetNewLoginTokenReceivedCallback. If the login is successful in the callback result the token and it's expiration time will be provided.
    /// @param UserId oly_common::String : the user ID associated with this login token
    /// @param LoginToken oly_common::String : token to be used for authenticating
    /// @param Callback LoginStateResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void LoginWithToken(const oly_common::String& UserId, const oly_common::String& LoginToken, LoginStateResultCallback Callback);

    /// @brief Log in to Magnopus Cloud Hosted services as a guest.
    /// @param Callback LoginStateResultCallback : callback to call when a response is received
    OLY_ASYNC_RESULT void LoginAsGuest(LoginStateResultCallback Callback);

    /// @ingroup Third Party Authentication
    /// @brief As a FDN user the 3rd party authentication flow consists of two steps, first calling GetThirdPartyProviderAuthoriseURL followed by
    /// LoginToThirdPartyAuthenticationProvider You can see a Sequence Diagram with all the parties involved including what a Client should be calling
    /// and when here https://miro.com/app/board/uXjVPflpu98=/.

    /// @brief API to retrieve the FDN supported 3rd party authentication providers
    /// @return Array of FDN supported 3rd party authentication providers
    [[nodiscard]] oly_common::Array<EThirdPartyAuthenticationProviders> GetSupportedThirdPartyAuthenticationProviders() const;

    /// @brief First step of the 3rd party authentication flow
    /// If you call this API but for some reason you'd like to call this again, this is supported, the params you pass second time will replace the
    /// ones you've passed initially
    /// @param AuthProvider EThirdPartyAuthenticationProviders : one of the supported Authentication Providers
    /// @param RedirectURL oly_common::String : the RedirectURL you want to be used for this authentication flow
    /// @param Callback StringResultCallback : callback that contains the Authorise URL that the Client should be navigating next before moving to the
    /// second FDN Authentication step
    OLY_ASYNC_RESULT void GetThirdPartyProviderAuthoriseURL(
        EThirdPartyAuthenticationProviders AuthProvider, const oly_common::String& RedirectURL, StringResultCallback Callback);

    /// @brief Second step of the 3rd party authentication flow
    /// Note: The Authentication Provider and the Redirect URL you've passed in the first step will be used now
    /// @param ThirdPartyToken oly_common::String : The authentication token returned by the Provider
    /// @param ThirdPartyStateId oly_common::String : The state Id returned by the Provider
    /// @param Callback LoginStateResultCallback : callback that contains the result of the CHS Authentication operation
    OLY_ASYNC_RESULT void LoginToThirdPartyAuthenticationProvider(
        const oly_common::String& ThirdPartyToken, const oly_common::String& ThirdPartyStateId, LoginStateResultCallback Callback);

    /// @brief Log in to Magnopus Cloud Hosted services as a guest.
    /// @param DeviceId oly_common::String : The device Id to use when logging in
    /// @param Callback LoginStateResultCallback : callback to call when a response is received
    [[deprecated("This is no longer needed as we now have proper device ID generation! Please use LoginAsGuest() instead.")]] OLY_ASYNC_RESULT void
    LoginAsGuestWithId(const oly_common::String& DeviceId, LoginStateResultCallback Callback);

    /// @brief Log in to Magnopus Cloud Hosted services using the given one-time password/key.
    /// @param UserId oly_common::String : the user Id
    /// @param Key oly_common::String : the one-time key to exchange
    /// @param Callback LoginStateResultCallback : callback to call when a response is received
    OLY_ASYNC_RESULT void ExchangeKey(const oly_common::String& UserId, const oly_common::String& Key, LoginStateResultCallback Callback);

    /// @brief Logout from Magnopus Cloud Hosted services.
    /// @param Callback LogoutResultCallback : callback to call when a response is received
    OLY_ASYNC_RESULT void Logout(LogoutResultCallback Callback);

    // Profile

    /// @brief Creates a new user profile.
    /// @param UserName oly_common::Optional<oly_common::String> : user name associated with the new profile
    /// @param DisplayName oly_common::Optional<oly_common::String> : user display name associated with the new profile
    /// @param Email oly_common::String : email address associated with the new profile
    /// @param Password oly_common::String : password associated with the new profile
    /// @param ReceiveNewsletter bool : `true` if the user wants to receive the OKO newsletter
    /// @param RedirectUrl oly_common::Optional<oly_common::String> : the URL to redirect the user to after they have registered
    /// @param InviteToken oly_common::Optional<oly_common::String> : A token provided to the user that can be used to auto-confirm their account
    /// @param Callback ProfileResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void CreateUser(const oly_common::Optional<oly_common::String>& UserName,
        const oly_common::Optional<oly_common::String>& DisplayName, const oly_common::String& Email, const oly_common::String& Password,
        bool ReceiveNewsletter, const oly_common::Optional<oly_common::String>& RedirectUrl,
        const oly_common::Optional<oly_common::String>& InviteToken, ProfileResultCallback Callback);

    /// @brief Upgrade guest user to full user profile.
    /// @param UserName oly_common::String : user name associated with the new profile
    /// @param DisplayName oly_common::String : user display name associated with the new profile
    /// @param Email oly_common::String : email address associated with the new profile
    /// @param Password oly_common::String : password associated with the new profile
    /// @param Callback ProfileResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void UpgradeGuestAccount(const oly_common::String& UserName, const oly_common::String& DisplayName,
        const oly_common::String& Email, const oly_common::String& Password, ProfileResultCallback Callback);

    /// @brief Send a confirmation email.
    /// @param Callback NullResultCallback : callback to call when a response is received
    OLY_ASYNC_RESULT void ConfirmUserEmail(NullResultCallback Callback);

    /// @brief Reset the users password.
    /// @param Callback NullResultCallback : callback to call when a response is received
    OLY_ASYNC_RESULT void ResetUserPassword(const oly_common::Optional<oly_common::String>& RedirectUrl, NullResultCallback Callback);

    /// @brief Updates the user display name information.
    /// @param UserId oly_common::String : id of the user that will be updated
    /// @param NewUserDisplayName oly_common::String : new display name that will replace the previous value
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void UpdateUserDisplayName(
        const oly_common::String& UserId, const oly_common::String& NewUserDisplayName, NullResultCallback Callback);

    /// @brief Delete the user.
    /// @param UserId oly_common::String : id of the user that will be deleted
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void DeleteUser(const oly_common::String& UserId, NullResultCallback Callback);

    /// @brief Allow a user to reset their password if forgotten by providing an email address.
    /// @param Email oly_common::String : account to recover password for
    /// @param RedirectUrl oly_common::Optional<oly_common::String> : the URL to redirect the user to after they have registered
    /// @param Callback NullResultCallback : callback to call when a response is received
    OLY_ASYNC_RESULT void ForgotPassword(
        const oly_common::String& Email, const oly_common::Optional<oly_common::String>& RedirectUrl, NullResultCallback Callback);

    /// @brief Get a user profile by user ID.
    /// @param InUserId oly_common::String : the ID of the user to get
    /// @param Callback ProfileResultCallback : callback to call when a response is received
    OLY_ASYNC_RESULT void GetProfileByUserId(const oly_common::String& InUserId, ProfileResultCallback Callback);

    /// @brief Get a list of minimal profiles (avatarId, personalityType, userName, and platform) by user IDs.
    /// @param InUserIds oly_common::Array<oly_common::String> : an array of user IDs to search for users by
    /// @param Callback BasicProfilesResultCallback : callback to call when a response is received
    OLY_ASYNC_RESULT void GetProfilesByUserId(const oly_common::Array<oly_common::String>& InUserIds, BasicProfilesResultCallback Callback);

    /// @brief Ping CHS
    /// @param Callback PingResponseReceivedCallback : callback to call when a response is received
    OLY_ASYNC_RESULT void Ping(PingResponseReceivedCallback Callback);

    /// @brief Retrieve User token from Agora
    /// @param Params AgoraUserTokenParams : Params to configure the User token
    /// @param Callback UserTokenResultCallback : callback to call when a response is received
    OLY_ASYNC_RESULT void GetAgoraUserToken(const AgoraUserTokenParams& Params, UserTokenResultCallback Callback);

protected:
    OLY_NO_EXPORT UserSystem(oly_web::WebClient* InWebClient);

    void RefreshAuthenticationSession(const oly_common::String& UserId, const oly_common::String& RefreshToken, const oly_common::String& DeviceId,
        const LoginStateResultCallback& Callback);

    void NotifyRefreshTokenHasChanged();

private:
    UserSystem(); // This constructor is only provided to appease the wrapper generator and should not be used

    [[nodiscard]] bool EmailCheck(const std::string& Email) const;

    void ResetAuthenticationState();

    oly_services::ApiBase* AuthenticationAPI;
    oly_services::ApiBase* ProfileAPI;
    oly_services::ApiBase* PingAPI;
    oly_services::ApiBase* ExternalServiceProxyApi;

    LoginState CurrentLoginState;

    NewLoginTokenReceivedCallback RefreshTokenChangedCallback;

    oly_common::String ThirdPartyAuthStateId;
    oly_common::String ThirdPartyAuthRedirectURL;
    EThirdPartyAuthenticationProviders ThirdPartyRequestedAuthProvider = EThirdPartyAuthenticationProviders::Invalid;
};

} // namespace oly_systems
