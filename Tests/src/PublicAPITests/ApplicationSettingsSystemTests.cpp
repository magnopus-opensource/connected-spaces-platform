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
#include "Common/Convert.h"
#include "RAIIMockLogger.h"
#include "Services/UserService/UserServiceApiMock.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"
#include "Json/JsonSerializer.h"

#include "gtest/gtest.h"
#include <future>
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

}

CSP_PUBLIC_TEST(CSPEngine, ApplicationSettingsSystemTests, CreateSettingsByContextTest)
{
    if (std::string(AdminAccountEmail()).empty())
    {
        GTEST_SKIP() << "Admin account email not set. This test cannot be run.";
    }

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
        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Log, GetSettingsByContextMsg)).Times(1);

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
    if (std::string(AdminAccountEmail()).empty())
    {
        GTEST_SKIP() << "Admin account email not set. This test cannot be run.";
    }

    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* ApplicationSettingsSystem = SystemsManager.GetApplicationSettingsSystem();

    // Login
    csp::common::String UserId;
    LogInAsAdminUser(UserSystem, UserId);

    auto ApplicationSettingsTestData = GetApplicationSettingsTestData("MAG_APPLICATION_SETTINGS_ANONYMOUS_CONTEXT_TESTS", true);

    // Create Application Settings
    {
        RAIIMockLogger MockLogger {};
        csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Log);

        // Set an expectation that the mock logger will receive message for a successful creation of settings by context
        const String GetSettingsByContextMsg = "ApplicationSettingsSystem::CreateSettingsByContext successfully created application settings";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Log, GetSettingsByContextMsg)).Times(1);

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

