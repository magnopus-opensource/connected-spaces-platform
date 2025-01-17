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
#include <iostream>
#include <string>

namespace MultiplayerTestRunner::ProcessDescriptors
{
/*
 * Process descriptors are emitted in stdout. The string will be emitted followed by a newline.
 * These are intended to be used by invoking processes to manage when they conduct their test assertions.
 * These may be disabled by setting the `--descriptors` flag in the CLI.
 * It's important that you don't output these strings to the stream except in the correct places, and
 * definitely not more than once per process. Be careful with debug logging.
 */

/*
 * Emitted when the test has completed its setup and is ready for any controlling process to run tests
 * assertions. This is the main one you'll want to use.
 */
constexpr const char* READY_FOR_ASSERTIONS_DESCRIPTOR = "READY_FOR_ASSERTIONS_DESCRIPTOR";

/*
 *  Emitted when the test has logged in.
 */
constexpr const char* LOGGED_IN_DESCRIPTOR = "LOGGED_IN_DESCRIPTOR";

/*
 * Emitted when the test has logged out. Logout may not be emitted if process is terminated.
 */
constexpr const char* LOGGED_OUT_DESCRIPTOR = "LOGGED_OUT_DESCRIPTOR";

/*
 * Emitted when a new space is created. Not always emitted as existing space may be
 * specified for use via the `--space` CLI param.
 */
constexpr const char* CREATED_SPACE_DESCRIPTOR = "CREATED_SPACE_DESCRIPTOR";

/*
 * Emitted during cleanup when a space is deleted. Will not be emitted if the `--space` CLI param was used to
 * specify a custom space, as cleanup is not performed in that instance. Space cleanup may not be emitted if
 * process is terminated.
 */
constexpr const char* DESTROYED_SPACE_DESCRIPTOR = "DESTROYED_SPACE_DESCRIPTOR";

/*
 * Emitted when a logged in user joins a space
 */
constexpr const char* JOINED_SPACE_DESCRIPTOR = "JOINED_SPACE_DESCRIPTOR";

/*
 * Emitted when a user leaves a space
 */
constexpr const char* EXIT_SPACE_DESCRIPTOR = "EXIT_SPACE_DESCRIPTOR";

// This method exists to encode the decision about there always being a newline,
// as well as to make sure we flush, which we need to do when using stdout as
// an async channel of communication.
inline void PrintProcessDescriptor(const char* Descriptor) { std::cout << Descriptor << "\n" << std::flush; }
} // namespace MultiplayerTestRunner::ProcessDescriptors
