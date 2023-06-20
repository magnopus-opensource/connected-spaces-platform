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
#include "Awaitable.h"
#include "CSP/CSPFoundation.h"
#include "CSP/Systems/Settings/SettingsSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "SpaceSystemTestHelpers.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"
#include "Web/HttpPayload.h"

#include "gtest/gtest.h"
#include <filesystem>
#include <fstream>

using namespace std::chrono_literals;


namespace
{

bool RequestPredicate(const csp::services::ResultBase& Result)
{
	return Result.GetResultCode() != csp::services::EResultCode::InProgress;
}

} // namespace


csp::common::String DefaultLoginEmail;
csp::common::String DefaultLoginPassword;
csp::common::String AlternativeLoginEmail;
csp::common::String AlternativeLoginPassword;


void LoadTestAccountCredentials()
{
	if (!std::filesystem::exists("test_account_creds.txt"))
	{
		LogFatal("test_account_creds.txt not found! This file must exist and must contain the following information:\n<DefaultLoginEmail> "
				 "<DefaultLoginPassword>\n<AlternativeLoginEmail> <AlternativeLoginPassword>");
	}

	std::ifstream CredsFile;
	CredsFile.open("test_account_creds.txt");

	std::string _DefaultLoginEmail, _DefaultLoginPassword, _AlternativeLoginEmail, _AlternativeLoginPassword;

	CredsFile >> _DefaultLoginEmail >> _DefaultLoginPassword;
	CredsFile >> _AlternativeLoginEmail >> _AlternativeLoginPassword;

	if (_DefaultLoginEmail.empty() || _DefaultLoginPassword.empty() || _AlternativeLoginEmail.empty() || _AlternativeLoginPassword.empty())
	{
		LogFatal("test_account_creds.txt must be in the following format:\n<DefaultLoginEmail> <DefaultLoginPassword>\n<AlternativeLoginEmail> "
				 "<AlternativeLoginPassword>");
	}

	DefaultLoginEmail		 = _DefaultLoginEmail.c_str();
	DefaultLoginPassword	 = _DefaultLoginPassword.c_str();
	AlternativeLoginEmail	 = _AlternativeLoginEmail.c_str();
	AlternativeLoginPassword = _AlternativeLoginPassword.c_str();
}

void LogIn(csp::systems::UserSystem* UserSystem,
		   csp::common::String& OutUserId,
		   const csp::common::String& Email,
		   const csp::common::String& Password,
		   csp::services::EResultCode ExpectedResultCode)
{
	auto [Result] = Awaitable(&csp::systems::UserSystem::Login, UserSystem, "", Email, Password).Await(RequestPredicate);

	EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);

	if (Result.GetResultCode() == csp::services::EResultCode::Success)
	{
		OutUserId = Result.GetLoginState().UserId;
	}
}

void LogInAsGuest(csp::systems::UserSystem* UserSystem, csp::common::String& OutUserId, csp::services::EResultCode ExpectedResult)
{
	auto [Result] = Awaitable(&csp::systems::UserSystem::LoginAsGuest, UserSystem).Await(RequestPredicate);

	EXPECT_EQ(Result.GetResultCode(), ExpectedResult);

	if (Result.GetResultCode() == csp::services::EResultCode::Success)
	{
		OutUserId = Result.GetLoginState().UserId;
	}
}

void LogOut(csp::systems::UserSystem* UserSystem, csp::services::EResultCode ExpectedResultCode)
{
	auto [Result] = Awaitable(&csp::systems::UserSystem::Logout, UserSystem).Await(RequestPredicate);

	EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);
}

csp::systems::Profile GetFullProfileByUserId(csp::systems::UserSystem* UserSystem, const csp::common::String& UserId)
{
	auto [GetProfileResult] = AWAIT_PRE(UserSystem, GetProfileByUserId, RequestPredicate, UserId);
	EXPECT_EQ(GetProfileResult.GetResultCode(), csp::services::EResultCode::Success);

	return GetProfileResult.GetProfile();
}

