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

#include "SpaceRAII.h"

#include "../include/ErrorCodes.h"
#include "../include/ProcessDescriptors.h"
#include "CSP/Systems/SystemsManager.h"
#include "Utils.h"
#include "uuid_v4.h"

#include <future>

namespace
{

} // namespace

SpaceRAII::SpaceRAII(std::optional<std::string> ExistingSpaceId)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto& SpaceSystem = *SystemsManager.GetSpaceSystem();

    // If the user hasn't provided a spaceID, we'll make our own.
    // Beware, on termination this may not get cleaned up.
    if (ExistingSpaceId.has_value())
    {
        SpaceId = ExistingSpaceId.value();
    }
    else
    {
        SpaceId = SpaceRAII::CreateDefaultTestSpace(SpaceSystem).Id;
        CreatedThisSpace = true;
    }

    // Enter space
    std::promise<csp::systems::NullResult> ResultPromise;
    std::future<csp::systems::NullResult> ResultFuture = ResultPromise.get_future();

    SpaceSystem.EnterSpace(SpaceId.c_str(),
        [&ResultPromise](csp::systems::NullResult Result)
        {
            // Callbacks are called both in progress and at the end, guard against double promise sets
            if (Result.GetResultCode() == csp::systems::EResultCode::Success || Result.GetResultCode() == csp::systems::EResultCode::Failed)
            {
                ResultPromise.set_value(Result);
            }
        });

    csp::systems::NullResult EnterSpaceResult = ResultFuture.get();

    if (EnterSpaceResult.GetResultCode() != csp::systems::EResultCode::Success)
    {
        std::string FailureReason
            = "HTTP Code: " + std::to_string(EnterSpaceResult.GetHttpResultCode()) + " Body: " + EnterSpaceResult.GetResponseBody().c_str();
        throw Utils::ExceptionWithCode { MultiplayerTestRunner::ErrorCodes::FAILED_TO_ENTER_SPACE, FailureReason };
    }
    MultiplayerTestRunner::ProcessDescriptors::PrintProcessDescriptor(MultiplayerTestRunner::ProcessDescriptors::JOINED_SPACE_DESCRIPTOR);
}

SpaceRAII::~SpaceRAII()
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto& SpaceSystem = *SystemsManager.GetSpaceSystem();

    // Leave space
    std::promise<csp::systems::NullResult> ResultPromise;
    std::future<csp::systems::NullResult> ResultFuture = ResultPromise.get_future();

    SpaceSystem.ExitSpace(
        [&ResultPromise](csp::systems::NullResult Result)
        {
            // Callbacks are called both in progress and at the end, guard against double promise sets
            if (Result.GetResultCode() == csp::systems::EResultCode::Success || Result.GetResultCode() == csp::systems::EResultCode::Failed)
            {
                ResultPromise.set_value(Result);
            }
        });

    csp::systems::NullResult ExitSpaceResult = ResultFuture.get();

    if (ExitSpaceResult.GetResultCode() == csp::systems::EResultCode::Success)
    {
        // We're not throwing on errors here cause we're destructing, but don't erroneously print bad descriptors.
        MultiplayerTestRunner::ProcessDescriptors::PrintProcessDescriptor(MultiplayerTestRunner::ProcessDescriptors::EXIT_SPACE_DESCRIPTOR);
    }

    if (CreatedThisSpace)
    {
        std::promise<csp::systems::NullResult> ResultPromise;
        std::future<csp::systems::NullResult> ResultFuture = ResultPromise.get_future();

        SpaceSystem.DeleteSpace(SpaceId.c_str(),
            [&ResultPromise](csp::systems::NullResult Result)
            {
                // Callbacks are called both in progress and at the end, guard against double promise sets
                if (Result.GetResultCode() == csp::systems::EResultCode::Success || Result.GetResultCode() == csp::systems::EResultCode::Failed)
                {
                    ResultPromise.set_value(Result);
                }
            });

        csp::systems::NullResult DeleteSpaceResult = ResultFuture.get();

        if (DeleteSpaceResult.GetResultCode() == csp::systems::EResultCode::Success)
        {
            // We're not throwing on errors here cause we're destructing, but don't erroneously print bad descriptors.
            MultiplayerTestRunner::ProcessDescriptors::PrintProcessDescriptor(MultiplayerTestRunner::ProcessDescriptors::DESTROYED_SPACE_DESCRIPTOR);
        }
    }
}

std::string SpaceRAII::GetSpaceId() { return SpaceId; }

csp::systems::Space SpaceRAII::CreateDefaultTestSpace(csp::systems::SpaceSystem& SpaceSystem)
{
    // Create space
    constexpr char* TestSpaceName = "CSP-MULTIPLAYERTEST-SPACE-MAG";
    constexpr char* TestSpaceDescription = "CSP-MULTIPLAYERTEST--SPACEDESC-MAG";

    UUIDv4::UUIDGenerator<std::mt19937_64> uuidGenerator;
    const UUIDv4::UUID uuid = uuidGenerator.getUUID();
    std::string UniqueSpaceName = TestSpaceName + std::string("-") + uuid.str();

    std::promise<csp::systems::SpaceResult> ResultPromise;
    std::future<csp::systems::SpaceResult> ResultFuture = ResultPromise.get_future();

    SpaceSystem.CreateSpace(UniqueSpaceName.c_str(), TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr,
        csp::common::Map<csp::common::String, csp::common::String> { { "site", "Void" } }, nullptr, nullptr,
        [&ResultPromise](csp::systems::SpaceResult Result)
        {
            // Callbacks are called both in progress and at the end, guard against double promise sets
            if (Result.GetResultCode() == csp::systems::EResultCode::Success || Result.GetResultCode() == csp::systems::EResultCode::Failed)
            {
                ResultPromise.set_value(Result);
            }
        });

    csp::systems::SpaceResult Result = ResultFuture.get();

    if (Result.GetResultCode() != csp::systems::EResultCode::Success)
    {
        std::string FailureReason = "HTTP Code: " + std::to_string(Result.GetHttpResultCode()) + " Body: " + Result.GetResponseBody().c_str();
        throw Utils::ExceptionWithCode { MultiplayerTestRunner::ErrorCodes::FAILED_TO_CREATE_SPACE, FailureReason };
    }
    MultiplayerTestRunner::ProcessDescriptors::PrintProcessDescriptor(MultiplayerTestRunner::ProcessDescriptors::CREATED_SPACE_DESCRIPTOR);

    return Result.GetSpace();
}