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

#include "ClearScriptSource.h"

#include <CSP/Multiplayer/ComponentBase.h>
#include <CSP/Multiplayer/Components/ScriptSpaceComponent.h>
#include <CSP/Multiplayer/OnlineRealtimeEngine.h>
#include <CSP/Multiplayer/SpaceEntity.h>

#include <iostream>
#include <stdexcept>

namespace ClearScriptSource
{

void RunTest(csp::multiplayer::OnlineRealtimeEngine& RealtimeEngine)
{

    // Must match the name of the entity created in the controlling test
    csp::multiplayer::SpaceEntity* Entity = RealtimeEngine.FindSpaceEntity("ScriptEntity");

    if (Entity == nullptr)
    {
        throw std::runtime_error("Could not find ScriptEntity. The controlling process should create and replicate it before starting this one.");
    }

    auto* ScriptComponent
        = static_cast<csp::multiplayer::ScriptSpaceComponent*>(Entity->FindFirstComponentOfType(csp::multiplayer::ComponentType::ScriptData));

    if (ScriptComponent == nullptr)
    {
        throw std::runtime_error("ScriptEntity has no ScriptSpaceComponent.");
    }

    // Just so you know a script was for-sure set and replicated, you can see if it's zero characters.
    std::cout << "Clearing script source of ScriptEntity, which was " << ScriptComponent->GetScriptSource().Length() << " characters" << std::endl;

    // The thing we're testing, clearing the script source remotely should stop script execution, including registered events (like reacting to ticks)
    ScriptComponent->SetScriptSource("");

    Entity->QueueUpdate();
    RealtimeEngine.ProcessPendingEntityOperations();
}

} // namespace ClearScriptSource
