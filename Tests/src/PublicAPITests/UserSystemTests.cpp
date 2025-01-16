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
#include "CSP/Systems/Spaces/Space.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/Profile.h"
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

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

csp::common::String SuperUserLoginEmail;
csp::common::String SuperUserLoginPassword;

void LoadTestAccountCredentials()
{
    if (!std::filesystem::exists("test_account_creds.txt"))
    {
        LogFatal("test_account_creds.txt not found! This file must exist and must contain the following information:\n<DefaultLoginEmail> "
                 "<DefaultLoginPassword>\n<AlternativeLoginEmail> <AlternativeLoginPassword>\n<SuperUserLoginEmail> <SuperUserLoginPassword>");
    }

    std::ifstream CredsFile;
    CredsFile.open("test_account_creds.txt");

    std::string _DefaultLoginEmail, _DefaultLoginPassword, _AlternativeLoginEmail, _AlternativeLoginPassword, _SuperUserLoginEmail,
        _SuperUserLoginPassword;

    CredsFile >> _DefaultLoginEmail >> _DefaultLoginPassword;
    CredsFile >> _AlternativeLoginEmail >> _AlternativeLoginPassword;
    CredsFile >> _SuperUserLoginEmail >> _SuperUserLoginPassword;

    if (_DefaultLoginEmail.empty() || _DefaultLoginPassword.empty() || _AlternativeLoginEmail.empty() || _AlternativeLoginPassword.empty()
        || _SuperUserLoginEmail.empty() || _SuperUserLoginPassword.empty())
    {
        LogFatal("test_account_creds.txt must be in the following format:\n<DefaultLoginEmail> <DefaultLoginPassword>\n<AlternativeLoginEmail> "
                 "<AlternativeLoginPassword>\n<SuperUserLoginEmail> <SuperUserLoginPassword>");
    }

    SuperUserLoginEmail = _SuperUserLoginEmail.c_str();
    SuperUserLoginPassword = _SuperUserLoginPassword.c_str();
}

csp::systems::Profile CreateTestUser()
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();

    const char* TestUserName = "CSP-TEST-NAME";
    const char* TestDisplayName = "CSP-TEST-DISPLAY";

    std::string UniqueUserName = TestUserName + GetUniqueString();

    char UniqueEmail[256];
    SPRINTF(UniqueEmail, GeneratedTestAccountEmailFormat, GetUniqueString().c_str());

    // Create new user
    auto [Result] = AWAIT_PRE(UserSystem, CreateUser, RequestPredicate, UniqueUserName.c_str(), TestDisplayName, UniqueEmail,
        GeneratedTestAccountPassword, false, true, nullptr, nullptr);

    SCOPED_TRACE("Failed to create temporary test user in CreateTestUser.");
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    const auto& CreatedProfile = Result.GetProfile();

    SCOPED_TRACE("CreateTestUser returned unexpected details for temporary test user.");
    EXPECT_EQ(CreatedProfile.UserName, UniqueUserName.c_str());
    EXPECT_EQ(CreatedProfile.DisplayName, TestDisplayName);
    EXPECT_EQ(CreatedProfile.Email, UniqueEmail);

    return CreatedProfile;
}

void LogIn(csp::systems::UserSystem* UserSystem, csp::common::String& OutUserId, const csp::common::String& Email,
    const csp::common::String& Password, bool AgeVerified, csp::systems::EResultCode ExpectedResultCode,
    csp::systems::ERequestFailureReason ExpectedResultFailureCode)
{
    auto [Result] = Awaitable(&csp::systems::UserSystem::Login, UserSystem, "", Email, Password, AgeVerified).Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);

    EXPECT_EQ(Result.GetFailureReason(), ExpectedResultFailureCode);

    if (Result.GetResultCode() == csp::systems::EResultCode::Success)
    {
        OutUserId = Result.GetLoginState().UserId;
    }
}

void LogInAsGuest(csp::systems::UserSystem* UserSystem, csp::common::String& OutUserId, csp::systems::EResultCode ExpectedResult)
{
    auto [Result] = Awaitable(&csp::systems::UserSystem::LoginAsGuest, UserSystem, true).Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), ExpectedResult);

    if (Result.GetResultCode() == csp::systems::EResultCode::Success)
    {
        OutUserId = Result.GetLoginState().UserId;
    }
}

