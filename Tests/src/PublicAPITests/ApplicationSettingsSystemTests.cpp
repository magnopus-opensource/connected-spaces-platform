/*
 * Copyright 2025 Magnopus LLC

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
#include "CSP/Systems/Settings/ApplicationSettingsSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "RAIIMockLogger.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"

#include "gtest/gtest.h"
#include <gmock/gmock.h>

using namespace csp::common;
using namespace csp::systems;

namespace
{

ApplicationSettings GetApplicationSettingsTestData()
{
    auto Settings = csp::common::Map<csp::common::String, csp::common::String>();
    Settings["TestSettings_1"] = "TestData_1";
    Settings["TestSettings_2"] = "TestData_2";
    Settings["TestSettings_3"] = "TestData_3";
    Settings["TestSettings_4"] = "TestData_4";

    auto ApplicationSettings = ::ApplicationSettings();
    ApplicationSettings.ApplicationName = "MAG_APPLICATION_SETTINGS_TESTS";
    ApplicationSettings.Context = "MAG_APPLICATION_SETTINGS_CONTEXT_TESTS";
    ApplicationSettings.AllowAnonymous = false;
    ApplicationSettings.Settings = Settings;

    return ApplicationSettings;
}

ApplicationSettings GetApplicationSettingsAnonymousTestData()
{
    auto Settings = csp::common::Map<csp::common::String, csp::common::String>();
    Settings["TestSettings_1"] = "TestData_1";
    Settings["TestSettings_2"] = "TestData_2";
    Settings["TestSettings_3"] = "TestData_3";
    Settings["TestSettings_4"] = "TestData_4";

    auto ApplicationSettings = ::ApplicationSettings();
    ApplicationSettings.ApplicationName = "MAG_APPLICATION_SETTINGS_TESTS";
    ApplicationSettings.Context = "MAG_APPLICATION_SETTINGS_ANONTMOUS_CONTEXT_TESTS";
    ApplicationSettings.AllowAnonymous = true;
    ApplicationSettings.Settings = Settings;

    return ApplicationSettings;
}

void AttemptToSeedLocalCHSApplicationSettings()
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* ApplicationSettingsSystem = SystemsManager.GetApplicationSettingsSystem();

    csp::common::String UserId;
    LogInAsLocalAdminUser(UserSystem, UserId);

    // In the event that login fails, we do not want to attempt to seed the service with application settings.
    // It is expected that the login attempt will fail for any deployment that is not the local magnopus services.
    if (UserId.IsEmpty())
        return;

    // Seed application settings test data
    {
        auto ApplicationSettings = GetApplicationSettingsTestData();
        auto [Result] = AWAIT(ApplicationSettingsSystem, CreateSettingsByContext, ApplicationSettings);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Seed anonymous application settings test data
    {
        auto ApplicationSettings = GetApplicationSettingsAnonymousTestData();
        auto [Result] = AWAIT(ApplicationSettingsSystem, CreateSettingsByContext, ApplicationSettings);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Log out
    LogOut(UserSystem);
}

}

CSP_PUBLIC_TEST(CSPEngine, ApplicationSettingsSystemTests, CreateSettingsByContextTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* ApplicationSettingsSystem = SystemsManager.GetApplicationSettingsSystem();

    // Login
    csp::common::String UserId;
    LogInAsLocalAdminUser(UserSystem, UserId);

    // In the event that login fails, we do not want to attempt to create the application settings.
    // It is expected that the login attempt will fail for any deployment that is not the local magnopus services.
    if (UserId.IsEmpty())
        return;

    auto ApplicationSettingsTestData = GetApplicationSettingsAnonymousTestData();

    // Create Application Settings
    {
        RAIIMockLogger MockLogger {};
        csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Log);

        // Set an expectation that the mock logger will receive message for a successful creation of settings by context
        const String GetSettingsByContextMsg = "ApplicationSettingsSystem::CreateSettingsByContext successfully created application settings";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(GetSettingsByContextMsg)).Times(1);

        // Set an expectation that the mock logger will receive message for a successful creation
        const String SendResultMsg = "Successfully created application settings.";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(SendResultMsg)).Times(1);

        auto [Result] = AWAIT(ApplicationSettingsSystem, CreateSettingsByContext, ApplicationSettingsTestData);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const auto ApplicationSettings = Result.GetApplicationSettings();

        EXPECT_EQ(ApplicationSettings.AllowAnonymous, ApplicationSettingsTestData.AllowAnonymous);
        EXPECT_EQ(ApplicationSettings.ApplicationName, ApplicationSettingsTestData.ApplicationName);
        EXPECT_EQ(ApplicationSettings.Context, ApplicationSettingsTestData.Context);
        EXPECT_EQ(ApplicationSettings.Settings, ApplicationSettingsTestData.Settings);
    }

    // Log out
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, ApplicationSettingsSystemTests, CreateSettingsByContextAnonymousTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* ApplicationSettingsSystem = SystemsManager.GetApplicationSettingsSystem();

    // Login
    csp::common::String UserId;
    LogInAsLocalAdminUser(UserSystem, UserId);

    // In the event that login fails, we do not want to attempt to create the application settings.
    // It is expected that the login attempt will fail for any deployment that is not the local magnopus services.
    if (UserId.IsEmpty())
        return;

    auto ApplicationSettingsTestData = GetApplicationSettingsAnonymousTestData();

    // Create Application Settings
    {
        RAIIMockLogger MockLogger {};
        csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Log);

        // Set an expectation that the mock logger will receive message for a successful creation of settings by context
        const String GetSettingsByContextMsg = "ApplicationSettingsSystem::CreateSettingsByContext successfully created application settings";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(GetSettingsByContextMsg)).Times(1);

        // Set an expectation that the mock logger will receive message for a successful creation
        const String SendResultMsg = "Successfully created application settings.";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(SendResultMsg)).Times(1);

        auto [Result] = AWAIT(ApplicationSettingsSystem, CreateSettingsByContext, ApplicationSettingsTestData);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const auto ApplicationSettings = Result.GetApplicationSettings();

        EXPECT_EQ(ApplicationSettings.AllowAnonymous, ApplicationSettingsTestData.AllowAnonymous);
        EXPECT_EQ(ApplicationSettings.ApplicationName, ApplicationSettingsTestData.ApplicationName);
        EXPECT_EQ(ApplicationSettings.Context, ApplicationSettingsTestData.Context);
        EXPECT_EQ(ApplicationSettings.Settings, ApplicationSettingsTestData.Settings);
    }

    // Log out
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, ApplicationSettingsSystemTests, CreateInvalidSettingsByContextTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* ApplicationSettingsSystem = SystemsManager.GetApplicationSettingsSystem();

    // Login
    csp::common::String UserId;
    LogInAsLocalAdminUser(UserSystem, UserId);

    // In the event that login fails, we do not want to attempt to create the application settings.
    // It is expected that the login attempt will fail for any deployment that is not the local magnopus services.
    if (UserId.IsEmpty())
        return;

    auto ApplicationSettingsTestData = ::ApplicationSettings();

    // Create Application Settings
    {
        RAIIMockLogger MockLogger {};
        csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Log);

        // Set an expectation that the mock logger will receive message for a failed result 404 no payload/error message.
        const String GetRequestErrorMsg = "has returned a failed response (404) but with no payload/error message.";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(testing::HasSubstr(GetRequestErrorMsg))).Times(1);

        const String ErrorMsg = "Failed to create application settings";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(ErrorMsg)).Times(1);

        auto [Result] = AWAIT(ApplicationSettingsSystem, CreateSettingsByContext, ApplicationSettingsTestData);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
    }

    // Log out
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, ApplicationSettingsSystemTests, GetSettingsByContextTest)
{
    SetRandSeed();

    // Create Application Settings
    AttemptToSeedLocalCHSApplicationSettings();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* ApplicationSettingsSystem = SystemsManager.GetApplicationSettingsSystem();

    // Login
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    auto const ApplicationSettingsTestData = ::GetApplicationSettingsTestData();

    // Get Application Settings
    {
        RAIIMockLogger MockLogger {};
        csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Log);

        // Set an expectation that the mock logger will receive message for a successful result retrieval of settings by context
        const String GetSettingsByContextMsg = "ApplicationSettingsSystem::GetSettingsByContext successfully retrieved application settings";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(GetSettingsByContextMsg)).Times(1);

        // Set an expectation that the mock logger will receive message for a successful result retrieval
        const String SendResultMsg = "Successfully retrieved application settings.";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(SendResultMsg)).Times(1);

        auto [Result] = AWAIT(ApplicationSettingsSystem, GetSettingsByContext, ApplicationSettingsTestData.ApplicationName,
            ApplicationSettingsTestData.Context, nullptr);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const auto ApplicationSettings = Result.GetApplicationSettings();

        EXPECT_EQ(ApplicationSettings.AllowAnonymous, ApplicationSettingsTestData.AllowAnonymous);
        EXPECT_EQ(ApplicationSettings.ApplicationName, ApplicationSettingsTestData.ApplicationName);
        EXPECT_EQ(ApplicationSettings.Context, ApplicationSettingsTestData.Context);
        EXPECT_EQ(ApplicationSettings.Settings, ApplicationSettingsTestData.Settings);
    }

    // Log out
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, ApplicationSettingsSystemTests, GetSettingsByContextWithKeysTest)
{
    SetRandSeed();

    // Create Application Settings
    AttemptToSeedLocalCHSApplicationSettings();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* ApplicationSettingsSystem = SystemsManager.GetApplicationSettingsSystem();

    // Login
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    auto const ApplicationSettingsTestData = ::GetApplicationSettingsTestData();

    // Get Application Settings
    {
        RAIIMockLogger MockLogger {};
        csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Log);

        // Set an expectation that the mock logger will receive message for a successful result retrieval of settings by context
        const String GetSettingsByContextMsg = "ApplicationSettingsSystem::GetSettingsByContext successfully retrieved application settings";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(GetSettingsByContextMsg)).Times(1);

        // Set an expectation that the mock logger will receive message for a successful result retrieval
        const String SendResultMsg = "Successfully retrieved application settings.";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(SendResultMsg)).Times(1);

        auto Keys = csp::common::Array<csp::common::String>(1);
        Keys[0] = "TestSettings_3";

        auto [Result] = AWAIT(
            ApplicationSettingsSystem, GetSettingsByContext, ApplicationSettingsTestData.ApplicationName, ApplicationSettingsTestData.Context, Keys);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const auto ApplicationSettings = Result.GetApplicationSettings();

        EXPECT_EQ(ApplicationSettings.AllowAnonymous, ApplicationSettingsTestData.AllowAnonymous);
        EXPECT_EQ(ApplicationSettings.ApplicationName, ApplicationSettingsTestData.ApplicationName);
        EXPECT_EQ(ApplicationSettings.Context, ApplicationSettingsTestData.Context);
        EXPECT_EQ(ApplicationSettings.Settings.Size(), Keys.Size());
        EXPECT_EQ(ApplicationSettings.Settings[Keys[0]], "TestData_3");
    }

    // Log out
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, ApplicationSettingsSystemTests, GetInvalidSettingsByContextTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* ApplicationSettingsSystem = SystemsManager.GetApplicationSettingsSystem();

    // Login
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Get Application Settings
    {
        RAIIMockLogger MockLogger {};
        csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Log);

        // Set an expectation that the mock logger will receive message for a failed result 404 no payload/error message.
        const String GetRequestErrorMsg = "has returned a failed response (404) but with no payload/error message.";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(testing::HasSubstr(GetRequestErrorMsg))).Times(1);

        const String ErrorMsg = "Failed to get application settings";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(ErrorMsg)).Times(1);

        auto [Result] = AWAIT(ApplicationSettingsSystem, GetSettingsByContext, GetUniqueString().c_str(), GetUniqueString().c_str(), nullptr);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
    }

    // Log out
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, ApplicationSettingsSystemTests, GetSettingsByContextAnonymousTest)
{
    SetRandSeed();

    // Create Application Settings
    AttemptToSeedLocalCHSApplicationSettings();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* ApplicationSettingsSystem = SystemsManager.GetApplicationSettingsSystem();

    auto const ApplicationSettingsTestData = ::GetApplicationSettingsAnonymousTestData();

    // Get Application Settings
    {
        RAIIMockLogger MockLogger {};
        csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Log);

        // Set an expectation that the mock logger will receive message for a successful result retrieval of settings by context
        const String GetSettingsByContextMsg = "ApplicationSettingsSystem::GetSettingsByContextAnonymous successfully retrieved application settings";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(GetSettingsByContextMsg)).Times(1);

        // Set an expectation that the mock logger will receive message for a successful result retrieval
        const String SendResultMsg = "Successfully retrieved application settings.";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(SendResultMsg)).Times(1);

        auto [Result] = AWAIT(ApplicationSettingsSystem, GetSettingsByContextAnonymous, csp::CSPFoundation::GetTenant(),
            ApplicationSettingsTestData.ApplicationName, ApplicationSettingsTestData.Context, nullptr);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const auto ApplicationSettings = Result.GetApplicationSettings();

        EXPECT_EQ(ApplicationSettings.AllowAnonymous, ApplicationSettingsTestData.AllowAnonymous);
        EXPECT_EQ(ApplicationSettings.ApplicationName, ApplicationSettingsTestData.ApplicationName);
        EXPECT_EQ(ApplicationSettings.Context, ApplicationSettingsTestData.Context);
        EXPECT_EQ(ApplicationSettings.Settings, ApplicationSettingsTestData.Settings);
    }
}

CSP_PUBLIC_TEST(CSPEngine, ApplicationSettingsSystemTests, GetSettingsByContextAnonymousWithKeysTest)
{
    SetRandSeed();

    // Create Application Settings
    AttemptToSeedLocalCHSApplicationSettings();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* ApplicationSettingsSystem = SystemsManager.GetApplicationSettingsSystem();

    auto const ApplicationSettingsTestData = ::GetApplicationSettingsAnonymousTestData();

    // Get Application Settings
    {
        RAIIMockLogger MockLogger {};
        csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Log);

        // Set an expectation that the mock logger will receive message for a successful result retrieval of settings by context
        const String GetSettingsByContextMsg = "ApplicationSettingsSystem::GetSettingsByContextAnonymous successfully retrieved application settings";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(GetSettingsByContextMsg)).Times(1);

        // Set an expectation that the mock logger will receive message for a successful result retrieval
        const String SendResultMsg = "Successfully retrieved application settings.";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(SendResultMsg)).Times(1);

        auto Keys = csp::common::Array<csp::common::String>(1);
        Keys[0] = "TestSettings_3";

        auto [Result] = AWAIT(ApplicationSettingsSystem, GetSettingsByContextAnonymous, csp::CSPFoundation::GetTenant(),
            ApplicationSettingsTestData.ApplicationName, ApplicationSettingsTestData.Context, Keys);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const auto ApplicationSettings = Result.GetApplicationSettings();

        EXPECT_EQ(ApplicationSettings.AllowAnonymous, ApplicationSettingsTestData.AllowAnonymous);
        EXPECT_EQ(ApplicationSettings.ApplicationName, ApplicationSettingsTestData.ApplicationName);
        EXPECT_EQ(ApplicationSettings.Context, ApplicationSettingsTestData.Context);
        EXPECT_EQ(ApplicationSettings.Settings.Size(), Keys.Size());
        EXPECT_EQ(ApplicationSettings.Settings[Keys[0]], "TestData_3");
    }
}

CSP_PUBLIC_TEST(CSPEngine, ApplicationSettingsSystemTests, GetInvalidSettingsByContextAnonymousTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* ApplicationSettingsSystem = SystemsManager.GetApplicationSettingsSystem();

    // Get Application Settings
    {
        RAIIMockLogger MockLogger {};
        csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Log);

        // Set an expectation that the mock logger will receive message for a failed result 404 no payload/error message.
        const String GetRequestErrorMsg = "has returned a failed response (404) but with no payload/error message.";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(testing::HasSubstr(GetRequestErrorMsg))).Times(1);

        const String ErrorMsg = "Failed to get application settings";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(ErrorMsg)).Times(1);

        auto [Result] = AWAIT(ApplicationSettingsSystem, GetSettingsByContextAnonymous, csp::CSPFoundation::GetTenant(), GetUniqueString().c_str(),
            GetUniqueString().c_str(), nullptr);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
    }
}
