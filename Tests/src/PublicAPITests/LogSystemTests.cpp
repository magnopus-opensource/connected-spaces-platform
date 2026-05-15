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
#include "CSP/Systems/SystemsManager.h"
#include "Debug/Logging.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"

#include "gtest/gtest.h"
#include <atomic>

void LogMessageLevelTest(const csp::common::LogLevel level, const csp::common::String& testMsg, std::atomic_bool& logConfirmed, bool expected)
{
    logConfirmed = false;
    CSP_LOG_MSG(level, testMsg);
    if (expected)
    {
        EXPECT_TRUE(logConfirmed);
    }
    else
    {
        EXPECT_FALSE(logConfirmed);
    }
}

void LogFormatLevelTest(
    const csp::common::LogLevel level, const csp::common::String& testMsg, const int& testValue, std::atomic_bool& logConfirmed, bool expected)
{
    logConfirmed = false;
    CSP_LOG_FORMAT(level, testMsg, testValue);
    if (expected)
    {
        EXPECT_TRUE(logConfirmed);
    }
    else
    {
        EXPECT_FALSE(logConfirmed);
    }
}

CSP_INTERNAL_TEST(CSPEngine, LogSystemTests, LogMessageTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto& logSystem = *systemsManager.GetLogSystem();

    std::atomic_bool logConfirmed = false;
    const csp::common::String testMsg = "Log Message";

    logSystem.SetLogCallback([&](csp::common::LogLevel, csp::common::String inMessage) { logConfirmed = inMessage == testMsg; });

    // Test the default
    CSP_LOG_MSG(csp::common::LogLevel::All, testMsg);
    EXPECT_TRUE(logConfirmed);

    // No Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::NoLogging);
    LogMessageLevelTest(csp::common::LogLevel::Fatal, testMsg, logConfirmed, false);
    LogMessageLevelTest(csp::common::LogLevel::Error, testMsg, logConfirmed, false);
    LogMessageLevelTest(csp::common::LogLevel::Warning, testMsg, logConfirmed, false);
    LogMessageLevelTest(csp::common::LogLevel::Display, testMsg, logConfirmed, false);
    LogMessageLevelTest(csp::common::LogLevel::Log, testMsg, logConfirmed, false);
    LogMessageLevelTest(csp::common::LogLevel::Verbose, testMsg, logConfirmed, false);
    LogMessageLevelTest(csp::common::LogLevel::VeryVerbose, testMsg, logConfirmed, false);

    // Fatal Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::Fatal);
    LogMessageLevelTest(csp::common::LogLevel::Fatal, testMsg, logConfirmed, true);
    LogMessageLevelTest(csp::common::LogLevel::Error, testMsg, logConfirmed, false);
    LogMessageLevelTest(csp::common::LogLevel::Warning, testMsg, logConfirmed, false);
    LogMessageLevelTest(csp::common::LogLevel::Display, testMsg, logConfirmed, false);
    LogMessageLevelTest(csp::common::LogLevel::Log, testMsg, logConfirmed, false);
    LogMessageLevelTest(csp::common::LogLevel::Verbose, testMsg, logConfirmed, false);
    LogMessageLevelTest(csp::common::LogLevel::VeryVerbose, testMsg, logConfirmed, false);

    // Error Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::Error);
    LogMessageLevelTest(csp::common::LogLevel::Fatal, testMsg, logConfirmed, true);
    LogMessageLevelTest(csp::common::LogLevel::Error, testMsg, logConfirmed, true);
    LogMessageLevelTest(csp::common::LogLevel::Warning, testMsg, logConfirmed, false);
    LogMessageLevelTest(csp::common::LogLevel::Display, testMsg, logConfirmed, false);
    LogMessageLevelTest(csp::common::LogLevel::Log, testMsg, logConfirmed, false);
    LogMessageLevelTest(csp::common::LogLevel::Verbose, testMsg, logConfirmed, false);
    LogMessageLevelTest(csp::common::LogLevel::VeryVerbose, testMsg, logConfirmed, false);

    // Warning Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::Warning);
    LogMessageLevelTest(csp::common::LogLevel::Fatal, testMsg, logConfirmed, true);
    LogMessageLevelTest(csp::common::LogLevel::Error, testMsg, logConfirmed, true);
    LogMessageLevelTest(csp::common::LogLevel::Warning, testMsg, logConfirmed, true);
    LogMessageLevelTest(csp::common::LogLevel::Display, testMsg, logConfirmed, false);
    LogMessageLevelTest(csp::common::LogLevel::Log, testMsg, logConfirmed, false);
    LogMessageLevelTest(csp::common::LogLevel::Verbose, testMsg, logConfirmed, false);
    LogMessageLevelTest(csp::common::LogLevel::VeryVerbose, testMsg, logConfirmed, false);

    // Display Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::Display);
    LogMessageLevelTest(csp::common::LogLevel::Fatal, testMsg, logConfirmed, true);
    LogMessageLevelTest(csp::common::LogLevel::Error, testMsg, logConfirmed, true);
    LogMessageLevelTest(csp::common::LogLevel::Warning, testMsg, logConfirmed, true);
    LogMessageLevelTest(csp::common::LogLevel::Display, testMsg, logConfirmed, true);
    LogMessageLevelTest(csp::common::LogLevel::Log, testMsg, logConfirmed, false);
    LogMessageLevelTest(csp::common::LogLevel::Verbose, testMsg, logConfirmed, false);
    LogMessageLevelTest(csp::common::LogLevel::VeryVerbose, testMsg, logConfirmed, false);

    // Log Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::Log);
    LogMessageLevelTest(csp::common::LogLevel::Fatal, testMsg, logConfirmed, true);
    LogMessageLevelTest(csp::common::LogLevel::Error, testMsg, logConfirmed, true);
    LogMessageLevelTest(csp::common::LogLevel::Warning, testMsg, logConfirmed, true);
    LogMessageLevelTest(csp::common::LogLevel::Display, testMsg, logConfirmed, true);
    LogMessageLevelTest(csp::common::LogLevel::Log, testMsg, logConfirmed, true);
    LogMessageLevelTest(csp::common::LogLevel::Verbose, testMsg, logConfirmed, false);
    LogMessageLevelTest(csp::common::LogLevel::VeryVerbose, testMsg, logConfirmed, false);

    // Verbose Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::Verbose);
    LogMessageLevelTest(csp::common::LogLevel::Fatal, testMsg, logConfirmed, true);
    LogMessageLevelTest(csp::common::LogLevel::Error, testMsg, logConfirmed, true);
    LogMessageLevelTest(csp::common::LogLevel::Warning, testMsg, logConfirmed, true);
    LogMessageLevelTest(csp::common::LogLevel::Display, testMsg, logConfirmed, true);
    LogMessageLevelTest(csp::common::LogLevel::Log, testMsg, logConfirmed, true);
    LogMessageLevelTest(csp::common::LogLevel::Verbose, testMsg, logConfirmed, true);
    LogMessageLevelTest(csp::common::LogLevel::VeryVerbose, testMsg, logConfirmed, false);

    // VeryVerbose Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::VeryVerbose);
    LogMessageLevelTest(csp::common::LogLevel::Fatal, testMsg, logConfirmed, true);
    LogMessageLevelTest(csp::common::LogLevel::Error, testMsg, logConfirmed, true);
    LogMessageLevelTest(csp::common::LogLevel::Warning, testMsg, logConfirmed, true);
    LogMessageLevelTest(csp::common::LogLevel::Display, testMsg, logConfirmed, true);
    LogMessageLevelTest(csp::common::LogLevel::Log, testMsg, logConfirmed, true);
    LogMessageLevelTest(csp::common::LogLevel::Verbose, testMsg, logConfirmed, true);
    LogMessageLevelTest(csp::common::LogLevel::VeryVerbose, testMsg, logConfirmed, true);

    // All Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::All);
    LogMessageLevelTest(csp::common::LogLevel::Fatal, testMsg, logConfirmed, true);
    LogMessageLevelTest(csp::common::LogLevel::Error, testMsg, logConfirmed, true);
    LogMessageLevelTest(csp::common::LogLevel::Warning, testMsg, logConfirmed, true);
    LogMessageLevelTest(csp::common::LogLevel::Display, testMsg, logConfirmed, true);
    LogMessageLevelTest(csp::common::LogLevel::Log, testMsg, logConfirmed, true);
    LogMessageLevelTest(csp::common::LogLevel::Verbose, testMsg, logConfirmed, true);
    LogMessageLevelTest(csp::common::LogLevel::VeryVerbose, testMsg, logConfirmed, true);

    logSystem.ClearAllCallbacks();

    csp::CSPFoundation::Shutdown();
}

