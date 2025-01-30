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

#include "../include/ErrorCodes.h"
#include "../include/ProcessDescriptors.h"
#include "CLIArgs.h"
#include "LoginRAII.h"
#include "RunnableTests/CreateAvatar.h"
#include "SpaceRAII.h"
#include "Utils.h"

#include <CSP/Common/String.h>
#include <chrono>
#include <string>
#include <thread>

#ifdef RUN_MULTIPLAYER_RUNNER_TESTS
#include <gtest/gtest.h>
#endif

/********************************
 * MULTIPLAYER TEST RUNNER
 * This is a CLI application designed to launch specified tests on independent processes (as CSP currently supports only one user per process), in
 * order to support testing multiple clients on multiple processes interacting in the same space.
 * When used in an automated testing context, it is
 * designed to communicate back with the calling process via stdout process descriptors (see `include/ProcessDescriptors.h`), such that the calling
 * process can reliably call assertions with the correct timings.
 * The CLI itself provides some flexibility for use in a more direct manner as an ad-hoc test application.
 *
 * Dependencies :
 *	- CSP. From the ConnectedSpacePlatform Solution. Linked.
 *  - UUID_v4. For generating unique space names. Header only.
 *  - CLI11. For defining the command line interface. Header only.
 ********************************/

/*
 * Calls the appropriate test based on the provided test identifier stored in settings
 * Will print the READY_FOR_ASSERTIONS_DESCRIPTOR when after the test code has been executed
 * By this point, the client should be logged in and inside a space, hence tests need not concern themselves with space creation and cleanup.
 * This function blocks until the timeout has elapsed.
 */
void RunTest(CLIArgs::RunnerSettings Settings, std::chrono::steady_clock::time_point ProgramStartTime)
{
    using namespace MultiplayerTestRunner::TestIdentifiers;
    switch (Settings.TestIdentifier)
    {
    case TestIdentifier::CREATE_AVATAR:
        CreateAvatar::RunTest();
        break;
    default:
        throw Utils::ExceptionWithCode(
            MultiplayerTestRunner::ErrorCodes::INVALID_TEST_SPECIFIER, "Could not find test specifier in RunTest, this is probably a bug.");
    }

    /*
     * Perform the timeout wait
     * The idea of the tests above is that they setup a state in a space and then exit, so calling processes can validate that state
     * This serves to make the test application wait until the timeout occurs, so the space is not cleaned up before the check can happen.
     * The process won't always exit this way, sometimes it will be terminated.
     * If terminating, the caller is responsible for any space cleanup, and outside of throwaway executions,
     * should have been using the `spaceID` option to provide a managed space.
     *
     * If you do need a test that performs continual actions (such as constant position updates), feel free to put a busy loop
     * in your test to do this, just be aware that if you don't have an exit condition, the test will continue forever unless terminated. (Which isn't
     * the worst thing)
     */
    MultiplayerTestRunner::ProcessDescriptors::PrintProcessDescriptor(MultiplayerTestRunner::ProcessDescriptors::READY_FOR_ASSERTIONS_DESCRIPTOR);
    const auto TargetTime = ProgramStartTime + std::chrono::seconds(Settings.TimeoutInSeconds);
    std::this_thread::sleep_until(TargetTime);
}

int main(int argc, char* argv[])
{
// GoogleTest.
#ifdef RUN_MULTIPLAYER_RUNNER_TESTS
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
#endif

    // We grab the program start time right at the beginning so the timeout can be calculated.
    const auto ProgramStartTime = std::chrono::steady_clock::now();

    try
    {
        // Create the CLI, and get the validated data from the inputs.
        CLIArgs::RunnerSettings Settings = CLIArgs::ProcessCLI(argc, argv);

        // Get setup with CSP and CHS.
        Utils::InitialiseCSPWithUserAgentInfo(Settings.Endpoint.c_str());

        // Log in
        LoginRAII loggedIn { Settings.LoginEmailAndPassword.first, Settings.LoginEmailAndPassword.second };

        // Enter space (creating one if it dosen't exist)
        SpaceRAII space { Settings.SpaceId };
        Settings.SpaceId = space.GetSpaceId(); // We need to update the settings as a new space may have been created

        // Run the specified test according to the TestIdentifier. Wont return earlier than timeout.
        RunTest(Settings, ProgramStartTime);

        return 0;
    }
    /*
     * The test runner uses exceptions to propogate errors up, as it is a process-based executable
     * we want to (or really, must) use return codes to communicate errors. Convert them here.
     */
    catch (const Utils::ExceptionWithCode& Exception)
    {
        std::cerr << Exception.what();
        return Exception.ErrorCode;
    }
    // Sometimes CSP itself may throw an exception, cover ourself.
    catch (const std::exception& Exception)
    {
        std::cerr << Exception.what();
        return MultiplayerTestRunner::ErrorCodes::OTHER_EXCEPTION;
    }
}