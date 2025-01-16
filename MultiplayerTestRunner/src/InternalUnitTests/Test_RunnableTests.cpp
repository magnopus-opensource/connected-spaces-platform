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

#include "../../include/TestIdentifiers.h"
#include "../LoginRAII.h"
#include "../RunnableTests/CreateAvatar.h"
#include "../SpaceRAII.h"
#include "../Utils.h"

#include <CSP/Common/String.h>
#include <gtest/gtest.h>

/*
 * The only purpose of this test suite is to execute the runnable tests themselves
 * We do no validation that they're doing what they should do, to do so would be
 * testing tests, which is too much testing for me.
 * Nonetheless, we should ensure they don't crash.
 */

namespace
{
/* Some tests only run if there's a credentials file */
std::optional<Utils::TestAccountCredentials> CredentialsFromFile()
{
    try
    {
        return Utils::LoadTestAccountCredentials();
    }
    catch (...)
    {
        return {};
    }
}
} // namespace

/* Initialze CSP before the suite begins with a fixture */
class RunnableTests : public ::testing::Test
{
protected:
    static void SetUpTestSuite() { Utils::InitialiseCSPWithUserAgentInfo(Utils::DEFAULT_TEST_ENDPOINT); }
};

TEST_F(RunnableTests, CreateAvatar)
{
    std::optional<Utils::TestAccountCredentials> Credentials = CredentialsFromFile();
    if (!Credentials.has_value())
    {
        GTEST_SKIP() << "No credentials file found, Skipping Test.";
    }

    // Login
    LoginRAII login { Credentials.value().DefaultLoginEmail, Credentials.value().DefaultLoginPassword };
    // Make a throwaway space
    SpaceRAII Space({});

    CreateAvatar::RunTest();
}