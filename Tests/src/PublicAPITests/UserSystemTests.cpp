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
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Systems/Settings/SettingsSystem.h"
#include "CSP/Systems/Spaces/Space.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/Profile.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "Common/DateTime.h"
#include "Common/Web/HttpPayload.h"
#include "RAIIMockLogger.h"
#include "SpaceSystemTestHelpers.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"

#include "gtest/gtest.h"
#include <filesystem>
#include <fstream>
#include <future>

using namespace std::chrono_literals;

namespace
{

bool RequestPredicate(const csp::systems::ResultBase& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

csp::systems::Profile CreateTestUser()
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    const char* testDisplayName = "CSP-TEST-DISPLAY";

    char uniqueEmail[256];
    SPRINTF(uniqueEmail, GeneratedTestAccountEmailFormat, GetUniqueString().c_str());

    // Create new user
    auto [Result] = AWAIT_PRE(
        userSystem, CreateUser, RequestPredicate, testDisplayName, uniqueEmail, GeneratedTestAccountPassword, false, true, nullptr, nullptr);

    SCOPED_TRACE("Failed to create temporary test user in CreateTestUser.");
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    const auto& createdProfile = Result.GetProfile();

    SCOPED_TRACE("CreateTestUser returned unexpected details for temporary test user.");
    EXPECT_EQ(createdProfile.DisplayName, testDisplayName);
    EXPECT_EQ(createdProfile.Email, uniqueEmail);

    return createdProfile;
}

void LogIn(csp::systems::UserSystem* userSystem, csp::common::String& outUserId, const csp::common::String& email,
    const csp::common::String& password, bool createMultiplayerConnection, bool ageVerified, const csp::systems::TokenOptions& tokenOptions,
    csp::systems::EResultCode expectedResultCode, csp::systems::ERequestFailureReason expectedResultFailureCode)
{
    auto [Result] = Awaitable(&csp::systems::UserSystem::Login, userSystem, email, password, createMultiplayerConnection, ageVerified, tokenOptions)
                        .Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), expectedResultCode);

    EXPECT_EQ(Result.GetFailureReason(), expectedResultFailureCode);

    if (Result.GetResultCode() == csp::systems::EResultCode::Success)
    {
        outUserId = Result.GetLoginState().UserId;
    }
}

void LogInAsGuest(csp::systems::UserSystem* userSystem, csp::common::String& outUserId, bool createMultiplayerConnection,
    const csp::systems::TokenOptions& tokenOptions, csp::systems::EResultCode expectedResult)
{
    auto [Result]
        = Awaitable(&csp::systems::UserSystem::LoginAsGuest, userSystem, createMultiplayerConnection, true, tokenOptions).Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), expectedResult);

    if (Result.GetResultCode() == csp::systems::EResultCode::Success)
    {
        outUserId = Result.GetLoginState().UserId;
    }
}

void LogInAsGuestWithDeferredProfileCreation(
    csp::systems::UserSystem* userSystem, csp::common::String& outUserId, csp::systems::EResultCode expectedResult)
{
    auto [Result] = Awaitable(&csp::systems::UserSystem::LoginAsGuestWithDeferredProfileCreation, userSystem, true).Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), expectedResult);

    if (Result.GetResultCode() == csp::systems::EResultCode::Success)
    {
        outUserId = Result.GetLoginState().UserId;
    }
}

void LogInAsNewTestUser(csp::systems::UserSystem* userSystem, csp::common::String& outUserId, bool createMultiplayerConnection, bool ageVerified,
    csp::systems::TokenOptions tokenOptions, csp::systems::EResultCode expectedResultCode,
    csp::systems::ERequestFailureReason expectedResultFailureCode)
{
    csp::systems::Profile newTestUser = CreateTestUser();

    LogIn(userSystem, outUserId, newTestUser.Email, GeneratedTestAccountPassword, createMultiplayerConnection, ageVerified, tokenOptions,
        expectedResultCode, expectedResultFailureCode);
}

void LogInAsAdminUser(csp::systems::UserSystem* userSystem, csp::common::String& outUserId, bool createMultiplayerConnection, bool ageVerified,
    csp::systems::TokenOptions tokenOptions, csp::systems::EResultCode expectedResultCode,
    csp::systems::ERequestFailureReason expectedResultFailureCode)
{
    // Attempt to log in with an admin account with elevated permissions.
    LogIn(userSystem, outUserId, AdminAccountEmail(), AdminAccountPassword(), createMultiplayerConnection, ageVerified, tokenOptions,
        expectedResultCode, expectedResultFailureCode);
}