void LogInAsNewTestUser(csp::systems::UserSystem* UserSystem, csp::common::String& OutUserId, bool AgeVerified,
    csp::systems::EResultCode ExpectedResultCode, csp::systems::ERequestFailureReason ExpectedResultFailureCode)
{
    csp::systems::Profile NewTestUser = CreateTestUser();

    LogIn(UserSystem, OutUserId, NewTestUser.Email, GeneratedTestAccountPassword, AgeVerified, ExpectedResultCode, ExpectedResultFailureCode);
}

void LogOut(csp::systems::UserSystem* UserSystem, csp::systems::EResultCode ExpectedResultCode)
{
    auto [Result] = Awaitable(&csp::systems::UserSystem::Logout, UserSystem).Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), ExpectedResultCode);
}

csp::systems::Profile GetFullProfileByUserId(csp::systems::UserSystem* UserSystem, const csp::common::String& UserId)
{
    auto [GetProfileResult] = AWAIT_PRE(UserSystem, GetProfileByUserId, RequestPredicate, UserId);
    EXPECT_EQ(GetProfileResult.GetResultCode(), csp::systems::EResultCode::Success);

    return GetProfileResult.GetProfile();
}

void ValidateThirdPartyAuthoriseURL(const csp::common::String& AuthoriseURL, const csp::common::String& RedirectURL)
{
    EXPECT_FALSE(AuthoriseURL.IsEmpty());
    EXPECT_NE(AuthoriseURL, "error");

    const char* STATE_ID_URL_PARAM = "state=";
    const char* CLIENT_ID_URL_PARAM = "client_id=";
    const char* SCOPE_URL_PARAM = "scope=";
    const char* REDIRECT_URL_PARAM = "redirect_uri=";
    const char* INVALID_URL_PARAM_VALUE = "N/A";

    std::string StateId = INVALID_URL_PARAM_VALUE;
    std::string ClientId = INVALID_URL_PARAM_VALUE;
    std::string Scope = INVALID_URL_PARAM_VALUE;
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

    EXPECT_GT(StateId.length(), 0);
    EXPECT_GT(ClientId.length(), 0);
    EXPECT_GE(Scope.length(), 0);
    EXPECT_EQ(RetrievedRedirectURL, RedirectURL.c_str());
}

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_FORGOTPASSWORD_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, ForgotPasswordTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();

    // Tests passing false for UseTokenChangePasswordUrl
    auto [Result] = AWAIT_PRE(UserSystem, UserSystem::ForgotPassword, RequestPredicate, "testnopus.pokemon@magnopus.com", nullptr, nullptr, false);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    auto [Result2] = AWAIT_PRE(UserSystem, UserSystem::ForgotPassword, RequestPredicate, "testnopus.pokemon@magnopus.com", nullptr, nullptr, false);

    EXPECT_EQ(Result2.GetResultCode(), csp::systems::EResultCode::Success);

    auto [FailResult] = AWAIT_PRE(UserSystem, UserSystem::ForgotPassword, RequestPredicate, "email", nullptr, nullptr, false);

    EXPECT_EQ(FailResult.GetResultCode(), csp::systems::EResultCode::Failed);

    // Tests passing true for UseTokenChangePasswordUrl
    auto [Result3] = AWAIT_PRE(UserSystem, UserSystem::ForgotPassword, RequestPredicate, "testnopus.pokemon@magnopus.com", nullptr, nullptr, true);

    EXPECT_EQ(Result3.GetResultCode(), csp::systems::EResultCode::Success);

    auto [Result4] = AWAIT_PRE(UserSystem, UserSystem::ForgotPassword, RequestPredicate, "testnopus.pokemon+1@magnopus.com", nullptr, nullptr, true);

    EXPECT_EQ(Result4.GetResultCode(), csp::systems::EResultCode::Success);

    auto [FailResult2] = AWAIT_PRE(UserSystem, UserSystem::ForgotPassword, RequestPredicate, "email", nullptr, nullptr, true);

    EXPECT_EQ(FailResult2.GetResultCode(), csp::systems::EResultCode::Failed);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_RESETPASSWORD_BADTOKEN_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, ResetPasswordBadTokenTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();

    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    auto [Result] = AWAIT_PRE(UserSystem, UserSystem::ResetUserPassword, RequestPredicate, "badtoken", UserId, "NewPassword");

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_LOGIN_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, LogInTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();

    csp::common::String UserId;

    // Create test user
    csp::systems::Profile TestUser = CreateTestUser();

    // Log in
    LogIn(UserSystem, UserId, TestUser.Email, GeneratedTestAccountPassword);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_LOGINASNEWTESTUSER_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, LogInAsNewTestUserTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_LOGIN_AS_GUEST_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, LogInAsGuestTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();

    csp::common::String UserId;

    // Log in
    LogInAsGuest(UserSystem, UserId);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_BADTOKENLOGIN_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, BadTokenLogInTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();

    csp::common::String UserId;

    // Log in to get UserId
    LogInAsNewTestUser(UserSystem, UserId);

    // Log out
    LogOut(UserSystem);

    // Log in
    auto [Result] = AWAIT_PRE(UserSystem, LoginWithRefreshToken, RequestPredicate, UserId, "badtoken");

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
    EXPECT_EQ(Result.GetFailureReason(), csp::systems::ERequestFailureReason::UserTokenRefreshFailed);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_BADLOGOUT_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, BadLogOutTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();

    csp::common::String UserId;

    // Log out without logging in first
    LogOut(UserSystem, csp::systems::EResultCode::Failed);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_BADDUALLOGIN_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, BadDualLoginTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();

    csp::common::String UserId;

    // Create test user
    csp::systems::Profile TestUser = CreateTestUser();

    // Log in
    LogIn(UserSystem, UserId, TestUser.Email, GeneratedTestAccountPassword);

    // Attempt to log in again
    LogIn(UserSystem, UserId, TestUser.Email, GeneratedTestAccountPassword, true, csp::systems::EResultCode::Failed);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_LOGINERROR_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, LoginErrorTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();

    csp::common::String UserId;

    // Log in with invalid credentials
    LogIn(UserSystem, UserId, "invalidlogin@rewind.co", "", true, csp::systems::EResultCode::Failed);

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Log out
    LogOut(UserSystem);
}
#endif