void ValidateThirdPartyAuthoriseURL(const csp::common::String& AuthoriseURL, const csp::common::String& RedirectURL)
{
	EXPECT_FALSE(AuthoriseURL.IsEmpty());
	EXPECT_NE(AuthoriseURL, "error");

	const char* STATE_ID_URL_PARAM		= "state=";
	const char* CLIENT_ID_URL_PARAM		= "client_id=";
	const char* SCOPE_URL_PARAM			= "scope=";
	const char* REDIRECT_URL_PARAM		= "redirect_uri=";
	const char* INVALID_URL_PARAM_VALUE = "N/A";

	std::string StateId				 = INVALID_URL_PARAM_VALUE;
	std::string ClientId			 = INVALID_URL_PARAM_VALUE;
	std::string Scope				 = INVALID_URL_PARAM_VALUE;
	std::string RetrievedRedirectURL = INVALID_URL_PARAM_VALUE;

	const auto& Tokens = AuthoriseURL.Split('&');
	for (size_t idx = 0; idx < Tokens.Size(); ++idx)
	{
		std::string URLElement(Tokens[idx].c_str());
		if (URLElement.find(STATE_ID_URL_PARAM, 0) == 0)
		{
			StateId = URLElement.substr(strlen(STATE_ID_URL_PARAM));
			continue;
		}
		else if (URLElement.find(CLIENT_ID_URL_PARAM, 0) == 0)
		{
			ClientId = URLElement.substr(strlen(CLIENT_ID_URL_PARAM));
			continue;
		}
		else if (URLElement.find(SCOPE_URL_PARAM, 0) == 0)
		{
			Scope = URLElement.substr(strlen(SCOPE_URL_PARAM));
			continue;
		}
		else if (URLElement.find(REDIRECT_URL_PARAM, 0) == 0)
		{
			RetrievedRedirectURL = URLElement.substr(strlen(REDIRECT_URL_PARAM));
			continue;
		}
	}

	const auto NewTokens = Tokens[0].Split('?');
	EXPECT_EQ(NewTokens.Size(), 2);

	std::string URLElement(NewTokens[1].c_str());
	if (URLElement.find(CLIENT_ID_URL_PARAM, 0) == 0)
	{
		ClientId = URLElement.substr(strlen(CLIENT_ID_URL_PARAM));
	}

	// validate that the following contain something that potentially makes sense
	EXPECT_NE(StateId, INVALID_URL_PARAM_VALUE);
	EXPECT_NE(ClientId, INVALID_URL_PARAM_VALUE);
	EXPECT_NE(Scope, INVALID_URL_PARAM_VALUE);
	EXPECT_NE(RetrievedRedirectURL, INVALID_URL_PARAM_VALUE);

	EXPECT_GT(StateId.length(), 10);
	EXPECT_GT(ClientId.length(), 10);
	EXPECT_GE(Scope.length(), 5);
	EXPECT_EQ(RetrievedRedirectURL, RedirectURL.c_str());
}

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_FORGOTPASSWORD_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, ForgotPasswordTest)
{
	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();

	auto [Result] = AWAIT_PRE(UserSystem, UserSystem::ForgotPassword, RequestPredicate, "testnopus.pokemon@magnopus.com", nullptr);

	EXPECT_EQ(Result.GetResultCode(), csp::services::EResultCode::Success);

	auto [Result2] = AWAIT_PRE(UserSystem, UserSystem::ForgotPassword, RequestPredicate, "testnopus.pokemon+1@magnopus.com", nullptr);

	EXPECT_EQ(Result2.GetResultCode(), csp::services::EResultCode::Success);

	auto [FailResult] = AWAIT_PRE(UserSystem, UserSystem::ForgotPassword, RequestPredicate, "email", nullptr);

	EXPECT_EQ(FailResult.GetResultCode(), csp::services::EResultCode::Failed);
}
#endif