CSP_INTERNAL_TEST(CSPEngine, LogSystemTests, LogFormatTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto& logSystem = *systemsManager.GetLogSystem();

    const csp::common::String testMsg = "Test Value is 12345";
    const csp::common::String testFormatStr = "Test Value is %d";
    const int testValue = 12345;

    std::atomic_bool logConfirmed = false;

    logSystem.SetLogCallback([&](csp::common::LogLevel, csp::common::String inMessage) { logConfirmed = inMessage == testMsg; });

    // Test default
    CSP_LOG_FORMAT(csp::common::LogLevel::Log, testFormatStr, testValue);
    EXPECT_TRUE(logConfirmed);

    // No Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::NoLogging);
    LogFormatLevelTest(csp::common::LogLevel::Fatal, testMsg, testValue, logConfirmed, false);
    LogFormatLevelTest(csp::common::LogLevel::Error, testMsg, testValue, logConfirmed, false);
    LogFormatLevelTest(csp::common::LogLevel::Warning, testMsg, testValue, logConfirmed, false);
    LogFormatLevelTest(csp::common::LogLevel::Display, testMsg, testValue, logConfirmed, false);
    LogFormatLevelTest(csp::common::LogLevel::Log, testMsg, testValue, logConfirmed, false);
    LogFormatLevelTest(csp::common::LogLevel::Verbose, testMsg, testValue, logConfirmed, false);
    LogFormatLevelTest(csp::common::LogLevel::VeryVerbose, testMsg, testValue, logConfirmed, false);

    // Fatal Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::Fatal);
    LogFormatLevelTest(csp::common::LogLevel::Fatal, testMsg, testValue, logConfirmed, true);
    LogFormatLevelTest(csp::common::LogLevel::Error, testMsg, testValue, logConfirmed, false);
    LogFormatLevelTest(csp::common::LogLevel::Warning, testMsg, testValue, logConfirmed, false);
    LogFormatLevelTest(csp::common::LogLevel::Display, testMsg, testValue, logConfirmed, false);
    LogFormatLevelTest(csp::common::LogLevel::Log, testMsg, testValue, logConfirmed, false);
    LogFormatLevelTest(csp::common::LogLevel::Verbose, testMsg, testValue, logConfirmed, false);
    LogFormatLevelTest(csp::common::LogLevel::VeryVerbose, testMsg, testValue, logConfirmed, false);

    // Error Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::Error);
    LogFormatLevelTest(csp::common::LogLevel::Fatal, testMsg, testValue, logConfirmed, true);
    LogFormatLevelTest(csp::common::LogLevel::Error, testMsg, testValue, logConfirmed, true);
    LogFormatLevelTest(csp::common::LogLevel::Warning, testMsg, testValue, logConfirmed, false);
    LogFormatLevelTest(csp::common::LogLevel::Display, testMsg, testValue, logConfirmed, false);
    LogFormatLevelTest(csp::common::LogLevel::Log, testMsg, testValue, logConfirmed, false);
    LogFormatLevelTest(csp::common::LogLevel::Verbose, testMsg, testValue, logConfirmed, false);
    LogFormatLevelTest(csp::common::LogLevel::VeryVerbose, testMsg, testValue, logConfirmed, false);

    // Warning Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::Warning);
    LogFormatLevelTest(csp::common::LogLevel::Fatal, testMsg, testValue, logConfirmed, true);
    LogFormatLevelTest(csp::common::LogLevel::Error, testMsg, testValue, logConfirmed, true);
    LogFormatLevelTest(csp::common::LogLevel::Warning, testMsg, testValue, logConfirmed, true);
    LogFormatLevelTest(csp::common::LogLevel::Display, testMsg, testValue, logConfirmed, false);
    LogFormatLevelTest(csp::common::LogLevel::Log, testMsg, testValue, logConfirmed, false);
    LogFormatLevelTest(csp::common::LogLevel::Verbose, testMsg, testValue, logConfirmed, false);
    LogFormatLevelTest(csp::common::LogLevel::VeryVerbose, testMsg, testValue, logConfirmed, false);

    // Display Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::Display);
    LogFormatLevelTest(csp::common::LogLevel::Fatal, testMsg, testValue, logConfirmed, true);
    LogFormatLevelTest(csp::common::LogLevel::Error, testMsg, testValue, logConfirmed, true);
    LogFormatLevelTest(csp::common::LogLevel::Warning, testMsg, testValue, logConfirmed, true);
    LogFormatLevelTest(csp::common::LogLevel::Display, testMsg, testValue, logConfirmed, true);
    LogFormatLevelTest(csp::common::LogLevel::Log, testMsg, testValue, logConfirmed, false);
    LogFormatLevelTest(csp::common::LogLevel::Verbose, testMsg, testValue, logConfirmed, false);
    LogFormatLevelTest(csp::common::LogLevel::VeryVerbose, testMsg, testValue, logConfirmed, false);

    // Log Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::Log);
    LogFormatLevelTest(csp::common::LogLevel::Fatal, testMsg, testValue, logConfirmed, true);
    LogFormatLevelTest(csp::common::LogLevel::Error, testMsg, testValue, logConfirmed, true);
    LogFormatLevelTest(csp::common::LogLevel::Warning, testMsg, testValue, logConfirmed, true);
    LogFormatLevelTest(csp::common::LogLevel::Display, testMsg, testValue, logConfirmed, true);
    LogFormatLevelTest(csp::common::LogLevel::Log, testMsg, testValue, logConfirmed, true);
    LogFormatLevelTest(csp::common::LogLevel::Verbose, testMsg, testValue, logConfirmed, false);
    LogFormatLevelTest(csp::common::LogLevel::VeryVerbose, testMsg, testValue, logConfirmed, false);

    // Verbose Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::Verbose);
    LogFormatLevelTest(csp::common::LogLevel::Fatal, testMsg, testValue, logConfirmed, true);
    LogFormatLevelTest(csp::common::LogLevel::Error, testMsg, testValue, logConfirmed, true);
    LogFormatLevelTest(csp::common::LogLevel::Warning, testMsg, testValue, logConfirmed, true);
    LogFormatLevelTest(csp::common::LogLevel::Display, testMsg, testValue, logConfirmed, true);
    LogFormatLevelTest(csp::common::LogLevel::Log, testMsg, testValue, logConfirmed, true);
    LogFormatLevelTest(csp::common::LogLevel::Verbose, testMsg, testValue, logConfirmed, true);
    LogFormatLevelTest(csp::common::LogLevel::VeryVerbose, testMsg, testValue, logConfirmed, false);

    // VeryVerbose Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::VeryVerbose);
    LogFormatLevelTest(csp::common::LogLevel::Fatal, testMsg, testValue, logConfirmed, true);
    LogFormatLevelTest(csp::common::LogLevel::Error, testMsg, testValue, logConfirmed, true);
    LogFormatLevelTest(csp::common::LogLevel::Warning, testMsg, testValue, logConfirmed, true);
    LogFormatLevelTest(csp::common::LogLevel::Display, testMsg, testValue, logConfirmed, true);
    LogFormatLevelTest(csp::common::LogLevel::Log, testMsg, testValue, logConfirmed, true);
    LogFormatLevelTest(csp::common::LogLevel::Verbose, testMsg, testValue, logConfirmed, true);
    LogFormatLevelTest(csp::common::LogLevel::VeryVerbose, testMsg, testValue, logConfirmed, true);

    // All Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::All);
    LogFormatLevelTest(csp::common::LogLevel::Fatal, testMsg, testValue, logConfirmed, true);
    LogFormatLevelTest(csp::common::LogLevel::Error, testMsg, testValue, logConfirmed, true);
    LogFormatLevelTest(csp::common::LogLevel::Warning, testMsg, testValue, logConfirmed, true);
    LogFormatLevelTest(csp::common::LogLevel::Display, testMsg, testValue, logConfirmed, true);
    LogFormatLevelTest(csp::common::LogLevel::Log, testMsg, testValue, logConfirmed, true);
    LogFormatLevelTest(csp::common::LogLevel::Verbose, testMsg, testValue, logConfirmed, true);
    LogFormatLevelTest(csp::common::LogLevel::VeryVerbose, testMsg, testValue, logConfirmed, true);

    logSystem.ClearAllCallbacks();

    csp::CSPFoundation::Shutdown();
}

