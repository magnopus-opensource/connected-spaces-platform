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

#include "Utils.h"

#include "../include/ErrorCodes.h"

#include <CSP/CSPFoundation.h>
#include <CSP/Common/String.h>
#include <CSP/Systems/WebService.h> //For resultbase
#include <filesystem>
#include <fstream>

namespace Utils
{

/*
 * Load the test account credentials from a location adjacent to the binary
 * Currently only provides 2 accounts, will need to be extended for tests that require more than 2 agents.
 */
TestAccountCredentials LoadTestAccountCredentials()
{
    if (!std::filesystem::exists("test_account_creds.txt"))
    {
        constexpr const char* msg
            = "test_account_creds.txt not found! This file must exist and must contain the following information:\n<DefaultLoginEmail> "
              "<DefaultLoginPassword>\n<AlternativeLoginEmail> <AlternativeLoginPassword>\n<SuperUserLoginEmail> <SuperUserLoginPassword>";
        throw ExceptionWithCode(MultiplayerTestRunner::ErrorCodes::COULD_NOT_FIND_CREDENTIALS_FILE, msg);
    }

    std::ifstream CredsFile;
    CredsFile.open("test_account_creds.txt");

    std::string DefaultLoginEmail, DefaultLoginPassword, AlternativeLoginEmail, AlternativeLoginPassword, SuperUserLoginEmail, SuperUserLoginPassword;

    CredsFile >> DefaultLoginEmail >> DefaultLoginPassword;
    CredsFile >> AlternativeLoginEmail >> AlternativeLoginPassword;
    CredsFile >> SuperUserLoginEmail >> SuperUserLoginPassword;

    if (DefaultLoginEmail.empty() || DefaultLoginPassword.empty() || AlternativeLoginEmail.empty() || AlternativeLoginPassword.empty()
        || SuperUserLoginEmail.empty() || SuperUserLoginPassword.empty())
    {
        constexpr const char* msg
            = "test_account_creds.txt must be in the following format:\n<DefaultLoginEmail> <DefaultLoginPassword>\n<AlternativeLoginEmail> "
              "<AlternativeLoginPassword>\n<SuperUserLoginEmail> <SuperUserLoginPassword>";
        throw ExceptionWithCode(MultiplayerTestRunner::ErrorCodes::MALFORMED_CREDENTIALS_FILE, msg);
    }

    return TestAccountCredentials { DefaultLoginEmail.c_str(), DefaultLoginPassword.c_str(), AlternativeLoginEmail.c_str(),
        AlternativeLoginPassword.c_str(), SuperUserLoginEmail.c_str(), SuperUserLoginPassword.c_str() };
}

void InitialiseCSPWithUserAgentInfo(const csp::common::String& EndpointRootURI)
{
    constexpr char* TESTS_CLIENT_SKU = "MultiplayerTestRunner";

    csp::CSPFoundation::Initialise(EndpointRootURI, "OKO_TESTS");

    csp::ClientUserAgent ClientHeaderInfo;
    ClientHeaderInfo.CSPVersion = csp::CSPFoundation::GetVersion();
    ClientHeaderInfo.ClientOS = "MultiplayerTestOS";
    ClientHeaderInfo.ClientSKU = TESTS_CLIENT_SKU;
    ClientHeaderInfo.ClientVersion = csp::CSPFoundation::GetVersion();
    ClientHeaderInfo.ClientEnvironment = "ODev";
    ClientHeaderInfo.CHSEnvironment = "oDev";

    csp::CSPFoundation::SetClientUserAgentInfo(ClientHeaderInfo);
}

} // namespace Utils