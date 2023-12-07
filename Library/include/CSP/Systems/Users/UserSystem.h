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

}


namespace csp::systems
{

/// @ingroup User System
/// @brief Public facing system that allows interfacing with Magnopus Connected Services' user service.
/// Offers methods for creating accounts, authenticating, and retrieving user profiles.
class CSP_API CSP_NO_DISPOSE UserSystem : public SystemBase
{
public:
	~UserSystem();

	// Authentication

	/// @brief Get the current login state.
	/// @return LoginState : Current login state
	const LoginState& GetLoginState() const;

	/// @brief Sets a callback that will get fired when the login token has changed as a result of logging in with credentials or with a token or
	/// after the Connected Spaces Platform internal system has refreshed the session.
	/// In the callback result the token and it's expiration time will be provided.
	/// The expiration time is in OSI format {Year}-{Month}-{Date}T{Hour}:{Min}:{Sec}
	/// For C#: register a callback to the OnNewLoginTokenReceived event
	/// @param Callback NewLoginTokenReceivedCallback : callback that gets called as described above
	CSP_EVENT void SetNewLoginTokenReceivedCallback(NewLoginTokenReceivedCallback Callback);

	/// @brief Log in to Magnopus Connected Services services using a username-password or email-password combination.
	/// @param UserName csp::common::String
	/// @param Email csp::common::String
	/// @param Password csp::common::String
	/// @param UserHasVerifiedAge csp::common::Optional<bool> : An optional bool to specify whether or not the user has verified that they are over 18
	/// @param Callback LoginStateResultCallback : callback to call when a response is received
	CSP_ASYNC_RESULT void Login(const csp::common::String& UserName,
								const csp::common::String& Email,
								const csp::common::String& Password,
								const csp::common::Optional<bool>& UserHasVerifiedAge,
								LoginStateResultCallback Callback);

	/// @brief Log in to Magnopus Connected Services using a login token
	/// The login token can be obtained after using the Login API with credentials and having registered a callback through
	/// SetNewLoginTokenReceivedCallback. If the login is successful in the callback result the token and it's expiration time will be provided.
	/// @param UserId csp::common::String : the user ID associated with this login token
	/// @param LoginToken csp::common::String : token to be used for authenticating
	/// @param Callback LoginStateResultCallback : callback when asynchronous task finishes
	CSP_ASYNC_RESULT void LoginWithToken(const csp::common::String& UserId, const csp::common::String& LoginToken, LoginStateResultCallback Callback);

	/// @brief Log in to Magnopus Connected Services as a guest.
	/// @param UserHasVerifiedAge csp::common::Optional<bool> : An optional bool to specify whether or not the user has verified that they are over 18
	/// @param Callback LoginStateResultCallback : callback to call when a response is received
	CSP_ASYNC_RESULT void LoginAsGuest(const csp::common::Optional<bool>& UserHasVerifiedAge, LoginStateResultCallback Callback);

	/// @ingroup Third Party Authentication
	/// @brief As a Connected Spaces Platform user the 3rd party authentication flow consists of two steps, first calling
	/// GetThirdPartyProviderAuthoriseURL followed by LoginToThirdPartyAuthenticationProvider You can see a Sequence Diagram with all the parties
	/// involved including what a Client should be calling and when here https://miro.com/app/board/uXjVPflpu98=/.

	/// @brief API to retrieve the Connected Spaces Platform supported 3rd party authentication providers
	/// @return Array of Connected Spaces Platform supported 3rd party authentication providers
	[[nodiscard]] csp::common::Array<EThirdPartyAuthenticationProviders> GetSupportedThirdPartyAuthenticationProviders() const;

	/// @brief First step of the 3rd party authentication flow
	/// If you call this API but for some reason you'd like to call this again, this is supported, the params you pass second time will replace the
	/// ones you've passed initially
	/// @param AuthProvider EThirdPartyAuthenticationProviders : one of the supported Authentication Providers
	/// @param RedirectURL csp::common::String : the RedirectURL you want to be used for this authentication flow
	/// @param Callback StringResultCallback : callback that contains the Authorise URL that the Client should be navigating next before moving to the
	/// second Connected Spaces Platform Authentication step
	CSP_ASYNC_RESULT void GetThirdPartyProviderAuthoriseURL(EThirdPartyAuthenticationProviders AuthProvider,
															const csp::common::String& RedirectURL,
															StringResultCallback Callback);