CSP_INTERNAL_TEST(CSPEngine, LogSystemTests, LogErrorMessageTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto& logSystem = *systemsManager.GetLogSystem();

    std::atomic_bool logConfirmed = false;
    const csp::common::String testMsg = "Log Message";

    logSystem.SetLogCallback([&](csp::common::LogLevel, csp::common::String inMessage) { logConfirmed = inMessage == testMsg; });

    // Test the default
    CSP_LOG_ERROR_MSG(testMsg);
    EXPECT_TRUE(logConfirmed);

    // No Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::NoLogging);
    logConfirmed = false;
    CSP_LOG_ERROR_MSG(testMsg);
    EXPECT_FALSE(logConfirmed);

    // Fatal Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::Fatal);
    logConfirmed = false;
    CSP_LOG_ERROR_MSG(testMsg);
    EXPECT_FALSE(logConfirmed);

    // Error Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::Error);
    logConfirmed = false;
    CSP_LOG_ERROR_MSG(testMsg);
    EXPECT_TRUE(logConfirmed);

    // Warning Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::Warning);
    logConfirmed = false;
    CSP_LOG_ERROR_MSG(testMsg);
    EXPECT_TRUE(logConfirmed);

    // Display Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::Display);
    logConfirmed = false;
    CSP_LOG_ERROR_MSG(testMsg);
    EXPECT_TRUE(logConfirmed);

    // Log Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::Log);
    logConfirmed = false;
    CSP_LOG_ERROR_MSG(testMsg);
    EXPECT_TRUE(logConfirmed);

    // Verbose Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::Verbose);
    logConfirmed = false;
    CSP_LOG_ERROR_MSG(testMsg);
    EXPECT_TRUE(logConfirmed);

    // VeryVerbose Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::VeryVerbose);
    logConfirmed = false;
    CSP_LOG_ERROR_MSG(testMsg);
    EXPECT_TRUE(logConfirmed);

    // All Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::All);
    logConfirmed = false;
    CSP_LOG_ERROR_MSG(testMsg);
    EXPECT_TRUE(logConfirmed);

    logSystem.ClearAllCallbacks();

    csp::CSPFoundation::Shutdown();
}

