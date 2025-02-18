/*
 * Copyright 2023 Magnopus LLC

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

#include "CSP/Systems/Spaces/Space.h"
#include <gtest/gtest.h>

namespace
{
constexpr char* EndpointEnvironmentName = "MAGNOPUS_SERVICES_ENDPOINT";

const char* GetEnvironmentVariableOrDefault(const char* EnvironmentKey, const char* DefaultValue)
{
    const auto EnvironmentVariable = std::getenv(EnvironmentKey);
    return (EnvironmentVariable) ? EnvironmentVariable : DefaultValue;
}
} // namespace

inline const char* EndpointBaseURI() { return GetEnvironmentVariableOrDefault(EndpointEnvironmentName, "https://ogs-internal.magnopus-dev.cloud"); }

class PublicTestBase : public ::testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override;
};

// For parameterized (data driven) tests
template <typename T> class PublicTestBaseWithParam : public ::testing::TestWithParam<T>
{
protected:
    void SetUp() override;
    void TearDown() override;
};

// If you want to use a new parameterized test structure, you need to explicitly instantiate here (and in the .cpp)
extern template class PublicTestBaseWithParam<std::tuple<csp::systems::SpaceAttributes, csp::systems::EResultCode, std::string>>;