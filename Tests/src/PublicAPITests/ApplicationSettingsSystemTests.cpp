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

ApplicationSettings GetApplicationSettingsTestData(const csp::common::String& context, const bool allowAnonymous)
{
    auto settings = csp::common::Map<csp::common::String, csp::common::String>();
    settings["TestSettings_1"] = "TestData_1";
    settings["TestSettings_2"] = "TestData_2";
    settings["TestSettings_3"] = "TestData_3";
    settings["TestSettings_4"] = "TestData_4";

    auto applicationSettings = ::ApplicationSettings();
    applicationSettings.ApplicationName = "MAG_APPLICATION_SETTINGS_TESTS";
    applicationSettings.Context = context;
    applicationSettings.AllowAnonymous = allowAnonymous;
    applicationSettings.Settings = settings;

    return applicationSettings;
}

}

CSP_PUBLIC_TEST(CSPEngine, ApplicationSettingsSystemTests, CreateSettingsByContextTest)
{
    if (std::string(AdminAccountEmail()).empty())
    {
        GTEST_SKIP() << "Admin account email not set. This test cannot be run.";
    }

    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* applicationSettingsSystem = systemsManager.GetApplicationSettingsSystem();

    // Login
    csp::common::String userId;
    LogInAsAdminUser(userSystem, userId);

    auto applicationSettingsTestData = GetApplicationSettingsTestData("MAG_APPLICATION_SETTINGS_CONTEXT_TESTS", false);

    // Create Application Settings
    {
        RAIIMockLogger mockLogger {};
        csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Log);

        // Set an expectation that the mock logger will receive message for a successful creation of settings by context
        const String getSettingsByContextMsg = "ApplicationSettingsSystem::CreateSettingsByContext successfully created application settings";
        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Log, getSettingsByContextMsg)).Times(1);

        auto [Result] = AWAIT(applicationSettingsSystem, CreateSettingsByContext, applicationSettingsTestData);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const auto applicationSettings = Result.GetApplicationSettings();

        EXPECT_EQ(applicationSettings.AllowAnonymous, applicationSettingsTestData.AllowAnonymous);
        EXPECT_EQ(applicationSettings.ApplicationName, applicationSettingsTestData.ApplicationName);
        EXPECT_EQ(applicationSettings.Context, applicationSettingsTestData.Context);
        EXPECT_EQ(applicationSettings.Settings, applicationSettingsTestData.Settings);
    }

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, ApplicationSettingsSystemTests, CreateAnonymousSettingsByContextTest)
{
    if (std::string(AdminAccountEmail()).empty())
    {
        GTEST_SKIP() << "Admin account email not set. This test cannot be run.";
    }

    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* applicationSettingsSystem = systemsManager.GetApplicationSettingsSystem();

    // Login
    csp::common::String userId;
    LogInAsAdminUser(userSystem, userId);

    auto applicationSettingsTestData = GetApplicationSettingsTestData("MAG_APPLICATION_SETTINGS_ANONYMOUS_CONTEXT_TESTS", true);

    // Create Application Settings
    {
        RAIIMockLogger mockLogger {};
        csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Log);

        // Set an expectation that the mock logger will receive message for a successful creation of settings by context
        const String getSettingsByContextMsg = "ApplicationSettingsSystem::CreateSettingsByContext successfully created application settings";
        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Log, getSettingsByContextMsg)).Times(1);

        auto [Result] = AWAIT(applicationSettingsSystem, CreateSettingsByContext, applicationSettingsTestData);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const auto applicationSettings = Result.GetApplicationSettings();

        EXPECT_EQ(applicationSettings.AllowAnonymous, applicationSettingsTestData.AllowAnonymous);
        EXPECT_EQ(applicationSettings.ApplicationName, applicationSettingsTestData.ApplicationName);
        EXPECT_EQ(applicationSettings.Context, applicationSettingsTestData.Context);
        EXPECT_EQ(applicationSettings.Settings, applicationSettingsTestData.Settings);
    }

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, ApplicationSettingsSystemTests, GetSettingsByContextTest)
{
    if (std::string(AdminAccountEmail()).empty())
    {
        GTEST_SKIP() << "Admin account email not set. This test cannot be run.";
    }

    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* applicationSettingsSystem = systemsManager.GetApplicationSettingsSystem();

    // Seed application settings test data
    {
        csp::common::String userId;
        LogInAsAdminUser(userSystem, userId);

        auto applicationSettings = GetApplicationSettingsTestData("MAG_APPLICATION_SETTINGS_CONTEXT_TESTS", false);
        auto [Result] = AWAIT(applicationSettingsSystem, CreateSettingsByContext, applicationSettings);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        // Log out
        LogOut(userSystem);
    }

    // Login
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    auto const applicationSettingsTestData = GetApplicationSettingsTestData("MAG_APPLICATION_SETTINGS_CONTEXT_TESTS", false);

    // Get Application Settings
    {
        RAIIMockLogger mockLogger {};
        csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Log);

        // Set an expectation that the mock logger will receive message for a successful result retrieval of settings by context
        const String getSettingsByContextMsg = "ApplicationSettingsSystem::GetSettingsByContext successfully retrieved application settings";
        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Log, getSettingsByContextMsg)).Times(1);

        auto [Result] = AWAIT(applicationSettingsSystem, GetSettingsByContext, applicationSettingsTestData.ApplicationName,
            applicationSettingsTestData.Context, nullptr);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const auto applicationSettings = Result.GetApplicationSettings();

        EXPECT_EQ(applicationSettings.AllowAnonymous, applicationSettingsTestData.AllowAnonymous);
        EXPECT_EQ(applicationSettings.ApplicationName, applicationSettingsTestData.ApplicationName);
        EXPECT_EQ(applicationSettings.Context, applicationSettingsTestData.Context);
        EXPECT_EQ(applicationSettings.Settings, applicationSettingsTestData.Settings);
    }

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, ApplicationSettingsSystemTests, GetSettingsByContextWithKeysTest)
{
    if (std::string(AdminAccountEmail()).empty())
    {
        GTEST_SKIP() << "Admin account email not set. This test cannot be run.";
    }

    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* applicationSettingsSystem = systemsManager.GetApplicationSettingsSystem();

    // Seed application settings test data
    {
        csp::common::String userId;
        LogInAsAdminUser(userSystem, userId);

        auto applicationSettings = GetApplicationSettingsTestData("MAG_APPLICATION_SETTINGS_CONTEXT_TESTS", false);
        auto [Result] = AWAIT(applicationSettingsSystem, CreateSettingsByContext, applicationSettings);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        // Log out
        LogOut(userSystem);
    }

    // Login
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    auto const applicationSettingsTestData = GetApplicationSettingsTestData("MAG_APPLICATION_SETTINGS_CONTEXT_TESTS", false);

    // Get Application Settings
    {
        RAIIMockLogger mockLogger {};
        csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Log);

        // Set an expectation that the mock logger will receive message for a successful result retrieval of settings by context
        const String getSettingsByContextMsg = "ApplicationSettingsSystem::GetSettingsByContext successfully retrieved application settings";
        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Log, getSettingsByContextMsg)).Times(1);

        auto keys = csp::common::Array<csp::common::String>(1);
        keys[0] = "TestSettings_3";

        auto [Result] = AWAIT(
            applicationSettingsSystem, GetSettingsByContext, applicationSettingsTestData.ApplicationName, applicationSettingsTestData.Context, keys);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const auto applicationSettings = Result.GetApplicationSettings();

        EXPECT_EQ(applicationSettings.AllowAnonymous, applicationSettingsTestData.AllowAnonymous);
        EXPECT_EQ(applicationSettings.ApplicationName, applicationSettingsTestData.ApplicationName);
        EXPECT_EQ(applicationSettings.Context, applicationSettingsTestData.Context);
        EXPECT_EQ(applicationSettings.Settings.Size(), keys.Size());
        EXPECT_EQ(applicationSettings.Settings[keys[0]], "TestData_3");
    }

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, ApplicationSettingsSystemTests, GetInvalidSettingsByContextTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* applicationSettingsSystem = systemsManager.GetApplicationSettingsSystem();

    // Login
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Get Application Settings
    {
        RAIIMockLogger mockLogger {};
        csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Log);

        const String responseMsg = "ApplicationSettingsSystem::GetSettingsByContext successfully retrieved application settings";
        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Log, responseMsg)).Times(1);

        auto [Result] = AWAIT(applicationSettingsSystem, GetSettingsByContext, GetUniqueString().c_str(), GetUniqueString().c_str(), nullptr);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseNoContent));
        EXPECT_EQ(Result.GetFailureReason(), csp::systems::ERequestFailureReason::None);
    }

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, ApplicationSettingsSystemTests, GetSettingsByContextAnonymousTest)
{
    if (std::string(AdminAccountEmail()).empty())
    {
        GTEST_SKIP() << "Admin account email not set. This test cannot be run.";
    }

    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* applicationSettingsSystem = systemsManager.GetApplicationSettingsSystem();

    // Seed application settings test data
    {
        csp::common::String userId;
        LogInAsAdminUser(userSystem, userId);

        auto applicationSettings = GetApplicationSettingsTestData("MAG_APPLICATION_SETTINGS_ANONYMOUS_CONTEXT_TESTS", true);
        auto [Result] = AWAIT(applicationSettingsSystem, CreateSettingsByContext, applicationSettings);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        // Log out
        LogOut(userSystem);
    }

    auto const applicationSettingsTestData = GetApplicationSettingsTestData("MAG_APPLICATION_SETTINGS_ANONYMOUS_CONTEXT_TESTS", true);

    // Get Application Settings
    {
        RAIIMockLogger mockLogger {};
        csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Log);

        // Set an expectation that the mock logger will receive message for a successful result retrieval of settings by context
        const String getSettingsByContextMsg = "ApplicationSettingsSystem::GetSettingsByContextAnonymous successfully retrieved application settings";
        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Log, getSettingsByContextMsg)).Times(1);

        auto [Result] = AWAIT(applicationSettingsSystem, GetSettingsByContextAnonymous, csp::CSPFoundation::GetTenant(),
            applicationSettingsTestData.ApplicationName, applicationSettingsTestData.Context, nullptr);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const auto applicationSettings = Result.GetApplicationSettings();

        EXPECT_EQ(applicationSettings.AllowAnonymous, applicationSettingsTestData.AllowAnonymous);
        EXPECT_EQ(applicationSettings.ApplicationName, applicationSettingsTestData.ApplicationName);
        EXPECT_EQ(applicationSettings.Context, applicationSettingsTestData.Context);
        EXPECT_EQ(applicationSettings.Settings, applicationSettingsTestData.Settings);
    }
}