#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_LOGIN_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, LogInTest)
{
	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();

	csp::common::String UserId;

	// Log in
	LogIn(UserSystem, UserId);

	// Log out
	LogOut(UserSystem);
}
#endif
#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_BADLOGOUT_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, BadLogOutTest)
{
	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();

	csp::common::String UserId;

	// Log out without logging in first
	LogOut(UserSystem, csp::services::EResultCode::Failed);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_BADDUALLOGIN_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, BadDualLoginTest)
{
	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();

	csp::common::String UserId;

	// Log in
	LogIn(UserSystem, UserId);

	// Attempt to log in again
	LogIn(UserSystem, UserId, DefaultLoginEmail, DefaultLoginPassword, csp::services::EResultCode::Failed);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_LOGIN_WITH_TOKEN_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, LogInWithTokenTest)
{
	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();

	csp::common::String UserId;

	csp::common::String LoginToken;
	bool LoginTokenAvailable = false;

	csp::systems::NewLoginTokenReceivedCallback LoginTokenReceivedCallback = [&](csp::systems::LoginTokenReceived& Result)
	{
		EXPECT_EQ(Result.GetResultCode(), csp::services::EResultCode::Success);

		if (Result.GetResultCode() == csp::services::EResultCode::Success)
		{
			LoginToken			= Result.GetLoginTokenInfo().RefreshToken;
			LoginTokenAvailable = true;

			std::cerr << "New Refresh token: " << Result.GetLoginTokenInfo().RefreshToken
					  << " expires at: " << Result.GetLoginTokenInfo().RefreshExpiryTime << std::endl;
		}
	};

	UserSystem->SetNewLoginTokenReceivedCallback(LoginTokenReceivedCallback);

	// Logging in with credentials to get the token
	LogIn(UserSystem, UserId, DefaultLoginEmail, DefaultLoginPassword);

	int WaitForTestTimeoutCountMs		= 0;
	const int WaitForTestTimeoutLimitMs = 1000;

	// wait for the login token to become available
	while (LoginTokenAvailable == false && WaitForTestTimeoutCountMs < WaitForTestTimeoutLimitMs)
	{
		std::this_thread::sleep_for(50ms);
		WaitForTestTimeoutCountMs += 50;
	}

	LoginTokenAvailable = false;

	auto [Result] = Awaitable(&csp::systems::UserSystem::LoginWithToken, UserSystem, UserId, LoginToken).Await(RequestPredicate);

	EXPECT_EQ(Result.GetResultCode(), csp::services::EResultCode::Success);

	WaitForTestTimeoutCountMs = 0;
	// wait for the login token to be updated
	while (LoginTokenAvailable == false && WaitForTestTimeoutCountMs < WaitForTestTimeoutLimitMs)
	{
		std::this_thread::sleep_for(50ms);
		WaitForTestTimeoutCountMs += 50;
	}

	LoginTokenAvailable = false;

	auto& CurrentLoginState = UserSystem->GetLoginState();

	EXPECT_EQ(CurrentLoginState.State, csp::systems::ELoginState::LoggedIn);

	// check that we're successfully logged in to CHS by creating a space
	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueHexString().c_str());

	// Create space
	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	DeleteSpace(SpaceSystem, Space.Id);

	UserSystem->SetNewLoginTokenReceivedCallback(nullptr);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_LOGINERROR_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, LoginErrorTest)
{
	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();

	csp::common::String UserId;

	// Log in with invalid credentials
	LogIn(UserSystem, UserId, "invalidlogin@rewind.co", "", csp::services::EResultCode::Failed);

	// Log in
	LogIn(UserSystem, UserId);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_USERSYSTEM_REFRESH_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, RefreshTest)
{
	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();

	csp::common::String UserId;

	// Log in
	LogIn(UserSystem, UserId);

	// Tokens are issued with a 30 min expiry but may be accepted up to 5 mins after their expiry.
	// We set at 40 mins to make sure we're definitely dealing with a fully expired token that will
	// not be accepted.
	std::this_thread::sleep_for(40min);

	auto Profile = GetFullProfileByUserId(UserSystem, UserId);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_UPDATE_DISPLAY_NAME_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, UpdateDisplayNameTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();

	csp::common::String UniqueTestDisplayName = csp::common::String("NAME ") + GetUniqueHexString().c_str();

	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Update display name
	{
		auto [Result] = AWAIT_PRE(UserSystem, UpdateUserDisplayName, RequestPredicate, UserId, UniqueTestDisplayName);

		EXPECT_EQ(Result.GetResultCode(), csp::services::EResultCode::Success);
	}

	// Retrieve user profile and verify display name has been updated
	{
		auto FullProfile = GetFullProfileByUserId(UserSystem, UserId);

		EXPECT_EQ(FullProfile.UserId, UserId);
		EXPECT_EQ(FullProfile.DisplayName, UniqueTestDisplayName);
	}

	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_UPDATE_DISPLAY_NAME_INCLUDING_BLANK_SPACES_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, UpdateDisplayNameIncludingBlankSpacesTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();

	csp::common::String UniqueTestDisplayName = csp::common::String("Test ") + GetUniqueHexString().c_str();

	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Update display name
	{
		auto [Result] = AWAIT_PRE(UserSystem, UpdateUserDisplayName, RequestPredicate, UserId, UniqueTestDisplayName);

		EXPECT_EQ(Result.GetResultCode(), csp::services::EResultCode::Success);
	}

	// Retrieve user profile and verify display name has been updated
	{
		auto FullProfile = GetFullProfileByUserId(UserSystem, UserId);

		EXPECT_EQ(FullProfile.UserId, UserId);
		EXPECT_EQ(FullProfile.DisplayName, UniqueTestDisplayName);
	}

	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_UPDATE_DISPLAY_NAME_INCLUDING_SYMBOLS_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, UpdateDisplayNameIncludingSymbolsTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();

	csp::common::String UniqueTestDisplayName = csp::common::String("Test ()= - ") + GetUniqueHexString(8).c_str();

	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Update display name
	{
		auto [Result] = AWAIT_PRE(UserSystem, UpdateUserDisplayName, RequestPredicate, UserId, UniqueTestDisplayName);

		EXPECT_EQ(Result.GetResultCode(), csp::services::EResultCode::Success);
	}

	// Retrieve user profile and verify display name has been updated
	{
		auto FullProfile = GetFullProfileByUserId(UserSystem, UserId);

		EXPECT_EQ(FullProfile.UserId, UserId);
		EXPECT_EQ(FullProfile.DisplayName, UniqueTestDisplayName);
	}

	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_PING_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, PingTest)
{
	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();

	// check that ping function returns success and doesn't timeout
	auto [Result] = AWAIT_PRE(UserSystem, Ping, RequestPredicate);
	EXPECT_EQ(Result.GetResultCode(), csp::services::EResultCode::Success);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_CREATE_USER_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, CreateUserTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SettingsSystem = SystemsManager.GetSettingsSystem();

	const char* TestUserName	= "OLY-TEST-NAME";
	const char* TestDisplayName = "OLY-TEST-DISPLAY";

	char UniqueUserName[256];
	SPRINTF(UniqueUserName, "%s-%s", TestUserName, GetUniqueHexString().c_str());

	char UniqueEmail[256];
	SPRINTF(UniqueEmail, GeneratedTestAccountEmailFormat, GetUniqueHexString().c_str());

	csp::common::String CreatedUserId;

	// Create new user
	{
		auto [Result] = AWAIT_PRE(UserSystem,
								  CreateUser,
								  RequestPredicate,
								  UniqueUserName,
								  TestDisplayName,
								  UniqueEmail,
								  GeneratedTestAccountPassword,
								  true,
								  nullptr,
								  nullptr);

		EXPECT_EQ(Result.GetResultCode(), csp::services::EResultCode::Success);

		auto CreatedProfile = Result.GetProfile();
		CreatedUserId		= CreatedProfile.UserId;

		EXPECT_EQ(CreatedProfile.UserName, UniqueUserName);
		EXPECT_EQ(CreatedProfile.DisplayName, TestDisplayName);
		EXPECT_EQ(CreatedProfile.Email, UniqueEmail);
	}

	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Verify that newsletter preference was set
	{
		auto [Result] = AWAIT(SettingsSystem, GetNewsletterStatus, CreatedUserId);

		EXPECT_EQ(Result.GetResultCode(), csp::services::EResultCode::Success);
		EXPECT_TRUE(Result.GetValue());
	}

	// Retrieve the lite profile
	{
		csp::common::Array<csp::common::String> Ids = {CreatedUserId};
		auto [Result]								= AWAIT_PRE(UserSystem, GetProfilesByUserId, RequestPredicate, Ids);

		EXPECT_EQ(Result.GetResultCode(), csp::services::EResultCode::Success);

		auto LiteProfile = Result.GetProfiles()[0];

		EXPECT_EQ(LiteProfile.UserId, CreatedUserId);
		EXPECT_EQ(LiteProfile.DisplayName, TestDisplayName);
	}

	// Retrieve the full profile
	{
		auto FullProfile = GetFullProfileByUserId(UserSystem, CreatedUserId);

		EXPECT_EQ(FullProfile.UserId, CreatedUserId);
		EXPECT_EQ(FullProfile.UserName, UniqueUserName);
		EXPECT_EQ(FullProfile.DisplayName, TestDisplayName);
		EXPECT_EQ(FullProfile.Email, UniqueEmail);
	}

	// Delete the created user
	{
		auto [Result] = AWAIT_PRE(UserSystem, DeleteUser, RequestPredicate, CreatedUserId);

		EXPECT_EQ(Result.GetResultCode(), csp::services::EResultCode::Success);
	}

	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_CREATE_USER_EMPTY_USERNAME_DISPLAYNAME_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, CreateUserEmptyUsernameDisplaynameTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();

	const char* TestUserName = "OLY-TEST-NAME";

	char UniqueEmail[256];
	SPRINTF(UniqueEmail, GeneratedTestAccountEmailFormat, GetUniqueHexString().c_str());

	csp::common::String CreatedUserId;

	// Create new user
	{
		auto [Result] = AWAIT_PRE(UserSystem,
								  CreateUser,
								  RequestPredicate,
								  nullptr,
								  nullptr,
								  UniqueEmail,
								  GeneratedTestAccountPassword,
								  false,
								  nullptr,
								  nullptr);

		EXPECT_EQ(Result.GetResultCode(), csp::services::EResultCode::Success);

		auto CreatedProfile = Result.GetProfile();
		CreatedUserId		= CreatedProfile.UserId;

		EXPECT_TRUE(CreatedProfile.UserName.IsEmpty());
		EXPECT_FALSE(CreatedProfile.DisplayName.IsEmpty());
		EXPECT_EQ(CreatedProfile.Email, UniqueEmail);
	}

	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Retrieve the lite profile
	{
		csp::common::Array<csp::common::String> Ids = {CreatedUserId};
		auto [Result]								= AWAIT_PRE(UserSystem, GetProfilesByUserId, RequestPredicate, Ids);

		EXPECT_EQ(Result.GetResultCode(), csp::services::EResultCode::Success);

		auto LiteProfile = Result.GetProfiles()[0];

		EXPECT_EQ(LiteProfile.UserId, CreatedUserId);
		EXPECT_FALSE(LiteProfile.DisplayName.IsEmpty());
	}

	// Retrieve the full profile
	{
		auto FullProfile = GetFullProfileByUserId(UserSystem, CreatedUserId);

		EXPECT_EQ(FullProfile.UserId, CreatedUserId);
		EXPECT_TRUE(FullProfile.UserName.IsEmpty());
		EXPECT_FALSE(FullProfile.DisplayName.IsEmpty());
		EXPECT_EQ(FullProfile.Email, UniqueEmail);
	}

	// Delete the created user
	{
		auto [Result] = AWAIT_PRE(UserSystem, DeleteUser, RequestPredicate, CreatedUserId);

		EXPECT_EQ(Result.GetResultCode(), csp::services::EResultCode::Success);
	}

	LogOut(UserSystem);
}
#endif

