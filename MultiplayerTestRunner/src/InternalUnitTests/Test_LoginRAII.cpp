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
#include "../../include/ProcessDescriptors.h"
#include "../LoginRAII.h"
#include "../Utils.h"

#include <CSP/Common/String.h>
#include <gtest/gtest.h>
#include <optional>

namespace
{
/* Some tests only run if there's a credentials file */
std::optional<Utils::TestAccountCredentials> CredentialsFromFile()
{
    try
    {
        return Utils::LoadTestAccountCredentials();
    }
    catch (...)
    {
        return {};
    }
}
} // namespace

/* Initialze CSP before the suite begins with a fixture */
class LoginRAIITest : public ::testing::Test
{
protected:
    static void SetUpTestSuite() { Utils::InitialiseCSPWithUserAgentInfo(Utils::DEFAULT_TEST_ENDPOINT); }
};

TEST_F(LoginRAIITest, TestValidLogin)
{
    std::optional<Utils::TestAccountCredentials> Credentials = CredentialsFromFile();
    if (!Credentials.has_value())
    {
        GTEST_SKIP() << "No credentials file found, Skipping Test.";
    }

    {
        // Check the process descriptor is printed.
        ::testing::internal::CaptureStdout();
        LoginRAII login { Credentials.value().DefaultLoginEmail, Credentials.value().DefaultLoginPassword };
        EXPECT_NE(::testing::internal::GetCapturedStdout().find(MultiplayerTestRunner::ProcessDescriptors::LOGGED_IN_DESCRIPTOR), std::string::npos);
        ::testing::internal::CaptureStdout(); // GetCapturedStdout stops capturing, so we want to start again for the logout descriptor
    }
    // Scope exit, should logout
    EXPECT_NE(::testing::internal::GetCapturedStdout().find(MultiplayerTestRunner::ProcessDescriptors::LOGGED_OUT_DESCRIPTOR), std::string::npos);
}

TEST_F(LoginRAIITest, TestInvalidLogin)
{
    ::testing::internal::CaptureStdout();
    try
    {
        LoginRAII login { "FakeName", "FakePassword" };
    }
    catch (const Utils::ExceptionWithCode& Exception)
    {
        EXPECT_EQ(Exception.ErrorCode, MultiplayerTestRunner::ErrorCodes::FAILED_TO_LOGIN);
        EXPECT_NE(std::string(Exception.what()).find("Failed to login to service, got result code 3"), std::string::npos);
    }
    catch (...)
    {
        FAIL() << "Unexpected exception type thrown";
    }

    // Check neither of the process descriptors are emitted
    const auto CapturedOut = ::testing::internal::GetCapturedStdout();
    EXPECT_EQ(CapturedOut.find(MultiplayerTestRunner::ProcessDescriptors::LOGGED_IN_DESCRIPTOR), std::string::npos);
    EXPECT_EQ(CapturedOut.find(MultiplayerTestRunner::ProcessDescriptors::LOGGED_OUT_DESCRIPTOR), std::string::npos);
}