CSP_PUBLIC_TEST(CSPEngine, ApplicationSettingsSystemTests, GetSettingsByContextAnonymousWithKeysTest)
{
    if (std::string(AdminAccountEmail()).empty())
    {
        GTEST_SKIP() << "Admin account email not set. This test cannot be run.";
    }

    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* applicationSettingsSystem = systemsManager.GetApplicationSettingsSystem();

    // Seed application settings test data
    {
        csp::common::String userId;
        LogInAsAdminUser(userSystem, userId);

        auto applicationSettings = GetApplicationSettingsTestData("MAG_APPLICATION_SETTINGS_ANONYMOUS_CONTEXT_TESTS", true);
        auto [Result] = AWAIT(applicationSettingsSystem, CreateSettingsByContext, applicationSettings);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        // Log out
        LogOut(userSystem);
    }

    auto const applicationSettingsTestData = GetApplicationSettingsTestData("MAG_APPLICATION_SETTINGS_ANONYMOUS_CONTEXT_TESTS", true);

    // Get Application Settings
    {
        RAIIMockLogger mockLogger {};
        csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Log);

        // Set an expectation that the mock logger will receive message for a successful result retrieval of settings by context
        const String getSettingsByContextMsg = "ApplicationSettingsSystem::GetSettingsByContextAnonymous successfully retrieved application settings";
        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Log, getSettingsByContextMsg)).Times(1);

        auto keys = csp::common::Array<csp::common::String>(1);
        keys[0] = "TestSettings_3";

        auto [Result] = AWAIT(applicationSettingsSystem, GetSettingsByContextAnonymous, csp::CSPFoundation::GetTenant(),
            applicationSettingsTestData.ApplicationName, applicationSettingsTestData.Context, keys);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const auto applicationSettings = Result.GetApplicationSettings();

        EXPECT_EQ(applicationSettings.AllowAnonymous, applicationSettingsTestData.AllowAnonymous);
        EXPECT_EQ(applicationSettings.ApplicationName, applicationSettingsTestData.ApplicationName);
        EXPECT_EQ(applicationSettings.Context, applicationSettingsTestData.Context);
        EXPECT_EQ(applicationSettings.Settings.Size(), keys.Size());
        EXPECT_EQ(applicationSettings.Settings[keys[0]], "TestData_3");
    }
}

