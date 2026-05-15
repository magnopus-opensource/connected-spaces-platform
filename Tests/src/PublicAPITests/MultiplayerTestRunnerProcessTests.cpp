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

#include "MultiplayerTestRunnerProcess.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"

#include "PublicAPITests/UserSystemTestHelpers.h"
#include "gtest/gtest.h"

CSP_PUBLIC_TEST(CSPEngine, MultiplayerTestRunnerProcessTests, ArgTest)
{
    MultiplayerTestRunnerProcess process(MultiplayerTestRunner::TestIdentifiers::TestIdentifier::CREATE_AVATAR);
    EXPECT_EQ(process.GetTestToRun(), MultiplayerTestRunner::TestIdentifiers::TestIdentifier::CREATE_AVATAR);
    process.SetLoginEmail("FakeEmail@MrMoustacheMan.com");
    EXPECT_EQ(process.GetLoginEmail(), std::string { "FakeEmail@MrMoustacheMan.com" });
    process.SetPassword("Hunter2");
    EXPECT_EQ(process.GetPassword(), std::string { "Hunter2" });
    EXPECT_EQ(process.GetInvocationArgs(),
        (std::vector<std::string> {
            MULTIPLAYER_TEST_RUNNER_PATH, "--test", "CreateAvatar", "--email", "FakeEmail@MrMoustacheMan.com", "--password", "Hunter2" }));

    // Optional arguments have no value initially
    EXPECT_FALSE(process.GetSpaceId().has_value());
    EXPECT_FALSE(process.GetTimeoutInSeconds().has_value());
    EXPECT_FALSE(process.GetEndpoint().has_value());

    process.SetSpaceId("MyFakeSpaceId");
    EXPECT_EQ(process.GetSpaceId(), std::optional<std::string> { "MyFakeSpaceId" });
    EXPECT_EQ(process.GetInvocationArgs(),
        (std::vector<std::string> { MULTIPLAYER_TEST_RUNNER_PATH, "--test", "CreateAvatar", "--email", "FakeEmail@MrMoustacheMan.com", "--password",
            "Hunter2", "--space", "MyFakeSpaceId" }));

    process.SetTimeoutInSeconds(5);
    EXPECT_EQ(process.GetTimeoutInSeconds(), std::optional<int> { 5 });
    EXPECT_EQ(process.GetInvocationArgs(),
        (std::vector<std::string> { MULTIPLAYER_TEST_RUNNER_PATH, "--test", "CreateAvatar", "--email", "FakeEmail@MrMoustacheMan.com", "--password",
            "Hunter2", "--space", "MyFakeSpaceId", "--timeout", "5" }));

    process.SetEndpoint("https://www.website.com");
    EXPECT_EQ(process.GetEndpoint(), std::optional<std::string> { "https://www.website.com" });
    EXPECT_EQ(process.GetInvocationArgs(),
        (std::vector<std::string> { MULTIPLAYER_TEST_RUNNER_PATH, "--test", "CreateAvatar", "--email", "FakeEmail@MrMoustacheMan.com", "--password",
            "Hunter2", "--space", "MyFakeSpaceId", "--timeout", "5", "--endpoint", "https://www.website.com" }));
}

CSP_PUBLIC_TEST(CSPEngine, MultiplayerTestRunnerProcessTests, FutureTest)
{
    // Actually invoke the runner and make sure the futures are all set
    MultiplayerTestRunnerProcess process(MultiplayerTestRunner::TestIdentifiers::TestIdentifier::CREATE_AVATAR);
    auto testUser = CreateTestUser();
    process.SetLoginEmail(testUser.Email.c_str());
    process.SetPassword(GeneratedTestAccountPassword);
    process.SetTimeoutInSeconds(0); // So we don't sit at ready for assertions for any real time.
    process.SetEndpoint(EndpointBaseURI());
    process.StartProcess();

    // We need to spin up a process, login, create a space, join it, ... so we're a bit permissive with the timeouts to try and prevent flakiness.
    auto loggedInFutureStatus = process.LoggedInFuture().wait_for(std::chrono::seconds(20));
    EXPECT_EQ(loggedInFutureStatus, std::future_status::ready);

    auto joinedSpaceFutureStatus = process.JoinedSpaceFuture().wait_for(std::chrono::seconds(20));
    EXPECT_EQ(joinedSpaceFutureStatus, std::future_status::ready);

    auto readForAssertionsFutureStatus = process.ReadyForAssertionsFuture().wait_for(std::chrono::seconds(20));
    EXPECT_EQ(readForAssertionsFutureStatus, std::future_status::ready);

    auto exitSpaceFutureStatus = process.ExitSpaceFuture().wait_for(std::chrono::seconds(20));
    EXPECT_EQ(exitSpaceFutureStatus, std::future_status::ready);

    auto loggedOutFutureStatus = process.LoggedOutFuture().wait_for(std::chrono::seconds(20));
    EXPECT_EQ(loggedOutFutureStatus, std::future_status::ready);
}
