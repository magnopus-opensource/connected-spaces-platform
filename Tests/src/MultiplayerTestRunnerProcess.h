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

#pragma once

#include "TestIdentifiers.h"

#include <future>
#include <memory>
#include <optional>

namespace TinyProcessLib
{
class Process;
}

/*
 * Invokes the MultiplayerTestRunner, built as a prerequisite to the tests project
 * as a process (CLI invocation) via tiny-process-library.
 * Parses the stdout of MultiplayerTestClient, and provides a future async
 * interface to allow users to know when the off-process test has reached
 * certain points, as specified by the process descriptors.
 *
 * WARNING: Using this will necessarily add a lot of realtime overhead to your
 * test functions, it takes a good 5-10 seconds for processes to spin up and
 * become ready. Use this sparingly, and account for runtime fluctuations to
 * prevent undue flakiness.
 */
class MultiplayerTestRunnerProcess
{
public:
    MultiplayerTestRunnerProcess(MultiplayerTestRunner::TestIdentifiers::TestIdentifier _TestToRun);
    ~MultiplayerTestRunnerProcess();

    /* Copying this object copies all the configuration data, but does nothing
       about the process. You still need to call `StartProcess` to start
       a new process, even if you copied from an already running process. */
    MultiplayerTestRunnerProcess(const MultiplayerTestRunnerProcess& other);
    MultiplayerTestRunnerProcess& operator=(const MultiplayerTestRunnerProcess& other);

    /* Moving steals all the internals, so any already running process stays running. */
    MultiplayerTestRunnerProcess(MultiplayerTestRunnerProcess&& other);
    MultiplayerTestRunnerProcess& operator=(MultiplayerTestRunnerProcess&& other);

    /* Chained methods (fluent interface pattern) to set the parameters.
       All of these are optional.
       Reminder that if either login or password is not found, then the
       MultiplayerTestRunner will attempt to look for a credentials file.
       If a space is not specified, the MultiplayerTestRunner makes a
       temporary one. You'll almost certainly want to specify a spaceID
       when doing multi-client tests, or you'll get lots of clients in
       isolated spaces.
       */
    MultiplayerTestRunnerProcess& SetLoginEmail(std::string Email);
    MultiplayerTestRunnerProcess& SetPassword(std::string Password);
    MultiplayerTestRunnerProcess& SetSpaceId(std::string SpaceId);
    MultiplayerTestRunnerProcess& SetTimeoutInSeconds(int TimeoutInSeconds);
    MultiplayerTestRunnerProcess& SetEndpoint(std::string Endpoint);

    /* Getters. Mostly for testing, but handy.
       Return null optionals if values have not been set. */
    MultiplayerTestRunner::TestIdentifiers::TestIdentifier GetTestToRun() const;
    std::optional<std::string> GetLoginEmail() const;
    std::optional<std::string> GetPassword() const;
    std::optional<std::string> GetSpaceId() const;
    std::optional<int> GetTimeoutInSeconds() const;
    std::optional<std::string> GetEndpoint() const;

    /* Return the vector of strings that will be used to invoke the multiplayer test runner.
       Depending on what values you've set, will look something like :
       {"MultiplayerTestRunner", "--test", "CreateAvatar", "--timeout", "10"} */
    std::vector<std::string> GetInvocationArgs() const;

    /* Invoke the process with the provided parameters */
    void StartProcess();

    /* Hard terminate the process. This happens in destruction anyway,
       but this method provided to support alternate styles. */
    void TerminateProcess();

    /* You can acquire the future that the process desriptor
       promises via these methods.
       We set the promise when we parse the appropriate
       process descriptor when it is recieved through stdout. */
    std::future<void> LoggedInFuture();
    std::future<void> JoinedSpaceFuture();
    std::future<void> ReadyForAssertionsFuture();
    std::future<void> ExitSpaceFuture();
    std::future<void> LoggedOutFuture();

private:
    // These promises set via parsing stdout for process descriptors.
    std::promise<void> LoggedInPromise;
    std::promise<void> JoinedSpacePromise;
    std::promise<void> ReadyForAssertionsPromise;
    std::promise<void> ExitSpacePromise;
    std::promise<void> LoggedOutPromise;

    // The test we are telling the multiplayer test runner to invoke. Set on construction. Non-optional.
    MultiplayerTestRunner::TestIdentifiers::TestIdentifier TestToRun;

    // Optional parameters, MultiplayerTestRunner has default behaviour if not set
    std::optional<std::string> LoginEmail;
    std::optional<std::string> Password;
    std::optional<std::string> SpaceId;
    std::optional<int> TimeoutInSeconds;
    std::optional<std::string> Endpoint;

    // Created in StartProcess
    std::unique_ptr<TinyProcessLib::Process> ProcessHandle = nullptr;
};