// commented out as the callback of it gets triggered loooong after the test has finished and this is causing a lot of random issues
#if 0
	#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_CANCEL_LOGIN_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, CancelLoginTest)
{
	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();

	csp::common::String UserId;

	// Send log in request and then exit early
	{
		csp::systems::LoginStateResultCallback Callback = [](csp::systems::LoginStateResult& Result)
		{
			if (Result.GetResultCode() == csp::services::EResultCode::InProgress)
			{
				return;
			}

			EXPECT_EQ(Result.GetResultCode(), csp::services::EResultCode::Failed);
		};

		UserSystem->Login("", DefaultLoginEmail, DefaultLoginPassword, Callback);
	}
}
	#endif
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_GET_SUPPORTED_PROVIDERS_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, GetThirdPartySupportedProvidersTest)
{
	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();

	// Check the FDN supported providers
	auto SupportedProviders = UserSystem->GetSupportedThirdPartyAuthenticationProviders();
	EXPECT_EQ(SupportedProviders.Size(), 3L);

	bool FoundGoogle  = false;
	bool FoundDiscord = false;
	bool FoundApple	  = false;
	for (auto idx = 0; idx < SupportedProviders.Size(); ++idx)
	{
		if (SupportedProviders[idx] == csp::systems::EThirdPartyAuthenticationProviders::Google)
		{
			FoundGoogle = true;
		}
		else if (SupportedProviders[idx] == csp::systems::EThirdPartyAuthenticationProviders::Discord)
		{
			FoundDiscord = true;
		}
		else if (SupportedProviders[idx] == csp::systems::EThirdPartyAuthenticationProviders::Apple)
		{
			FoundApple = true;
		}
		else
		{
			ASSERT_TRUE(false) << "Please update this test with this new FDN auth provider: " << SupportedProviders[idx];
		}
	}

	EXPECT_TRUE(FoundGoogle && FoundDiscord && FoundApple);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_GET_AUTHORISE_URL_FOR_GOOGLE_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, GetAuthoriseURLForGoogleTest)
{
	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();

	const auto RedirectURL = "https://odev.magnoverse.space/oauth";

	// Retrieve Authorise URL for Google
	auto [ResGoogle] = AWAIT_PRE(UserSystem,
								 GetThirdPartyProviderAuthoriseURL,
								 RequestPredicate,
								 csp::systems::EThirdPartyAuthenticationProviders::Google,
								 RedirectURL);
	EXPECT_EQ(ResGoogle.GetResultCode(), csp::services::EResultCode::Success);

	const auto& AuthoriseURL = ResGoogle.GetValue();
	ValidateThirdPartyAuthoriseURL(AuthoriseURL, RedirectURL);
}
#endif


