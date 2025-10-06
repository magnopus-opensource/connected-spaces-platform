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
#include "PublicTestBase.h"

#include "Awaitable.h"
#include "CSP/CSPFoundation.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "Mocks/SignalRConnectionMock.h"
#include "TestHelpers.h"

void PublicTestBase::SetUp()
{
    ::testing::Test::SetUp();

    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    csp::common::LogSystem* LogSystem = csp::systems::SystemsManager::Get().GetLogSystem();
    LogSystem->SetSystemLevel(csp::common::LogLevel::VeryVerbose);
    LogSystem->SetLogCallback([](csp::common::String Message) { fprintf(stderr, "%s\n", Message.c_str()); });
    LogSystem->LogMsg(csp::common::LogLevel::Verbose, "Foundation initialised!");

    auto Connection = csp::systems::SystemsManager::Get().GetMultiplayerConnection();

    AWAIT(Connection, SetAllowSelfMessagingFlag, false);
}

void PublicTestBase::TearDown()
{
    ::testing::Test::TearDown();

    if (!csp::CSPFoundation::GetIsInitialised())
    {
        fprintf(stderr, "%s\n",
            "csp::CSPFoundation::Shutdown() already called! Please remove any explicit calls to Initialise() and Shutdown() from this test.");

        return;
    }

    csp::systems::SystemsManager::Get().GetLogSystem()->LogMsg(csp::common::LogLevel::Verbose, "Foundation shutdown!");
    csp::CSPFoundation::Shutdown();
}

void PublicTestBaseWithMocks::SetUp()
{
    SignalRMock = new SignalRConnectionMock();

    ::testing::Test::SetUp();

    // Yield SignalRMock ownership
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI(), SignalRMock);

    csp::common::LogSystem* LogSystem = csp::systems::SystemsManager::Get().GetLogSystem();
    LogSystem->SetSystemLevel(csp::common::LogLevel::VeryVerbose);
    LogSystem->SetLogCallback([](csp::common::String Message) { fprintf(stderr, "%s\n", Message.c_str()); });
    LogSystem->LogMsg(csp::common::LogLevel::Verbose, "Foundation initialised!");

    auto Connection = csp::systems::SystemsManager::Get().GetMultiplayerConnection();

    AWAIT(Connection, SetAllowSelfMessagingFlag, false);
}

void PublicTestBaseWithMocks::TearDown()
{
    ::testing::Test::TearDown();

    if (!csp::CSPFoundation::GetIsInitialised())
    {
        fprintf(stderr, "%s\n",
            "csp::CSPFoundation::Shutdown() already called! Please remove any explicit calls to Initialise() and Shutdown() from this test.");

        return;
    }

    csp::systems::SystemsManager::Get().GetLogSystem()->LogMsg(csp::common::LogLevel::Verbose, "Foundation shutdown!");
    csp::CSPFoundation::Shutdown();
}

template <typename T> void PublicTestBaseWithParam<T>::SetUp()
{
    ::testing::Test::SetUp();

    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    csp::common::LogSystem* LogSystem = csp::systems::SystemsManager::Get().GetLogSystem();
    LogSystem->SetSystemLevel(csp::common::LogLevel::VeryVerbose);
    LogSystem->SetLogCallback([](csp::common::String Message) { fprintf(stderr, "%s\n", Message.c_str()); });
    LogSystem->LogMsg(csp::common::LogLevel::Verbose, "Foundation initialised!");

    auto Connection = csp::systems::SystemsManager::Get().GetMultiplayerConnection();

    AWAIT(Connection, SetAllowSelfMessagingFlag, false);
}

template <typename T> void PublicTestBaseWithParam<T>::TearDown()
{
    ::testing::Test::TearDown();

    auto Connection = csp::systems::SystemsManager::Get().GetMultiplayerConnection();

    AWAIT(Connection, SetAllowSelfMessagingFlag, false);

    if (!csp::CSPFoundation::GetIsInitialised())
    {
        fprintf(stderr, "%s\n",
            "csp::CSPFoundation::Shutdown() already called! Please remove any explicit calls to Initialise() and Shutdown() from this test.");

        return;
    }

    csp::systems::SystemsManager::Get().GetLogSystem()->LogMsg(csp::common::LogLevel::Verbose, "Foundation shutdown!");
    csp::CSPFoundation::Shutdown();
}

// Explicit instantiations
template class PublicTestBaseWithParam<std::tuple<csp::systems::SpaceAttributes, csp::systems::EResultCode, std::string>>;
template class PublicTestBaseWithParam<std::tuple<csp::common::RealtimeEngineType, bool, csp::systems::EResultCode, std::string>>;
template class PublicTestBaseWithParam<csp::common::RealtimeEngineType>;
template class PublicTestBaseWithParam<std::tuple<csp::common::RealtimeEngineType, bool>>;
template class PublicTestBaseWithParam<std::tuple<csp::systems::AvatarType, csp::common::String, bool>>;
template class PublicTestBaseWithParam<std::tuple<csp::systems::EResultCode, csp::web::EResponseCodes, csp::common::String, bool>>;