CSP_PUBLIC_TEST(CSPEngine, ApplicationSettingsSystemTests, GetSettingsByContextTest)
{
    if (std::string(AdminAccountEmail()).empty())
    {
        GTEST_SKIP() << "Admin account email not set. This test cannot be run.";
    }

    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* ApplicationSettingsSystem = SystemsManager.GetApplicationSettingsSystem();

    // Seed application settings test data
    {
        csp::common::String UserId;
        LogInAsAdminUser(UserSystem, UserId);

        auto ApplicationSettings = GetApplicationSettingsTestData("MAG_APPLICATION_SETTINGS_CONTEXT_TESTS", false);
        auto [Result] = AWAIT(ApplicationSettingsSystem, CreateSettingsByContext, ApplicationSettings);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        // Log out
        LogOut(UserSystem);
    }

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
        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Log, GetSettingsByContextMsg)).Times(1);

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
    if (std::string(AdminAccountEmail()).empty())
    {
        GTEST_SKIP() << "Admin account email not set. This test cannot be run.";
    }

    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* ApplicationSettingsSystem = SystemsManager.GetApplicationSettingsSystem();

    // Seed application settings test data
    {
        csp::common::String UserId;
        LogInAsAdminUser(UserSystem, UserId);

        auto ApplicationSettings = GetApplicationSettingsTestData("MAG_APPLICATION_SETTINGS_CONTEXT_TESTS", false);
        auto [Result] = AWAIT(ApplicationSettingsSystem, CreateSettingsByContext, ApplicationSettings);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        // Log out
        LogOut(UserSystem);
    }

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
        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Log, GetSettingsByContextMsg)).Times(1);

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
        EXPECT_CALL(MockLogger.MockLogCallback, Call(testing::_, testing::HasSubstr(GetRequestErrorMsg))).Times(1);

        const String ErrorMsg = "Failed to get application settings";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(testing::_, ErrorMsg)).Times(1);

        auto [Result] = AWAIT(ApplicationSettingsSystem, GetSettingsByContext, GetUniqueString().c_str(), GetUniqueString().c_str(), nullptr);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
    }

    // Log out
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, ApplicationSettingsSystemTests, GetSettingsByContextAnonymousTest)
{
    if (std::string(AdminAccountEmail()).empty())
    {
        GTEST_SKIP() << "Admin account email not set. This test cannot be run.";
    }

    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* ApplicationSettingsSystem = SystemsManager.GetApplicationSettingsSystem();

    // Seed application settings test data
    {
        csp::common::String UserId;
        LogInAsAdminUser(UserSystem, UserId);

        auto ApplicationSettings = GetApplicationSettingsTestData("MAG_APPLICATION_SETTINGS_ANONYMOUS_CONTEXT_TESTS", true);
        auto [Result] = AWAIT(ApplicationSettingsSystem, CreateSettingsByContext, ApplicationSettings);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        // Log out
        LogOut(UserSystem);
    }

    auto const ApplicationSettingsTestData = GetApplicationSettingsTestData("MAG_APPLICATION_SETTINGS_ANONYMOUS_CONTEXT_TESTS", true);

    // Get Application Settings
    {
        RAIIMockLogger MockLogger {};
        csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Log);

        // Set an expectation that the mock logger will receive message for a successful result retrieval of settings by context
        const String GetSettingsByContextMsg = "ApplicationSettingsSystem::GetSettingsByContextAnonymous successfully retrieved application settings";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Log, GetSettingsByContextMsg)).Times(1);

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
    if (std::string(AdminAccountEmail()).empty())
    {
        GTEST_SKIP() << "Admin account email not set. This test cannot be run.";
    }

    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* ApplicationSettingsSystem = SystemsManager.GetApplicationSettingsSystem();

    // Seed application settings test data
    {
        csp::common::String UserId;
        LogInAsAdminUser(UserSystem, UserId);

        auto ApplicationSettings = GetApplicationSettingsTestData("MAG_APPLICATION_SETTINGS_ANONYMOUS_CONTEXT_TESTS", true);
        auto [Result] = AWAIT(ApplicationSettingsSystem, CreateSettingsByContext, ApplicationSettings);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        // Log out
        LogOut(UserSystem);
    }

    auto const ApplicationSettingsTestData = GetApplicationSettingsTestData("MAG_APPLICATION_SETTINGS_ANONYMOUS_CONTEXT_TESTS", true);

    // Get Application Settings
    {
        RAIIMockLogger MockLogger {};
        csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Log);

        // Set an expectation that the mock logger will receive message for a successful result retrieval of settings by context
        const String GetSettingsByContextMsg = "ApplicationSettingsSystem::GetSettingsByContextAnonymous successfully retrieved application settings";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Log, GetSettingsByContextMsg)).Times(1);

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
        EXPECT_CALL(MockLogger.MockLogCallback, Call(testing::_, testing::HasSubstr(GetRequestErrorMsg))).Times(1);

        const String ErrorMsg = "Failed to get application settings";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(testing::_, ErrorMsg)).Times(1);

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

    auto ApplicationSettingsTestData = GetApplicationSettingsTestData("MAG_APPLICATION_SETTINGS_ANONYMOUS_CONTEXT_TESTS", true);

    // Get Application Settings
    {
        RAIIMockLogger MockLogger {};
        csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Log);

        // Set an expectation that the mock logger will receive message for a failed result 404 no payload/error message.
        const String GetRequestErrorMsg = "has returned a failed response (404) but with no payload/error message.";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(testing::_, testing::HasSubstr(GetRequestErrorMsg))).Times(1);

        const String ErrorMsg = "Failed to get application settings";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(testing::_, ErrorMsg)).Times(1);

        auto [Result] = AWAIT(ApplicationSettingsSystem, GetSettingsByContextAnonymous, GetUniqueString().c_str(),
            ApplicationSettingsTestData.ApplicationName, ApplicationSettingsTestData.Context, nullptr);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
    }
}

namespace chs = csp::services::generated::userservice;