	/// @brief Second step of the 3rd party authentication flow
	/// Note: The Authentication Provider and the Redirect URL you've passed in the first step will be used now
	/// @param ThirdPartyToken csp::common::String : The authentication token returned by the Provider
	/// @param ThirdPartyStateId csp::common::String : The state Id returned by the Provider
	/// @param UserHasVerifiedAge csp::common::Optional<bool> : An optional bool to specify whether or not the user has verified that they are over 18
	/// @param Callback LoginStateResultCallback : callback that contains the result of the Magnopus Connected Services Authentication operation
	CSP_ASYNC_RESULT void LoginToThirdPartyAuthenticationProvider(const csp::common::String& ThirdPartyToken,
																  const csp::common::String& ThirdPartyStateId,
																  const csp::common::Optional<bool>& UserHasVerifiedAge,
																  LoginStateResultCallback Callback);

	/// @brief Log in to Magnopus Connected Services using the given one-time password/key.
	/// @param UserId csp::common::String : the user Id
	/// @param Key csp::common::String : the one-time key to exchange
	/// @param Callback LoginStateResultCallback : callback to call when a response is received
	CSP_ASYNC_RESULT void ExchangeKey(const csp::common::String& UserId, const csp::common::String& Key, LoginStateResultCallback Callback);

	/// @brief Logout from Magnopus Connected Services.
	/// @param Callback LogoutResultCallback : callback to call when a response is received
	CSP_ASYNC_RESULT void Logout(LogoutResultCallback Callback);

	// Profile

	/// @brief Creates a new user profile.
	/// @param UserName csp::common::Optional<csp::common::String> : user name associated with the new profile
	/// @param DisplayName csp::common::Optional<csp::common::String> : user display name associated with the new profile
	/// @param Email csp::common::String : email address associated with the new profile
	/// @param Password csp::common::String : password associated with the new profile
	/// @param ReceiveNewsletter bool : `true` if the user wants to receive the Magnopus Connected Services newsletter
	/// @param UserHasVerifiedAge csp::common::Optional<bool> : An optional bool to specify whether or not the user has verified that they are over 18
	/// @param RedirectUrl csp::common::Optional<csp::common::String> : the URL to redirect the user to after they have registered
	/// @param InviteToken csp::common::Optional<csp::common::String> : A token provided to the user that can be used to auto-confirm their account
	/// @param Callback ProfileResultCallback : callback when asynchronous task finishes
	CSP_ASYNC_RESULT void CreateUser(const csp::common::Optional<csp::common::String>& UserName,
									 const csp::common::Optional<csp::common::String>& DisplayName,
									 const csp::common::String& Email,
									 const csp::common::String& Password,
									 bool ReceiveNewsletter,
									 bool UserHasVerifiedAge,
									 const csp::common::Optional<csp::common::String>& RedirectUrl,
									 const csp::common::Optional<csp::common::String>& InviteToken,
									 ProfileResultCallback Callback);

	/// @brief Upgrade guest user to full user profile.
	/// @param UserName csp::common::String : user name associated with the new profile
	/// @param DisplayName csp::common::String : user display name associated with the new profile
	/// @param Email csp::common::String : email address associated with the new profile
	/// @param Password csp::common::String : password associated with the new profile
	/// @param Callback ProfileResultCallback : callback when asynchronous task finishes
	CSP_ASYNC_RESULT void UpgradeGuestAccount(const csp::common::String& UserName,
											  const csp::common::String& DisplayName,
											  const csp::common::String& Email,
											  const csp::common::String& Password,
											  ProfileResultCallback Callback);

	/// @brief Send a confirmation email.
	/// @param Callback NullResultCallback : callback to call when a response is received
	CSP_ASYNC_RESULT void ConfirmUserEmail(NullResultCallback Callback);

	/// @brief Reset the users password.
	/// @param Token csp::common::String : Token received through email by user
	/// @param UserId csp::common::String : The id of the user resetting their password
	/// @param NewPassword csp::common::String : The new password for the associated account
	/// @param Callback NullResultCallback : callback to call when a response is received
	CSP_ASYNC_RESULT void ResetUserPassword(const csp::common::String& Token,
											const csp::common::String& UserId,
											const csp::common::String& NewPassword,
											NullResultCallback Callback);

