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

#include <utility>

MultiplayerTestRunnerProcess::MultiplayerTestRunnerProcess(MultiplayerTestRunner::TestIdentifiers::TestIdentifier testToRun)
    : m_testToRun(testToRun)
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
    : m_testToRun(other.m_testToRun)
    , m_loginEmail(other.m_loginEmail)
    , m_password(other.m_password)
    , m_spaceId(other.m_spaceId)
    , m_timeoutInSeconds(other.m_timeoutInSeconds)
    , m_endpoint(other.m_endpoint)
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
    : m_loggedInPromise(std::exchange(other.m_loggedInPromise, std::promise<void>()))
    , m_joinedSpacePromise(std::exchange(other.m_joinedSpacePromise, std::promise<void>()))
    , m_readyForAssertionsPromise(std::exchange(other.m_readyForAssertionsPromise, std::promise<void>()))
    , m_exitSpacePromise(std::exchange(other.m_exitSpacePromise, std::promise<void>()))
    , m_loggedOutPromise(std::exchange(other.m_loggedOutPromise, std::promise<void>()))
    , m_testToRun(other.m_testToRun)
    , m_loginEmail(other.m_loginEmail)
    , m_password(other.m_password)
    , m_spaceId(std::exchange(other.m_spaceId, std::optional<std::string>()))
    , m_timeoutInSeconds(std::exchange(other.m_timeoutInSeconds, std::optional<int>()))
    , m_endpoint(std::exchange(other.m_endpoint, std::optional<std::string>()))
    , m_processHandle(std::exchange(other.m_processHandle, nullptr))
{
}

MultiplayerTestRunnerProcess& MultiplayerTestRunnerProcess::operator=(MultiplayerTestRunnerProcess&& other)
{
    if (this != &other)
    {
        // Steal the juicy internals, whilst being nice and resetting the moved-from object to a sensible state.
        this->m_loggedInPromise = std::exchange(other.m_loggedInPromise, std::promise<void>());
        this->m_joinedSpacePromise = std::exchange(other.m_joinedSpacePromise, std::promise<void>());
        this->m_readyForAssertionsPromise = std::exchange(other.m_readyForAssertionsPromise, std::promise<void>());
        this->m_exitSpacePromise = std::exchange(other.m_exitSpacePromise, std::promise<void>());
        this->m_loggedOutPromise = std::exchange(other.m_loggedOutPromise, std::promise<void>());

        this->m_testToRun = other.m_testToRun;
        this->m_loginEmail = other.m_loginEmail;
        this->m_password = other.m_password;

        this->m_spaceId = std::exchange(other.m_spaceId, std::optional<std::string>());
        this->m_timeoutInSeconds = std::exchange(other.m_timeoutInSeconds, std::optional<int>());
        this->m_endpoint = std::exchange(other.m_endpoint, std::optional<std::string>());

        this->m_processHandle = std::exchange(other.m_processHandle, nullptr);
    }
    return *this;
}

MultiplayerTestRunnerProcess& MultiplayerTestRunnerProcess::SetLoginEmail(std::string setLoginEmail)
{
    this->m_loginEmail = std::move(setLoginEmail);
    return *this;
}

MultiplayerTestRunnerProcess& MultiplayerTestRunnerProcess::SetPassword(std::string setPassword)
{
    this->m_password = std::move(setPassword);
    return *this;
}

MultiplayerTestRunnerProcess& MultiplayerTestRunnerProcess::SetSpaceId(std::string setSpaceId)
{
    this->m_spaceId = std::move(setSpaceId);
    return *this;
}

MultiplayerTestRunnerProcess& MultiplayerTestRunnerProcess::SetTimeoutInSeconds(int setTimeoutInSeconds)
{
    this->m_timeoutInSeconds = setTimeoutInSeconds;
    return *this;
}

MultiplayerTestRunnerProcess& MultiplayerTestRunnerProcess::SetEndpoint(std::string setEndpoint)
{
    this->m_endpoint = std::move(setEndpoint);
    return *this;
}

namespace
{
/* Construct the CLI arguments to pass to MultiplayerTestRunner with spawning a TinyProcessLib::Process*/
std::vector<std::string> BuildProcessArgList(MultiplayerTestRunner::TestIdentifiers::TestIdentifier testToRun, std::string loginEmail,
    std::string password, std::optional<std::string> spaceId, std::optional<int> timeoutInSeconds, std::optional<std::string> endpoint)
{
    std::vector<std::string> cliArgs;

    /* The multiplayer test runner application is copied to the active directory
    as a post build command, so we just call it directly*/
    cliArgs.push_back(MULTIPLAYER_TEST_RUNNER_PATH);
    cliArgs.push_back("--test");
    cliArgs.push_back(MultiplayerTestRunner::TestIdentifiers::TestIdentifierToString(testToRun));
    cliArgs.push_back("--email");
    cliArgs.push_back(loginEmail);
    cliArgs.push_back("--password");
    cliArgs.push_back(password);

    if (spaceId.has_value())
    {
        cliArgs.push_back("--space");
        cliArgs.push_back(spaceId.value());
    }

    if (timeoutInSeconds.has_value())
    {
        cliArgs.push_back("--timeout");
        cliArgs.push_back(std::to_string(timeoutInSeconds.value()));
    }

    if (endpoint.has_value())
    {
        cliArgs.push_back("--endpoint");
        cliArgs.push_back(endpoint.value());
    }

    return cliArgs;
}

bool ContainsStr(const std::string& containingString, const std::string& containedString)
{
    auto findR = containingString.find(containedString);
    return findR != std::string::npos;
}

} // namespace

