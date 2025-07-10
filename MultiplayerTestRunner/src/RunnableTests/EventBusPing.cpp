/*
 * Copyright 2025 Magnopus LLC

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

#include "CreateAvatar.h"

#include "uuid_v4.h"
#include <CSP/Multiplayer/SpaceTransform.h>
#include <CSP/Systems/Spaces/SpaceSystem.h>
#include <CSP/Systems/SystemsManager.h>
#include <future>

namespace EventBusPing
{

void RunTest()
{
    // Listen for an event, then ping it back to the client that sent it.
    csp::systems::SystemsManager::Get().GetEventBus()->ListenNetworkEvent(csp::multiplayer::NetworkEventRegistration("Receiver", "EventPingRequest"),
        [](const csp::common::NetworkEventData& NetworkEventData)
        {
            csp::systems::SystemsManager::Get().GetEventBus()->SendNetworkEventToClient(
                "EventPingResponse", {}, NetworkEventData.SenderClientId, [](csp::multiplayer::ErrorCode) {});
        });
}
} // namespace CreateAvatar