CSP_INTERNAL_TEST(CSPEngine, LogSystemTests, LogWarnMessageTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto& logSystem = *systemsManager.GetLogSystem();

    std::atomic_bool logConfirmed = false;
    const csp::common::String testMsg = "Log Message";

    logSystem.SetLogCallback([&](csp::common::LogLevel, csp::common::String inMessage) { logConfirmed = inMessage == testMsg; });

    // Test the default
    CSP_LOG_WARN_MSG(testMsg);
    EXPECT_TRUE(logConfirmed);

    // No Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::NoLogging);
    logConfirmed = false;
    CSP_LOG_WARN_MSG(testMsg);
    EXPECT_FALSE(logConfirmed);

    // Fatal Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::Fatal);
    logConfirmed = false;
    CSP_LOG_WARN_MSG(testMsg);
    EXPECT_FALSE(logConfirmed);

    // Error Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::Error);
    logConfirmed = false;
    CSP_LOG_WARN_MSG(testMsg);
    EXPECT_FALSE(logConfirmed);

    // Warning Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::Warning);
    logConfirmed = false;
    CSP_LOG_WARN_MSG(testMsg);
    EXPECT_TRUE(logConfirmed);

    // Display Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::Display);
    logConfirmed = false;
    CSP_LOG_WARN_MSG(testMsg);
    EXPECT_TRUE(logConfirmed);

    // Log Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::Log);
    logConfirmed = false;
    CSP_LOG_WARN_MSG(testMsg);
    EXPECT_TRUE(logConfirmed);

    // Verbose Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::Verbose);
    logConfirmed = false;
    CSP_LOG_WARN_MSG(testMsg);
    EXPECT_TRUE(logConfirmed);

    // VeryVerbose Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::VeryVerbose);
    logConfirmed = false;
    CSP_LOG_WARN_MSG(testMsg);
    EXPECT_TRUE(logConfirmed);

    // All Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::All);
    logConfirmed = false;
    CSP_LOG_WARN_MSG(testMsg);
    EXPECT_TRUE(logConfirmed);

    logSystem.ClearAllCallbacks();

    csp::CSPFoundation::Shutdown();
}

