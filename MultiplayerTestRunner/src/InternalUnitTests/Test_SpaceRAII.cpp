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
#include "../SpaceRAII.h"
#include "../Utils.h"

#include <CSP/Common/String.h>
#include <CSP/Systems/Spaces/SpaceSystem.h>
#include <CSP/Systems/SystemsManager.h>
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
class SpaceRAIITest : public ::testing::Test
{
protected:
    static void SetUpTestSuite() { Utils::InitialiseCSPWithUserAgentInfo(Utils::DEFAULT_TEST_ENDPOINT); }
};

TEST_F(SpaceRAIITest, TestCreateNewSpaceWhenLoggedIn)
{
    std::optional<Utils::TestAccountCredentials> Credentials = CredentialsFromFile();
    if (!Credentials.has_value())
    {
        GTEST_SKIP() << "No credentials file found, Skipping Test.";
    }
    ::testing::internal::CaptureStdout();

    // Login
    LoginRAII login { Credentials.value().DefaultLoginEmail, Credentials.value().DefaultLoginPassword };

    {
        // Check the create space and joined space process descriptors are printed when we don't provide a spaceID
        SpaceRAII Space({});
        const auto CapturedOut = ::testing::internal::GetCapturedStdout();
        const auto CreatedSpaceDescriptorPos = CapturedOut.find(MultiplayerTestRunner::ProcessDescriptors::CREATED_SPACE_DESCRIPTOR);
        const auto JoinedSpaceDescriptorPos = CapturedOut.find(MultiplayerTestRunner::ProcessDescriptors::JOINED_SPACE_DESCRIPTOR);
        EXPECT_NE(CreatedSpaceDescriptorPos, std::string::npos);
        EXPECT_NE(JoinedSpaceDescriptorPos, std::string::npos);
        EXPECT_TRUE(CreatedSpaceDescriptorPos < JoinedSpaceDescriptorPos); // Assert that we emitted creating the space before joining it.
        ::testing::internal::CaptureStdout(); // GetCapturedStdout stops capturing, so we want to start again for the leave space descriptor
    }
    // Scope exit, should logout
    const auto CapturedOut = ::testing::internal::GetCapturedStdout();
    const auto LeftSpaceDescriptorPos = CapturedOut.find(MultiplayerTestRunner::ProcessDescriptors::EXIT_SPACE_DESCRIPTOR);
    const auto DestroyedSpaceDescriptorPos = CapturedOut.find(MultiplayerTestRunner::ProcessDescriptors::DESTROYED_SPACE_DESCRIPTOR);
    EXPECT_NE(LeftSpaceDescriptorPos, std::string::npos);
    EXPECT_NE(DestroyedSpaceDescriptorPos, std::string::npos);
    EXPECT_TRUE(LeftSpaceDescriptorPos < DestroyedSpaceDescriptorPos); // Assert that we emitted leaving the space before we destroyed it
}

TEST_F(SpaceRAIITest, TestCreateNewSpaceWhenNotLoggedIn)
{
    try
    {
        ::testing::internal::CaptureStdout();
        SpaceRAII Space({});
    }
    catch (const Utils::ExceptionWithCode& Exception)
    {
        EXPECT_EQ(Exception.ErrorCode, MultiplayerTestRunner::ErrorCodes::FAILED_TO_CREATE_SPACE);
        EXPECT_NE(std::string(Exception.what()).find("HTTP Code: 401 Body: "), std::string::npos);
    }
    catch (...)
    {
        FAIL() << "Unexpected exception type thrown";
    }

    // Check none of the process descriptors are emitted
    const auto CapturedOut = ::testing::internal::GetCapturedStdout();
    EXPECT_EQ(CapturedOut.find(MultiplayerTestRunner::ProcessDescriptors::CREATED_SPACE_DESCRIPTOR), std::string::npos);
    EXPECT_EQ(CapturedOut.find(MultiplayerTestRunner::ProcessDescriptors::JOINED_SPACE_DESCRIPTOR), std::string::npos);
    EXPECT_EQ(CapturedOut.find(MultiplayerTestRunner::ProcessDescriptors::EXIT_SPACE_DESCRIPTOR), std::string::npos);
    EXPECT_EQ(CapturedOut.find(MultiplayerTestRunner::ProcessDescriptors::DESTROYED_SPACE_DESCRIPTOR), std::string::npos);
}