CSP_PUBLIC_TEST(CSPEngine, ApplicationSettingsSystemMockTests, WhenApplicationSettingsPutResponseCreatedThenRecieveSuccessResponseTests)
{
    const auto ApplicationSettingsMock = std::make_unique<chs::ApplicationSettingsApiMock>();

    std::promise<ApplicationSettingsResult> ResultPromise;
    std::future<ApplicationSettingsResult> ResultFuture = ResultPromise.get_future();

    // Create default application settings expected data, use to validate and construct the return response for mock.
    auto ApplicationSettings = csp::common::ApplicationSettings();
    ApplicationSettings.ApplicationName = "MockApplicationName";
    ApplicationSettings.Context = "MockContext";
    ApplicationSettings.AllowAnonymous = false;
    ApplicationSettings.Settings = { { "MockTestSettings", "MockTestData" } };

    // Sets the expectation that a specific method on a mock object will be called, and will fail the test if these conditions are not met.
    EXPECT_CALL(*ApplicationSettingsMock, applicationsApplicationNameSettingsContextPut)
        .WillOnce(
            [&](const chs::ApplicationSettingsApiMock::applicationsApplicationNameSettingsContextPutParams& Params,
                csp::services::ApiResponseHandlerBase* ResponseHandler, csp::common::CancellationToken& /*CancellationToken*/
            )
            {
                // Basic validation that the information provided matches expectations.
                EXPECT_EQ(ApplicationSettings.ApplicationName, Params.applicationName);
                EXPECT_EQ(ApplicationSettings.Context, Params.context);

                // Construct the payload using the ApplicationSettings to populate the request body for the response.
                csp::web::HttpPayload Payload;
                auto json = csp::json::JsonSerializer::Serialize(ApplicationSettings);
                Payload.AddHeader(CSP_TEXT("Content-Type"), CSP_TEXT("application/json"));
                Payload.SetContent(json);

                // Construct the response with the expected HTTP response code and payload.
                auto Response = csp::web::HttpResponse();
                Response.SetResponseCode(csp::web::EResponseCodes::ResponseCreated);
                Response.GetMutablePayload() = Payload;

                // Invoke the response on the handler to emulate the RESTful call.
                ResponseHandler->OnHttpResponse(Response);
            });

    // Create a callback to capture the response to fulfill the promise.
    auto Callback = [&ResultPromise](const ApplicationSettingsResult& Result) { ResultPromise.set_value(Result); };

    // Create a handler for the current mock function, which will allow emulation of the RESTful response in the EXPECT_CALL.
    auto ResponseHandler
        = ApplicationSettingsMock->CreateHandler<ApplicationSettingsResultCallback, ApplicationSettingsResult, void, chs::ApplicationSettingsDto>(
            Callback, nullptr, csp::web::EResponseCodes::ResponseCreated);

    auto Request = std::make_shared<chs::ApplicationSettingsDto>();
    Request->SetAllowAnonymous(ApplicationSettings.AllowAnonymous);
    Request->SetSettings(Convert(ApplicationSettings.Settings));

    auto Params
        = chs::ApplicationSettingsApiMock::applicationsApplicationNameSettingsContextPutParams { "MockApplicationName", "MockContext", Request };

    // Call the expected mock function to trigger the expected call and fulfill the promise.
    ApplicationSettingsMock->applicationsApplicationNameSettingsContextPut(Params, ResponseHandler, CancellationToken::Dummy());

    auto Result = ResultFuture.get();
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(Result.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseCreated));
    EXPECT_EQ(Result.GetFailureReason(), csp::systems::ERequestFailureReason::None);

    const auto ApplicationSettingsResult = Result.GetApplicationSettings();
    EXPECT_EQ(ApplicationSettingsResult.ApplicationName, ApplicationSettings.ApplicationName);
    EXPECT_EQ(ApplicationSettingsResult.Context, ApplicationSettings.Context);
    EXPECT_EQ(ApplicationSettingsResult.AllowAnonymous, ApplicationSettings.AllowAnonymous);
    EXPECT_EQ(ApplicationSettingsResult.Settings.Size(), ApplicationSettings.Settings.Size());
}

