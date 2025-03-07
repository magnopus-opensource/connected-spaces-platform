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
#include "uuid_v4.h"

#include "CSP/Systems/Users/UserSystem.h"
#include <CSP/CSPFoundation.h>
#include <CSP/Systems/SystemsManager.h>
#include <CSP/Systems/WebService.h> //For resultbase
#include <filesystem>
#include <fstream>
#include <future>

namespace Utils
{

std::string Utils::GetUniqueString()
{
    UUIDv4::UUIDGenerator<std::mt19937_64> uuidGenerator;
    const UUIDv4::UUID uuid = uuidGenerator.getUUID();

    return uuid.str();
}

/* Create a new user. Return the profile on success */
csp::systems::Profile Utils::CreateTestUser(std::string UniqueEmail, std::string Password, bool AgeVerified /* = true */,
    csp::systems::EResultCode ExpectedResultCode /* = Success */, csp::systems::ERequestFailureReason ExpectedResultFailureCode /* = None */)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto& UserSystem = *SystemsManager.GetUserSystem();

    std::promise<csp::systems::ProfileResult> ResultPromise;
    std::future<csp::systems::ProfileResult> ResultFuture = ResultPromise.get_future();

    UserSystem.CreateUser(nullptr, nullptr, UniqueEmail.c_str(), Password.c_str(), false, AgeVerified, nullptr, nullptr,
        [&ResultPromise](csp::systems::ProfileResult Result)
        {
            // Callbacks are called both in progress and at the end, guard against double promise sets
            if (Result.GetResultCode() == csp::systems::EResultCode::Success || Result.GetResultCode() == csp::systems::EResultCode::Failed)
            {
                ResultPromise.set_value(Result);
            }
        });

    csp::systems::ProfileResult CreatedProfile = ResultFuture.get();

    if (CreatedProfile.GetResultCode() == csp::systems::EResultCode::Success)
    {
        return CreatedProfile.GetProfile();
    }
    else
    {
        const std::string msg = "Failed to create user, got result code " + std::to_string(static_cast<uint8_t>(CreatedProfile.GetResultCode()))
            + "\n Response Body: " + CreatedProfile.GetResponseBody().c_str();
        throw Utils::ExceptionWithCode(MultiplayerTestRunner::ErrorCodes::FAILED_TO_CREATE_USER, msg);
    }
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