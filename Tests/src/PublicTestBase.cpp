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
#include "CSP/Systems/Log/LogSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "TestHelpers.h"

void PublicTestBase::SetUp()
{
    ::testing::Test::SetUp();

    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(csp::systems::LogLevel::VeryVerbose);

    csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback([](csp::common::String Message) { fprintf(stderr, "%s\n", Message.c_str()); });

    csp::systems::SystemsManager::Get().GetLogSystem()->LogMsg(csp::systems::LogLevel::Verbose, "Foundation initialised!");

    auto Connection = csp::systems::SystemsManager::Get().GetMultiplayerConnection();

    AWAIT(Connection, SetAllowSelfMessagingFlag, false);
}

void PublicTestBase::TearDown()
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

    csp::systems::SystemsManager::Get().GetLogSystem()->LogMsg(csp::systems::LogLevel::Verbose, "Foundation shutdown!");
    csp::CSPFoundation::Shutdown();
}

template <typename T> void PublicTestBaseWithParam<T>::SetUp()
{
    ::testing::Test::SetUp();

    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(csp::systems::LogLevel::VeryVerbose);

    csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback([](csp::common::String Message) { fprintf(stderr, "%s\n", Message.c_str()); });

    csp::systems::SystemsManager::Get().GetLogSystem()->LogMsg(csp::systems::LogLevel::Verbose, "Foundation initialised!");

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

    csp::systems::SystemsManager::Get().GetLogSystem()->LogMsg(csp::systems::LogLevel::Verbose, "Foundation shutdown!");
    csp::CSPFoundation::Shutdown();
}

// Explicit instantiations
template class PublicTestBaseWithParam<std::tuple<csp::systems::SpaceAttributes, csp::systems::EResultCode, std::string>>;