CSP_PUBLIC_TEST(CSPEngine, ApplicationSettingsSystemMockTests, WhenApplicationSettingsPutResponseBadRequestThenRecieveFailedResponseTests)
{
    const auto ApplicationSettingsMock = std::make_unique<chs::ApplicationSettingsApiMock>();

    std::promise<ApplicationSettingsResult> ResultPromise;
    std::future<ApplicationSettingsResult> ResultFuture = ResultPromise.get_future();

    EXPECT_CALL(*ApplicationSettingsMock, applicationsApplicationNameSettingsContextPut)
        .WillOnce(
            [&](const chs::ApplicationSettingsApiMock::applicationsApplicationNameSettingsContextPutParams& /*Params*/,
                csp::services::ApiResponseHandlerBase* ResponseHandler, csp::common::CancellationToken& /*CancellationToken*/
            )
            {
                csp::web::HttpPayload Payload;
                Payload.SetContent("}{ Invalid JSON ...");

                auto Response = csp::web::HttpResponse();
                Response.SetResponseCode(csp::web::EResponseCodes::ResponseBadRequest);
                Response.GetMutablePayload() = Payload;
                ResponseHandler->OnHttpResponse(Response);
            });

    auto Callback = [&ResultPromise](const ApplicationSettingsResult& Result) { ResultPromise.set_value(Result); };

    auto ResponseHandler
        = ApplicationSettingsMock->CreateHandler<ApplicationSettingsResultCallback, ApplicationSettingsResult, void, chs::ApplicationSettingsDto>(
            Callback, nullptr, csp::web::EResponseCodes::ResponseCreated);

    auto Params = chs::ApplicationSettingsApiMock::applicationsApplicationNameSettingsContextPutParams { "", "", {} };

    ApplicationSettingsMock->applicationsApplicationNameSettingsContextPut(Params, ResponseHandler, CancellationToken::Dummy());

    auto Result = ResultFuture.get();
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
    EXPECT_EQ(Result.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseBadRequest));
    EXPECT_EQ(Result.GetFailureReason(), csp::systems::ERequestFailureReason::None);
}

CSP_PUBLIC_TEST(CSPEngine, ApplicationSettingsSystemMockTests, WhenApplicationSettingsGetResponseOkThenRecieveSuccessResponseTests)
{
    const auto ApplicationSettingsMock = std::make_unique<chs::ApplicationSettingsApiMock>();

    std::promise<ApplicationSettingsResult> ResultPromise;
    std::future<ApplicationSettingsResult> ResultFuture = ResultPromise.get_future();

    auto ApplicationSettings = csp::common::ApplicationSettings();
    ApplicationSettings.ApplicationName = "MockApplicationName";
    ApplicationSettings.Context = "MockContext";
    ApplicationSettings.AllowAnonymous = false;
    ApplicationSettings.Settings = { { "MockTestSettings", "MockTestData" } };

    EXPECT_CALL(*ApplicationSettingsMock, applicationsApplicationNameSettingsContextGet)
        .WillOnce(
            [&ApplicationSettings](const chs::ApplicationSettingsApiMock::applicationsApplicationNameSettingsContextGetParams& Params,
                csp::services::ApiResponseHandlerBase* ResponseHandler, csp::common::CancellationToken& /*CancellationToken*/
            )
            {
                EXPECT_EQ(ApplicationSettings.ApplicationName, Params.applicationName);
                EXPECT_EQ(ApplicationSettings.Context, Params.context);

                csp::web::HttpPayload Payload;
                auto json = csp::json::JsonSerializer::Serialize(ApplicationSettings);
                Payload.AddHeader(CSP_TEXT("Content-Type"), CSP_TEXT("application/json"));
                Payload.SetContent(json);

                auto Response = csp::web::HttpResponse();
                Response.SetResponseCode(csp::web::EResponseCodes::ResponseOK);
                Response.GetMutablePayload() = Payload;

                ResponseHandler->OnHttpResponse(Response);
            });

    auto Callback = [&ResultPromise](const ApplicationSettingsResult& Result) { ResultPromise.set_value(Result); };

    auto ResponseHandler
        = ApplicationSettingsMock->CreateHandler<ApplicationSettingsResultCallback, ApplicationSettingsResult, void, chs::ApplicationSettingsDto>(
            Callback, nullptr, csp::web::EResponseCodes::ResponseOK);

    auto Params = chs::ApplicationSettingsApiMock::applicationsApplicationNameSettingsContextGetParams { ApplicationSettings.ApplicationName,
        ApplicationSettings.Context, {} };

    ApplicationSettingsMock->applicationsApplicationNameSettingsContextGet(Params, ResponseHandler, CancellationToken::Dummy());

    auto Result = ResultFuture.get();
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(Result.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseOK));
    EXPECT_EQ(Result.GetFailureReason(), csp::systems::ERequestFailureReason::None);

    const auto ApplicationSettingsResult = Result.GetApplicationSettings();
    EXPECT_EQ(ApplicationSettingsResult.ApplicationName, ApplicationSettings.ApplicationName);
    EXPECT_EQ(ApplicationSettingsResult.Context, ApplicationSettings.Context);
    EXPECT_EQ(ApplicationSettingsResult.AllowAnonymous, ApplicationSettings.AllowAnonymous);
    EXPECT_EQ(ApplicationSettingsResult.Settings.Size(), ApplicationSettings.Settings.Size());
}

