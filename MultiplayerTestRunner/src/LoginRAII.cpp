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

#include "LoginRAII.h"

#include "../include/ErrorCodes.h"
#include "../include/ProcessDescriptors.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "Utils.h"

#include <future>

namespace
{
/* Login to CHS. Return the userID on success */
csp::common::String LogIn(csp::systems::UserSystem& UserSystem, const csp::common::String& Email, const csp::common::String& Password,
    bool AgeVerified, csp::systems::EResultCode ExpectedResultCode, csp::systems::ERequestFailureReason ExpectedResultFailureCode)
{
    std::promise<csp::systems::LoginStateResult> ResultPromise;
    std::future<csp::systems::LoginStateResult> ResultFuture = ResultPromise.get_future();

    UserSystem.Login("", Email, Password, AgeVerified,
        [&ResultPromise](csp::systems::LoginStateResult Result)
        {
            // Callbacks are called both in progress and at the end, guard against double promise sets
            if (Result.GetResultCode() == csp::systems::EResultCode::Success || Result.GetResultCode() == csp::systems::EResultCode::Failed)
            {
                ResultPromise.set_value(Result);
            }
        });

    csp::systems::LoginStateResult LoginResult = ResultFuture.get();

    if (LoginResult.GetResultCode() == csp::systems::EResultCode::Success)
    {
        MultiplayerTestRunner::ProcessDescriptors::PrintProcessDescriptor(MultiplayerTestRunner::ProcessDescriptors::LOGGED_IN_DESCRIPTOR);
        return LoginResult.GetLoginState().UserId;
    }
    else
    {
        const std::string msg = "Failed to login to service, got result code " + std::to_string(static_cast<uint8_t>(LoginResult.GetResultCode()))
            + "\n Response Body: " + LoginResult.GetResponseBody().c_str();
        throw Utils::ExceptionWithCode(MultiplayerTestRunner::ErrorCodes::FAILED_TO_LOGIN, msg);
    }
}
} // namespace

LoginRAII::LoginRAII(const std::string& AccountLoginEmail, const std::string& AccountPassword)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto& UserSystem = *SystemsManager.GetUserSystem();

    LogIn(UserSystem, AccountLoginEmail.c_str(), AccountPassword.c_str(), true, csp::systems::EResultCode::Success,
        csp::systems::ERequestFailureReason::None);
}

LoginRAII::~LoginRAII()
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto& UserSystem = *SystemsManager.GetUserSystem();

    std::promise<csp::systems::NullResult> ResultPromise;
    std::future<csp::systems::NullResult> ResultFuture = ResultPromise.get_future();

    UserSystem.Logout(
        [&ResultPromise](csp::systems::NullResult Result)
        {
            // Callbacks are called both in progress and at the end, guard against double promise sets
            if (Result.GetResultCode() == csp::systems::EResultCode::Success || Result.GetResultCode() == csp::systems::EResultCode::Failed)
            {
                ResultPromise.set_value(Result);
            }
        });

    csp::systems::NullResult LogoutResult = ResultFuture.get();
    MultiplayerTestRunner::ProcessDescriptors::PrintProcessDescriptor(MultiplayerTestRunner::ProcessDescriptors::LOGGED_OUT_DESCRIPTOR);
}