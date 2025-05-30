/*
 * Copyright 2024 Magnopus LLC

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

#include "../../include/ErrorCodes.h"
#include "../../include/TestIdentifiers.h"
#include "../CLIArgs.h"

#include <gtest/gtest.h>

TEST(CLITest, AllArgsBasic)
{
    using namespace MultiplayerTestRunner::TestIdentifiers;
    std::string TestID = TestIdentifierToString(TestIdentifier::CREATE_AVATAR);
    std::vector<char*> args = { "MultiplayerTestRunner", "--test", TestID.data(), "--email", "test@example.com", "--password", "password123",
        "--space", "space-id-123", "--timeout", "60", "--endpoint", "https://example.com" };
    CLIArgs::RunnerSettings Settings = CLIArgs::ProcessCLI(static_cast<int>(args.size()), args.data());

    EXPECT_EQ(Settings.LoginEmailAndPassword.first, "test@example.com");
    EXPECT_EQ(Settings.LoginEmailAndPassword.second, "password123");
    EXPECT_EQ(Settings.TestIdentifier, TestIdentifier::CREATE_AVATAR);
    EXPECT_EQ(Settings.Endpoint, "https://example.com");
    EXPECT_EQ(Settings.TimeoutInSeconds, 60);
    EXPECT_EQ(Settings.SpaceId, "space-id-123");
}

TEST(CLITest, TestIdentifierRequired)
{
    std::vector<char*> args = { "MultiplayerTestRunner", "--email", "test@example.com", "--password", "password123", "--space", "space-id-123",
        "--timeout", "60", "--endpoint", "https://example.com" };

    try
    {
        CLIArgs::RunnerSettings Settings = CLIArgs::ProcessCLI(static_cast<int>(args.size()), args.data());
    }
    catch (const Utils::ExceptionWithCode& Exception)
    {
        EXPECT_EQ(Exception.ErrorCode, MultiplayerTestRunner::ErrorCodes::CLI_PARSE_ERROR);
        EXPECT_EQ(std::string(Exception.what()), std::string("--test is required"));
    }
    catch (...)
    {
        FAIL() << "Unexpected exception type thrown";
    }
}

TEST(CLITest, LoginEmailRequired)
{
    using namespace MultiplayerTestRunner::TestIdentifiers;
    std::string TestID = TestIdentifierToString(TestIdentifier::CREATE_AVATAR);
    std::vector<char*> args = { "MultiplayerTestRunner", "--test", TestID.data(), "--password", "password123", "--space", "space-id-123", "--timeout",
        "60", "--endpoint", "https://example.com" };

    try
    {
        CLIArgs::RunnerSettings Settings = CLIArgs::ProcessCLI(static_cast<int>(args.size()), args.data());
    }
    catch (const Utils::ExceptionWithCode& Exception)
    {
        EXPECT_EQ(Exception.ErrorCode, MultiplayerTestRunner::ErrorCodes::CLI_PARSE_ERROR);
        EXPECT_EQ(std::string(Exception.what()), std::string("--email is required"));
    }
    catch (...)
    {
        FAIL() << "Unexpected exception type thrown";
    }
}

TEST(CLITest, PasswordRequired)
{
    using namespace MultiplayerTestRunner::TestIdentifiers;
    std::string TestID = TestIdentifierToString(TestIdentifier::CREATE_AVATAR);
    std::vector<char*> args = { "MultiplayerTestRunner", "--test", TestID.data(), "--email", "test@example.com", "--password", "ergeqrheh", "--space",
        "space-id-123", "--timeout", "60", "--endpoint", "https://example.com" };

    try
    {
        CLIArgs::RunnerSettings Settings = CLIArgs::ProcessCLI(static_cast<int>(args.size()), args.data());
    }
    catch (const Utils::ExceptionWithCode& Exception)
    {
        EXPECT_EQ(Exception.ErrorCode, MultiplayerTestRunner::ErrorCodes::CLI_PARSE_ERROR);
        EXPECT_EQ(std::string(Exception.what()), std::string("--password is required"));
    }
    catch (...)
    {
        FAIL() << "Unexpected exception type thrown";
    }
}

TEST(CLITest, WhenInvalidTestIdentiferThenExceptionThrow)
{
    std::vector<char*> args
        = { "MultiplayerTestRunner", "--test", "NotARealTestIdentifier", "--email", "test@example.com", "--password", "password123" };

    try
    {
        CLIArgs::RunnerSettings Settings = CLIArgs::ProcessCLI(static_cast<int>(args.size()), args.data());
    }
    catch (const Utils::ExceptionWithCode& Exception)
    {
        EXPECT_EQ(Exception.ErrorCode, MultiplayerTestRunner::ErrorCodes::INVALID_TEST_SPECIFIER);
        EXPECT_EQ(std::string(Exception.what()), std::string("String `NotARealTestIdentifier` does not match any TestIdentifier"));
    }
    catch (...)
    {
        FAIL() << "Unexpected exception type thrown";
    }
}

TEST(CLITest, DefaultsSet)
{
    using namespace MultiplayerTestRunner::TestIdentifiers;
    std::string TestID = TestIdentifierToString(TestIdentifier::CREATE_AVATAR);
    std::vector<char*> args = { "MultiplayerTestRunner", "--test", TestID.data(), "--email", "test@example.com", "--password", "password123" };
    CLIArgs::RunnerSettings Settings = CLIArgs::ProcessCLI(static_cast<int>(args.size()), args.data());

    EXPECT_EQ(Settings.LoginEmailAndPassword.first, "test@example.com");
    EXPECT_EQ(Settings.LoginEmailAndPassword.second, "password123");
    EXPECT_EQ(Settings.TestIdentifier, TestIdentifier::CREATE_AVATAR);
    EXPECT_EQ(Settings.Endpoint, "https://ogs-internal.magnopus-dev.cloud");
    EXPECT_EQ(Settings.TimeoutInSeconds, 30);
    EXPECT_FALSE(Settings.SpaceId.has_value());
}