#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_GET_AUTHORISE_URL_FOR_DISCORD_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, GetAuthoriseURLForDiscordTest)
{
	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();

	const auto RedirectURL = "https://odev.magnoverse.space/oauth";

	// Retrieve Authorise URL for Discord
	auto [Result] = AWAIT_PRE(UserSystem,
							  GetThirdPartyProviderAuthoriseURL,
							  RequestPredicate,
							  csp::systems::EThirdPartyAuthenticationProviders::Discord,
							  RedirectURL);
	EXPECT_EQ(Result.GetResultCode(), csp::services::EResultCode::Success);

	const auto& AuthoriseURL = Result.GetValue();
	ValidateThirdPartyAuthoriseURL(AuthoriseURL, RedirectURL);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_GET_AUTHORISE_URL_FOR_APPLE_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, GetAuthoriseURLForAppleTest)
{
	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();

	const auto RedirectURL = "https://odev.magnoverse.space/oauth";

	// Retrieve Authorise URL for Apple
	auto [Result] = AWAIT_PRE(UserSystem,
							  GetThirdPartyProviderAuthoriseURL,
							  RequestPredicate,
							  csp::systems::EThirdPartyAuthenticationProviders::Apple,
							  RedirectURL);
	EXPECT_EQ(Result.GetResultCode(), csp::services::EResultCode::Success);

	const auto& AuthoriseURL = Result.GetValue();
	ValidateThirdPartyAuthoriseURL(AuthoriseURL, RedirectURL);
}
#endif