CSP_INTERNAL_TEST(CSPEngine, LogSystemTests, LogWarnFormatTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto& logSystem = *systemsManager.GetLogSystem();

    const csp::common::String testMsg = "Test Value is 12345";
    const csp::common::String testFormatStr = "Test Value is %d";
    const int testValue = 12345;

    std::atomic_bool logConfirmed = false;

    logSystem.SetLogCallback([&](const csp::common::LogLevel, csp::common::String inMessage) { logConfirmed = inMessage == testMsg; });

    // Test default
    CSP_LOG_WARN_FORMAT(testFormatStr, testValue);
    EXPECT_TRUE(logConfirmed);

    // No Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::NoLogging);
    logConfirmed = false;
    CSP_LOG_WARN_FORMAT(testFormatStr, testValue);
    EXPECT_FALSE(logConfirmed);

    // Fatal Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::Fatal);
    logConfirmed = false;
    CSP_LOG_WARN_FORMAT(testFormatStr, testValue);
    EXPECT_FALSE(logConfirmed);

    // Error Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::Error);
    logConfirmed = false;
    CSP_LOG_WARN_FORMAT(testFormatStr, testValue);
    EXPECT_FALSE(logConfirmed);

    // Warning Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::Warning);
    logConfirmed = false;
    CSP_LOG_WARN_FORMAT(testFormatStr, testValue);
    EXPECT_TRUE(logConfirmed);

    // Display Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::Display);
    logConfirmed = false;
    CSP_LOG_WARN_FORMAT(testFormatStr, testValue);
    EXPECT_TRUE(logConfirmed);

    // Log Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::Log);
    logConfirmed = false;
    CSP_LOG_WARN_FORMAT(testFormatStr, testValue);
    EXPECT_TRUE(logConfirmed);

    // Verbose Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::Verbose);
    logConfirmed = false;
    CSP_LOG_WARN_FORMAT(testFormatStr, testValue);
    EXPECT_TRUE(logConfirmed);

    // VeryVerbose Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::VeryVerbose);
    logConfirmed = false;
    CSP_LOG_WARN_FORMAT(testFormatStr, testValue);
    EXPECT_TRUE(logConfirmed);

    // All Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::All);
    logConfirmed = false;
    CSP_LOG_WARN_FORMAT(testFormatStr, testValue);
    EXPECT_TRUE(logConfirmed);

    logSystem.ClearAllCallbacks();

    csp::CSPFoundation::Shutdown();
}