CSP_PUBLIC_TEST(CSPEngine, ApplicationSettingsSystemMockTests, WhenApplicationSettingsAnonymousGetResponseOkThenRecieveSuccessResponseTests)
{
    const auto ApplicationSettingsMock = std::make_unique<chs::ApplicationSettingsApiMock>();

    std::promise<ApplicationSettingsResult> ResultPromise;
    std::future<ApplicationSettingsResult> ResultFuture = ResultPromise.get_future();

    auto ApplicationSettings = csp::common::ApplicationSettings();
    ApplicationSettings.ApplicationName = "MockApplicationName";
    ApplicationSettings.Context = "MockContext";
    ApplicationSettings.AllowAnonymous = true;
    ApplicationSettings.Settings = { { "MockTestSettings", "MockTestData" } };

    EXPECT_CALL(*ApplicationSettingsMock, tenantsTenantApplicationsApplicationNameSettingsContextGet)
        .WillOnce(
            [&ApplicationSettings](const chs::ApplicationSettingsApiMock::tenantsTenantApplicationsApplicationNameSettingsContextGetParams& Params,
                csp::services::ApiResponseHandlerBase* ResponseHandler, csp::common::CancellationToken& /*CancellationToken*/
            )
            {
                EXPECT_EQ(ApplicationSettings.ApplicationName, Params.applicationName);
                EXPECT_EQ(ApplicationSettings.Context, Params.context);

                csp::web::HttpPayload Payload;
                auto json = csp::json::JsonSerializer::Serialize(ApplicationSettings);
                Payload.AddHeader(CSP_TEXT("Content-Type"), CSP_TEXT("application/json"));
                Payload.SetContent(json);

                auto Response = csp::web::HttpResponse();
                Response.SetResponseCode(csp::web::EResponseCodes::ResponseOK);
                Response.GetMutablePayload() = Payload;

                ResponseHandler->OnHttpResponse(Response);
            });

    auto Callback = [&ResultPromise](const ApplicationSettingsResult& Result) { ResultPromise.set_value(Result); };

    auto ResponseHandler
        = ApplicationSettingsMock->CreateHandler<ApplicationSettingsResultCallback, ApplicationSettingsResult, void, chs::ApplicationSettingsDto>(
            Callback, nullptr, csp::web::EResponseCodes::ResponseOK);

    auto Params = chs::ApplicationSettingsApiMock::tenantsTenantApplicationsApplicationNameSettingsContextGetParams { "OKO_TESTS",
        ApplicationSettings.ApplicationName, ApplicationSettings.Context, {} };

    ApplicationSettingsMock->tenantsTenantApplicationsApplicationNameSettingsContextGet(Params, ResponseHandler, CancellationToken::Dummy());

    auto Result = ResultFuture.get();
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(Result.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseOK));
    EXPECT_EQ(Result.GetFailureReason(), csp::systems::ERequestFailureReason::None);

    const auto ApplicationSettingsResult = Result.GetApplicationSettings();
    EXPECT_EQ(ApplicationSettingsResult.ApplicationName, ApplicationSettings.ApplicationName);
    EXPECT_EQ(ApplicationSettingsResult.Context, ApplicationSettings.Context);
    EXPECT_EQ(ApplicationSettingsResult.AllowAnonymous, ApplicationSettings.AllowAnonymous);
    EXPECT_EQ(ApplicationSettingsResult.Settings.Size(), ApplicationSettings.Settings.Size());
}