CSP_PUBLIC_TEST(CSPEngine, ApplicationSettingsSystemTests, GetInvalidSettingsByContextAnonymousTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* applicationSettingsSystem = systemsManager.GetApplicationSettingsSystem();

    // Get Application Settings
    {
        RAIIMockLogger mockLogger {};
        csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Log);

        // Set an expectation that the mock logger will receive message for a failed result 404 no payload/error message.
        const String getRequestErrorMsg = "has returned a failed response (404) but with no payload/error message.";
        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, testing::HasSubstr(getRequestErrorMsg))).Times(1);

        const String errorMsg = "Failed to get application settings";
        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Log, errorMsg)).Times(1);

        auto [Result] = AWAIT(applicationSettingsSystem, GetSettingsByContextAnonymous, csp::CSPFoundation::GetTenant(), GetUniqueString().c_str(),
            GetUniqueString().c_str(), nullptr);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
        EXPECT_EQ(Result.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseNotFound));
        EXPECT_EQ(Result.GetFailureReason(), csp::systems::ERequestFailureReason::None);
    }
}

CSP_PUBLIC_TEST(CSPEngine, ApplicationSettingsSystemTests, GetInvalidTentantSettingsByContextAnonymousTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* applicationSettingsSystem = systemsManager.GetApplicationSettingsSystem();

    auto applicationSettingsTestData = GetApplicationSettingsTestData("MAG_APPLICATION_SETTINGS_ANONYMOUS_CONTEXT_TESTS", true);

    // Get Application Settings
    {
        RAIIMockLogger mockLogger {};
        csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(LogLevel::Log);

        // Set an expectation that the mock logger will receive message for a failed result 404 no payload/error message.
        const String getRequestErrorMsg = "has returned a failed response (404) but with no payload/error message.";
        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, testing::HasSubstr(getRequestErrorMsg))).Times(1);

        const String errorMsg = "Failed to get application settings";
        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Log, errorMsg)).Times(1);

        auto [Result] = AWAIT(applicationSettingsSystem, GetSettingsByContextAnonymous, GetUniqueString().c_str(),
            applicationSettingsTestData.ApplicationName, applicationSettingsTestData.Context, nullptr);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
        EXPECT_EQ(Result.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseNotFound));
        EXPECT_EQ(Result.GetFailureReason(), csp::systems::ERequestFailureReason::None);
    }
}

