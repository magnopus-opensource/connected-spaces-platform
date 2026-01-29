/*
 * Copyright 2026 Magnopus LLC

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

#include "LeaderElectionEvent.h"

#include <CSP/CSPFoundation.h>
#include <CSP/Multiplayer/OnlineRealtimeEngine.h>
#include <CSP/Multiplayer/SpaceEntity.h>
#include <CSP/Systems/Multiplayer/MultiplayerSystem.h>
#include <CSP/Systems/Spaces/SpaceSystem.h>
#include <CSP/Systems/SystemsManager.h>
#include <CSP/Systems/Users/UserSystem.h>

#include <future>
#include <iostream>

namespace LeaderElectionEvent
{

void RunTest(csp::multiplayer::OnlineRealtimeEngine& RealtimeEngine)
{
    csp::multiplayer::SpaceEntity* Entity = RealtimeEngine.GetEntityByIndex(0);

    if (Entity == nullptr)
    {
        throw std::runtime_error("Script entity doesn't exist");
    }

    Entity->GetScript().PostMessageToScript("customEvent");
}
} // namespace LeaderElectionEvent