// As the following two tests require manual actions explained inside, they are currently disabled
// ATM only the WASM tests would be able to have a end-to-end testing flow using Selenium for the URL redirects
#if 0
	#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_GOOGLE_LOGIN_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, GoogleLogInTest)
{
	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();

	const auto RedirectURL = "https://odev.magnoverse.space/oauth";

	// Retrieve Authorise URL for Google
	auto [Result] = AWAIT_PRE(UserSystem, GetThirdPartyProviderAuthoriseURL, RequestPredicate, csp::systems::EThirdPartyAuthenticationProviders::Google, RedirectURL);
	EXPECT_EQ(Result.GetResultCode(), csp::services::EResultCode::Success);

	//retrieve the StateId from the URL
	const auto& AuthoriseURL = Result.GetValue();
	const auto Tokens = AuthoriseURL.Split('&');
	std::string StateId = ""; 
	for(size_t idx = 0; idx< Tokens.Size(); ++idx)
	{
		std::string URLElement(Tokens[idx].c_str());
		size_t pos = URLElement.find("state=", 0);
		if(pos == 0)
		{
			StateId = URLElement.substr(strlen("state="));
			break;
		}
	}

	std::cerr << "AuthoriseURL: " << AuthoriseURL << std::endl;

	// 1. Set a breakpoint on the next line before reading from the file
	// 2. Navigate to the AuthoriseURL in a browser, but make sure that for the Third party account you're using there's already a created CHS account (same email address)
	// 3. Get the "code" param value from the response URL and drop it in the file below (this file should be next to the Test binary)
	if (!std::filesystem::exists("third_party_auth_token.txt"))
	{
		LogFatal("third_party_auth_token.txt not found! This file must exist and must contain the provider authentication code/token");
	}

	std::ifstream TokenFile;
	TokenFile.open("third_party_auth_token.txt");
	std::string GoogleToken;
	TokenFile >> GoogleToken;

	auto [LoginResult] = AWAIT_PRE(UserSystem, LoginToThirdPartyProvider, RequestPredicate, csp::systems::EThirdPartyAuthenticationProviders::Google, RedirectURL, csp::common::String(GoogleToken.c_str()), csp::common::String(StateId.c_str()));
	EXPECT_EQ(LoginResult.GetResultCode(), csp::services::EResultCode::Success);
	const auto UserId = LoginResult.GetLoginState()->UserId;

	// test that we are in fact logged in
	auto FullProfile = GetFullProfileByUserId(UserSystem, UserId);

	// Log out
	LogOut(UserSystem);
}
	#endif

	#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_DISCORD_LOGIN_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, DiscordLogInTest)
{
	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();

	const auto RedirectURL = "https://odev.magnoverse.space/oauth";

	// Retrieve Authorise URL for Google
	auto [Result] = AWAIT_PRE(UserSystem, GetThirdPartyProviderAuthoriseURL, RequestPredicate, csp::systems::EThirdPartyAuthenticationProviders::Discord, RedirectURL);
	EXPECT_EQ(Result.GetResultCode(), csp::services::EResultCode::Success);

	//retrieve the StateId from the URL
	const auto& AuthoriseURL = Result.GetValue();
	const auto Tokens = AuthoriseURL.Split('&');
	std::string StateId = ""; 
	for(size_t idx = 0; idx< Tokens.Size(); ++idx)
	{
		std::string URLElement(Tokens[idx].c_str());
		size_t pos = URLElement.find("state=", 0);
		if(pos == 0)
		{
			StateId = URLElement.substr(strlen("state="));
			break;
		}
	}

	std::cerr << "AuthoriseURL: " << AuthoriseURL << std::endl;

	// 1. Set a breakpoint on the next line before reading from the file
	// 2. Navigate to the AuthoriseURL in a browser, but make sure that for the Third party account you're using there's already a created CHS account (same email address)
	// 3. Get the "code" param value from the response URL and drop it in the file below (this file should be next to the Test binary)
	if (!std::filesystem::exists("third_party_auth_token.txt"))
	{
		LogFatal("third_party_auth_token.txt not found! This file must exist and must contain the provider authentication code/token");
	}

	std::ifstream TokenFile;
	TokenFile.open("third_party_auth_token.txt");
	std::string DiscordToken;
	TokenFile >> DiscordToken;

	auto [LoginResult] = AWAIT_PRE(UserSystem, LoginToThirdPartyProvider, RequestPredicate, csp::systems::EThirdPartyAuthenticationProviders::Discord, RedirectURL, csp::common::String(DiscordToken.c_str()), csp::common::String(StateId.c_str()));
	EXPECT_EQ(LoginResult.GetResultCode(), csp::services::EResultCode::Success);
	const auto UserId = LoginResult.GetLoginState()->UserId;

	// test that we are in fact logged in
	auto FullProfile = GetFullProfileByUserId(UserSystem, UserId);

	// Log out
	LogOut(UserSystem);
}
	#endif

	#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_APPLE_LOGIN_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, AppleLogInTest)
{
	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();

	const auto RedirectURL = "https://example-app.com/redirect";

	// Retrieve Authorise URL for Apple
	auto [Result] = AWAIT_PRE(UserSystem, GetThirdPartyProviderAuthoriseURL, RequestPredicate, csp::systems::EThirdPartyAuthenticationProviders::Apple, RedirectURL);
	EXPECT_EQ(Result.GetResultCode(), csp::services::EResultCode::Success);

	//retrieve the StateId from the URL
	const auto& AuthoriseURL = Result.GetValue();
	const auto Tokens = AuthoriseURL.Split('&');
	std::string StateId = ""; 
	for(size_t idx = 0; idx< Tokens.Size(); ++idx)
	{
		std::string URLElement(Tokens[idx].c_str());
		size_t pos = URLElement.find("state=", 0);
		if(pos == 0)
		{
			StateId = URLElement.substr(strlen("state="));
			break;
		}
	}

	std::cerr << "AuthoriseURL: " << AuthoriseURL << std::endl;

	// 1. Set a breakpoint on the next line before reading from the file
	// 2. Navigate to the AuthoriseURL in a browser, but make sure that for the Third party account you're using there's already a created CHS account (same email address)
	// 3. Get the "code" param value from the response POST_FROM and drop it in the file below (this file should be next to the Test binary)
	if (!std::filesystem::exists("third_party_auth_token.txt"))
	{
		LogFatal("third_party_auth_token.txt not found! This file must exist and must contain the provider authentication code/token");
	}

	std::ifstream TokenFile;
	TokenFile.open("third_party_auth_token.txt");
	std::string GoogleToken;
	TokenFile >> GoogleToken;

	auto [LoginResult] = AWAIT_PRE(UserSystem, LoginToThirdPartyProvider, RequestPredicate, csp::systems::EThirdPartyAuthenticationProviders::Google, RedirectURL, csp::common::String(GoogleToken.c_str()), csp::common::String(StateId.c_str()));
	EXPECT_EQ(LoginResult.GetResultCode(), csp::services::EResultCode::Success);
	const auto UserId = LoginResult.GetLoginState()->UserId;

	// test that we are in fact logged in
	auto FullProfile = GetFullProfileByUserId(UserSystem, UserId);

	// Log out
	LogOut(UserSystem);
}
	#endif
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_GET_AGORA_USER_TOKEN_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, GetAgoraUserTokenTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	/*
	  Setup token params
	  For this test, it's exeptable to use 0 for the UserId and ChannelId
	  This is because the endpoint is just using an algorithm to generate the token
	  So no valid Ids are needed for verification
	*/
	csp::systems::AgoraUserTokenParams Params;
	Params.AgoraUserId = "0";
	Params.ChannelName = "0";
	Params.Lifespan	   = 10000;
	Params.ShareAudio  = true;
	Params.ShareScreen = false;
	Params.ShareVideo  = false;
	Params.ReadOnly	   = false;

	// Get token
	auto [Result] = AWAIT_PRE(UserSystem, GetAgoraUserToken, RequestPredicate, Params);

	EXPECT_EQ(Result.GetResultCode(), csp::services::EResultCode::Success);
	EXPECT_NE(Result.GetUserToken(), "");

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_GETPROFILEASGUEST_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, GetGuestProfileTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();

	csp::common::String UserId;
	LogInAsGuest(UserSystem, UserId);

	GetFullProfileByUserId(UserSystem, UserId);

	LogOut(UserSystem);
}
#endif
