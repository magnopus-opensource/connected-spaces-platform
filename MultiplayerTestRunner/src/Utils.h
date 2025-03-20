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

#include <CSP/Systems/Users/Profile.h>

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

std::string GetUniqueString();

const char GeneratedTestAccountPassword[] = "3R{d2}3C<x[J7=jU";

csp::systems::Profile CreateTestUser(bool AgeVerified = true, csp::systems::EResultCode ExpectedResultCode = csp::systems::EResultCode::Success,
    csp::systems::ERequestFailureReason ExpectedResultFailureCode = csp::systems::ERequestFailureReason::None);

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
 * For the moment, this uses the known test header info so CHS doesn't reject us
 * In the future, it may become prudent to allow some or all of this information
 * to be passed as arguments.
 */
void InitialiseCSPWithUserAgentInfo(const csp::common::String& EndpointRootURI);

constexpr char* DEFAULT_TEST_ENDPOINT = "https://ogs-internal.magnopus-dev.cloud";
constexpr int DEFAULT_TIMEOUT_IN_SECONDS = 30;
constexpr bool DEFAULT_EMIT_PROCESS_DESCRIPTORS = true;

} // namespace Utils