namespace chs = csp::services::generated::userservice;

CSP_PUBLIC_TEST(CSPEngine, ApplicationSettingsSystemMockTests, WhenApplicationSettingsPutResponseCreatedThenRecieveSuccessResponseTests)
{
    const auto applicationSettingsMock = std::make_unique<chs::ApplicationSettingsApiMock>();

    std::promise<ApplicationSettingsResult> resultPromise;
    std::future<ApplicationSettingsResult> resultFuture = resultPromise.get_future();

    // Create default application settings expected data, use to validate and construct the return response for mock.
    auto applicationSettings = csp::common::ApplicationSettings();
    applicationSettings.ApplicationName = "MockApplicationName";
    applicationSettings.Context = "MockContext";
    applicationSettings.AllowAnonymous = false;
    applicationSettings.Settings = { { "MockTestSettings", "MockTestData" } };

    // Sets the expectation that a specific method on a mock object will be called, and will fail the test if these conditions are not met.
    EXPECT_CALL(*applicationSettingsMock, applicationsApplicationNameSettingsContextPut)
        .WillOnce(
            [&](const chs::ApplicationSettingsApiMock::applicationsApplicationNameSettingsContextPutParams& params,
                csp::services::ApiResponseHandlerBase* responseHandler, csp::common::CancellationToken& /*CancellationToken*/
            )
            {
                // Basic validation that the information provided matches expectations.
                EXPECT_EQ(applicationSettings.ApplicationName, params.applicationName);
                EXPECT_EQ(applicationSettings.Context, params.context);

                // Construct the payload using the ApplicationSettings to populate the request body for the response.
                csp::web::HttpPayload payload;
                auto json = csp::json::JsonSerializer::Serialize(applicationSettings);
                payload.AddHeader(CSP_TEXT("Content-Type"), CSP_TEXT("application/json"));
                payload.SetContent(json);

                // Construct the response with the expected HTTP response code and payload.
                auto response = csp::web::HttpResponse();
                response.SetResponseCode(csp::web::EResponseCodes::ResponseCreated);
                response.GetMutablePayload() = payload;

                // Invoke the response on the handler to emulate the RESTful call.
                responseHandler->OnHttpResponse(response);
            });

    // Create a callback to capture the response to fulfill the promise.
    auto callback = [&resultPromise](const ApplicationSettingsResult& result) { resultPromise.set_value(result); };

    // Create a handler for the current mock function, which will allow emulation of the RESTful response in the EXPECT_CALL.
    auto responseHandler
        = applicationSettingsMock->CreateHandler<ApplicationSettingsResultCallback, ApplicationSettingsResult, void, chs::ApplicationSettingsDto>(
            callback, nullptr, csp::web::EResponseCodes::ResponseCreated);

    auto request = std::make_shared<chs::ApplicationSettingsDto>();
    request->SetAllowAnonymous(applicationSettings.AllowAnonymous);
    request->SetSettings(Convert(applicationSettings.Settings));

    auto params
        = chs::ApplicationSettingsApiMock::applicationsApplicationNameSettingsContextPutParams { "MockApplicationName", "MockContext", request };

    // Call the expected mock function to trigger the expected call and fulfill the promise.
    applicationSettingsMock->applicationsApplicationNameSettingsContextPut(params, responseHandler, CancellationToken::Dummy());

    auto result = resultFuture.get();
    EXPECT_EQ(result.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(result.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseCreated));
    EXPECT_EQ(result.GetFailureReason(), csp::systems::ERequestFailureReason::None);

    const auto applicationSettingsResult = result.GetApplicationSettings();
    EXPECT_EQ(applicationSettingsResult.ApplicationName, applicationSettings.ApplicationName);
    EXPECT_EQ(applicationSettingsResult.Context, applicationSettings.Context);
    EXPECT_EQ(applicationSettingsResult.AllowAnonymous, applicationSettings.AllowAnonymous);
    EXPECT_EQ(applicationSettingsResult.Settings.Size(), applicationSettings.Settings.Size());
}

