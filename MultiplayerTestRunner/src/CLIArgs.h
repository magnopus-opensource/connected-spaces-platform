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