	/// @brief Updates the user display name information.
	/// @param UserId csp::common::String : id of the user that will be updated
	/// @param NewUserDisplayName csp::common::String : new display name that will replace the previous value
	/// @param Callback NullResultCallback : callback when asynchronous task finishes
	CSP_ASYNC_RESULT void
		UpdateUserDisplayName(const csp::common::String& UserId, const csp::common::String& NewUserDisplayName, NullResultCallback Callback);

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
	CSP_ASYNC_RESULT void ForgotPassword(const csp::common::String& Email,
										 const csp::common::Optional<csp::common::String>& RedirectUrl,
										 const csp::common::Optional<csp::common::String>& EmailLinkUrl,
										 bool UseTokenChangePasswordUrl,
										 NullResultCallback Callback);

	/// @brief Get a user profile by user ID.
	/// @param InUserId csp::common::String : the ID of the user to get
	/// @param Callback ProfileResultCallback : callback to call when a response is received
	CSP_ASYNC_RESULT void GetProfileByUserId(const csp::common::String& InUserId, ProfileResultCallback Callback);

	/// @brief Get a list of minimal profiles (avatarId, personalityType, userName, and platform) by user IDs.
	/// @param InUserIds csp::common::Array<csp::common::String> : an array of user IDs to search for users by
	/// @param Callback BasicProfilesResultCallback : callback to call when a response is received
	CSP_ASYNC_RESULT void GetProfilesByUserId(const csp::common::Array<csp::common::String>& InUserIds, BasicProfilesResultCallback Callback);

	/// @brief Ping Magnopus Connected Services
	/// @param Callback PingResponseReceivedCallback : callback to call when a response is received
	CSP_ASYNC_RESULT void Ping(PingResponseReceivedCallback Callback);

	/// @brief Retrieve User token from Agora
	/// @param Params AgoraUserTokenParams : Params to configure the User token
	/// @param Callback UserTokenResultCallback : callback to call when a response is received
	CSP_ASYNC_RESULT void GetAgoraUserToken(const AgoraUserTokenParams& Params, UserTokenResultCallback Callback);

	/// @brief Re-send user verification email
	/// @param InEmail csp::common::String : User's email address
	/// @param InRedirectUrl csp::common::Optional<csp::common::String> : URL to redirect user to after they have registered
	/// @param Callback NullResultCallback : Callback to call when response is received
	CSP_ASYNC_RESULT void ResendVerificationEmail(const csp::common::String& InEmail,
												  const csp::common::Optional<csp::common::String>& InRedirectUrl,
												  NullResultCallback Callback);

	/// @brief Get the Customer Portal Url for a user from Stripe
	/// @param UserId csp::common::String : the id of the user associated with the customer portal
	/// @param Callback StringResultCallback : callback that contains the customer portal URL of the User
	CSP_ASYNC_RESULT void GetCustomerPortalUrl(const csp::common::String& UserId, CustomerPortalUrlResultCallback Callback);

	/// @brief Get the checkout session Url for a user from Stripe
	/// @param Tier csp::systems::TierNames : the tier of the checkout session needed
	/// @param Callback CheckoutSessionUrlResultCallback : callback that contains the checkout session URL of the tier
	CSP_ASYNC_RESULT void GetCheckoutSessionUrl(TierNames Tier, CheckoutSessionUrlResultCallback Callback);

protected:
	CSP_NO_EXPORT UserSystem(csp::web::WebClient* InWebClient);

	void RefreshAuthenticationSession(const csp::common::String& UserId,
									  const csp::common::String& RefreshToken,
									  const csp::common::String& DeviceId,
									  const LoginStateResultCallback& Callback);

	void NotifyRefreshTokenHasChanged();

private:
	UserSystem(); // This constructor is only provided to appease the wrapper generator and should not be used

	[[nodiscard]] bool EmailCheck(const std::string& Email) const;

	void ResetAuthenticationState();

	csp::services::ApiBase* AuthenticationAPI;
	csp::services::ApiBase* ProfileAPI;
	csp::services::ApiBase* PingAPI;
	csp::services::ApiBase* ExternalServiceProxyApi;
	csp::services::ApiBase* StripeAPI;

	LoginState CurrentLoginState;

	NewLoginTokenReceivedCallback RefreshTokenChangedCallback;

	csp::common::String ThirdPartyAuthStateId;
	csp::common::String ThirdPartyAuthRedirectURL;
	EThirdPartyAuthenticationProviders ThirdPartyRequestedAuthProvider = EThirdPartyAuthenticationProviders::Invalid;
};

} // namespace csp::systems