CSP_PUBLIC_TEST(CSPEngine, ApplicationSettingsSystemMockTests, WhenApplicationSettingsAnonymousGetResponseNotFoundThenRecieveNotFoundResponseTests)
{
    const auto ApplicationSettingsMock = std::make_unique<chs::ApplicationSettingsApiMock>();

    std::promise<ApplicationSettingsResult> ResultPromise;
    std::future<ApplicationSettingsResult> ResultFuture = ResultPromise.get_future();

    auto ApplicationSettings = csp::common::ApplicationSettings();
    ApplicationSettings.ApplicationName = "MockApplicationName";
    ApplicationSettings.Context = "MockContext";
    ApplicationSettings.AllowAnonymous = true;
    ApplicationSettings.Settings = { { "MockTestSettings", "MockTestData" } };

    EXPECT_CALL(*ApplicationSettingsMock, tenantsTenantApplicationsApplicationNameSettingsContextGet)
        .WillOnce(
            [&ApplicationSettings](const chs::ApplicationSettingsApiMock::tenantsTenantApplicationsApplicationNameSettingsContextGetParams& Params,
                csp::services::ApiResponseHandlerBase* ResponseHandler, csp::common::CancellationToken& /*CancellationToken*/
            )
            {
                EXPECT_EQ(ApplicationSettings.ApplicationName, Params.applicationName);
                EXPECT_EQ(ApplicationSettings.Context, Params.context);

                csp::web::HttpPayload Payload;
                auto json = csp::json::JsonSerializer::Serialize(ApplicationSettings);
                Payload.AddHeader(CSP_TEXT("Content-Type"), CSP_TEXT("application/json"));
                Payload.SetContent(json);

                auto Response = csp::web::HttpResponse();
                Response.SetResponseCode(csp::web::EResponseCodes::ResponseNotFound);
                Response.GetMutablePayload() = Payload;

                ResponseHandler->OnHttpResponse(Response);
            });

    auto Callback = [&ResultPromise](const ApplicationSettingsResult& Result) { ResultPromise.set_value(Result); };

    auto ResponseHandler
        = ApplicationSettingsMock->CreateHandler<ApplicationSettingsResultCallback, ApplicationSettingsResult, void, chs::ApplicationSettingsDto>(
            Callback, nullptr, csp::web::EResponseCodes::ResponseNotFound);

    auto Params = chs::ApplicationSettingsApiMock::tenantsTenantApplicationsApplicationNameSettingsContextGetParams { "OKO_TESTS",
        ApplicationSettings.ApplicationName, ApplicationSettings.Context, {} };

    ApplicationSettingsMock->tenantsTenantApplicationsApplicationNameSettingsContextGet(Params, ResponseHandler, CancellationToken::Dummy());

    auto Result = ResultFuture.get();
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(Result.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseNotFound));
    EXPECT_EQ(Result.GetFailureReason(), csp::systems::ERequestFailureReason::None);

    const auto ApplicationSettingsResult = Result.GetApplicationSettings();
    EXPECT_EQ(ApplicationSettingsResult.ApplicationName, ApplicationSettings.ApplicationName);
    EXPECT_EQ(ApplicationSettingsResult.Context, ApplicationSettings.Context);
    EXPECT_EQ(ApplicationSettingsResult.AllowAnonymous, ApplicationSettings.AllowAnonymous);
    EXPECT_EQ(ApplicationSettingsResult.Settings.Size(), ApplicationSettings.Settings.Size());
}