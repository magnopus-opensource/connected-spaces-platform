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

#include "CLIArgs.h"

#include "../include/ErrorCodes.h"
#include "../thirdparty/CLI11.hpp"

#include <iostream>

namespace
{
constexpr char* DEFAULT_TEST_ENDPOINT = "https://ogs-internal.magnopus-dev.cloud";
constexpr int DEFAULT_TIMEOUT_IN_SECONDS = 30;

/*
 * Take the raw input from the CLI, validate, and convert to structured data.
 * If Login or Password not provided, this will attempt to read `test_account_creds.txt`, and throw if it cannot.
 * TestIdentifier is the only mandatory arg, this will validate that the string provided is an actual identifier.
 * For optional arguments, if empty data is provided, will populate them with defaults.
 * SpaceId will not be defaulted, and will be left empty if empty, in the expectation a space is about to be created and the value set externally.
 */
CLIArgs::RunnerSettings ValidateInvocationArgs(const std::string& TestIdentifier, std::optional<std::string> LoginEmail,
    std::optional<std::string> LoginPassword, const std::optional<std::string>& Endpoint, const std::optional<std::string>& SpaceId,
    const std::optional<int>& TimeoutInSeconds)
{
    if (!LoginEmail.has_value() && !LoginPassword.has_value())
    {
        std::cout << "Credentials not provided, attempting to find credentials file.\n";
        Utils::TestAccountCredentials credentials = Utils::LoadTestAccountCredentials();
        LoginEmail = credentials.DefaultLoginEmail;
        LoginPassword = credentials.DefaultLoginPassword;
    }
    else if (!LoginEmail.has_value() || !LoginPassword.has_value())
    {
        // If only one of the email/password pair has been provided, error out entirely, it's probably a mistake
        throw Utils::ExceptionWithCode(MultiplayerTestRunner::ErrorCodes::CLI_PARSE_ERROR,
            "Both email and password must be provided together. Missing one likely indicates a mistake. Omit both if you "
            "wish to use the credentials file.");
    }

    CLIArgs::RunnerSettings settings;
    // Test identifiers need to be valid, and are not optional. An incorrect one is grounds to abort.
    try
    {
        settings.TestIdentifier = MultiplayerTestRunner::TestIdentifiers::StringToTestIdentifier(TestIdentifier);
    }
    catch (std::exception& exception)
    {
        // The reason this rethrow is here is an annoying quirk of `StringToTestIdentifier` being a public method.
        throw Utils::ExceptionWithCode(MultiplayerTestRunner::ErrorCodes::INVALID_TEST_SPECIFIER, exception.what());
    }
    settings.LoginEmailAndPassword = std::make_pair(LoginEmail.value(), LoginPassword.value());
    settings.Endpoint = Endpoint.value_or(DEFAULT_TEST_ENDPOINT);
    settings.TimeoutInSeconds = TimeoutInSeconds.value_or(DEFAULT_TIMEOUT_IN_SECONDS);
    settings.SpaceId = SpaceId; // This value is not defaulted, as an empty value means a space is about to be created, and this value will be set
                                // externally. (Messy!)

    return settings;
}
} // namespace

namespace CLIArgs
{
RunnerSettings ProcessCLI(int argc, char* argv[])
{
    // Build the CLI
    constexpr char* AppDescription = "The multiplayer test runner is a CSP test application designed to be spawned and managed cross-process. "
                                     "Multiple instances of the multiplayer test runner may be launched "
                                     "in order to simulate multiple users interacting with a space simultaneously.";
    CLI::App App(AppDescription);
    App.description(AppDescription);

    // These raw CLI variables populated in CLI11_PARSE, and then converted into validated and structured data below in `ValidateInvocationArgs`.
    std::string TestIdentifier;
    std::optional<int> TimeoutInSeconds;
    std::optional<std::string> Endpoint, LoginEmail, LoginPassword, SpaceId;

    App.add_option("-t,--test", TestIdentifier, "The test to run. See `include/TestIdentifiers.h` for available options.")->required();
    App.add_option("-e,--email", LoginEmail,
        "Login email for the test CHS account. If not set, the application will attempt to source this from a `test_account_creds.txt` "
        "next to the binary.");
    App.add_option("-p,--password", LoginPassword,
        "Password for the test CHS account. If not set, the application will attempt to source this from a `test_account_creds.txt` next "
        "to the binary.");
    App.add_option("-s,--space", SpaceId,
        "SpaceId to use in the invoked test. If none is provided, creates a random space. If a space id is provided, the space is assumed "
        "to already exist, and will not be cleaned up.");
    App.add_option("-o,--timeout", TimeoutInSeconds, "How long until the process self-terminates, in seconds. If not set, defaults to 30");
    App.add_option("-c,--endpoint", Endpoint, "Cloud services endpoint. If not set, defaults to `https://ogs-internal.magnopus-dev.cloud`");

    try
    {
        App.parse(argc, argv);
    }
    catch (const CLI::Success& e)
    {
        // App.exit() deals with help string printing. It's an odd pattern, an exception being a "Success".
        App.exit(e);
        throw Utils::ExceptionWithCode { MultiplayerTestRunner::ErrorCodes::SUCCESS, "" };
    }
    catch (const CLI::ParseError& e)
    {
        throw Utils::ExceptionWithCode { MultiplayerTestRunner::ErrorCodes::CLI_PARSE_ERROR, e.what() };
    }

    return ValidateInvocationArgs(TestIdentifier, LoginEmail, LoginPassword, Endpoint, SpaceId, TimeoutInSeconds);
}

} // namespace CLIArgs