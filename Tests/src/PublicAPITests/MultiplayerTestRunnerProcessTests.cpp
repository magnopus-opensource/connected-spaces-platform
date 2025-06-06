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

#include "gtest/gtest.h"
#include <PublicAPITests/UserSystemTestHelpers.h>

CSP_PUBLIC_TEST(CSPEngine, MultiplayerTestRunnerProcessTests, ArgTest)
{
    MultiplayerTestRunnerProcess Process(MultiplayerTestRunner::TestIdentifiers::TestIdentifier::CREATE_AVATAR);
    EXPECT_EQ(Process.GetTestToRun(), MultiplayerTestRunner::TestIdentifiers::TestIdentifier::CREATE_AVATAR);
    Process.SetLoginEmail("FakeEmail@MrMoustacheMan.com");
    EXPECT_EQ(Process.GetLoginEmail(), std::string { "FakeEmail@MrMoustacheMan.com" });
    Process.SetPassword("Hunter2");
    EXPECT_EQ(Process.GetPassword(), std::string { "Hunter2" });
    EXPECT_EQ(Process.GetInvocationArgs(),
        (std::vector<std::string> {
            "MultiplayerTestRunner", "--test", "CreateAvatar", "--email", "FakeEmail@MrMoustacheMan.com", "--password", "Hunter2" }));

    // Optional arguments have no value initially
    EXPECT_FALSE(Process.GetSpaceId().has_value());
    EXPECT_FALSE(Process.GetTimeoutInSeconds().has_value());
    EXPECT_FALSE(Process.GetEndpoint().has_value());

    Process.SetSpaceId("MyFakeSpaceId");
    EXPECT_EQ(Process.GetSpaceId(), std::optional<std::string> { "MyFakeSpaceId" });
    EXPECT_EQ(Process.GetInvocationArgs(),
        (std::vector<std::string> { "MultiplayerTestRunner", "--test", "CreateAvatar", "--email", "FakeEmail@MrMoustacheMan.com", "--password",
            "Hunter2", "--space", "MyFakeSpaceId" }));

    Process.SetTimeoutInSeconds(5);
    EXPECT_EQ(Process.GetTimeoutInSeconds(), std::optional<int> { 5 });
    EXPECT_EQ(Process.GetInvocationArgs(),
        (std::vector<std::string> { "MultiplayerTestRunner", "--test", "CreateAvatar", "--email", "FakeEmail@MrMoustacheMan.com", "--password",
            "Hunter2", "--space", "MyFakeSpaceId", "--timeout", "5" }));

    Process.SetEndpoint("https://www.website.com");
    EXPECT_EQ(Process.GetEndpoint(), std::optional<std::string> { "https://www.website.com" });
    EXPECT_EQ(Process.GetInvocationArgs(),
        (std::vector<std::string> { "MultiplayerTestRunner", "--test", "CreateAvatar", "--email", "FakeEmail@MrMoustacheMan.com", "--password",
            "Hunter2", "--space", "MyFakeSpaceId", "--timeout", "5", "--endpoint", "https://www.website.com" }));
}

CSP_PUBLIC_TEST(CSPEngine, MultiplayerTestRunnerProcessTests, FutureTest)
{
    // Actually invoke the runner and make sure the futures are all set
    MultiplayerTestRunnerProcess Process(MultiplayerTestRunner::TestIdentifiers::TestIdentifier::CREATE_AVATAR);
    auto TestUser = CreateTestUser();
    Process.SetLoginEmail(TestUser.Email.c_str());
    Process.SetPassword(GeneratedTestAccountPassword);
    Process.SetTimeoutInSeconds(0); // So we don't sit at ready for assertions for any real time.
    Process.SetEndpoint(EndpointBaseURI());
    Process.StartProcess();

    // We need to spin up a process, login, create a space, join it, ... so we're a bit permissive with the timeouts to try and prevent flakiness.
    auto LoggedInFutureStatus = Process.LoggedInFuture().wait_for(std::chrono::seconds(20));
    EXPECT_EQ(LoggedInFutureStatus, std::future_status::ready);

    auto JoinedSpaceFutureStatus = Process.JoinedSpaceFuture().wait_for(std::chrono::seconds(20));
    EXPECT_EQ(JoinedSpaceFutureStatus, std::future_status::ready);

    auto ReadForAssertionsFutureStatus = Process.ReadyForAssertionsFuture().wait_for(std::chrono::seconds(20));
    EXPECT_EQ(ReadForAssertionsFutureStatus, std::future_status::ready);

    auto ExitSpaceFutureStatus = Process.ExitSpaceFuture().wait_for(std::chrono::seconds(20));
    EXPECT_EQ(ExitSpaceFutureStatus, std::future_status::ready);

    auto LoggedOutFutureStatus = Process.LoggedOutFuture().wait_for(std::chrono::seconds(20));
    EXPECT_EQ(LoggedOutFutureStatus, std::future_status::ready);
}