CSP_INTERNAL_TEST(CSPEngine, LogSystemTests, LogErrorFormatTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto& logSystem = *systemsManager.GetLogSystem();

    const csp::common::String testMsg = "Test Value is 12345";
    const csp::common::String testFormatStr = "Test Value is %d";
    const int testValue = 12345;

    std::atomic_bool logConfirmed = false;

    logSystem.SetLogCallback([&](const csp::common::LogLevel, csp::common::String inMessage) { logConfirmed = inMessage == testMsg; });

    // Test default
    CSP_LOG_ERROR_FORMAT(testFormatStr, testValue);
    EXPECT_TRUE(logConfirmed);

    // No Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::NoLogging);
    logConfirmed = false;
    CSP_LOG_ERROR_FORMAT(testFormatStr, testValue);
    EXPECT_FALSE(logConfirmed);

    // Fatal Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::Fatal);
    logConfirmed = false;
    CSP_LOG_ERROR_FORMAT(testFormatStr, testValue);
    EXPECT_FALSE(logConfirmed);

    // Error Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::Error);
    logConfirmed = false;
    CSP_LOG_ERROR_FORMAT(testFormatStr, testValue);
    EXPECT_TRUE(logConfirmed);

    // Warning Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::Warning);
    logConfirmed = false;
    CSP_LOG_ERROR_FORMAT(testFormatStr, testValue);
    EXPECT_TRUE(logConfirmed);

    // Display Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::Display);
    logConfirmed = false;
    CSP_LOG_ERROR_FORMAT(testFormatStr, testValue);
    EXPECT_TRUE(logConfirmed);

    // Log Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::Log);
    logConfirmed = false;
    CSP_LOG_ERROR_FORMAT(testFormatStr, testValue);
    EXPECT_TRUE(logConfirmed);

    // Verbose Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::Verbose);
    logConfirmed = false;
    CSP_LOG_ERROR_FORMAT(testFormatStr, testValue);
    EXPECT_TRUE(logConfirmed);

    // VeryVerbose Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::VeryVerbose);
    logConfirmed = false;
    CSP_LOG_ERROR_FORMAT(testFormatStr, testValue);
    EXPECT_TRUE(logConfirmed);

    // All Logging Level
    logSystem.SetSystemLevel(csp::common::LogLevel::All);
    logConfirmed = false;
    CSP_LOG_ERROR_FORMAT(testFormatStr, testValue);
    EXPECT_TRUE(logConfirmed);

    logSystem.ClearAllCallbacks();

    csp::CSPFoundation::Shutdown();
}

