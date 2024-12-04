#pragma once
#include <iostream>
#include <string>

namespace MultiplayerTestRunner::ProcessDescriptors
{
/*
 * Process descriptors are emitted in stdout. The string will be emitted followed by a newline.
 * These are intended to be used by invoking processes to manage when they conduct their test assertions.
 * These may be disabled by setting the `--descriptors` flag in the CLI.
 */

/*
 * Emitted when the test has completed its setup and is ready for any controlling process to run tests
 * assertions. This is the main one you'll want to use.
 */
constexpr const char* READY_FOR_ASSERTIONS_DESCRIPTOR = "READY_FOR_ASSERTIONS";

/*
 *  Emitted when the test has logged in.
 */
constexpr const char* LOGGED_IN_DESCRIPTOR = "LOGGED_IN";

/*
 * Emitted when the test has logged out. Logout may not be emitted if process is terminated.
 */
constexpr const char* LOGGED_OUT_DESCRIPTOR = "LOGGED_OUT";

/*
 * Emitted when a new space is created. Not always emitted as existing space may be
 * specified for use via the `--space` CLI param.
 */
constexpr const char* CREATED_SPACE_DESCRIPTOR = "CREATED_SPACE";

/*
 * Emitted during cleanup when a space is deleted. Will not be emitted if the `--space` CLI param was used to
 * specify a custom space, as cleanup is not performed in that instance. Room cleanup may not be emitted if
 * process is terminated.
 */
constexpr const char* DESTROYED_SPACE_DESCRIPTOR = "DESTROYED_SPACE";

/*
 * Emitted when a logged in user joins a space
 */
constexpr const char* JOINED_SPACE_DESCRIPTOR = "JOINED_SPACE";

/*
 * Emitted when a user leaves a space
 */
constexpr const char* EXIT_SPACE_DESCRIPTOR = "LEFT_SPACE";

// This method exists just to encode the decision about there always being a newline.
inline void PrintProcessDescriptor(const char* Descriptor)
{
	std::cout << Descriptor << "\n";
}
} // namespace MultiplayerTestRunner::ProcessDescriptors