CSP_PUBLIC_TEST(CSPEngine, ApplicationSettingsSystemMockTests, WhenApplicationSettingsPutResponseBadRequestThenRecieveFailedResponseTests)
{
    const auto applicationSettingsMock = std::make_unique<chs::ApplicationSettingsApiMock>();

    std::promise<ApplicationSettingsResult> resultPromise;
    std::future<ApplicationSettingsResult> resultFuture = resultPromise.get_future();

    EXPECT_CALL(*applicationSettingsMock, applicationsApplicationNameSettingsContextPut)
        .WillOnce(
            [&](const chs::ApplicationSettingsApiMock::applicationsApplicationNameSettingsContextPutParams& /*Params*/,
                csp::services::ApiResponseHandlerBase* responseHandler, csp::common::CancellationToken& /*CancellationToken*/
            )
            {
                csp::web::HttpPayload payload;
                payload.SetContent("}{ Invalid JSON ...");

                auto response = csp::web::HttpResponse();
                response.SetResponseCode(csp::web::EResponseCodes::ResponseBadRequest);
                response.GetMutablePayload() = payload;
                responseHandler->OnHttpResponse(response);
            });

    auto callback = [&resultPromise](const ApplicationSettingsResult& result) { resultPromise.set_value(result); };

    auto responseHandler
        = applicationSettingsMock->CreateHandler<ApplicationSettingsResultCallback, ApplicationSettingsResult, void, chs::ApplicationSettingsDto>(
            callback, nullptr, csp::web::EResponseCodes::ResponseCreated);

    auto params = chs::ApplicationSettingsApiMock::applicationsApplicationNameSettingsContextPutParams { "", "", {} };

    applicationSettingsMock->applicationsApplicationNameSettingsContextPut(params, responseHandler, CancellationToken::Dummy());

    auto result = resultFuture.get();
    EXPECT_EQ(result.GetResultCode(), csp::systems::EResultCode::Failed);
    EXPECT_EQ(result.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseBadRequest));
    EXPECT_EQ(result.GetFailureReason(), csp::systems::ERequestFailureReason::None);
}

