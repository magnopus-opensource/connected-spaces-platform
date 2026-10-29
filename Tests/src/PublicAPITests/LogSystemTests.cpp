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

void LogMessageLevelTest(const csp::common::LogLevel Level, const csp::common::String& TestMsg, std::atomic_bool& LogConfirmed, bool Expected,
    csp::common::LogSystem& LogSystem)
{
    LogConfirmed = false;
    LogSystem.LogMsg(Level, TestMsg);
    if (Expected)
    {
        EXPECT_TRUE(LogConfirmed);
    }
    else
    {
        EXPECT_FALSE(LogConfirmed);
    }
}

CSP_INTERNAL_TEST(CSPEngine, LogSystemTests, LogMessageTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto& LogSystem = *SystemsManager.GetLogSystem();

    std::atomic_bool LogConfirmed = false;
    const csp::common::String TestMsg = "Log Message";

    LogSystem.SetLogCallback([&](csp::common::LogLevel, csp::common::String InMessage) { LogConfirmed = InMessage == TestMsg; });

    // Test the default
    LogSystem.LogMsg(csp::common::LogLevel::All, TestMsg);
    EXPECT_TRUE(LogConfirmed);

    // No Logging Level
    LogSystem.SetSystemLevel(csp::common::LogLevel::NoLogging);
    LogMessageLevelTest(csp::common::LogLevel::Fatal, TestMsg, LogConfirmed, false, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Error, TestMsg, LogConfirmed, false, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Warning, TestMsg, LogConfirmed, false, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Display, TestMsg, LogConfirmed, false, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Log, TestMsg, LogConfirmed, false, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Verbose, TestMsg, LogConfirmed, false, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::VeryVerbose, TestMsg, LogConfirmed, false, LogSystem);

    // Fatal Logging Level
    LogSystem.SetSystemLevel(csp::common::LogLevel::Fatal);
    LogMessageLevelTest(csp::common::LogLevel::Fatal, TestMsg, LogConfirmed, true, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Error, TestMsg, LogConfirmed, false, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Warning, TestMsg, LogConfirmed, false, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Display, TestMsg, LogConfirmed, false, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Log, TestMsg, LogConfirmed, false, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Verbose, TestMsg, LogConfirmed, false, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::VeryVerbose, TestMsg, LogConfirmed, false, LogSystem);

    // Error Logging Level
    LogSystem.SetSystemLevel(csp::common::LogLevel::Error);
    LogMessageLevelTest(csp::common::LogLevel::Fatal, TestMsg, LogConfirmed, true, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Error, TestMsg, LogConfirmed, true, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Warning, TestMsg, LogConfirmed, false, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Display, TestMsg, LogConfirmed, false, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Log, TestMsg, LogConfirmed, false, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Verbose, TestMsg, LogConfirmed, false, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::VeryVerbose, TestMsg, LogConfirmed, false, LogSystem);

    // Warning Logging Level
    LogSystem.SetSystemLevel(csp::common::LogLevel::Warning);
    LogMessageLevelTest(csp::common::LogLevel::Fatal, TestMsg, LogConfirmed, true, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Error, TestMsg, LogConfirmed, true, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Warning, TestMsg, LogConfirmed, true, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Display, TestMsg, LogConfirmed, false, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Log, TestMsg, LogConfirmed, false, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Verbose, TestMsg, LogConfirmed, false, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::VeryVerbose, TestMsg, LogConfirmed, false, LogSystem);

    // Display Logging Level
    LogSystem.SetSystemLevel(csp::common::LogLevel::Display);
    LogMessageLevelTest(csp::common::LogLevel::Fatal, TestMsg, LogConfirmed, true, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Error, TestMsg, LogConfirmed, true, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Warning, TestMsg, LogConfirmed, true, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Display, TestMsg, LogConfirmed, true, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Log, TestMsg, LogConfirmed, false, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Verbose, TestMsg, LogConfirmed, false, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::VeryVerbose, TestMsg, LogConfirmed, false, LogSystem);

    // Log Logging Level
    LogSystem.SetSystemLevel(csp::common::LogLevel::Log);
    LogMessageLevelTest(csp::common::LogLevel::Fatal, TestMsg, LogConfirmed, true, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Error, TestMsg, LogConfirmed, true, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Warning, TestMsg, LogConfirmed, true, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Display, TestMsg, LogConfirmed, true, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Log, TestMsg, LogConfirmed, true, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Verbose, TestMsg, LogConfirmed, false, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::VeryVerbose, TestMsg, LogConfirmed, false, LogSystem);

    // Verbose Logging Level
    LogSystem.SetSystemLevel(csp::common::LogLevel::Verbose);
    LogMessageLevelTest(csp::common::LogLevel::Fatal, TestMsg, LogConfirmed, true, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Error, TestMsg, LogConfirmed, true, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Warning, TestMsg, LogConfirmed, true, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Display, TestMsg, LogConfirmed, true, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Log, TestMsg, LogConfirmed, true, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Verbose, TestMsg, LogConfirmed, true, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::VeryVerbose, TestMsg, LogConfirmed, false, LogSystem);

    // VeryVerbose Logging Level
    LogSystem.SetSystemLevel(csp::common::LogLevel::VeryVerbose);
    LogMessageLevelTest(csp::common::LogLevel::Fatal, TestMsg, LogConfirmed, true, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Error, TestMsg, LogConfirmed, true, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Warning, TestMsg, LogConfirmed, true, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Display, TestMsg, LogConfirmed, true, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Log, TestMsg, LogConfirmed, true, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Verbose, TestMsg, LogConfirmed, true, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::VeryVerbose, TestMsg, LogConfirmed, true, LogSystem);

    // All Logging Level
    LogSystem.SetSystemLevel(csp::common::LogLevel::All);
    LogMessageLevelTest(csp::common::LogLevel::Fatal, TestMsg, LogConfirmed, true, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Error, TestMsg, LogConfirmed, true, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Warning, TestMsg, LogConfirmed, true, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Display, TestMsg, LogConfirmed, true, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Log, TestMsg, LogConfirmed, true, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::Verbose, TestMsg, LogConfirmed, true, LogSystem);
    LogMessageLevelTest(csp::common::LogLevel::VeryVerbose, TestMsg, LogConfirmed, true, LogSystem);

    LogSystem.ClearAllCallbacks();

    csp::CSPFoundation::Shutdown();
}

CSP_INTERNAL_TEST(CSPEngine, LogSystemTests, MarkerAndEventCallbackTest)
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

    LogSystem.BeginMarker(TestTag);
    LogSystem.EndMarker();
    LogSystem.LogEvent(TestEvent);

    EXPECT_TRUE(BeginConfirmed);
    EXPECT_TRUE(EndConfirmed);
    EXPECT_TRUE(EventConfirmed);

    LogSystem.ClearAllCallbacks();

    csp::CSPFoundation::Shutdown();
}

CSP_INTERNAL_TEST(CSPEngine, LogSystemTests, FailureMessageTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto& LogSystem = *SystemsManager.GetLogSystem();

    std::atomic_bool LogConfirmed = false;

    LogSystem.SetLogCallback([&LogConfirmed](const csp::common::LogLevel, csp::common::String InMessage) { LogConfirmed = !InMessage.IsEmpty(); });

    csp::common::String UserId;

    // Log in with invalid credentials
    LogIn(UserSystem, UserId, "invalidlogin@csp.co", "", true, true, csp::systems::TokenOptions(), csp::systems::EResultCode::Failed);

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

    csp::CSPFoundation::Shutdown();
}