CSP_INTERNAL_TEST(CSPEngine, LogSystemTests, ProfileTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto& logSystem = *systemsManager.GetLogSystem();

    std::atomic_bool beginConfirmed = false;
    std::atomic_bool endConfirmed = false;
    std::atomic_bool eventConfirmed = false;

    const csp::common::String testTag = "Profile Marker";
    const csp::common::String testEvent = "Event Marker";

    logSystem.SetBeginMarkerCallback(
        [&](csp::common::String inMessage)
        {
            if (inMessage == testTag)
            {
                beginConfirmed = true;
            }

            std::cout << inMessage << std::endl;
        });

    logSystem.SetEndMarkerCallback([&](void*) { endConfirmed = true; });

    logSystem.SetEventCallback(
        [&](csp::common::String inMessage)
        {
            if (inMessage == testEvent)
            {
                eventConfirmed = true;
            }

            std::cout << inMessage << std::endl;
        });

#if CSP_PROFILING_ENABLED
    const int testValue = 12345;

    CSP_PROFILE_SCOPED_TAG(testTag);

    CSP_PROFILE_BEGIN(testTag);
    CSP_PROFILE_END();

    CSP_PROFILE_BEGIN_FORMAT("Marker %d", testValue);
    CSP_PROFILE_END();

    CSP_PROFILE_SCOPED_FORMAT("Marker %d", testValue);

    CSP_PROFILE_EVENT_TAG(testEvent);
    CSP_PROFILE_EVENT_FORMAT("Event %d", testValue)

    EXPECT_TRUE(beginConfirmed);
    EXPECT_TRUE(endConfirmed);
    EXPECT_TRUE(eventConfirmed);
#else
    EXPECT_FALSE(BeginConfirmed);
    EXPECT_FALSE(EndConfirmed);
    EXPECT_FALSE(EventConfirmed);
#endif
    logSystem.ClearAllCallbacks();

    csp::CSPFoundation::Shutdown();
}

CSP_INTERNAL_TEST(CSPEngine, LogSystemTests, FailureMessageTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto& logSystem = *systemsManager.GetLogSystem();

    std::atomic_bool logConfirmed = false;

    logSystem.SetLogCallback([&logConfirmed](const csp::common::LogLevel, csp::common::String inMessage) { logConfirmed = !inMessage.IsEmpty(); });

    csp::common::String userId;

    // Log in with invalid credentials
    LogIn(userSystem, userId, "invalidlogin@csp.co", "", true, true, csp::systems::TokenOptions(), csp::systems::EResultCode::Failed);

    auto start = std::chrono::steady_clock::now();
    auto current = std::chrono::steady_clock::now();
    long long testTime = 0;

    while (!logConfirmed && testTime < 20)
    {
        std::this_thread::sleep_for(50ms);

        current = std::chrono::steady_clock::now();
        testTime = std::chrono::duration_cast<std::chrono::seconds>(current - start).count();
    }

    EXPECT_TRUE(logConfirmed);

    csp::CSPFoundation::Shutdown();
}