CSP_PUBLIC_TEST(CSPEngine, ApplicationSettingsSystemMockTests, WhenApplicationSettingsGetResponseOkThenRecieveSuccessResponseTests)
{
    const auto applicationSettingsMock = std::make_unique<chs::ApplicationSettingsApiMock>();

    std::promise<ApplicationSettingsResult> resultPromise;
    std::future<ApplicationSettingsResult> resultFuture = resultPromise.get_future();

    auto applicationSettings = csp::common::ApplicationSettings();
    applicationSettings.ApplicationName = "MockApplicationName";
    applicationSettings.Context = "MockContext";
    applicationSettings.AllowAnonymous = false;
    applicationSettings.Settings = { { "MockTestSettings", "MockTestData" } };

    EXPECT_CALL(*applicationSettingsMock, applicationsApplicationNameSettingsContextGet)
        .WillOnce(
            [&applicationSettings](const chs::ApplicationSettingsApiMock::applicationsApplicationNameSettingsContextGetParams& params,
                csp::services::ApiResponseHandlerBase* responseHandler, csp::common::CancellationToken& /*CancellationToken*/
            )
            {
                EXPECT_EQ(applicationSettings.ApplicationName, params.applicationName);
                EXPECT_EQ(applicationSettings.Context, params.context);

                csp::web::HttpPayload payload;
                auto json = csp::json::JsonSerializer::Serialize(applicationSettings);
                payload.AddHeader(CSP_TEXT("Content-Type"), CSP_TEXT("application/json"));
                payload.SetContent(json);

                auto response = csp::web::HttpResponse();
                response.SetResponseCode(csp::web::EResponseCodes::ResponseOK);
                response.GetMutablePayload() = payload;

                responseHandler->OnHttpResponse(response);
            });

    auto callback = [&resultPromise](const ApplicationSettingsResult& result) { resultPromise.set_value(result); };

    auto responseHandler
        = applicationSettingsMock->CreateHandler<ApplicationSettingsResultCallback, ApplicationSettingsResult, void, chs::ApplicationSettingsDto>(
            callback, nullptr, csp::web::EResponseCodes::ResponseOK);

    auto params = chs::ApplicationSettingsApiMock::applicationsApplicationNameSettingsContextGetParams { applicationSettings.ApplicationName,
        applicationSettings.Context, {} };

    applicationSettingsMock->applicationsApplicationNameSettingsContextGet(params, responseHandler, CancellationToken::Dummy());

    auto result = resultFuture.get();
    EXPECT_EQ(result.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(result.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseOK));
    EXPECT_EQ(result.GetFailureReason(), csp::systems::ERequestFailureReason::None);

    const auto applicationSettingsResult = result.GetApplicationSettings();
    EXPECT_EQ(applicationSettingsResult.ApplicationName, applicationSettings.ApplicationName);
    EXPECT_EQ(applicationSettingsResult.Context, applicationSettings.Context);
    EXPECT_EQ(applicationSettingsResult.AllowAnonymous, applicationSettings.AllowAnonymous);
    EXPECT_EQ(applicationSettingsResult.Settings.Size(), applicationSettings.Settings.Size());
}