// This will be updated and re-instated in OF-1533
#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_REFRESH_TEST
CSP_PUBLIC_TEST(DISABLED_CSPEngine, UserSystemTests, RefreshTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

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
    auto* UserSystem = SystemsManager.GetUserSystem();

    csp::common::String UniqueTestDisplayName = csp::common::String("TEST") + GetUniqueString().substr(0, 16).c_str();

    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Update display name
    {
        auto [Result] = AWAIT_PRE(UserSystem, UpdateUserDisplayName, RequestPredicate, UserId, UniqueTestDisplayName);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Retrieve user profile and verify display name has been updated
    {
        auto FullProfile = GetFullProfileByUserId(UserSystem, UserId);

        EXPECT_EQ(FullProfile.UserId, UserId);
        EXPECT_EQ(FullProfile.DisplayName, UniqueTestDisplayName);
    }

    // Attempt Update - bad display name
    {
        UniqueTestDisplayName = csp::common::String("??//-\"#~*") + GetUniqueString().c_str();

        auto [Result] = AWAIT_PRE(UserSystem, UpdateUserDisplayName, RequestPredicate, UserId, UniqueTestDisplayName);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
    }

    // Attempt update other user
    {
        UniqueTestDisplayName = csp::common::String("Test") + GetUniqueString().c_str();

        auto [Result] = AWAIT_PRE(UserSystem, UpdateUserDisplayName, RequestPredicate, "6551f988dd6b04c1e99a71b8", UniqueTestDisplayName);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
    }

    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_UPDATE_DISPLAY_NAME_INCLUDING_BLANK_SPACES_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, UpdateDisplayNameIncludingBlankSpacesTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();

    csp::common::String UniqueTestDisplayName = csp::common::String("TEST ") + GetUniqueString().substr(0, 16).c_str();

    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Update display name
    {
        auto [Result] = AWAIT_PRE(UserSystem, UpdateUserDisplayName, RequestPredicate, UserId, UniqueTestDisplayName);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
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
    auto* UserSystem = SystemsManager.GetUserSystem();

    csp::common::String UniqueTestDisplayName = csp::common::String("()= - ") + GetUniqueString().substr(0, 8).c_str();

    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Update display name
    {
        auto [Result] = AWAIT_PRE(UserSystem, UpdateUserDisplayName, RequestPredicate, UserId, UniqueTestDisplayName);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
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
    auto* UserSystem = SystemsManager.GetUserSystem();

    // check that ping function returns success and doesn't timeout
    auto [Result] = AWAIT_PRE(UserSystem, Ping, RequestPredicate);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_CREATE_USER_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, CreateUserTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SettingsSystem = SystemsManager.GetSettingsSystem();

    const char* TestUserName = "CSP-TEST-NAME";
    const char* TestDisplayName = "CSP-TEST-DISPLAY";

    char UniqueUserName[256];
    SPRINTF(UniqueUserName, "%s-%s", TestUserName, GetUniqueString().c_str());

    char UniqueEmail[256];
    SPRINTF(UniqueEmail, GeneratedTestAccountEmailFormat, GetUniqueString().c_str());

    csp::common::String CreatedUserId;

    // Create new user
    {
        auto [Result] = AWAIT_PRE(UserSystem, CreateUser, RequestPredicate, UniqueUserName, TestDisplayName, UniqueEmail,
            GeneratedTestAccountPassword, true, true, nullptr, nullptr);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const auto& CreatedProfile = Result.GetProfile();
        CreatedUserId = CreatedProfile.UserId;

        EXPECT_EQ(CreatedProfile.UserName, UniqueUserName);
        EXPECT_EQ(CreatedProfile.DisplayName, TestDisplayName);
        EXPECT_EQ(CreatedProfile.Email, UniqueEmail);
    }

    csp::common::String UserId;
    LogIn(UserSystem, UserId, UniqueEmail, GeneratedTestAccountPassword);

    // At this point, the created account is already verified automatically because of the tenant used,
    // so we can retrieve a lite profile
    {
        csp::common::Array<csp::common::String> Ids = { CreatedUserId };
        auto [Result] = AWAIT_PRE(UserSystem, GetBasicProfilesByUserId, RequestPredicate, Ids);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        auto LiteProfile = Result.GetProfiles()[0];

        EXPECT_EQ(LiteProfile.UserId, CreatedUserId);
        EXPECT_EQ(LiteProfile.DisplayName, TestDisplayName);
    }

    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_DELETE_USER_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, DeleteUserTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SettingsSystem = SystemsManager.GetSettingsSystem();

    const char* TestUserName = "CSP-TEST-NAME";
    const char* TestDisplayName = "CSP-TEST-DISPLAY";

    char UniqueUserName[256];
    SPRINTF(UniqueUserName, "%s-%s-%s", TestUserName, GetUniqueString().c_str(), GetUniqueString().c_str());

    char UniqueEmail[256];
    SPRINTF(UniqueEmail, GeneratedTestAccountEmailFormat, GetUniqueString().c_str());

    csp::common::String CreatedUserId;

    // Create new user
    csp::systems::Profile CreatedProfile = CreateTestUser();
    CreatedUserId = CreatedProfile.UserId;

    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Whilst logged in as new test account attempt (and fail) to delete original user
    {
        auto [Result] = AWAIT_PRE(UserSystem, DeleteUser, RequestPredicate, CreatedUserId);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
    }

    LogOut(UserSystem);

    csp::common::String OriginalUserId;
    LogIn(UserSystem, OriginalUserId, CreatedProfile.Email, GeneratedTestAccountPassword);

    // Whilst logged in as created account attempt to delete self
    {
        auto [Result] = AWAIT_PRE(UserSystem, DeleteUser, RequestPredicate, CreatedUserId);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_CREATE_USER_EMPTY_USERNAME_DISPLAYNAME_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, CreateUserEmptyUsernameDisplaynameTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();

    const char* TestUserName = "CSP-TEST-NAME";

    char UniqueEmail[256];
    SPRINTF(UniqueEmail, GeneratedTestAccountEmailFormat, GetUniqueString().c_str());

    csp::common::String CreatedUserId;

    // Create new user
    {
        csp::systems::Profile CreatedProfile = CreateTestUser();
        CreatedUserId = CreatedProfile.UserId;
    }

    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Retrieve the lite profile
    {
        csp::common::Array<csp::common::String> Ids = { CreatedUserId };
        auto [Result] = AWAIT_PRE(UserSystem, GetBasicProfilesByUserId, RequestPredicate, Ids);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        auto LiteProfile = Result.GetProfiles()[0];

        EXPECT_EQ(LiteProfile.UserId, CreatedUserId);
        EXPECT_FALSE(LiteProfile.DisplayName.IsEmpty());
    }

    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_GET_SUPPORTED_PROVIDERS_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, GetThirdPartySupportedProvidersTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();

    // Check the FDN supported providers
    auto SupportedProviders = UserSystem->GetSupportedThirdPartyAuthenticationProviders();
    EXPECT_EQ(SupportedProviders.Size(), 3L);

    bool FoundGoogle = false;
    bool FoundDiscord = false;
    bool FoundApple = false;
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
    auto* UserSystem = SystemsManager.GetUserSystem();

    const auto RedirectURL = "https://dev.magnoverse.space/oauth";

    // Retrieve Authorise URL for Google
    auto [ResGoogle] = AWAIT_PRE(
        UserSystem, GetThirdPartyProviderAuthoriseURL, RequestPredicate, csp::systems::EThirdPartyAuthenticationProviders::Google, RedirectURL);
    EXPECT_EQ(ResGoogle.GetResultCode(), csp::systems::EResultCode::Success);

    const auto& AuthoriseURL = ResGoogle.GetValue();
    ValidateThirdPartyAuthoriseURL(AuthoriseURL, RedirectURL);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_GET_AUTHORISE_URL_FOR_DISCORD_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, GetAuthoriseURLForDiscordTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();

    const auto RedirectURL = "https://dev.magnoverse.space/oauth";

    // Retrieve Authorise URL for Discord
    auto [Result] = AWAIT_PRE(
        UserSystem, GetThirdPartyProviderAuthoriseURL, RequestPredicate, csp::systems::EThirdPartyAuthenticationProviders::Discord, RedirectURL);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    const auto& AuthoriseURL = Result.GetValue();
    ValidateThirdPartyAuthoriseURL(AuthoriseURL, RedirectURL);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_GET_AUTHORISE_URL_FOR_APPLE_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, GetAuthoriseURLForAppleTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();

    const auto RedirectURL = "https://dev.magnoverse.space/oauth";

    // Retrieve Authorise URL for Apple
    auto [Result] = AWAIT_PRE(
        UserSystem, GetThirdPartyProviderAuthoriseURL, RequestPredicate, csp::systems::EThirdPartyAuthenticationProviders::Apple, RedirectURL);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

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

	const auto RedirectURL = "https://dev.magnoverse.space/oauth";

	// Retrieve Authorise URL for Google
	auto [Result] = AWAIT_PRE(UserSystem, GetThirdPartyProviderAuthoriseURL, RequestPredicate, csp::systems::EThirdPartyAuthenticationProviders::Google, RedirectURL);
	EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

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
	EXPECT_EQ(LoginResult.GetResultCode(), csp::systems::EResultCode::Success);
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

	const auto RedirectURL = "https://dev.magnoverse.space/oauth";

	// Retrieve Authorise URL for Google
	auto [Result] = AWAIT_PRE(UserSystem, GetThirdPartyProviderAuthoriseURL, RequestPredicate, csp::systems::EThirdPartyAuthenticationProviders::Discord, RedirectURL);
	EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

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
	EXPECT_EQ(LoginResult.GetResultCode(), csp::systems::EResultCode::Success);
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
	EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

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
	EXPECT_EQ(LoginResult.GetResultCode(), csp::systems::EResultCode::Success);
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
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
    const char* TestAssetCollectionName = "OLY-UNITTEST-ASSETCOLLECTION-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    char UniqueAssetCollectionName[256];
    SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    csp::systems::AgoraUserTokenParams Params;
    Params.AgoraUserId = UserId;
    Params.ChannelName = Space.Id;
    Params.ReferenceId = Space.Id;
    Params.Lifespan = 10000;
    Params.ShareAudio = true;
    Params.ShareScreen = false;
    Params.ShareVideo = false;
    Params.ReadOnly = false;

    // Get token
    auto [Result] = AWAIT_PRE(UserSystem, GetAgoraUserToken, RequestPredicate, Params);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_FALSE(Result.GetValue().IsEmpty());

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_GETPROFILEASGUEST_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, GetGuestProfileTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();

    csp::common::String UserId;
    LogInAsGuest(UserSystem, UserId);

    auto Profile = GetFullProfileByUserId(UserSystem, UserId);

    EXPECT_EQ(Profile.Email, "");
    EXPECT_EQ(Profile.CreatedBy, "");
    EXPECT_EQ(Profile.IsEmailConfirmed, false);

    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_AGE_NOT_VERIFIED_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, AgeNotVerifiedTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();

    csp::common::String UserId;

    // Create test user
    csp::systems::Profile TestUser = CreateTestUser();

    // False Log in
    LogIn(UserSystem, UserId, TestUser.Email, GeneratedTestAccountPassword, false, csp::systems::EResultCode::Failed,
        csp::systems::ERequestFailureReason::UserAgeNotVerified);

    // null Log in
    // does not use login helper function as the login helper function defaults to false.
    auto [Result]
        = Awaitable(&csp::systems::UserSystem::Login, UserSystem, "", TestUser.Email, GeneratedTestAccountPassword, nullptr).Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    EXPECT_EQ(Result.GetFailureReason(), csp::systems::ERequestFailureReason::None);

    LogOut(UserSystem);

    // true Log in
    LogIn(UserSystem, UserId, TestUser.Email, GeneratedTestAccountPassword, true, csp::systems::EResultCode::Success,
        csp::systems::ERequestFailureReason::None);

    LogOut(UserSystem);
}
#endif

// Currently disabled whilst stripe testing is unavailable for OKO_TESTS
// This test will be reviewed and reinstated as part of OF-1534.
#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_CUSTOMER_PORTAL_URL_TEST
CSP_PUBLIC_TEST(DISABLED_CSPEngine, UserSystemTests, GetCustomerPortalUrlTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();

    csp::common::String UserId;

    // Create test user
    csp::systems::Profile TestUser = CreateTestUser();

    // False Log in
    LogIn(UserSystem, UserId, TestUser.Email, GeneratedTestAccountPassword, true, csp::systems::EResultCode::Success,
        csp::systems::ERequestFailureReason::None);

    auto [Result] = AWAIT_PRE(UserSystem, GetCustomerPortalUrl, RequestPredicate, UserId);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    EXPECT_EQ(Result.GetFailureReason(), csp::systems::ERequestFailureReason::None);

    EXPECT_NE(Result.GetValue(), "");
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_USERSYSTEM_TESTS || RUN_USERSYSTEM_CHECKOUT_SESSION_URL_TEST
CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, GetCheckoutSessionUrlTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();

    csp::common::String UserId;

    // Create test user
    csp::systems::Profile TestUser = CreateTestUser();

    // False Log in
    LogIn(UserSystem, UserId, TestUser.Email, GeneratedTestAccountPassword, true, csp::systems::EResultCode::Success,
        csp::systems::ERequestFailureReason::None);

    auto [Result] = AWAIT_PRE(UserSystem, GetCheckoutSessionUrl, RequestPredicate, csp::systems::TierNames::Pro);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    EXPECT_EQ(Result.GetFailureReason(), csp::systems::ERequestFailureReason::None);

    EXPECT_NE(Result.GetValue(), "");
}
#endif