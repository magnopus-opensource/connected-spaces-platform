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

#pragma once

#include "../include/TestIdentifiers.h"
#include "Utils.h"

#include <optional>
#include <string>

namespace CLI
{
class App;
}

namespace CLIArgs
{

/*
 * The arguments passed to the runner as command line flags, validated and structured for use throughout the program.
 */
struct RunnerSettings
{
    std::pair<std::string, std::string> LoginEmailAndPassword;
    MultiplayerTestRunner::TestIdentifiers::TestIdentifier TestIdentifier;
    std::string Endpoint;
    int TimeoutInSeconds;
    // Validated when we try to enter the space. Should not be null after that point.
    // A bit of a hack, see main.cpp (at time of writing) for when this is actually set.
    std::optional<std::string> SpaceId;
};
ASSERT_MOVE_CAPABLE(RunnerSettings);

/*
 * Builds the CLI for the Multiplayer Test Runner.
 * Call this first in main to get your CLI.
 * Processes provided arguments, validating them and produces a `RunnerSettings` struct.
 */
RunnerSettings ProcessCLI(int argc, char* argv[]);

} // namespace CLIArgs