CSP_PUBLIC_TEST(CSPEngine, ApplicationSettingsSystemMockTests, WhenApplicationSettingsAnonymousGetResponseOkThenRecieveSuccessResponseTests)
{
    const auto applicationSettingsMock = std::make_unique<chs::ApplicationSettingsApiMock>();

    std::promise<ApplicationSettingsResult> resultPromise;
    std::future<ApplicationSettingsResult> resultFuture = resultPromise.get_future();

    auto applicationSettings = csp::common::ApplicationSettings();
    applicationSettings.ApplicationName = "MockApplicationName";
    applicationSettings.Context = "MockContext";
    applicationSettings.AllowAnonymous = true;
    applicationSettings.Settings = { { "MockTestSettings", "MockTestData" } };

    EXPECT_CALL(*applicationSettingsMock, tenantsTenantApplicationsApplicationNameSettingsContextGet)
        .WillOnce(
            [&applicationSettings](const chs::ApplicationSettingsApiMock::tenantsTenantApplicationsApplicationNameSettingsContextGetParams& params,
                csp::services::ApiResponseHandlerBase* responseHandler, csp::common::CancellationToken& /*CancellationToken*/
            )
            {
                EXPECT_EQ(applicationSettings.ApplicationName, params.applicationName);
                EXPECT_EQ(applicationSettings.Context, params.context);

                csp::web::HttpPayload payload;
                auto json = csp::json::JsonSerializer::Serialize(applicationSettings);
                payload.AddHeader(CSP_TEXT("Content-Type"), CSP_TEXT("application/json"));
                payload.SetContent(json);

                auto response = csp::web::HttpResponse();
                response.SetResponseCode(csp::web::EResponseCodes::ResponseOK);
                response.GetMutablePayload() = payload;

                responseHandler->OnHttpResponse(response);
            });

    auto callback = [&resultPromise](const ApplicationSettingsResult& result) { resultPromise.set_value(result); };

    auto responseHandler
        = applicationSettingsMock->CreateHandler<ApplicationSettingsResultCallback, ApplicationSettingsResult, void, chs::ApplicationSettingsDto>(
            callback, nullptr, csp::web::EResponseCodes::ResponseOK);

    auto params = chs::ApplicationSettingsApiMock::tenantsTenantApplicationsApplicationNameSettingsContextGetParams { "OKO_TESTS",
        applicationSettings.ApplicationName, applicationSettings.Context, {} };

    applicationSettingsMock->tenantsTenantApplicationsApplicationNameSettingsContextGet(params, responseHandler, CancellationToken::Dummy());

    auto result = resultFuture.get();
    EXPECT_EQ(result.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(result.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseOK));
    EXPECT_EQ(result.GetFailureReason(), csp::systems::ERequestFailureReason::None);

    const auto applicationSettingsResult = result.GetApplicationSettings();
    EXPECT_EQ(applicationSettingsResult.ApplicationName, applicationSettings.ApplicationName);
    EXPECT_EQ(applicationSettingsResult.Context, applicationSettings.Context);
    EXPECT_EQ(applicationSettingsResult.AllowAnonymous, applicationSettings.AllowAnonymous);
    EXPECT_EQ(applicationSettingsResult.Settings.Size(), applicationSettings.Settings.Size());
}