void LogOut(csp::systems::UserSystem* userSystem, csp::systems::EResultCode expectedResultCode)
{
    auto [Result] = Awaitable(&csp::systems::UserSystem::Logout, userSystem).Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), expectedResultCode);
}

csp::systems::Profile GetFullProfileByUserId(csp::systems::UserSystem* userSystem, const csp::common::String& userId)
{
    auto [GetProfileResult] = AWAIT_PRE(userSystem, GetProfileByUserId, RequestPredicate, userId);
    EXPECT_EQ(GetProfileResult.GetResultCode(), csp::systems::EResultCode::Success);

    return GetProfileResult.GetProfile();
}

struct AuthorizeURLTokens
{
    std::string StateId;
    std::string ClientId;
    std::string Scope;
    std::string RetrievedRedirectURL;
};

AuthorizeURLTokens ExtractTokensFromAuthorizeURL(const csp::common::String& authorizeUrl)
{
    AuthorizeURLTokens urlTokens;
    
    const char* stateIdUrlParam = "state=";
    const char* clientIdUrlParam = "client_id=";
    const char* scopeUrlParam = "scope=";
    const char* redirectUrlParam = "redirect_uri=";
    const char* invalidUrlParamValue = "N/A";

    urlTokens.StateId = invalidUrlParamValue;
    urlTokens.ClientId = invalidUrlParamValue;
    urlTokens.Scope = invalidUrlParamValue;
    urlTokens.RetrievedRedirectURL = invalidUrlParamValue;

    const auto& queryParams = authorizeUrl.Split('&');
    for (size_t idx = 0; idx < queryParams.Size(); ++idx)
    {
        std::string urlElement(queryParams[idx].c_str());
        if (urlElement.find(stateIdUrlParam, 0) == 0)
        {
            urlTokens.StateId = urlElement.substr(strlen(stateIdUrlParam));
            continue;
        }
        else if (urlElement.find(clientIdUrlParam, 0) == 0)
        {
            urlTokens.ClientId = urlElement.substr(strlen(clientIdUrlParam));
            continue;
        }
        else if (urlElement.find(scopeUrlParam, 0) == 0)
        {
            urlTokens.Scope = urlElement.substr(strlen(scopeUrlParam));
            continue;
        }
        else if (urlElement.find(redirectUrlParam, 0) == 0)
        {
            urlTokens.RetrievedRedirectURL = urlElement.substr(strlen(redirectUrlParam));
            continue;
        }
    }

    const auto newTokens = queryParams[0].Split('?');
    EXPECT_EQ(newTokens.Size(), 2);

    std::string urlElement(newTokens[1].c_str());
    if (urlElement.find(clientIdUrlParam, 0) == 0)
    {
        urlTokens.ClientId = urlElement.substr(strlen(clientIdUrlParam));
    }

    return urlTokens;
}

void ValidateThirdPartyAuthorizeURL(const AuthorizeURLTokens& urlTokens, const csp::common::String& redirectUrl)
{
    const char* invalidUrlParamValue = "N/A";

    // validate that the following contain something that potentially makes sense
    EXPECT_NE(urlTokens.StateId, invalidUrlParamValue);
    EXPECT_NE(urlTokens.ClientId, invalidUrlParamValue);
    EXPECT_NE(urlTokens.Scope, invalidUrlParamValue);
    EXPECT_NE(urlTokens.RetrievedRedirectURL, invalidUrlParamValue);

    EXPECT_GT(urlTokens.StateId.length(), 0);
    EXPECT_GT(urlTokens.ClientId.length(), 0);
    EXPECT_GE(urlTokens.Scope.length(), 0);

    EXPECT_EQ(urlTokens.RetrievedRedirectURL, redirectUrl.c_str());
}

CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, ForgotPasswordTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    // Tests passing false for UseTokenChangePasswordUrl
    auto [Result] = AWAIT_PRE(userSystem, UserSystem::ForgotPassword, RequestPredicate, "testnopus.pokemon@magnopus.com", nullptr, nullptr, false);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    auto [Result2] = AWAIT_PRE(userSystem, UserSystem::ForgotPassword, RequestPredicate, "testnopus.pokemon@magnopus.com", nullptr, nullptr, false);

    EXPECT_EQ(Result2.GetResultCode(), csp::systems::EResultCode::Success);

    auto [FailResult] = AWAIT_PRE(userSystem, UserSystem::ForgotPassword, RequestPredicate, "email", nullptr, nullptr, false);

    EXPECT_EQ(FailResult.GetResultCode(), csp::systems::EResultCode::Failed);

    // Tests passing true for UseTokenChangePasswordUrl
    auto [Result3] = AWAIT_PRE(userSystem, UserSystem::ForgotPassword, RequestPredicate, "testnopus.pokemon@magnopus.com", nullptr, nullptr, true);

    EXPECT_EQ(Result3.GetResultCode(), csp::systems::EResultCode::Success);

    auto [Result4] = AWAIT_PRE(userSystem, UserSystem::ForgotPassword, RequestPredicate, "testnopus.pokemon+1@magnopus.com", nullptr, nullptr, true);

    EXPECT_EQ(Result4.GetResultCode(), csp::systems::EResultCode::Success);

    auto [FailResult2] = AWAIT_PRE(userSystem, UserSystem::ForgotPassword, RequestPredicate, "email", nullptr, nullptr, true);

    EXPECT_EQ(FailResult2.GetResultCode(), csp::systems::EResultCode::Failed);
}

CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, ResetPasswordBadTokenTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    auto [Result] = AWAIT_PRE(userSystem, UserSystem::ResetUserPassword, RequestPredicate, "badtoken", userId, "NewPassword");

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
}

CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, LogInTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    csp::common::String userId;

    // Create test user
    csp::systems::Profile testUser = CreateTestUser();

    // Log in
    LogIn(userSystem, userId, testUser.Email, GeneratedTestAccountPassword);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, LogInAsNewTestUserTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, LogInAsGuestTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    csp::common::String userId;

    // Log in
    LogInAsGuest(userSystem, userId);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, LogInAsGuestDeferredProfileCreationTest)
{
    // We can't really test more than this. This deferred endpoint explicitly provides
    // no guarentees of actually creating a profile in any sort of timely manner.
    // Any subsequent failures to query endpoints would be within the bounds of permitted behaviour.

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    csp::common::String userId;

    // Log in
    LogInAsGuestWithDeferredProfileCreation(userSystem, userId);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, BadTokenLogInTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    csp::common::String userId;

    // Log in to get UserId
    LogInAsNewTestUser(userSystem, userId);

    // Log out
    LogOut(userSystem);

    // Log in
    auto [Result] = AWAIT_PRE(userSystem, LoginWithRefreshToken, RequestPredicate, userId, "badtoken", true, nullptr);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
    EXPECT_EQ(Result.GetFailureReason(), csp::systems::ERequestFailureReason::UserTokenRefreshFailed);
}

CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, EmptyUserCredentialsTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    const csp::common::String expectedErrorLog = "UserSystem::Login, Email must not be empty.";
    bool callbackCalled = false;

    csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
        [&callbackCalled, &expectedErrorLog](csp::common::LogLevel, const csp::common::String& message)
        {
            callbackCalled = true;
            EXPECT_EQ(expectedErrorLog, message);
        });

    csp::common::String userId;
    // Log in with empty email
    auto [Result]
        = Awaitable(&csp::systems::UserSystem::Login, userSystem, "", GeneratedTestAccountPassword, true, true, nullptr).Await(RequestPredicate);

    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(Result.GetHttpResultCode(), 0);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

    csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
}

CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, EmptyPasswordCredentialsTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    const csp::common::String expectedErrorLog = "UserSystem::Login, Password must not be empty.";
    bool callbackCalled = false;

    csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
        [&callbackCalled, &expectedErrorLog](csp::common::LogLevel, const csp::common::String& message)
        {
            callbackCalled = true;
            EXPECT_EQ(expectedErrorLog, message);
        });

    csp::common::String userId;
    // Log in with empty password
    auto [Result] = Awaitable(&csp::systems::UserSystem::Login, userSystem, "afakeemail@email.com", "", true, true, nullptr).Await(RequestPredicate);

    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(Result.GetHttpResultCode(), 0);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

    csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
}

CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, EmptyUserCredentialsRefreshTokenLoginTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    const csp::common::String expectedErrorLog = "UserSystem::LoginWithRefreshToken, UserId must not be empty.";
    bool callbackCalled = false;

    csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(
        [&callbackCalled, &expectedErrorLog](csp::common::LogLevel, const csp::common::String& message)
        {
            callbackCalled = true;
            EXPECT_EQ(expectedErrorLog, message);
        });

    csp::common::String userId;
    // Log in with empty userId
    auto [Result] = AWAIT_PRE(userSystem, LoginWithRefreshToken, RequestPredicate, "", "fakeToken", true, nullptr);

    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(Result.GetHttpResultCode(), 0);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);

    csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr);
}

CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, BadLogOutTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    csp::common::String userId;

    // Log out without logging in first
    LogOut(userSystem, csp::systems::EResultCode::Failed);
}

CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, BadDualLoginTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    csp::common::String userId;

    // Create test user
    csp::systems::Profile testUser = CreateTestUser();

    // Log in
    LogIn(userSystem, userId, testUser.Email, GeneratedTestAccountPassword);

    // Attempt to log in again
    LogIn(userSystem, userId, testUser.Email, GeneratedTestAccountPassword, true, true, csp::systems::TokenOptions(),
        csp::systems::EResultCode::Failed);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, LoginErrorTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    csp::common::String userId;

    // Log in with invalid credentials
    LogIn(userSystem, userId, "invalidlogin@rewind.co", "", true, true, csp::systems::TokenOptions(), csp::systems::EResultCode::Failed);

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, RefreshTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    csp::common::String userId;

    // Log in
    auto tokenOptions = csp::systems::TokenOptions();
    tokenOptions.AccessTokenExpiryLength = "00:00:05";

    LogInAsNewTestUser(userSystem, userId, true, true, tokenOptions);

    bool tokenHasBeenRefreshed = false;
    // Ensure that the token is refresh when attempting to make a authenticated call after expiry
    userSystem->SetNewLoginTokenReceivedCallback(
        [&tokenHasBeenRefreshed](const csp::systems::LoginTokenInfoResult& result)
        {
            EXPECT_EQ(result.GetResultCode(), csp::systems::EResultCode::Success);

            tokenHasBeenRefreshed = true;
        });

    std::this_thread::sleep_for(10s);

    auto profile = GetFullProfileByUserId(userSystem, userId);

    EXPECT_EQ(tokenHasBeenRefreshed, true);

    // Log out
    LogOut(userSystem);
}

// Currently disabled pending investigation from MCS related to intermittent failures at short duration for refresh token expiry.
// Refer to CHS-5441 Investigate why refresh tokens with 5 second expiry only consistently fail after 120 seconds for more information.
CSP_PUBLIC_TEST(DISABLED_CSPEngine, UserSystemTests, RefreshTokenFailedTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    auto tokenOptions = csp::systems::TokenOptions();
    tokenOptions.AccessTokenExpiryLength = "00:00:05";
    tokenOptions.RefreshTokenExpiryLength = "00:00:05";

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId, true, true, tokenOptions);

    RAIIMockLogger mockLogger {};
    csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(csp::common::LogLevel::Fatal);
    csp::common::String msg = "User authentication token refresh failed!";
    EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Fatal, msg)).Times(1);

    std::this_thread::sleep_for(120s);

    // We expect this call to fail as we no longer have a valid connection, hence we do not need to logout
    AWAIT_PRE(userSystem, GetProfileByUserId, RequestPredicate, userId);
}

CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, ValidExpiryLengthInTokenOptionsTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    auto tokenOptions = csp::systems::TokenOptions();
    tokenOptions.AccessTokenExpiryLength = "00:00:30";

    std::promise<csp::systems::LoginTokenInfoResult> tokenPromise;
    std::future<csp::systems::LoginTokenInfoResult> tokenFuture = tokenPromise.get_future();

    userSystem->SetNewLoginTokenReceivedCallback(
        [&tokenPromise](const csp::systems::LoginTokenInfoResult& result)
        {
            tokenPromise.set_value(result);
        });

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId, true, true, tokenOptions);

    ASSERT_EQ(tokenFuture.wait_for(30s), std::future_status::ready) << "NewLoginTokenReceivedCallback was not invoked";

    const auto result = tokenFuture.get();
    EXPECT_EQ(result.GetResultCode(), csp::systems::EResultCode::Success);

    const auto tokenInfo = result.GetLoginTokenInfo();
    const auto accessExpiryTime = csp::common::DateTime(tokenInfo.AccessExpiryTime);
    const auto currentTime = csp::common::DateTime::UtcTimeNow();

    // Calculate the delta from the available time points as we do not support duration in DateTime
    const auto delta = accessExpiryTime.GetTimePoint() - currentTime.GetTimePoint();
    EXPECT_LE(delta, 40s);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, InvalidExpiryLengthInTokenOptionsTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    systemsManager.GetLogSystem()->SetSystemLevel(csp::common::LogLevel::Warning);

    RAIIMockLogger mockLogger {};
    csp::common::String warningLog = "Expiry length token option does not match the expected format, and has been ignored.";
    EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Warning, warningLog)).Times(2);

    auto tokenOptions = csp::systems::TokenOptions();
    tokenOptions.AccessTokenExpiryLength = "INVALID_EXPIRATION_DURATION_STRING";
    tokenOptions.RefreshTokenExpiryLength = "00:60:60";

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId, true, true, tokenOptions);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, ExpiryLengthInTokenOptionsOutOfRangeTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    systemsManager.GetLogSystem()->SetSystemLevel(csp::common::LogLevel::Warning);
    auto tokenOptions = csp::systems::TokenOptions();

    // Ensure MCS clamps the value of expiry length when above bounds
    {
        tokenOptions.AccessTokenExpiryLength = "12:00:00";

        // Ensure that the token expiry time matched the default token options
        userSystem->SetNewLoginTokenReceivedCallback(
            [](const csp::systems::LoginTokenInfoResult& result)
            {
                EXPECT_EQ(result.GetResultCode(), csp::systems::EResultCode::Success);

                const auto tokenInfo = result.GetLoginTokenInfo();

                const csp::common::DateTime currentDateTime = csp::common::DateTime::UtcTimeNow();
                const std::chrono::system_clock::time_point timeFuture = currentDateTime.GetTimePoint() + std::chrono::system_clock::duration(2h);

                const csp::common::DateTime futureDateTime(timeFuture);
                const csp::common::DateTime expiryDateTime(tokenInfo.AccessExpiryTime);

                ASSERT_GE(futureDateTime, expiryDateTime);
            });

        // Log in
        csp::common::String userId;
        LogInAsNewTestUser(userSystem, userId, true, true, tokenOptions);

        // Log out
        LogOut(userSystem);
    }

    // Ensure MCS clamps the value of expiry length when below bounds
    {
        tokenOptions.AccessTokenExpiryLength = "00:00:00";

        // Ensure that the token expiry time matched the default token options
        userSystem->SetNewLoginTokenReceivedCallback(
            [](const csp::systems::LoginTokenInfoResult& result)
            {
                EXPECT_EQ(result.GetResultCode(), csp::systems::EResultCode::Success);

                const auto tokenInfo = result.GetLoginTokenInfo();

                const csp::common::DateTime currentDateTime = csp::common::DateTime::UtcTimeNow();
                const std::chrono::system_clock::time_point timeFuture = currentDateTime.GetTimePoint() + std::chrono::system_clock::duration(5min);

                const csp::common::DateTime futureDateTime(timeFuture);
                const csp::common::DateTime expiryDateTime(tokenInfo.AccessExpiryTime);

                ASSERT_GE(expiryDateTime, futureDateTime);
            });

        // Log in
        csp::common::String userId;
        LogInAsNewTestUser(userSystem, userId, true, true, tokenOptions);

        // Log out
        LogOut(userSystem);
    }
}

CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, UpdateDisplayNameTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    csp::common::String uniqueTestDisplayName = csp::common::String("TEST") + GetUniqueString().substr(0, 16).c_str();

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Update display name
    {
        auto [Result] = AWAIT_PRE(userSystem, UpdateUserDisplayName, RequestPredicate, userId, uniqueTestDisplayName);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Retrieve user profile and verify display name has been updated
    {
        auto fullProfile = GetFullProfileByUserId(userSystem, userId);

        EXPECT_EQ(fullProfile.UserId, userId);
        EXPECT_EQ(fullProfile.DisplayName, uniqueTestDisplayName);
    }

    // Attempt Update - bad display name
    {
        // Using \? to prevent a trigraph error on gcc.
        uniqueTestDisplayName = csp::common::String("?\?//-\"#~*") + GetUniqueString().c_str();

        auto [Result] = AWAIT_PRE(userSystem, UpdateUserDisplayName, RequestPredicate, userId, uniqueTestDisplayName);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
    }

    // Attempt update other user
    {
        uniqueTestDisplayName = csp::common::String("Test") + GetUniqueString().c_str();

        auto [Result] = AWAIT_PRE(userSystem, UpdateUserDisplayName, RequestPredicate, "6551f988dd6b04c1e99a71b8", uniqueTestDisplayName);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
    }

    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, UpdateDisplayNameIncludingBlankSpacesTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    csp::common::String uniqueTestDisplayName = csp::common::String("TEST ") + GetUniqueString().substr(0, 16).c_str();

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Update display name
    {
        auto [Result] = AWAIT_PRE(userSystem, UpdateUserDisplayName, RequestPredicate, userId, uniqueTestDisplayName);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Retrieve user profile and verify display name has been updated
    {
        auto fullProfile = GetFullProfileByUserId(userSystem, userId);

        EXPECT_EQ(fullProfile.UserId, userId);
        EXPECT_EQ(fullProfile.DisplayName, uniqueTestDisplayName);
    }

    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, UpdateDisplayNameIncludingSymbolsTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    csp::common::String uniqueTestDisplayName = csp::common::String("()= - ") + GetUniqueString().substr(0, 8).c_str();

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Update display name
    {
        auto [Result] = AWAIT_PRE(userSystem, UpdateUserDisplayName, RequestPredicate, userId, uniqueTestDisplayName);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Retrieve user profile and verify display name has been updated
    {
        auto fullProfile = GetFullProfileByUserId(userSystem, userId);

        EXPECT_EQ(fullProfile.UserId, userId);
        EXPECT_EQ(fullProfile.DisplayName, uniqueTestDisplayName);
    }

    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, PingTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    // check that ping function returns success and doesn't timeout
    auto [Result] = AWAIT_PRE(userSystem, Ping, RequestPredicate);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
}

CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, CreateUserTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    const char* testDisplayName = "CSP-TEST-DISPLAY";

    char uniqueEmail[256];
    SPRINTF(uniqueEmail, GeneratedTestAccountEmailFormat, GetUniqueString().c_str());

    csp::common::String createdUserId;

    // Create new user
    {
        auto [Result] = AWAIT_PRE(
            userSystem, CreateUser, RequestPredicate, testDisplayName, uniqueEmail, GeneratedTestAccountPassword, true, true, nullptr, nullptr);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const auto& createdProfile = Result.GetProfile();
        createdUserId = createdProfile.UserId;

        EXPECT_EQ(createdProfile.DisplayName, testDisplayName);
        EXPECT_EQ(createdProfile.Email, uniqueEmail);
    }

    csp::common::String userId;
    LogIn(userSystem, userId, uniqueEmail, GeneratedTestAccountPassword);

    // At this point, the created account is already verified automatically because of the tenant used,
    // so we can retrieve a lite profile
    {
        csp::common::Array<csp::common::String> ids = { createdUserId };
        auto [Result] = AWAIT_PRE(userSystem, GetBasicProfilesByUserId, RequestPredicate, ids);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        auto liteProfile = Result.GetProfiles()[0];

        EXPECT_EQ(liteProfile.UserId, createdUserId);
        EXPECT_EQ(liteProfile.DisplayName, testDisplayName);
    }

    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, DeleteUserTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    const char* testUserName = "CSP-TEST-NAME";

    char uniqueUserName[256];
    SPRINTF(uniqueUserName, "%s-%s-%s", testUserName, GetUniqueString().c_str(), GetUniqueString().c_str());

    char uniqueEmail[256];
    SPRINTF(uniqueEmail, GeneratedTestAccountEmailFormat, GetUniqueString().c_str());

    csp::common::String createdUserId;

    // Create new user
    csp::systems::Profile createdProfile = CreateTestUser();
    createdUserId = createdProfile.UserId;

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Whilst logged in as new test account attempt (and fail) to delete original user
    {
        auto [Result] = AWAIT_PRE(userSystem, DeleteUser, RequestPredicate, createdUserId);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
    }

    LogOut(userSystem);

    csp::common::String originalUserId;
    LogIn(userSystem, originalUserId, createdProfile.Email, GeneratedTestAccountPassword);

    // Whilst logged in as created account attempt to delete self
    {
        auto [Result] = AWAIT_PRE(userSystem, DeleteUser, RequestPredicate, createdUserId);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, CreateUserEmptyUsernameDisplaynameTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    char uniqueEmail[256];
    SPRINTF(uniqueEmail, GeneratedTestAccountEmailFormat, GetUniqueString().c_str());

    csp::common::String createdUserId;

    // Create new user
    {
        csp::systems::Profile createdProfile = CreateTestUser();
        createdUserId = createdProfile.UserId;
    }

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Retrieve the lite profile
    {
        csp::common::Array<csp::common::String> ids = { createdUserId };
        auto [Result] = AWAIT_PRE(userSystem, GetBasicProfilesByUserId, RequestPredicate, ids);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        auto liteProfile = Result.GetProfiles()[0];

        EXPECT_EQ(liteProfile.UserId, createdUserId);
        EXPECT_FALSE(liteProfile.DisplayName.IsEmpty());
    }

    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, GetThirdPartySupportedProvidersTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    // Check the FDN supported providers
    auto supportedProviders = userSystem->GetSupportedThirdPartyAuthenticationProviders();
    EXPECT_EQ(supportedProviders.Size(), 3L);

    bool foundGoogle = false;
    bool foundDiscord = false;
    bool foundApple = false;
    for (size_t idx = 0; idx < supportedProviders.Size(); ++idx)
    {
        if (supportedProviders[idx] == csp::systems::EThirdPartyAuthenticationProviders::Google)
        {
            foundGoogle = true;
        }
        else if (supportedProviders[idx] == csp::systems::EThirdPartyAuthenticationProviders::Discord)
        {
            foundDiscord = true;
        }
        else if (supportedProviders[idx] == csp::systems::EThirdPartyAuthenticationProviders::Apple)
        {
            foundApple = true;
        }
        else
        {
            ASSERT_TRUE(false) << "Please update this test with this new FDN auth provider: " << supportedProviders[idx];
        }
    }

    EXPECT_TRUE(foundGoogle && foundDiscord && foundApple);
}

CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, GetAuthorizeURLForGoogleTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    const auto redirectUrl = "https://dev.magnoverse.space/oauth";

    // Retrieve Authorize URL for Google
    auto [ResGoogle] = AWAIT_PRE(userSystem, GetThirdPartyProviderAuthorizeURL, RequestPredicate,
        csp::systems::EThirdPartyAuthenticationProviders::Google, redirectUrl, nullptr);
    EXPECT_EQ(ResGoogle.GetResultCode(), csp::systems::EResultCode::Success);

    const auto& authorizeUrl = ResGoogle.GetValue();

    AuthorizeURLTokens tokens = ExtractTokensFromAuthorizeURL(authorizeUrl);
    ValidateThirdPartyAuthorizeURL(tokens, redirectUrl);
}

CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, GetAuthorizeURLForGoogleTestWithClient)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    const auto redirectUrl = "https://dev.magnoverse.space/oauth";

    csp::systems::EThirdPartyPlatform client = csp::systems::EThirdPartyPlatform::Unity;

    // Retrieve Authorize URL for Google
    auto [ResGoogle] = AWAIT_PRE(userSystem, GetThirdPartyProviderAuthorizeURL, RequestPredicate,
        csp::systems::EThirdPartyAuthenticationProviders::Google, redirectUrl, client);
    EXPECT_EQ(ResGoogle.GetResultCode(), csp::systems::EResultCode::Success);

    const auto& authorizeUrl = ResGoogle.GetValue();

    AuthorizeURLTokens tokens = ExtractTokensFromAuthorizeURL(authorizeUrl);
    ValidateThirdPartyAuthorizeURL(tokens, redirectUrl);
}

CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, GetAuthorizeURLForDiscordTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    const auto redirectUrl = "https://dev.magnoverse.space/oauth";

    // Retrieve Authorize URL for Discord
    auto [Result] = AWAIT_PRE(userSystem, GetThirdPartyProviderAuthorizeURL, RequestPredicate,
        csp::systems::EThirdPartyAuthenticationProviders::Discord, redirectUrl, nullptr);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    const auto& authorizeUrl = Result.GetValue();

    AuthorizeURLTokens tokens = ExtractTokensFromAuthorizeURL(authorizeUrl);
    ValidateThirdPartyAuthorizeURL(tokens, redirectUrl);
}

CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, GetAuthorizeURLForAppleTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    const auto redirectUrl = "https://dev.magnoverse.space/oauth";

    // Retrieve Authorize URL for Apple
    auto [Result] = AWAIT_PRE(userSystem, GetThirdPartyProviderAuthorizeURL, RequestPredicate,
        csp::systems::EThirdPartyAuthenticationProviders::Apple, redirectUrl, nullptr);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    const auto& authorizeUrl = Result.GetValue();

    AuthorizeURLTokens tokens = ExtractTokensFromAuthorizeURL(authorizeUrl);
    ValidateThirdPartyAuthorizeURL(tokens, redirectUrl);
}

// As the following three tests require manual actions explained inside, they are currently disabled
// ATM only the WASM tests would be able to have a end-to-end testing flow using Selenium for the URL redirects
#if 0
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

CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, GetGuestProfileTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    csp::common::String userId;
    LogInAsGuest(userSystem, userId);

    auto profile = GetFullProfileByUserId(userSystem, userId);

    EXPECT_EQ(profile.Email, "");
    EXPECT_EQ(profile.CreatedBy, "");
    EXPECT_EQ(profile.IsEmailConfirmed, false);

    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, AgeNotVerifiedTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    csp::common::String userId;

    // Create test user
    csp::systems::Profile testUser = CreateTestUser();

    // False Log in
    LogIn(userSystem, userId, testUser.Email, GeneratedTestAccountPassword, true, false, csp::systems::TokenOptions(),
        csp::systems::EResultCode::Failed, csp::systems::ERequestFailureReason::UserAgeNotVerified);

    // null Log in
    // does not use login helper function as the login helper function defaults to false.
    auto [Result] = Awaitable(&csp::systems::UserSystem::Login, userSystem, testUser.Email, GeneratedTestAccountPassword, true, true, nullptr)
                        .Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    EXPECT_EQ(Result.GetFailureReason(), csp::systems::ERequestFailureReason::None);

    LogOut(userSystem);

    // true Log in
    LogIn(userSystem, userId, testUser.Email, GeneratedTestAccountPassword, true, true, csp::systems::TokenOptions(),
        csp::systems::EResultCode::Success, csp::systems::ERequestFailureReason::None);

    LogOut(userSystem);
}

// Currently disabled whilst stripe testing is unavailable for OKO_TESTS
// This test will be reviewed and reinstated as part of OF-1534.
CSP_PUBLIC_TEST(DISABLED_CSPEngine, UserSystemTests, GetCustomerPortalUrlTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    csp::common::String userId;

    // Create test user
    csp::systems::Profile testUser = CreateTestUser();

    // False Log in
    LogIn(userSystem, userId, testUser.Email, GeneratedTestAccountPassword, true, true, csp::systems::TokenOptions(),
        csp::systems::EResultCode::Success, csp::systems::ERequestFailureReason::None);

    auto [Result] = AWAIT_PRE(userSystem, GetCustomerPortalUrl, RequestPredicate, userId);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    EXPECT_EQ(Result.GetFailureReason(), csp::systems::ERequestFailureReason::None);

    EXPECT_NE(Result.GetValue(), "");
}

CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, GetCheckoutSessionUrlTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    csp::common::String userId;

    // Create test user
    csp::systems::Profile testUser = CreateTestUser();

    // False Log in
    LogIn(userSystem, userId, testUser.Email, GeneratedTestAccountPassword, true, true, csp::systems::TokenOptions(),
        csp::systems::EResultCode::Success, csp::systems::ERequestFailureReason::None);

    auto [Result] = AWAIT_PRE(userSystem, GetCheckoutSessionUrl, RequestPredicate, csp::systems::TierNames::Pro);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    EXPECT_EQ(Result.GetFailureReason(), csp::systems::ERequestFailureReason::None);

    EXPECT_NE(Result.GetValue(), "");
}

CSP_PUBLIC_TEST(CSPEngine, UserSystemTests, DefaultApplicationSettingsTest)
{
    if (std::string(EndpointBaseURI()).find(":8081") != std::string::npos)
    {
        // Skip if we're running on Local MCS. This is hopefully a temporary hack as CHS do intend that this data be seeded in local MCS, it just
        // hasn't managed to take quite yet. Doing this to unblock the work.
        GTEST_SKIP() << "Default application settings not seeded on local MCS";
    }

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    csp::common::String userId;

    csp::systems::Profile testUser = CreateTestUser();

    std::promise<csp::common::LoginState> settingsPromise;
    std::future<csp::common::LoginState> settingsFuture = settingsPromise.get_future();

    userSystem->Login(testUser.Email, GeneratedTestAccountPassword, false, true, {},
        [&settingsPromise](const csp::systems::LoginStateResult& result) { settingsPromise.set_value(result.GetLoginState()); });

    csp::common::LoginState loginState = settingsFuture.get();

    // OKO_TESTS Tenant has a default applications settings setup. All these values are arbitrary just for tests
    ASSERT_EQ(loginState.DefaultApplicationSettings.Size(), 1);
    ASSERT_EQ(loginState.DefaultSettings.Size(), 0);

    csp::common::ApplicationSettings applicationSetting = loginState.DefaultApplicationSettings[0];
    ASSERT_EQ(applicationSetting.AllowAnonymous, false);
    ASSERT_EQ(applicationSetting.Context, "checkpoint");
    // application name not listed because it uses a project codename which is secret ... it's six characters long though :P Ooooh what could it be?
    ASSERT_EQ(applicationSetting.ApplicationName.Length(), 6);

    ASSERT_EQ(applicationSetting.Settings.Size(), 1);
    ASSERT_TRUE(applicationSetting.Settings.HasKey("URL"));
    ASSERT_EQ(applicationSetting.Settings["URL"], "https://www.google.com/search?q=why+google");
}