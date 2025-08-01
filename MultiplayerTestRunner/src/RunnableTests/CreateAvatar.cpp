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

#include "CreateAvatar.h"

#include "uuid_v4.h"
#include <CSP/Multiplayer/SpaceEntitySystem.h>
#include <CSP/Multiplayer/SpaceTransform.h>
#include <CSP/Systems/Spaces/SpaceSystem.h>
#include <CSP/Systems/SystemsManager.h>
#include <CSP/Systems/Users/UserSystem.h>
#include <future>

namespace CreateAvatar
{

void RunTest()
{
    using namespace csp::multiplayer;

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto& EntitySystem = *SystemsManager.GetSpaceEntitySystem();
    auto& UserSystem = *SystemsManager.GetUserSystem();

    UUIDv4::UUIDGenerator<std::mt19937_64> uuidGenerator;
    const UUIDv4::UUID uuid = uuidGenerator.getUUID();
    std::string UniqueSpaceName = "MultiplayerTestRunnerSpace" + std::string("-") + uuid.str();

    // Create avater
    csp::common::String UserName = "Player 1";
    SpaceTransform UserTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool IsVisible = true;
    AvatarState UserAvatarState = AvatarState::Idle;
    csp::common::String UserAvatarId = "MyCoolAvatar";
    AvatarPlayMode UserAvatarPlayMode = AvatarPlayMode::Default;

    EntitySystem.SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    std::promise<csp::multiplayer::SpaceEntity*> ResultPromise;
    std::future<csp::multiplayer::SpaceEntity*> ResultFuture = ResultPromise.get_future();

    const auto LoginState = UserSystem.GetLoginState();

    EntitySystem.CreateAvatar(UserName, LoginState, UserTransform, IsVisible, UserAvatarState, UserAvatarId, UserAvatarPlayMode,
        [&ResultPromise](csp::multiplayer::SpaceEntity* Result) { ResultPromise.set_value(Result); });

    ResultFuture.get();

    EntitySystem.ProcessPendingEntityOperations();

    // This is a hail mary attempt to get this to stop being flaky on CI. CHS is known to sometimes have a processing delay, which is unfortunate.
    std::this_thread::sleep_for(std::chrono::seconds(7));
}
} // namespace CreateAvatar