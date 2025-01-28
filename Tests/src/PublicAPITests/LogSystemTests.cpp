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
#include "CSP/Systems/Log/LogSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "Debug/Logging.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"

#include "gtest/gtest.h"
#include <atomic>

void LogMessageLevelTest(const csp::systems::LogLevel Level, const csp::common::String& TestMsg, std::atomic_bool& LogConfirmed, bool Expected)
{
    LogConfirmed = false;
    CSP_LOG_MSG(Level, TestMsg);
    if (Expected)
    {
        EXPECT_TRUE(LogConfirmed);
    }
    else
    {
        EXPECT_FALSE(LogConfirmed);
    }
}

void LogFormatLevelTest(
    const csp::systems::LogLevel Level, const csp::common::String& TestMsg, const int& TestValue, std::atomic_bool& LogConfirmed, bool Expected)
{
    LogConfirmed = false;
    CSP_LOG_FORMAT(Level, TestMsg, TestValue);
    if (Expected)
    {
        EXPECT_TRUE(LogConfirmed);
    }
    else
    {
        EXPECT_FALSE(LogConfirmed);
    }
}

#if RUN_ALL_UNIT_TESTS || RUN_LOGSYSTEM_TESTS || RUN_LOGSYSTEM_LOG_MESSAGE_TEST
CSP_INTERNAL_TEST(CSPEngine, LogSystemTests, LogMessageTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto& LogSystem = *SystemsManager.GetLogSystem();

    std::atomic_bool LogConfirmed = false;
    const csp::common::String TestMsg = "Log Message";

    LogSystem.SetLogCallback([&](csp::common::String InMessage) { LogConfirmed = InMessage == TestMsg; });

    // Test the default
    CSP_LOG_MSG(csp::systems::LogLevel::All, TestMsg);
    EXPECT_TRUE(LogConfirmed);

    // No Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::NoLogging);
    LogMessageLevelTest(csp::systems::LogLevel::Fatal, TestMsg, LogConfirmed, false);
    LogMessageLevelTest(csp::systems::LogLevel::Error, TestMsg, LogConfirmed, false);
    LogMessageLevelTest(csp::systems::LogLevel::Warning, TestMsg, LogConfirmed, false);
    LogMessageLevelTest(csp::systems::LogLevel::Display, TestMsg, LogConfirmed, false);
    LogMessageLevelTest(csp::systems::LogLevel::Log, TestMsg, LogConfirmed, false);
    LogMessageLevelTest(csp::systems::LogLevel::Verbose, TestMsg, LogConfirmed, false);
    LogMessageLevelTest(csp::systems::LogLevel::VeryVerbose, TestMsg, LogConfirmed, false);

    // Fatal Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::Fatal);
    LogMessageLevelTest(csp::systems::LogLevel::Fatal, TestMsg, LogConfirmed, true);
    LogMessageLevelTest(csp::systems::LogLevel::Error, TestMsg, LogConfirmed, false);
    LogMessageLevelTest(csp::systems::LogLevel::Warning, TestMsg, LogConfirmed, false);
    LogMessageLevelTest(csp::systems::LogLevel::Display, TestMsg, LogConfirmed, false);
    LogMessageLevelTest(csp::systems::LogLevel::Log, TestMsg, LogConfirmed, false);
    LogMessageLevelTest(csp::systems::LogLevel::Verbose, TestMsg, LogConfirmed, false);
    LogMessageLevelTest(csp::systems::LogLevel::VeryVerbose, TestMsg, LogConfirmed, false);

    // Error Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::Error);
    LogMessageLevelTest(csp::systems::LogLevel::Fatal, TestMsg, LogConfirmed, true);
    LogMessageLevelTest(csp::systems::LogLevel::Error, TestMsg, LogConfirmed, true);
    LogMessageLevelTest(csp::systems::LogLevel::Warning, TestMsg, LogConfirmed, false);
    LogMessageLevelTest(csp::systems::LogLevel::Display, TestMsg, LogConfirmed, false);
    LogMessageLevelTest(csp::systems::LogLevel::Log, TestMsg, LogConfirmed, false);
    LogMessageLevelTest(csp::systems::LogLevel::Verbose, TestMsg, LogConfirmed, false);
    LogMessageLevelTest(csp::systems::LogLevel::VeryVerbose, TestMsg, LogConfirmed, false);

    // Warning Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::Warning);
    LogMessageLevelTest(csp::systems::LogLevel::Fatal, TestMsg, LogConfirmed, true);
    LogMessageLevelTest(csp::systems::LogLevel::Error, TestMsg, LogConfirmed, true);
    LogMessageLevelTest(csp::systems::LogLevel::Warning, TestMsg, LogConfirmed, true);
    LogMessageLevelTest(csp::systems::LogLevel::Display, TestMsg, LogConfirmed, false);
    LogMessageLevelTest(csp::systems::LogLevel::Log, TestMsg, LogConfirmed, false);
    LogMessageLevelTest(csp::systems::LogLevel::Verbose, TestMsg, LogConfirmed, false);
    LogMessageLevelTest(csp::systems::LogLevel::VeryVerbose, TestMsg, LogConfirmed, false);

    // Display Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::Display);
    LogMessageLevelTest(csp::systems::LogLevel::Fatal, TestMsg, LogConfirmed, true);
    LogMessageLevelTest(csp::systems::LogLevel::Error, TestMsg, LogConfirmed, true);
    LogMessageLevelTest(csp::systems::LogLevel::Warning, TestMsg, LogConfirmed, true);
    LogMessageLevelTest(csp::systems::LogLevel::Display, TestMsg, LogConfirmed, true);
    LogMessageLevelTest(csp::systems::LogLevel::Log, TestMsg, LogConfirmed, false);
    LogMessageLevelTest(csp::systems::LogLevel::Verbose, TestMsg, LogConfirmed, false);
    LogMessageLevelTest(csp::systems::LogLevel::VeryVerbose, TestMsg, LogConfirmed, false);

    // Log Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::Log);
    LogMessageLevelTest(csp::systems::LogLevel::Fatal, TestMsg, LogConfirmed, true);
    LogMessageLevelTest(csp::systems::LogLevel::Error, TestMsg, LogConfirmed, true);
    LogMessageLevelTest(csp::systems::LogLevel::Warning, TestMsg, LogConfirmed, true);
    LogMessageLevelTest(csp::systems::LogLevel::Display, TestMsg, LogConfirmed, true);
    LogMessageLevelTest(csp::systems::LogLevel::Log, TestMsg, LogConfirmed, true);
    LogMessageLevelTest(csp::systems::LogLevel::Verbose, TestMsg, LogConfirmed, false);
    LogMessageLevelTest(csp::systems::LogLevel::VeryVerbose, TestMsg, LogConfirmed, false);

    // Verbose Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::Verbose);
    LogMessageLevelTest(csp::systems::LogLevel::Fatal, TestMsg, LogConfirmed, true);
    LogMessageLevelTest(csp::systems::LogLevel::Error, TestMsg, LogConfirmed, true);
    LogMessageLevelTest(csp::systems::LogLevel::Warning, TestMsg, LogConfirmed, true);
    LogMessageLevelTest(csp::systems::LogLevel::Display, TestMsg, LogConfirmed, true);
    LogMessageLevelTest(csp::systems::LogLevel::Log, TestMsg, LogConfirmed, true);
    LogMessageLevelTest(csp::systems::LogLevel::Verbose, TestMsg, LogConfirmed, true);
    LogMessageLevelTest(csp::systems::LogLevel::VeryVerbose, TestMsg, LogConfirmed, false);

    // VeryVerbose Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::VeryVerbose);
    LogMessageLevelTest(csp::systems::LogLevel::Fatal, TestMsg, LogConfirmed, true);
    LogMessageLevelTest(csp::systems::LogLevel::Error, TestMsg, LogConfirmed, true);
    LogMessageLevelTest(csp::systems::LogLevel::Warning, TestMsg, LogConfirmed, true);
    LogMessageLevelTest(csp::systems::LogLevel::Display, TestMsg, LogConfirmed, true);
    LogMessageLevelTest(csp::systems::LogLevel::Log, TestMsg, LogConfirmed, true);
    LogMessageLevelTest(csp::systems::LogLevel::Verbose, TestMsg, LogConfirmed, true);
    LogMessageLevelTest(csp::systems::LogLevel::VeryVerbose, TestMsg, LogConfirmed, true);

    // All Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::All);
    LogMessageLevelTest(csp::systems::LogLevel::Fatal, TestMsg, LogConfirmed, true);
    LogMessageLevelTest(csp::systems::LogLevel::Error, TestMsg, LogConfirmed, true);
    LogMessageLevelTest(csp::systems::LogLevel::Warning, TestMsg, LogConfirmed, true);
    LogMessageLevelTest(csp::systems::LogLevel::Display, TestMsg, LogConfirmed, true);
    LogMessageLevelTest(csp::systems::LogLevel::Log, TestMsg, LogConfirmed, true);
    LogMessageLevelTest(csp::systems::LogLevel::Verbose, TestMsg, LogConfirmed, true);
    LogMessageLevelTest(csp::systems::LogLevel::VeryVerbose, TestMsg, LogConfirmed, true);

    LogSystem.ClearAllCallbacks();

    csp::CSPFoundation::Shutdown();
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_LOGSYSTEM_TESTS || RUN_LOGSYSTEM_LOG_FORMAT_TEST
CSP_INTERNAL_TEST(CSPEngine, LogSystemTests, LogFormatTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto& LogSystem = *SystemsManager.GetLogSystem();

    const csp::common::String TestMsg = "Test Value is 12345";
    const csp::common::String TestFormatStr = "Test Value is %d";
    const int TestValue = 12345;

    std::atomic_bool LogConfirmed = false;

    LogSystem.SetLogCallback([&](csp::common::String InMessage) { LogConfirmed = InMessage == TestMsg; });

    // Test default
    CSP_LOG_FORMAT(csp::systems::LogLevel::Log, TestFormatStr, TestValue);
    EXPECT_TRUE(LogConfirmed);

    // No Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::NoLogging);
    LogFormatLevelTest(csp::systems::LogLevel::Fatal, TestMsg, TestValue, LogConfirmed, false);
    LogFormatLevelTest(csp::systems::LogLevel::Error, TestMsg, TestValue, LogConfirmed, false);
    LogFormatLevelTest(csp::systems::LogLevel::Warning, TestMsg, TestValue, LogConfirmed, false);
    LogFormatLevelTest(csp::systems::LogLevel::Display, TestMsg, TestValue, LogConfirmed, false);
    LogFormatLevelTest(csp::systems::LogLevel::Log, TestMsg, TestValue, LogConfirmed, false);
    LogFormatLevelTest(csp::systems::LogLevel::Verbose, TestMsg, TestValue, LogConfirmed, false);
    LogFormatLevelTest(csp::systems::LogLevel::VeryVerbose, TestMsg, TestValue, LogConfirmed, false);

    // Fatal Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::Fatal);
    LogFormatLevelTest(csp::systems::LogLevel::Fatal, TestMsg, TestValue, LogConfirmed, true);
    LogFormatLevelTest(csp::systems::LogLevel::Error, TestMsg, TestValue, LogConfirmed, false);
    LogFormatLevelTest(csp::systems::LogLevel::Warning, TestMsg, TestValue, LogConfirmed, false);
    LogFormatLevelTest(csp::systems::LogLevel::Display, TestMsg, TestValue, LogConfirmed, false);
    LogFormatLevelTest(csp::systems::LogLevel::Log, TestMsg, TestValue, LogConfirmed, false);
    LogFormatLevelTest(csp::systems::LogLevel::Verbose, TestMsg, TestValue, LogConfirmed, false);
    LogFormatLevelTest(csp::systems::LogLevel::VeryVerbose, TestMsg, TestValue, LogConfirmed, false);

    // Error Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::Error);
    LogFormatLevelTest(csp::systems::LogLevel::Fatal, TestMsg, TestValue, LogConfirmed, true);
    LogFormatLevelTest(csp::systems::LogLevel::Error, TestMsg, TestValue, LogConfirmed, true);
    LogFormatLevelTest(csp::systems::LogLevel::Warning, TestMsg, TestValue, LogConfirmed, false);
    LogFormatLevelTest(csp::systems::LogLevel::Display, TestMsg, TestValue, LogConfirmed, false);
    LogFormatLevelTest(csp::systems::LogLevel::Log, TestMsg, TestValue, LogConfirmed, false);
    LogFormatLevelTest(csp::systems::LogLevel::Verbose, TestMsg, TestValue, LogConfirmed, false);
    LogFormatLevelTest(csp::systems::LogLevel::VeryVerbose, TestMsg, TestValue, LogConfirmed, false);

    // Warning Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::Warning);
    LogFormatLevelTest(csp::systems::LogLevel::Fatal, TestMsg, TestValue, LogConfirmed, true);
    LogFormatLevelTest(csp::systems::LogLevel::Error, TestMsg, TestValue, LogConfirmed, true);
    LogFormatLevelTest(csp::systems::LogLevel::Warning, TestMsg, TestValue, LogConfirmed, true);
    LogFormatLevelTest(csp::systems::LogLevel::Display, TestMsg, TestValue, LogConfirmed, false);
    LogFormatLevelTest(csp::systems::LogLevel::Log, TestMsg, TestValue, LogConfirmed, false);
    LogFormatLevelTest(csp::systems::LogLevel::Verbose, TestMsg, TestValue, LogConfirmed, false);
    LogFormatLevelTest(csp::systems::LogLevel::VeryVerbose, TestMsg, TestValue, LogConfirmed, false);

    // Display Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::Display);
    LogFormatLevelTest(csp::systems::LogLevel::Fatal, TestMsg, TestValue, LogConfirmed, true);
    LogFormatLevelTest(csp::systems::LogLevel::Error, TestMsg, TestValue, LogConfirmed, true);
    LogFormatLevelTest(csp::systems::LogLevel::Warning, TestMsg, TestValue, LogConfirmed, true);
    LogFormatLevelTest(csp::systems::LogLevel::Display, TestMsg, TestValue, LogConfirmed, true);
    LogFormatLevelTest(csp::systems::LogLevel::Log, TestMsg, TestValue, LogConfirmed, false);
    LogFormatLevelTest(csp::systems::LogLevel::Verbose, TestMsg, TestValue, LogConfirmed, false);
    LogFormatLevelTest(csp::systems::LogLevel::VeryVerbose, TestMsg, TestValue, LogConfirmed, false);

    // Log Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::Log);
    LogFormatLevelTest(csp::systems::LogLevel::Fatal, TestMsg, TestValue, LogConfirmed, true);
    LogFormatLevelTest(csp::systems::LogLevel::Error, TestMsg, TestValue, LogConfirmed, true);
    LogFormatLevelTest(csp::systems::LogLevel::Warning, TestMsg, TestValue, LogConfirmed, true);
    LogFormatLevelTest(csp::systems::LogLevel::Display, TestMsg, TestValue, LogConfirmed, true);
    LogFormatLevelTest(csp::systems::LogLevel::Log, TestMsg, TestValue, LogConfirmed, true);
    LogFormatLevelTest(csp::systems::LogLevel::Verbose, TestMsg, TestValue, LogConfirmed, false);
    LogFormatLevelTest(csp::systems::LogLevel::VeryVerbose, TestMsg, TestValue, LogConfirmed, false);

    // Verbose Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::Verbose);
    LogFormatLevelTest(csp::systems::LogLevel::Fatal, TestMsg, TestValue, LogConfirmed, true);
    LogFormatLevelTest(csp::systems::LogLevel::Error, TestMsg, TestValue, LogConfirmed, true);
    LogFormatLevelTest(csp::systems::LogLevel::Warning, TestMsg, TestValue, LogConfirmed, true);
    LogFormatLevelTest(csp::systems::LogLevel::Display, TestMsg, TestValue, LogConfirmed, true);
    LogFormatLevelTest(csp::systems::LogLevel::Log, TestMsg, TestValue, LogConfirmed, true);
    LogFormatLevelTest(csp::systems::LogLevel::Verbose, TestMsg, TestValue, LogConfirmed, true);
    LogFormatLevelTest(csp::systems::LogLevel::VeryVerbose, TestMsg, TestValue, LogConfirmed, false);

    // VeryVerbose Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::VeryVerbose);
    LogFormatLevelTest(csp::systems::LogLevel::Fatal, TestMsg, TestValue, LogConfirmed, true);
    LogFormatLevelTest(csp::systems::LogLevel::Error, TestMsg, TestValue, LogConfirmed, true);
    LogFormatLevelTest(csp::systems::LogLevel::Warning, TestMsg, TestValue, LogConfirmed, true);
    LogFormatLevelTest(csp::systems::LogLevel::Display, TestMsg, TestValue, LogConfirmed, true);
    LogFormatLevelTest(csp::systems::LogLevel::Log, TestMsg, TestValue, LogConfirmed, true);
    LogFormatLevelTest(csp::systems::LogLevel::Verbose, TestMsg, TestValue, LogConfirmed, true);
    LogFormatLevelTest(csp::systems::LogLevel::VeryVerbose, TestMsg, TestValue, LogConfirmed, true);

    // All Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::All);
    LogFormatLevelTest(csp::systems::LogLevel::Fatal, TestMsg, TestValue, LogConfirmed, true);
    LogFormatLevelTest(csp::systems::LogLevel::Error, TestMsg, TestValue, LogConfirmed, true);
    LogFormatLevelTest(csp::systems::LogLevel::Warning, TestMsg, TestValue, LogConfirmed, true);
    LogFormatLevelTest(csp::systems::LogLevel::Display, TestMsg, TestValue, LogConfirmed, true);
    LogFormatLevelTest(csp::systems::LogLevel::Log, TestMsg, TestValue, LogConfirmed, true);
    LogFormatLevelTest(csp::systems::LogLevel::Verbose, TestMsg, TestValue, LogConfirmed, true);
    LogFormatLevelTest(csp::systems::LogLevel::VeryVerbose, TestMsg, TestValue, LogConfirmed, true);

    LogSystem.ClearAllCallbacks();

    csp::CSPFoundation::Shutdown();
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_LOGSYSTEM_TESTS || RUN_LOGSYSTEM_LOG_ERROR_MESSAGE_TEST
CSP_INTERNAL_TEST(CSPEngine, LogSystemTests, LogErrorMessageTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto& LogSystem = *SystemsManager.GetLogSystem();

    std::atomic_bool LogConfirmed = false;
    const csp::common::String TestMsg = "Log Message";

    LogSystem.SetLogCallback([&](csp::common::String InMessage) { LogConfirmed = InMessage == TestMsg; });

    // Test the default
    CSP_LOG_ERROR_MSG(TestMsg);
    EXPECT_TRUE(LogConfirmed);

    // No Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::NoLogging);
    LogConfirmed = false;
    CSP_LOG_ERROR_MSG(TestMsg);
    EXPECT_FALSE(LogConfirmed);

    // Fatal Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::Fatal);
    LogConfirmed = false;
    CSP_LOG_ERROR_MSG(TestMsg);
    EXPECT_FALSE(LogConfirmed);

    // Error Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::Error);
    LogConfirmed = false;
    CSP_LOG_ERROR_MSG(TestMsg);
    EXPECT_TRUE(LogConfirmed);

    // Warning Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::Warning);
    LogConfirmed = false;
    CSP_LOG_ERROR_MSG(TestMsg);
    EXPECT_TRUE(LogConfirmed);

    // Display Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::Display);
    LogConfirmed = false;
    CSP_LOG_ERROR_MSG(TestMsg);
    EXPECT_TRUE(LogConfirmed);

    // Log Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::Log);
    LogConfirmed = false;
    CSP_LOG_ERROR_MSG(TestMsg);
    EXPECT_TRUE(LogConfirmed);

    // Verbose Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::Verbose);
    LogConfirmed = false;
    CSP_LOG_ERROR_MSG(TestMsg);
    EXPECT_TRUE(LogConfirmed);

    // VeryVerbose Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::VeryVerbose);
    LogConfirmed = false;
    CSP_LOG_ERROR_MSG(TestMsg);
    EXPECT_TRUE(LogConfirmed);

    // All Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::All);
    LogConfirmed = false;
    CSP_LOG_ERROR_MSG(TestMsg);
    EXPECT_TRUE(LogConfirmed);

    LogSystem.ClearAllCallbacks();

    csp::CSPFoundation::Shutdown();
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_LOGSYSTEM_TESTS || RUN_LOGSYSTEM_LOG_WARN_MESSAGE_TEST
CSP_INTERNAL_TEST(CSPEngine, LogSystemTests, LogWarnMessageTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto& LogSystem = *SystemsManager.GetLogSystem();

    std::atomic_bool LogConfirmed = false;
    const csp::common::String TestMsg = "Log Message";

    LogSystem.SetLogCallback([&](csp::common::String InMessage) { LogConfirmed = InMessage == TestMsg; });

    // Test the default
    CSP_LOG_WARN_MSG(TestMsg);
    EXPECT_TRUE(LogConfirmed);

    // No Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::NoLogging);
    LogConfirmed = false;
    CSP_LOG_WARN_MSG(TestMsg);
    EXPECT_FALSE(LogConfirmed);

    // Fatal Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::Fatal);
    LogConfirmed = false;
    CSP_LOG_WARN_MSG(TestMsg);
    EXPECT_FALSE(LogConfirmed);

    // Error Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::Error);
    LogConfirmed = false;
    CSP_LOG_WARN_MSG(TestMsg);
    EXPECT_FALSE(LogConfirmed);

    // Warning Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::Warning);
    LogConfirmed = false;
    CSP_LOG_WARN_MSG(TestMsg);
    EXPECT_TRUE(LogConfirmed);

    // Display Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::Display);
    LogConfirmed = false;
    CSP_LOG_WARN_MSG(TestMsg);
    EXPECT_TRUE(LogConfirmed);

    // Log Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::Log);
    LogConfirmed = false;
    CSP_LOG_WARN_MSG(TestMsg);
    EXPECT_TRUE(LogConfirmed);

    // Verbose Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::Verbose);
    LogConfirmed = false;
    CSP_LOG_WARN_MSG(TestMsg);
    EXPECT_TRUE(LogConfirmed);

    // VeryVerbose Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::VeryVerbose);
    LogConfirmed = false;
    CSP_LOG_WARN_MSG(TestMsg);
    EXPECT_TRUE(LogConfirmed);

    // All Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::All);
    LogConfirmed = false;
    CSP_LOG_WARN_MSG(TestMsg);
    EXPECT_TRUE(LogConfirmed);

    LogSystem.ClearAllCallbacks();

    csp::CSPFoundation::Shutdown();
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_LOGSYSTEM_TESTS || RUN_LOGSYSTEM_LOG_WARN_FORMAT_TEST
CSP_INTERNAL_TEST(CSPEngine, LogSystemTests, LogWarnFormatTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto& LogSystem = *SystemsManager.GetLogSystem();

    const csp::common::String TestMsg = "Test Value is 12345";
    const csp::common::String TestFormatStr = "Test Value is %d";
    const int TestValue = 12345;

    std::atomic_bool LogConfirmed = false;

    LogSystem.SetLogCallback([&](csp::common::String InMessage) { LogConfirmed = InMessage == TestMsg; });

    // Test default
    CSP_LOG_WARN_FORMAT(TestFormatStr, TestValue);
    EXPECT_TRUE(LogConfirmed);

    // No Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::NoLogging);
    LogConfirmed = false;
    CSP_LOG_WARN_FORMAT(TestFormatStr, TestValue);
    EXPECT_FALSE(LogConfirmed);

    // Fatal Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::Fatal);
    LogConfirmed = false;
    CSP_LOG_WARN_FORMAT(TestFormatStr, TestValue);
    EXPECT_FALSE(LogConfirmed);

    // Error Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::Error);
    LogConfirmed = false;
    CSP_LOG_WARN_FORMAT(TestFormatStr, TestValue);
    EXPECT_FALSE(LogConfirmed);

    // Warning Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::Warning);
    LogConfirmed = false;
    CSP_LOG_WARN_FORMAT(TestFormatStr, TestValue);
    EXPECT_TRUE(LogConfirmed);

    // Display Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::Display);
    LogConfirmed = false;
    CSP_LOG_WARN_FORMAT(TestFormatStr, TestValue);
    EXPECT_TRUE(LogConfirmed);

    // Log Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::Log);
    LogConfirmed = false;
    CSP_LOG_WARN_FORMAT(TestFormatStr, TestValue);
    EXPECT_TRUE(LogConfirmed);

    // Verbose Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::Verbose);
    LogConfirmed = false;
    CSP_LOG_WARN_FORMAT(TestFormatStr, TestValue);
    EXPECT_TRUE(LogConfirmed);

    // VeryVerbose Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::VeryVerbose);
    LogConfirmed = false;
    CSP_LOG_WARN_FORMAT(TestFormatStr, TestValue);
    EXPECT_TRUE(LogConfirmed);

    // All Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::All);
    LogConfirmed = false;
    CSP_LOG_WARN_FORMAT(TestFormatStr, TestValue);
    EXPECT_TRUE(LogConfirmed);

    LogSystem.ClearAllCallbacks();

    csp::CSPFoundation::Shutdown();
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_LOGSYSTEM_TESTS || RUN_LOGSYSTEM_LOG_ERROR_FORMAT_TEST
CSP_INTERNAL_TEST(CSPEngine, LogSystemTests, LogErrorFormatTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto& LogSystem = *SystemsManager.GetLogSystem();

    const csp::common::String TestMsg = "Test Value is 12345";
    const csp::common::String TestFormatStr = "Test Value is %d";
    const int TestValue = 12345;

    std::atomic_bool LogConfirmed = false;

    LogSystem.SetLogCallback([&](csp::common::String InMessage) { LogConfirmed = InMessage == TestMsg; });

    // Test default
    CSP_LOG_ERROR_FORMAT(TestFormatStr, TestValue);
    EXPECT_TRUE(LogConfirmed);

    // No Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::NoLogging);
    LogConfirmed = false;
    CSP_LOG_ERROR_FORMAT(TestFormatStr, TestValue);
    EXPECT_FALSE(LogConfirmed);

    // Fatal Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::Fatal);
    LogConfirmed = false;
    CSP_LOG_ERROR_FORMAT(TestFormatStr, TestValue);
    EXPECT_FALSE(LogConfirmed);

    // Error Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::Error);
    LogConfirmed = false;
    CSP_LOG_ERROR_FORMAT(TestFormatStr, TestValue);
    EXPECT_TRUE(LogConfirmed);

    // Warning Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::Warning);
    LogConfirmed = false;
    CSP_LOG_ERROR_FORMAT(TestFormatStr, TestValue);
    EXPECT_TRUE(LogConfirmed);

    // Display Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::Display);
    LogConfirmed = false;
    CSP_LOG_ERROR_FORMAT(TestFormatStr, TestValue);
    EXPECT_TRUE(LogConfirmed);

    // Log Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::Log);
    LogConfirmed = false;
    CSP_LOG_ERROR_FORMAT(TestFormatStr, TestValue);
    EXPECT_TRUE(LogConfirmed);

    // Verbose Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::Verbose);
    LogConfirmed = false;
    CSP_LOG_ERROR_FORMAT(TestFormatStr, TestValue);
    EXPECT_TRUE(LogConfirmed);

    // VeryVerbose Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::VeryVerbose);
    LogConfirmed = false;
    CSP_LOG_ERROR_FORMAT(TestFormatStr, TestValue);
    EXPECT_TRUE(LogConfirmed);

    // All Logging Level
    LogSystem.SetSystemLevel(csp::systems::LogLevel::All);
    LogConfirmed = false;
    CSP_LOG_ERROR_FORMAT(TestFormatStr, TestValue);
    EXPECT_TRUE(LogConfirmed);

    LogSystem.ClearAllCallbacks();

    csp::CSPFoundation::Shutdown();
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_LOGSYSTEM_TESTS || RUN_LOGSYSTEM_PROFILE_TEST
CSP_INTERNAL_TEST(CSPEngine, LogSystemTests, ProfileTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto& LogSystem = *SystemsManager.GetLogSystem();

    std::atomic_bool BeginConfirmed = false;
    std::atomic_bool EndConfirmed = false;
    std::atomic_bool EventConfirmed = false;

    const csp::common::String TestTag = "Profile Marker";
    const csp::common::String TestEvent = "Event Marker";

    LogSystem.SetBeginMarkerCallback(
        [&](csp::common::String InMessage)
        {
            if (InMessage == TestTag)
            {
                BeginConfirmed = true;
            }

            std::cout << InMessage << std::endl;
        });

    LogSystem.SetEndMarkerCallback([&](void*) { EndConfirmed = true; });

    LogSystem.SetEventCallback(
        [&](csp::common::String InMessage)
        {
            if (InMessage == TestEvent)
            {
                EventConfirmed = true;
            }

            std::cout << InMessage << std::endl;
        });

    const int TestValue = 12345;

    CSP_PROFILE_SCOPED_TAG(TestTag);

    CSP_PROFILE_BEGIN(TestTag);
    CSP_PROFILE_END();

    CSP_PROFILE_BEGIN_FORMAT("Marker %d", TestValue);
    CSP_PROFILE_END();

    CSP_PROFILE_SCOPED_FORMAT("Marker %d", TestValue);

    CSP_PROFILE_EVENT_TAG(TestEvent);
    CSP_PROFILE_EVENT_FORMAT("Event %d", TestValue)

#if CSP_PROFILING_ENABLED
    EXPECT_TRUE(BeginConfirmed);
    EXPECT_TRUE(EndConfirmed);
    EXPECT_TRUE(EventConfirmed);
#else
    EXPECT_FALSE(BeginConfirmed);
    EXPECT_FALSE(EndConfirmed);
    EXPECT_FALSE(EventConfirmed);
#endif

    LogSystem.ClearAllCallbacks();

    csp::CSPFoundation::Shutdown();
}

#endif
#if RUN_ALL_UNIT_TESTS || RUN_LOGSYSTEM_TESTS || RUN_LOGSYSTEM_FAILURE_MESSAGE_TEST
CSP_INTERNAL_TEST(CSPEngine, LogSystemTests, FailureMessageTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto& LogSystem = *SystemsManager.GetLogSystem();

    const csp::common::String Prefix = "Services";
    std::atomic_bool LogConfirmed = false;

    LogSystem.SetLogCallback(
        [&LogConfirmed, Prefix](csp::common::String InMessage)
        {
            if (InMessage.Split(' ')[0] == Prefix)
            {
                LogConfirmed = InMessage.Split(' ')[0] == Prefix;
                std::cout << InMessage << std::endl;
            }
        });

    csp::common::String UserId;

    // Log in with invalid credentials
    LogIn(UserSystem, UserId, "invalidlogin@csp.co", "", true, csp::systems::EResultCode::Failed);

    auto Start = std::chrono::steady_clock::now();
    auto Current = std::chrono::steady_clock::now();
    long long TestTime = 0;

    while (!LogConfirmed && TestTime < 20)
    {
        std::this_thread::sleep_for(50ms);

        Current = std::chrono::steady_clock::now();
        TestTime = std::chrono::duration_cast<std::chrono::seconds>(Current - Start).count();
    }

    EXPECT_TRUE(LogConfirmed);
}
#endif