TEST_F(SpaceRAIITest, TestUseExistingSpace)
{
    std::optional<Utils::TestAccountCredentials> Credentials = CredentialsFromFile();
    if (!Credentials.has_value())
    {
        GTEST_SKIP() << "No credentials file found, Skipping Test.";
    }

    // Login
    LoginRAII login { Credentials.value().DefaultLoginEmail, Credentials.value().DefaultLoginPassword };

    // Create a space
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto& SpaceSystem = *SystemsManager.GetSpaceSystem();
    csp::systems::Space TestSpace = SpaceRAII::CreateDefaultTestSpace(SpaceSystem);

    {
        ::testing::internal::CaptureStdout();
        // Check the create space and joined space process descriptors are printed when we don't provide a spaceID
        SpaceRAII Space(std::optional<std::string> { TestSpace.Id.c_str() });
        const auto CapturedOut = ::testing::internal::GetCapturedStdout();
        const auto CreatedSpaceDescriptorPos = CapturedOut.find(MultiplayerTestRunner::ProcessDescriptors::CREATED_SPACE_DESCRIPTOR);
        const auto JoinedSpaceDescriptorPos = CapturedOut.find(MultiplayerTestRunner::ProcessDescriptors::JOINED_SPACE_DESCRIPTOR);
        EXPECT_EQ(CreatedSpaceDescriptorPos, std::string::npos); // Should not have emitted creating a space
        EXPECT_NE(JoinedSpaceDescriptorPos, std::string::npos); // Should have joined a space
        ::testing::internal::CaptureStdout(); // GetCapturedStdout stops capturing, so we want to start again for the leave space descriptor
    }
    // Scope exit, should logout
    const auto CapturedOut = ::testing::internal::GetCapturedStdout();
    const auto LeftSpaceDescriptorPos = CapturedOut.find(MultiplayerTestRunner::ProcessDescriptors::EXIT_SPACE_DESCRIPTOR);
    const auto DestroyedSpaceDescriptorPos = CapturedOut.find(MultiplayerTestRunner::ProcessDescriptors::DESTROYED_SPACE_DESCRIPTOR);
    EXPECT_NE(LeftSpaceDescriptorPos, std::string::npos);
    EXPECT_EQ(DestroyedSpaceDescriptorPos, std::string::npos); // Should not have emitted destroying a space
}

TEST_F(SpaceRAIITest, TestUseInvalidExistingSpace)
{
    constexpr const char* Invalid_SpaceID = "a-b-c-d-not-a-real-space-id";

    try
    {
        ::testing::internal::CaptureStdout();
        SpaceRAII Space({ Invalid_SpaceID });
    }
    catch (const Utils::ExceptionWithCode& Exception)
    {
        const std::string what = Exception.what();
        EXPECT_EQ(Exception.ErrorCode, MultiplayerTestRunner::ErrorCodes::FAILED_TO_ENTER_SPACE);
        EXPECT_NE(std::string(Exception.what()).find("HTTP Code: 401 Body: "), std::string::npos);
    }
    catch (...)
    {
        FAIL() << "Unexpected exception type thrown";
    }

    // Check none of the process descriptors are emitted
    const auto CapturedOut = ::testing::internal::GetCapturedStdout();
    EXPECT_EQ(CapturedOut.find(MultiplayerTestRunner::ProcessDescriptors::CREATED_SPACE_DESCRIPTOR), std::string::npos);
    EXPECT_EQ(CapturedOut.find(MultiplayerTestRunner::ProcessDescriptors::JOINED_SPACE_DESCRIPTOR), std::string::npos);
    EXPECT_EQ(CapturedOut.find(MultiplayerTestRunner::ProcessDescriptors::EXIT_SPACE_DESCRIPTOR), std::string::npos);
    EXPECT_EQ(CapturedOut.find(MultiplayerTestRunner::ProcessDescriptors::DESTROYED_SPACE_DESCRIPTOR), std::string::npos);
}