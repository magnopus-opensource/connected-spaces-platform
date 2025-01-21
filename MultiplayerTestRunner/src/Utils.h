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

#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace csp
{
namespace systems
{
    class ResultBase;
}
namespace common
{
    class String;
}
} // namespace csp

namespace Utils
{
// Assert a type is move capable
#define ASSERT_MOVE_CAPABLE(Type)                                                                                                                    \
    static_assert(std::is_move_constructible_v<Type>, #Type " must be move constructible");                                                          \
    static_assert(std::is_move_assignable_v<Type>, #Type " must be move assignable")

/*
 * Internal exception type that adds an error code.
 * The purpose of this type is to facilitate easy conversion of errors -> return codes before program exit.
 */
struct ExceptionWithCode : public std::runtime_error
{
    ExceptionWithCode(int errorCode, const std::string& message)
        : std::runtime_error(message)
        , ErrorCode(errorCode)
    {
    }
    const int ErrorCode;
};

/*
 * Credentials struct for storing the credentials read from `test_account_creds.txt`, in the case that Login/Password are not provided via the CLI.
 */
struct TestAccountCredentials
{
    std::string DefaultLoginEmail;
    std::string DefaultLoginPassword;
    std::string AlternativeLoginEmail;
    std::string AlternativeLoginPassword;
    std::string SuperUserLoginEmail;
    std::string SuperUserLoginPassword;
};
ASSERT_MOVE_CAPABLE(TestAccountCredentials);

/*
 * Load a `TestAccountCredentials` from `test_account_creds.txt`, in the case that Login/Password are not provided via the CLI.
 */
TestAccountCredentials LoadTestAccountCredentials();

/*
 * For the moment, this uses the known test header info so CHS dosen't reject us
 * In the future, it may become prudent to allow some or all of this information
 * to be passed as arguments.
 */
void InitialiseCSPWithUserAgentInfo(const csp::common::String& EndpointRootURI);

constexpr char* DEFAULT_TEST_ENDPOINT = "https://ogs-internal.magnopus-dev.cloud";
constexpr int DEFAULT_TIMEOUT_IN_SECONDS = 30;
constexpr bool DEFAULT_EMIT_PROCESS_DESCRIPTORS = true;

} // namespace Utils