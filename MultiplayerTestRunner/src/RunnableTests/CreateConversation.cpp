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

#pragma once

#include "CreateConversation.h"
#include "../CLIArgs.h"

#include <CSP/Multiplayer/Components/ConversationSpaceComponent.h>
#include <CSP/Multiplayer/SpaceEntity.h>
#include <CSP/Multiplayer/OnlineRealtimeEngine.h>
#include <CSP/Multiplayer/SpaceTransform.h>
#include <CSP/Systems/SystemsManager.h>

#include <future>
#include <iostream>

namespace CreateConversation
{
/*
This tests that the CreateConversation multiplayer event is correctly processed by another client
when receiving 2 patches, the first being the initial component creation, and the second being
the ConversationId property update.
This scenario would fail if events arent correctly stored, and then flushed when receiving the conversation id
*/
void RunTest(csp::multiplayer::OnlineRealtimeEngine& RealtimeEngine)
{
    // Ensure patch rate limiting is off, as we're sending patches in quick succession.
    RealtimeEngine.SetEntityPatchRateLimitEnabled(false);

    csp::multiplayer::SpaceTransform ObjectTransform { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };

    std::promise<csp::multiplayer::SpaceEntity*> CreateObjectResultPromise;
    std::future<csp::multiplayer::SpaceEntity*> CreateObjectResultFuture = CreateObjectResultPromise.get_future();

    RealtimeEngine.CreateEntity("TestObject", ObjectTransform, csp::common::Optional<uint64_t> {},
        [&CreateObjectResultPromise](csp::multiplayer::SpaceEntity* Result) { CreateObjectResultPromise.set_value(Result); });

    csp::multiplayer::SpaceEntity* Object = CreateObjectResultFuture.get();

    if (Object == nullptr)
    {
        throw std::runtime_error("Failed to create TestObject");
    }

    // Create the conversation component
    auto* ConversationComponent = (csp::multiplayer::ConversationSpaceComponent*)Object->AddComponent(csp::multiplayer::ComponentType::Conversation);

    // Send patch before CreateConversation is called,
    // so clients get a patch which contains the conversation component with an invalid id
    Object->QueueUpdate();
    RealtimeEngine.ProcessPendingEntityOperations();

    // Create the conversation using the component
    std::promise<csp::common::String> CreateConversationResultPromise;
    std::future<csp::common::String> CreateConversationResultFuture = CreateConversationResultPromise.get_future();

    static constexpr const char* ConversationMessage = "Test Conversation";

    ConversationComponent->CreateConversation(ConversationMessage,
        [&CreateConversationResultPromise](const csp::systems::StringResult& Result)
        { CreateConversationResultPromise.set_value(Result.GetValue()); });

    csp::common::String ConversationId = CreateConversationResultFuture.get();

    std::cout << ConversationId << std::endl;

    // Send patch at the end so clients get a patch which contains the conversation component
    // with a valid conversation id
    Object->QueueUpdate();

    RealtimeEngine.ProcessPendingEntityOperations();
}

} // namespace CreateConversation