CSP_PUBLIC_TEST(CSPEngine, ApplicationSettingsSystemMockTests, WhenApplicationSettingsAnonymousGetResponseNotFoundThenRecieveNotFoundResponseTests)
{
    const auto applicationSettingsMock = std::make_unique<chs::ApplicationSettingsApiMock>();

    std::promise<ApplicationSettingsResult> resultPromise;
    std::future<ApplicationSettingsResult> resultFuture = resultPromise.get_future();

    auto applicationSettings = csp::common::ApplicationSettings();
    applicationSettings.ApplicationName = "MockApplicationName";
    applicationSettings.Context = "MockContext";
    applicationSettings.AllowAnonymous = true;
    applicationSettings.Settings = { { "MockTestSettings", "MockTestData" } };

    EXPECT_CALL(*applicationSettingsMock, tenantsTenantApplicationsApplicationNameSettingsContextGet)
        .WillOnce(
            [&applicationSettings](const chs::ApplicationSettingsApiMock::tenantsTenantApplicationsApplicationNameSettingsContextGetParams& params,
                csp::services::ApiResponseHandlerBase* responseHandler, csp::common::CancellationToken& /*CancellationToken*/
            )
            {
                EXPECT_EQ(applicationSettings.ApplicationName, params.applicationName);
                EXPECT_EQ(applicationSettings.Context, params.context);

                csp::web::HttpPayload payload;
                auto json = csp::json::JsonSerializer::Serialize(applicationSettings);
                payload.AddHeader(CSP_TEXT("Content-Type"), CSP_TEXT("application/json"));
                payload.SetContent(json);

                auto response = csp::web::HttpResponse();
                response.SetResponseCode(csp::web::EResponseCodes::ResponseNotFound);
                response.GetMutablePayload() = payload;

                responseHandler->OnHttpResponse(response);
            });

    auto callback = [&resultPromise](const ApplicationSettingsResult& result) { resultPromise.set_value(result); };

    auto responseHandler
        = applicationSettingsMock->CreateHandler<ApplicationSettingsResultCallback, ApplicationSettingsResult, void, chs::ApplicationSettingsDto>(
            callback, nullptr, csp::web::EResponseCodes::ResponseNotFound);

    auto params = chs::ApplicationSettingsApiMock::tenantsTenantApplicationsApplicationNameSettingsContextGetParams { "OKO_TESTS",
        applicationSettings.ApplicationName, applicationSettings.Context, {} };

    applicationSettingsMock->tenantsTenantApplicationsApplicationNameSettingsContextGet(params, responseHandler, CancellationToken::Dummy());

    auto result = resultFuture.get();
    EXPECT_EQ(result.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(result.GetHttpResultCode(), static_cast<uint16_t>(csp::web::EResponseCodes::ResponseNotFound));
    EXPECT_EQ(result.GetFailureReason(), csp::systems::ERequestFailureReason::None);

    const auto applicationSettingsResult = result.GetApplicationSettings();
    EXPECT_EQ(applicationSettingsResult.ApplicationName, applicationSettings.ApplicationName);
    EXPECT_EQ(applicationSettingsResult.Context, applicationSettings.Context);
    EXPECT_EQ(applicationSettingsResult.AllowAnonymous, applicationSettings.AllowAnonymous);
    EXPECT_EQ(applicationSettingsResult.Settings.Size(), applicationSettings.Settings.Size());
}