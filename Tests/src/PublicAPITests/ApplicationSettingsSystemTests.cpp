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

ApplicationSettings GetApplicationSettingsTestData(const csp::common::String& Context, const bool AllowAnonymous)
{
    auto Settings = csp::common::Map<csp::common::String, csp::common::String>();
    Settings["TestSettings_1"] = "TestData_1";
    Settings["TestSettings_2"] = "TestData_2";
    Settings["TestSettings_3"] = "TestData_3";
    Settings["TestSettings_4"] = "TestData_4";

    auto ApplicationSettings = ::ApplicationSettings();
    ApplicationSettings.ApplicationName = "MAG_APPLICATION_SETTINGS_TESTS";
    ApplicationSettings.Context = Context;
    ApplicationSettings.AllowAnonymous = AllowAnonymous;
    ApplicationSettings.Settings = Settings;

    return ApplicationSettings;
}

void SeedApplicationSettings()
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* ApplicationSettingsSystem = SystemsManager.GetApplicationSettingsSystem();

    csp::common::String UserId;
    LogInAsAdminUser(UserSystem, UserId);

    // Seed application settings test data
    {
        auto ApplicationSettings = GetApplicationSettingsTestData("MAG_APPLICATION_SETTINGS_CONTEXT_TESTS", false);
        auto [Result] = AWAIT(ApplicationSettingsSystem, CreateSettingsByContext, ApplicationSettings);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    }

    // Seed anonymous application settings test data
    {
        auto ApplicationSettings = GetApplicationSettingsTestData("MAG_APPLICATION_SETTINGS_ANONTMOUS_CONTEXT_TESTS", true);
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
    LogInAsAdminUser(UserSystem, UserId);

    auto ApplicationSettingsTestData = GetApplicationSettingsTestData("MAG_APPLICATION_SETTINGS_CONTEXT_TESTS", false);

    // Create Application Settings
    {
        RAIIMockLogger MockLogger {};
        csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Log);

        // Set an expectation that the mock logger will receive message for a successful creation of settings by context
        const String GetSettingsByContextMsg = "ApplicationSettingsSystem::CreateSettingsByContext successfully created application settings";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(GetSettingsByContextMsg)).Times(1);

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

CSP_PUBLIC_TEST(CSPEngine, ApplicationSettingsSystemTests, CreateAnonymousSettingsByContextTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* ApplicationSettingsSystem = SystemsManager.GetApplicationSettingsSystem();

    // Login
    csp::common::String UserId;
    LogInAsAdminUser(UserSystem, UserId);

    auto ApplicationSettingsTestData = GetApplicationSettingsTestData("MAG_APPLICATION_SETTINGS_ANONTMOUS_CONTEXT_TESTS", true);

    // Create Application Settings
    {
        RAIIMockLogger MockLogger {};
        csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Log);

        // Set an expectation that the mock logger will receive message for a successful creation of settings by context
        const String GetSettingsByContextMsg = "ApplicationSettingsSystem::CreateSettingsByContext successfully created application settings";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(GetSettingsByContextMsg)).Times(1);

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
    LogInAsAdminUser(UserSystem, UserId);

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
    SeedApplicationSettings();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* ApplicationSettingsSystem = SystemsManager.GetApplicationSettingsSystem();

    // Login
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    auto const ApplicationSettingsTestData = GetApplicationSettingsTestData("MAG_APPLICATION_SETTINGS_CONTEXT_TESTS", false);

    // Get Application Settings
    {
        RAIIMockLogger MockLogger {};
        csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Log);

        // Set an expectation that the mock logger will receive message for a successful result retrieval of settings by context
        const String GetSettingsByContextMsg = "ApplicationSettingsSystem::GetSettingsByContext successfully retrieved application settings";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(GetSettingsByContextMsg)).Times(1);

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
    SeedApplicationSettings();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* ApplicationSettingsSystem = SystemsManager.GetApplicationSettingsSystem();

    // Login
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    auto const ApplicationSettingsTestData = GetApplicationSettingsTestData("MAG_APPLICATION_SETTINGS_CONTEXT_TESTS", false);

    // Get Application Settings
    {
        RAIIMockLogger MockLogger {};
        csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Log);

        // Set an expectation that the mock logger will receive message for a successful result retrieval of settings by context
        const String GetSettingsByContextMsg = "ApplicationSettingsSystem::GetSettingsByContext successfully retrieved application settings";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(GetSettingsByContextMsg)).Times(1);

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
    SeedApplicationSettings();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* ApplicationSettingsSystem = SystemsManager.GetApplicationSettingsSystem();

    auto const ApplicationSettingsTestData = GetApplicationSettingsTestData("MAG_APPLICATION_SETTINGS_ANONTMOUS_CONTEXT_TESTS", true);

    // Get Application Settings
    {
        RAIIMockLogger MockLogger {};
        csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Log);

        // Set an expectation that the mock logger will receive message for a successful result retrieval of settings by context
        const String GetSettingsByContextMsg = "ApplicationSettingsSystem::GetSettingsByContextAnonymous successfully retrieved application settings";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(GetSettingsByContextMsg)).Times(1);

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
    SeedApplicationSettings();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* ApplicationSettingsSystem = SystemsManager.GetApplicationSettingsSystem();

    auto const ApplicationSettingsTestData = GetApplicationSettingsTestData("MAG_APPLICATION_SETTINGS_ANONTMOUS_CONTEXT_TESTS", true);

    // Get Application Settings
    {
        RAIIMockLogger MockLogger {};
        csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Log);

        // Set an expectation that the mock logger will receive message for a successful result retrieval of settings by context
        const String GetSettingsByContextMsg = "ApplicationSettingsSystem::GetSettingsByContextAnonymous successfully retrieved application settings";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(GetSettingsByContextMsg)).Times(1);

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

CSP_PUBLIC_TEST(CSPEngine, ApplicationSettingsSystemTests, GetInvalidTentantSettingsByContextAnonymousTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* ApplicationSettingsSystem = SystemsManager.GetApplicationSettingsSystem();

    auto ApplicationSettingsTestData = GetApplicationSettingsTestData("MAG_APPLICATION_SETTINGS_ANONTMOUS_CONTEXT_TESTS", true);

    // Get Application Settings
    {
        RAIIMockLogger MockLogger {};
        csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Log);

        // Set an expectation that the mock logger will receive message for a failed result 404 no payload/error message.
        const String GetRequestErrorMsg = "has returned a failed response (404) but with no payload/error message.";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(testing::HasSubstr(GetRequestErrorMsg))).Times(1);

        const String ErrorMsg = "Failed to get application settings";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(ErrorMsg)).Times(1);

        auto [Result] = AWAIT(ApplicationSettingsSystem, GetSettingsByContextAnonymous, "", ApplicationSettingsTestData.ApplicationName,
            ApplicationSettingsTestData.Context, nullptr);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
    }
}
