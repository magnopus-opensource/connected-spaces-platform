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

#include "ProcessDescriptors.h"
#include "process.hpp"

MultiplayerTestRunnerProcess::MultiplayerTestRunnerProcess(MultiplayerTestRunner::TestIdentifiers::TestIdentifier _TestToRun)
    : TestToRun(_TestToRun)
{
}

MultiplayerTestRunnerProcess ::~MultiplayerTestRunnerProcess()
{
    /* (EM) I'm still a little worried about windows nonsenses with this testing approach.
     * You cannot simply force kill a process on Windows, there's umpteen things that can lock it.
     * TinyProcessLib makes a good attempt to handle edge cases, but I'm still nervous.
     * With our (currently, 2025) sketchy CI configuration, we should keep an eye on this.
     * I'd be much less concerned if our CI runners were built from the ground up each time, then
     * zombie processes couldn't accumulate even if windows gets huffy sometimes. */
    TerminateProcess();
}

MultiplayerTestRunnerProcess::MultiplayerTestRunnerProcess(const MultiplayerTestRunnerProcess& other)
    : TestToRun(other.TestToRun)
    , LoginEmail(other.LoginEmail)
    , Password(other.Password)
    , SpaceId(other.SpaceId)
    , TimeoutInSeconds(other.TimeoutInSeconds)
    , Endpoint(other.Endpoint)
{
}

MultiplayerTestRunnerProcess& MultiplayerTestRunnerProcess::operator=(const MultiplayerTestRunnerProcess& other)
{
    if (this != &other)
    {
        MultiplayerTestRunnerProcess swapping(other);
        std::swap(*this, swapping);
    }
    return *this;
}

MultiplayerTestRunnerProcess::MultiplayerTestRunnerProcess(MultiplayerTestRunnerProcess&& other)
    : LoggedInPromise(std::exchange(other.LoggedInPromise, std::promise<void>()))
    , JoinedSpacePromise(std::exchange(other.JoinedSpacePromise, std::promise<void>()))
    , ReadyForAssertionsPromise(std::exchange(other.ReadyForAssertionsPromise, std::promise<void>()))
    , ExitSpacePromise(std::exchange(other.ExitSpacePromise, std::promise<void>()))
    , LoggedOutPromise(std::exchange(other.LoggedOutPromise, std::promise<void>()))
    , TestToRun(other.TestToRun)
    , LoginEmail(std::exchange(other.LoginEmail, std::optional<std::string>()))
    , Password(std::exchange(other.Password, std::optional<std::string>()))
    , SpaceId(std::exchange(other.SpaceId, std::optional<std::string>()))
    , TimeoutInSeconds(std::exchange(other.TimeoutInSeconds, std::optional<int>()))
    , Endpoint(std::exchange(other.Endpoint, std::optional<std::string>()))
    , ProcessHandle(std::exchange(other.ProcessHandle, nullptr))
{
}

MultiplayerTestRunnerProcess& MultiplayerTestRunnerProcess::operator=(MultiplayerTestRunnerProcess&& other)
{
    if (this != &other)
    {
        // Steal the juicy internals, whilst being nice and resetting the moved-from object to a sensible state.
        this->LoggedInPromise = std::exchange(other.LoggedInPromise, std::promise<void>());
        this->JoinedSpacePromise = std::exchange(other.JoinedSpacePromise, std::promise<void>());
        this->ReadyForAssertionsPromise = std::exchange(other.ReadyForAssertionsPromise, std::promise<void>());
        this->ExitSpacePromise = std::exchange(other.ExitSpacePromise, std::promise<void>());
        this->LoggedOutPromise = std::exchange(other.LoggedOutPromise, std::promise<void>());

        this->TestToRun = other.TestToRun;

        this->LoginEmail = std::exchange(other.LoginEmail, std::optional<std::string>());
        this->Password = std::exchange(other.Password, std::optional<std::string>());
        this->SpaceId = std::exchange(other.SpaceId, std::optional<std::string>());
        this->TimeoutInSeconds = std::exchange(other.TimeoutInSeconds, std::optional<int>());
        this->Endpoint = std::exchange(other.Endpoint, std::optional<std::string>());

        this->ProcessHandle = std::exchange(other.ProcessHandle, nullptr);
    }
    return *this;
}

MultiplayerTestRunnerProcess& MultiplayerTestRunnerProcess::SetLoginEmail(std::string LoginEmail)
{
    this->LoginEmail = std::move(LoginEmail);
    return *this;
}

MultiplayerTestRunnerProcess& MultiplayerTestRunnerProcess::SetPassword(std::string Password)
{
    this->Password = std::move(Password);
    return *this;
}

MultiplayerTestRunnerProcess& MultiplayerTestRunnerProcess::SetSpaceId(std::string SpaceId)
{
    this->SpaceId = std::move(SpaceId);
    return *this;
}

MultiplayerTestRunnerProcess& MultiplayerTestRunnerProcess::SetTimeoutInSeconds(int TimeoutInSeconds)
{
    this->TimeoutInSeconds = TimeoutInSeconds;
    return *this;
}

MultiplayerTestRunnerProcess& MultiplayerTestRunnerProcess::SetEndpoint(std::string Endpoint)
{
    this->Endpoint = std::move(Endpoint);
    return *this;
}