MultiplayerTestRunner::TestIdentifiers::TestIdentifier MultiplayerTestRunnerProcess::GetTestToRun() const { return m_testToRun; }

std::string MultiplayerTestRunnerProcess::GetLoginEmail() const { return m_loginEmail; }

std::string MultiplayerTestRunnerProcess::GetPassword() const { return m_password; }

std::optional<std::string> MultiplayerTestRunnerProcess::GetSpaceId() const { return m_spaceId; }

std::optional<int> MultiplayerTestRunnerProcess::GetTimeoutInSeconds() const { return m_timeoutInSeconds; }

std::optional<std::string> MultiplayerTestRunnerProcess::GetEndpoint() const { return m_endpoint; }

std::vector<std::string> MultiplayerTestRunnerProcess::GetInvocationArgs() const
{
    return BuildProcessArgList(m_testToRun, m_loginEmail, m_password, m_spaceId, m_timeoutInSeconds, m_endpoint);
}

void MultiplayerTestRunnerProcess::StartProcess()
{
    std::vector<std::string> invocationArgs = BuildProcessArgList(m_testToRun, m_loginEmail, m_password, m_spaceId, m_timeoutInSeconds, m_endpoint);

    // Be a bit loud in the output, I think this warrants special mention when test output is being displayed.
    std::cout << "Launching Multiplayer Test Runner Process with Test: " << MultiplayerTestRunner::TestIdentifiers::TestIdentifierToString(m_testToRun)
              << std::endl;

    /* Start the MultiplayerTestRunner process with provided CLI args
       The way this works is by registering callbacks for stdout and stderr streams.
       In the stdout stream, we check for the existence of the process descriptors, and
       toggle the appropriate promises. The callbacks are async, so beware!
       Don't assume that one cout in the MultiplayerTestRunner translates to one call
       of the callback, I've observed they can be batched, probably an OS thing. */
    m_processHandle = std::make_unique<TinyProcessLib::Process>(
        std::move(invocationArgs), "",
        [this](const char* bytes, size_t n)
        {
            // STDOUT
            std::string stdOutStr = std::string(bytes, n);
            std::cout << stdOutStr << std::endl;

            if (ContainsStr(stdOutStr, MultiplayerTestRunner::ProcessDescriptors::LOGGED_IN_DESCRIPTOR))
            {
                m_loggedInPromise.set_value();
            }

            if (ContainsStr(stdOutStr, MultiplayerTestRunner::ProcessDescriptors::JOINED_SPACE_DESCRIPTOR))
            {
                m_joinedSpacePromise.set_value();
            }

            if (ContainsStr(stdOutStr, MultiplayerTestRunner::ProcessDescriptors::READY_FOR_ASSERTIONS_DESCRIPTOR))
            {
                m_readyForAssertionsPromise.set_value();
            }

            if (ContainsStr(stdOutStr, MultiplayerTestRunner::ProcessDescriptors::EXIT_SPACE_DESCRIPTOR))
            {
                m_exitSpacePromise.set_value();
            }

            if (ContainsStr(stdOutStr, MultiplayerTestRunner::ProcessDescriptors::LOGGED_OUT_DESCRIPTOR))
            {
                m_loggedOutPromise.set_value();
            }
        },
        [](const char* bytes, size_t n)
        {
            // STDERR
            std::string stdErrStr = std::string(bytes, n);
            std::cout << stdErrStr << "\n";
            throw std::runtime_error(stdErrStr);
        });
}

void MultiplayerTestRunnerProcess::TerminateProcess()
{
    if (m_processHandle != nullptr)
    {
        std::cout << "Terminating Multiplayer Test Runner Process." << std::endl;
        m_processHandle->kill();
        m_processHandle = nullptr;
    }
}

std::future<void> MultiplayerTestRunnerProcess::LoggedInFuture() { return m_loggedInPromise.get_future(); }
std::future<void> MultiplayerTestRunnerProcess::JoinedSpaceFuture() { return m_joinedSpacePromise.get_future(); }
std::future<void> MultiplayerTestRunnerProcess::ReadyForAssertionsFuture() { return m_readyForAssertionsPromise.get_future(); }
std::future<void> MultiplayerTestRunnerProcess::ExitSpaceFuture() { return m_exitSpacePromise.get_future(); }
std::future<void> MultiplayerTestRunnerProcess::LoggedOutFuture() { return m_loggedOutPromise.get_future(); }
