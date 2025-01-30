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

/*
 * A listing of all the known return codes the Multiplayer Test Runner may return as an error.
 * Successful execution returns 0, as usual.
 */
namespace MultiplayerTestRunner::ErrorCodes
{
constexpr const int SUCCESS = 0;
constexpr const int COULD_NOT_FIND_CREDENTIALS_FILE = 100;
constexpr const int MALFORMED_CREDENTIALS_FILE = 101;
constexpr const int FAILED_TO_LOGIN = 102;
constexpr const int INVALID_TEST_SPECIFIER = 103;
constexpr const int OTHER_EXCEPTION = 104; // For any exception that isn't thrown explicitly by the Multiplayer Test Runner
constexpr const int FAILED_TO_CREATE_SPACE = 105;
constexpr const int FAILED_TO_ENTER_SPACE = 106;
constexpr const int CLI_PARSE_ERROR = 107;
} // namespace MultiplayerTestRunner::ErrorCodes