namespace
{
/* Construct the CLI arguments to pass to MultiplayerTestRunner with spawning a TinyProcessLib::Process*/
std::vector<std::string> BuildProcessArgList(MultiplayerTestRunner::TestIdentifiers::TestIdentifier TestToRun, std::optional<std::string> LoginEmail,
    std::optional<std::string> Password, std::optional<std::string> SpaceId, std::optional<int> TimeoutInSeconds, std::optional<std::string> Endpoint)
{
    std::vector<std::string> CLIArgs;

    /* The multiplayer test runner application is copied to the active directory
    as a post build command, so we just call it directly*/
    CLIArgs.push_back("MultiplayerTestRunner");
    CLIArgs.push_back("--test");
    CLIArgs.push_back(MultiplayerTestRunner::TestIdentifiers::TestIdentifierToString(TestToRun));

    if (LoginEmail.has_value())
    {
        CLIArgs.push_back("--email");
        CLIArgs.push_back(LoginEmail.value());
    }

    if (Password.has_value())
    {
        CLIArgs.push_back("--password");
        CLIArgs.push_back(Password.value());
    }

    if (SpaceId.has_value())
    {
        CLIArgs.push_back("--space");
        CLIArgs.push_back(SpaceId.value());
    }

    if (TimeoutInSeconds.has_value())
    {
        CLIArgs.push_back("--timeout");
        CLIArgs.push_back(std::to_string(TimeoutInSeconds.value()));
    }

    if (Endpoint.has_value())
    {
        CLIArgs.push_back("--endpoint");
        CLIArgs.push_back(Endpoint.value());
    }

    return CLIArgs;
}

bool ContainsStr(const std::string& ContainingString, const std::string& ContainedString)
{
    auto findR = ContainingString.find(ContainedString);
    return findR != std::string::npos;
}

} // namespace

MultiplayerTestRunner::TestIdentifiers::TestIdentifier MultiplayerTestRunnerProcess::GetTestToRun() const { return TestToRun; }

std::optional<std::string> MultiplayerTestRunnerProcess::GetLoginEmail() const { return LoginEmail; }

std::optional<std::string> MultiplayerTestRunnerProcess::GetPassword() const { return Password; }

std::optional<std::string> MultiplayerTestRunnerProcess::GetSpaceId() const { return SpaceId; }

std::optional<int> MultiplayerTestRunnerProcess::GetTimeoutInSeconds() const { return TimeoutInSeconds; }

std::optional<std::string> MultiplayerTestRunnerProcess::GetEndpoint() const { return Endpoint; }

std::vector<std::string> MultiplayerTestRunnerProcess::GetInvocationArgs() const
{
    return BuildProcessArgList(TestToRun, LoginEmail, Password, SpaceId, TimeoutInSeconds, Endpoint);
}

void MultiplayerTestRunnerProcess::StartProcess()
{
    std::vector<std::string> InvocationArgs = BuildProcessArgList(TestToRun, LoginEmail, Password, SpaceId, TimeoutInSeconds, Endpoint);

    // Be a bit loud in the output, I think this warrants special mention when test output is being displayed.
    std::cout << "Launching Multiplayer Test Runner Process with Test: " << MultiplayerTestRunner::TestIdentifiers::TestIdentifierToString(TestToRun)
              << std::endl;

    /* Start the MultiplayerTestRunner process with provided CLI args
       The way this works is by registering callbacks for stdout and stderr streams.
       In the stdout stream, we check for the existence of the process descriptors, and
       toggle the appropriate promises. The callbacks are async, so beware!
       Don't assume that one cout in the MultiplayerTestRunner translates to one call
       of the callback, I've observed they can be batched, probably an OS thing. */
    ProcessHandle = std::make_unique<TinyProcessLib::Process>(
        std::move(InvocationArgs), "",
        [this](const char* bytes, size_t n)
        {
            // STDOUT
            std::string StdOutStr = std::string(bytes, n);
            // You might want to put std::cout << StdOutStr << std::endl; here when debugging.

            if (ContainsStr(StdOutStr, MultiplayerTestRunner::ProcessDescriptors::LOGGED_IN_DESCRIPTOR))
            {
                LoggedInPromise.set_value();
            }

            if (ContainsStr(StdOutStr, MultiplayerTestRunner::ProcessDescriptors::JOINED_SPACE_DESCRIPTOR))
            {
                JoinedSpacePromise.set_value();
            }

            if (ContainsStr(StdOutStr, MultiplayerTestRunner::ProcessDescriptors::READY_FOR_ASSERTIONS_DESCRIPTOR))
            {
                ReadyForAssertionsPromise.set_value();
            }

            if (ContainsStr(StdOutStr, MultiplayerTestRunner::ProcessDescriptors::EXIT_SPACE_DESCRIPTOR))
            {
                ExitSpacePromise.set_value();
            }

            if (ContainsStr(StdOutStr, MultiplayerTestRunner::ProcessDescriptors::LOGGED_OUT_DESCRIPTOR))
            {
                LoggedOutPromise.set_value();
            }
        },
        [](const char* bytes, size_t n)
        {
            // STDERR
            std::string StdErrStr = std::string(bytes, n);
            std::cout << StdErrStr << "\n";
            throw std::runtime_error(StdErrStr);
        });
}

void MultiplayerTestRunnerProcess::TerminateProcess()
{
    if (ProcessHandle != nullptr)
    {
        std::cout << "Terminating Multiplayer Test Runner Process." << std::endl;
        ProcessHandle->kill();
        ProcessHandle = nullptr;
    }
}

std::future<void> MultiplayerTestRunnerProcess::LoggedInFuture() { return LoggedInPromise.get_future(); }
std::future<void> MultiplayerTestRunnerProcess::JoinedSpaceFuture() { return JoinedSpacePromise.get_future(); }
std::future<void> MultiplayerTestRunnerProcess::ReadyForAssertionsFuture() { return ReadyForAssertionsPromise.get_future(); }
std::future<void> MultiplayerTestRunnerProcess::ExitSpaceFuture() { return ExitSpacePromise.get_future(); }
std::future<void> MultiplayerTestRunnerProcess::LoggedOutFuture() { return LoggedOutPromise.get_future(); }
