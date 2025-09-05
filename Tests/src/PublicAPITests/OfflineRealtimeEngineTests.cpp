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

#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Multiplayer/CSPSceneDescription.h"
#include "CSP/Multiplayer/OfflineRealtimeEngine.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Multiplayer/SpaceTransform.h"
#include "CSP/Systems/CSPSceneData.h"
#include "CSP/Systems/Script/ScriptSystem.h"
#include "CSP/Systems/Spaces/SpaceSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "UserSystemTestHelpers.h"

#include "TestHelpers.h"

#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

using namespace csp;
using namespace csp::multiplayer;

namespace
{
bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }
}

/*
    Checks OfflineRealtimeEngine is returning the correct enum for GetRealtimeEngineType
*/
CSP_PUBLIC_TEST(CSPEngine, OfflineRealtimeEngineTests, GetRealtimeEngineType)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();

    CSPSceneDescription SceneDescription;
    OfflineRealtimeEngine Engine { SceneDescription, *SystemsManager.GetLogSystem(), *SystemsManager.GetScriptSystem() };

    EXPECT_EQ(Engine.GetRealtimeEngineType(), csp::common::RealtimeEngineType::Offline);
}

/*
    Tests the following behaviour for OfflineRealtimeEngine::CreateAvatar:
       * A non-null entity is returned from the EntityCreated callback
       * The local callback fires before the function ends, as the offline realtime engine is synchronous
       * The EntityCreatedCallback fires before the function ends
       * Entity properties are populated correctly with the given parameters
       * An AvatarComponent is created
       * The avatar component properties are populated correctly with the given parameters
       * The entity can be retrieved from GetEntities and GetAvatars
*/
CSP_PUBLIC_TEST(CSPEngine, OfflineRealtimeEngineTests, CreateAvatar)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();

    CSPSceneDescription SceneDescription;
    OfflineRealtimeEngine Engine { SceneDescription, *SystemsManager.GetLogSystem(), *SystemsManager.GetScriptSystem() };

    // Create test properties for our avatar.
    const common::String TestName = "TestName";
    const SpaceTransform Transform { common::Vector3::One(), common::Vector4::One(), common::Vector3::Zero() };
    static constexpr const bool IsVisible = false;
    static constexpr const auto State = AvatarState::Running;
    const common::String AvatarId = "Id";
    static constexpr const auto PlayMode = AvatarPlayMode::Creator;

    SpaceEntity* CreatedEntity = nullptr;

    Engine.CreateAvatar(
        TestName, nullptr, Transform, IsVisible, State, AvatarId, PlayMode, [&CreatedEntity](SpaceEntity* NewEntity) { CreatedEntity = NewEntity; });

    // Callback should be called before the function ends in offline mode, so this should be set.
    if (CreatedEntity == nullptr)
    {
        FAIL();
    }

    // Ensure created entity is populated correctly.
    EXPECT_EQ(CreatedEntity->GetName(), TestName);
    EXPECT_EQ(CreatedEntity->GetTransform(), Transform);
    EXPECT_EQ(CreatedEntity->GetParent(), nullptr);

    // Now check our AvatarComponent which should have been created by CreateAvatar.
    if (CreatedEntity->GetComponents()->Size() != 1)
    {
        FAIL();
    }

    const auto* AvatarComponent = static_cast<AvatarSpaceComponent*>((*CreatedEntity->GetComponents())[0]);

    // Ensure created avatar component is populated correctly.
    EXPECT_EQ(AvatarComponent->GetIsVisible(), IsVisible);
    EXPECT_EQ(AvatarComponent->GetState(), AvatarState::Running);
    EXPECT_EQ(AvatarComponent->GetAvatarId(), AvatarId);
    EXPECT_EQ(AvatarComponent->GetAvatarPlayMode(), PlayMode);

    // Check that our avatar is registered as an entity in the engine.
    if (Engine.GetNumEntities() != 1)
    {
        FAIL();
    }

    EXPECT_EQ(Engine.GetEntityByIndex(0)->GetId(), CreatedEntity->GetId());

    // Check our avatar is registered as an avatar in the engine.
    if (Engine.GetNumAvatars() != 1)
    {
        FAIL();
    }

    EXPECT_EQ(Engine.GetAvatarByIndex(0)->GetId(), CreatedEntity->GetId());

    // Check our avatar is NOT registered as an object in the engine.
    EXPECT_EQ(Engine.GetNumObjects(), 0);
}

/*
    This tests the behaviour of OfflineRealtimeEngine::CreateEntity.
    This is very similar to the CreateAvatar test, except an avatar component isn't create by the function.
*/
CSP_PUBLIC_TEST(CSPEngine, OfflineRealtimeEngineTests, CreateEntity)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();

    CSPSceneDescription SceneDescription;
    OfflineRealtimeEngine Engine { SceneDescription, *SystemsManager.GetLogSystem(), *SystemsManager.GetScriptSystem() };

    // Create test properties for our entity.
    const common::String TestName = "TestName";
    const SpaceTransform Transform { common::Vector3::One(), common::Vector4::One(), common::Vector3::Zero() };
    static constexpr const bool IsVisible = false;
    static constexpr const auto State = AvatarState::Running;
    const common::String AvatarId = "Id";
    static constexpr const auto PlayMode = AvatarPlayMode::Creator;

    SpaceEntity* CreatedEntity = nullptr;

    Engine.CreateEntity(TestName, Transform, nullptr, [&CreatedEntity](SpaceEntity* NewEntity) { CreatedEntity = NewEntity; });

    // Callback should be called before the function ends in offline mode, so this should be set.
    if (CreatedEntity == nullptr)
    {
        FAIL();
    }

    // Ensure created entity is populated correctly.
    EXPECT_EQ(CreatedEntity->GetName(), TestName);
    EXPECT_EQ(CreatedEntity->GetTransform(), Transform);
    EXPECT_EQ(CreatedEntity->GetParent(), nullptr);

    // Check that our entity is registered as an entity in the engine.
    if (Engine.GetNumEntities() != 1)
    {
        FAIL();
    }

    EXPECT_EQ(Engine.GetEntityByIndex(0)->GetId(), CreatedEntity->GetId());

    // Check our entity is NOT registered as an avatar in the engine.
    EXPECT_EQ(Engine.GetNumAvatars(), 0);

    // Check our entity is also registered as an object in the engine.
    EXPECT_EQ(Engine.GetNumObjects(), 1);
}

/*
    This tests the behaviour of OfflineRealtimeEngine::DestroyEntity
    by verifying it is removed from the engine when called.
    It also verifies that the SetDestroyCallback is called correctly.
*/
CSP_PUBLIC_TEST(CSPEngine, OfflineRealtimeEngineTests, DestroyEntity)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();

    CSPSceneDescription SceneDescription;
    OfflineRealtimeEngine Engine { SceneDescription, *SystemsManager.GetLogSystem(), *SystemsManager.GetScriptSystem() };

    Engine.CreateEntity("", SpaceTransform {}, nullptr, [](SpaceEntity*) {});

    if (Engine.GetNumEntities() != 1)
    {
        FAIL();
    }

    SpaceEntity* CreatedEntity = Engine.GetEntityByIndex(0);

    bool DestroyCalled = false;

    Engine.DestroyEntity(CreatedEntity,
        [&DestroyCalled](bool Destroyed)
        {
            EXPECT_TRUE(Destroyed);
            DestroyCalled = true;
        });

    EXPECT_TRUE(DestroyCalled);
    EXPECT_EQ(Engine.GetNumEntities(), 0);
}

/*
    This tests the behaviour of OfflineRealtimeEngine::DestroyEntity for Avatars.
    This is similar to DestroyEntity test, except it also verifies the avatar is removed
    from the avatar container.
*/
CSP_PUBLIC_TEST(CSPEngine, OfflineRealtimeEngineTests, DestroyAvatar)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();

    CSPSceneDescription SceneDescription;
    OfflineRealtimeEngine Engine { SceneDescription, *SystemsManager.GetLogSystem(), *SystemsManager.GetScriptSystem() };

    Engine.CreateAvatar("", nullptr, SpaceTransform {}, false, AvatarState::Idle, "", AvatarPlayMode::Default, [](SpaceEntity*) {});

    if (Engine.GetNumEntities() != 1)
    {
        FAIL();
    }

    SpaceEntity* CreatedEntity = Engine.GetEntityByIndex(0);

    bool DestroyCalled = false;

    Engine.DestroyEntity(CreatedEntity,
        [&DestroyCalled](bool Destroyed)
        {
            EXPECT_TRUE(Destroyed);
            DestroyCalled = true;
        });

    EXPECT_TRUE(DestroyCalled);
    EXPECT_EQ(Engine.GetNumEntities(), 0);
    EXPECT_EQ(Engine.GetNumAvatars(), 0);
}

/*
    This tests the behaviour AddEntityToSelectedEntities and RemoveEntityFromSelectedEntities
    by checking if the provided entity gets added and removed from the internal container.
*/
CSP_PUBLIC_TEST(CSPEngine, OfflineRealtimeEngineTests, SelectEntity)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();

    CSPSceneDescription SceneDescription;
    OfflineRealtimeEngine Engine { SceneDescription, *SystemsManager.GetLogSystem(), *SystemsManager.GetScriptSystem() };

    SpaceEntity* CreatedEntity = nullptr;
    SpaceEntity* CreatedEntity2 = nullptr;

    Engine.CreateEntity("", SpaceTransform {}, nullptr, [&CreatedEntity](SpaceEntity* NewEntity) { CreatedEntity = NewEntity; });
    Engine.CreateEntity("", SpaceTransform {}, nullptr, [&CreatedEntity2](SpaceEntity* NewEntity) { CreatedEntity2 = NewEntity; });

    if (CreatedEntity == nullptr)
    {
        FAIL();
    }

    Engine.AddEntityToSelectedEntities(CreatedEntity);

    if (Engine.SelectedEntities.Size() != 1)
    {
        FAIL();
    }

    EXPECT_EQ(Engine.SelectedEntities[0]->GetId(), CreatedEntity->GetId());

    Engine.AddEntityToSelectedEntities(CreatedEntity2);

    if (Engine.SelectedEntities.Size() != 2)
    {
        FAIL();
    }

    EXPECT_EQ(Engine.SelectedEntities[1]->GetId(), CreatedEntity2->GetId());

    // Remove the second entity
    Engine.RemoveEntityFromSelectedEntities(CreatedEntity2);

    EXPECT_EQ(Engine.SelectedEntities.Size(), 1);

    if (Engine.SelectedEntities.Size() != 1)
    {
        FAIL();
    }

    EXPECT_EQ(Engine.SelectedEntities[0]->GetId(), CreatedEntity->GetId());

    // Remove the first entity
    Engine.RemoveEntityFromSelectedEntities(CreatedEntity);

    EXPECT_EQ(Engine.SelectedEntities.Size(), 0);
}

/*
    This tests the behaviour FindSpaceEntity.
    by creating 2 different entities and one avatar, checking they can all be retrieved.
    This alo tests that Avatars are registered as Enities.
*/
CSP_PUBLIC_TEST(CSPEngine, OfflineRealtimeEngineTests, FindSpaceEntity)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();

    CSPSceneDescription SceneDescription;
    OfflineRealtimeEngine Engine { SceneDescription, *SystemsManager.GetLogSystem(), *SystemsManager.GetScriptSystem() };

    const csp::common::String EntityName1 = "Entity1";
    const csp::common::String EntityName2 = "Entity2";
    const csp::common::String EntityName3 = "Entity3";

    SpaceEntity* Entity1 = nullptr;
    SpaceEntity* Entity2 = nullptr;
    SpaceEntity* Entity3 = nullptr;

    Engine.CreateEntity(EntityName1, SpaceTransform {}, nullptr, [&Entity1](SpaceEntity* NewEntity) { Entity1 = NewEntity; });
    Engine.CreateAvatar(EntityName2, nullptr, SpaceTransform {}, false, AvatarState::Idle, "", AvatarPlayMode::Default,
        [&Entity2](SpaceEntity* NewEntity) { Entity2 = NewEntity; });
    Engine.CreateEntity(EntityName3, SpaceTransform {}, nullptr, [&Entity3](SpaceEntity* NewEntity) { Entity3 = NewEntity; });

    SpaceEntity* FoundEntity1 = Engine.FindSpaceEntity(EntityName1);
    if (FoundEntity1 == nullptr)
    {
        FAIL();
    }

    EXPECT_EQ(FoundEntity1->GetId(), Entity1->GetId());

    SpaceEntity* FoundEntity2 = Engine.FindSpaceEntity(EntityName2);
    if (FoundEntity2 == nullptr)
    {
        FAIL();
    }

    EXPECT_EQ(FoundEntity2->GetId(), Entity2->GetId());

    SpaceEntity* FoundEntity3 = Engine.FindSpaceEntity(EntityName3);
    if (FoundEntity3 == nullptr)
    {
        FAIL();
    }

    EXPECT_EQ(FoundEntity3->GetId(), Entity3->GetId());
}

/*
    This tests the behaviour FindSpaceEntityById
    by creating 2 different entities and 1 avatar and checking they can all be retrieved.
*/
CSP_PUBLIC_TEST(CSPEngine, OfflineRealtimeEngineTests, FindSpaceEntityById)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();

    CSPSceneDescription SceneDescription;
    OfflineRealtimeEngine Engine { SceneDescription, *SystemsManager.GetLogSystem(), *SystemsManager.GetScriptSystem() };

    const csp::common::String EntityName1 = "Entity1";
    const csp::common::String EntityName2 = "Entity2";
    const csp::common::String EntityName3 = "Entity3";

    SpaceEntity* Entity1 = nullptr;
    SpaceEntity* Entity2 = nullptr;
    SpaceEntity* Entity3 = nullptr;

    Engine.CreateEntity(EntityName1, SpaceTransform {}, nullptr, [&Entity1](SpaceEntity* NewEntity) { Entity1 = NewEntity; });
    Engine.CreateAvatar(EntityName2, nullptr, SpaceTransform {}, false, AvatarState::Idle, "", AvatarPlayMode::Default,
        [&Entity2](SpaceEntity* NewEntity) { Entity2 = NewEntity; });
    Engine.CreateEntity(EntityName3, SpaceTransform {}, nullptr, [&Entity3](SpaceEntity* NewEntity) { Entity3 = NewEntity; });

    SpaceEntity* FoundEntity1 = Engine.FindSpaceEntityById(Entity1->GetId());
    if (FoundEntity1 == nullptr)
    {
        FAIL();
    }

    EXPECT_EQ(FoundEntity1->GetId(), Entity1->GetId());

    SpaceEntity* FoundEntity2 = Engine.FindSpaceEntityById(Entity2->GetId());
    if (FoundEntity2 == nullptr)
    {
        FAIL();
    }

    EXPECT_EQ(FoundEntity2->GetId(), Entity2->GetId());

    SpaceEntity* FoundEntity3 = Engine.FindSpaceEntityById(Entity3->GetId());
    if (FoundEntity3 == nullptr)
    {
        FAIL();
    }

    EXPECT_EQ(FoundEntity3->GetId(), Entity3->GetId());
}

/*
    This tests the behaviour FindSpaceAvatar
    by creating 2 avatars and one entity and checking they can all be retrieved.
*/
CSP_PUBLIC_TEST(CSPEngine, OfflineRealtimeEngineTests, FindSpaceAvatar)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();

    CSPSceneDescription SceneDescription;
    OfflineRealtimeEngine Engine { SceneDescription, *SystemsManager.GetLogSystem(), *SystemsManager.GetScriptSystem() };

    const csp::common::String AvatarName1 = "Avatar1";
    const csp::common::String EntityName1 = "Avatar2";
    const csp::common::String AvatarName3 = "Avatar3";

    SpaceEntity* Avatar1 = nullptr;
    SpaceEntity* Entity1 = nullptr;
    SpaceEntity* Avatar3 = nullptr;

    Engine.CreateAvatar(AvatarName1, nullptr, SpaceTransform {}, false, AvatarState::Idle, "", AvatarPlayMode::Default,
        [&Avatar1](SpaceEntity* NewEntity) { Avatar1 = NewEntity; });
    Engine.CreateEntity(EntityName1, SpaceTransform {}, nullptr, [&Entity1](SpaceEntity* NewEntity) { Entity1 = NewEntity; });
    Engine.CreateAvatar(AvatarName3, nullptr, SpaceTransform {}, false, AvatarState::Idle, "", AvatarPlayMode::Default,
        [&Avatar3](SpaceEntity* NewEntity) { Avatar3 = NewEntity; });

    SpaceEntity* FoundAvatar1 = Engine.FindSpaceEntity(AvatarName1);
    if (FoundAvatar1 == nullptr)
    {
        FAIL();
    }

    EXPECT_EQ(FoundAvatar1->GetId(), Avatar1->GetId());

    // Avatar should still be found using FindSpaceEntity
    SpaceEntity* FoundEntity1 = Engine.FindSpaceEntity(EntityName1);
    if (FoundEntity1 == nullptr)
    {
        FAIL();
    }

    EXPECT_EQ(FoundEntity1->GetId(), Entity1->GetId());

    SpaceEntity* FoundAvatar3 = Engine.FindSpaceEntity(AvatarName3);
    if (FoundAvatar3 == nullptr)
    {
        FAIL();
    }

    EXPECT_EQ(FoundAvatar3->GetId(), Avatar3->GetId());
}

/*
    This tests the behaviour FindSpaceObject by  creating 2 different entities and 1 avatar,
    checking ONLY the entities can be retieved from this function.
*/
CSP_PUBLIC_TEST(CSPEngine, OfflineRealtimeEngineTests, FindSpaceObject)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();

    CSPSceneDescription SceneDescription;
    OfflineRealtimeEngine Engine { SceneDescription, *SystemsManager.GetLogSystem(), *SystemsManager.GetScriptSystem() };

    const csp::common::String EntityName1 = "Entity1";
    const csp::common::String EntityName2 = "Entity2";
    const csp::common::String EntityName3 = "Entity3";

    SpaceEntity* Entity1 = nullptr;
    SpaceEntity* Entity2 = nullptr;
    SpaceEntity* Entity3 = nullptr;

    Engine.CreateEntity(EntityName1, SpaceTransform {}, nullptr, [&Entity1](SpaceEntity* NewEntity) { Entity1 = NewEntity; });
    Engine.CreateAvatar(EntityName2, nullptr, SpaceTransform {}, false, AvatarState::Idle, "", AvatarPlayMode::Default,
        [&Entity2](SpaceEntity* NewEntity) { Entity2 = NewEntity; });
    Engine.CreateEntity(EntityName3, SpaceTransform {}, nullptr, [&Entity3](SpaceEntity* NewEntity) { Entity3 = NewEntity; });

    SpaceEntity* FoundEntity1 = Engine.FindSpaceObject(Entity1->GetName());
    if (FoundEntity1 == nullptr)
    {
        FAIL();
    }

    EXPECT_EQ(FoundEntity1->GetName(), Entity1->GetName());

    // Our avatar should not be found.
    SpaceEntity* FoundEntity2 = Engine.FindSpaceObject(Entity2->GetName());
    EXPECT_EQ(FoundEntity2, nullptr);

    SpaceEntity* FoundEntity3 = Engine.FindSpaceObject(Entity3->GetName());
    if (FoundEntity3 == nullptr)
    {
        FAIL();
    }

    EXPECT_EQ(FoundEntity3->GetName(), Entity3->GetName());
}

/*
    This tests the behaviour GetEntityByIndex
    by creating 2 different entities and 1 avatar and checking they can all be retrieved.
    This also tests the GetNumX functions.
*/
CSP_PUBLIC_TEST(CSPEngine, OfflineRealtimeEngineTests, GetEntityByIndex)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();

    CSPSceneDescription SceneDescription;
    OfflineRealtimeEngine Engine { SceneDescription, *SystemsManager.GetLogSystem(), *SystemsManager.GetScriptSystem() };

    const csp::common::String EntityName1 = "Entity1";
    const csp::common::String EntityName2 = "Entity2";
    const csp::common::String EntityName3 = "Entity3";

    SpaceEntity* Entity1 = nullptr;
    SpaceEntity* Entity2 = nullptr;
    SpaceEntity* Entity3 = nullptr;

    Engine.CreateEntity(EntityName1, SpaceTransform {}, nullptr, [&Entity1](SpaceEntity* NewEntity) { Entity1 = NewEntity; });
    Engine.CreateAvatar(EntityName2, nullptr, SpaceTransform {}, false, AvatarState::Idle, "", AvatarPlayMode::Default,
        [&Entity2](SpaceEntity* NewEntity) { Entity2 = NewEntity; });
    Engine.CreateEntity(EntityName3, SpaceTransform {}, nullptr, [&Entity3](SpaceEntity* NewEntity) { Entity3 = NewEntity; });

    EXPECT_EQ(Engine.GetNumEntities(), 3);
    EXPECT_EQ(Engine.GetNumAvatars(), 1);
    EXPECT_EQ(Engine.GetNumObjects(), 2);

    SpaceEntity* FoundEntity1 = Engine.GetEntityByIndex(0);
    if (FoundEntity1 == nullptr)
    {
        FAIL();
    }

    EXPECT_EQ(FoundEntity1->GetId(), Entity1->GetId());

    SpaceEntity* FoundEntity2 = Engine.GetEntityByIndex(1);
    if (FoundEntity2 == nullptr)
    {
        FAIL();
    }

    EXPECT_EQ(FoundEntity2->GetId(), Entity2->GetId());

    SpaceEntity* FoundEntity3 = Engine.GetEntityByIndex(2);
    if (FoundEntity3 == nullptr)
    {
        FAIL();
    }

    EXPECT_EQ(FoundEntity3->GetId(), Entity3->GetId());
}

/*
    This tests the behaviour GetAvatarByIndex
    by creating 2 avatars and one entity and checking they can all be retrieved.
    This also tests the GetNumX functions.
*/
CSP_PUBLIC_TEST(CSPEngine, OfflineRealtimeEngineTests, GetAvatarByIndex)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();

    CSPSceneDescription SceneDescription;
    OfflineRealtimeEngine Engine { SceneDescription, *SystemsManager.GetLogSystem(), *SystemsManager.GetScriptSystem() };

    const csp::common::String AvatarName1 = "Avatar1";
    const csp::common::String AvatarName2 = "Avatar2";
    const csp::common::String AvatarName3 = "Avatar3";

    SpaceEntity* Avatar1 = nullptr;
    SpaceEntity* Avatar2 = nullptr;
    SpaceEntity* Avatar3 = nullptr;

    Engine.CreateAvatar(AvatarName1, nullptr, SpaceTransform {}, false, AvatarState::Idle, "", AvatarPlayMode::Default,
        [&Avatar1](SpaceEntity* NewEntity) { Avatar1 = NewEntity; });
    Engine.CreateEntity(AvatarName2, SpaceTransform {}, nullptr, [&Avatar2](SpaceEntity* NewEntity) { Avatar2 = NewEntity; });
    Engine.CreateAvatar(AvatarName3, nullptr, SpaceTransform {}, false, AvatarState::Idle, "", AvatarPlayMode::Default,
        [&Avatar3](SpaceEntity* NewEntity) { Avatar3 = NewEntity; });

    EXPECT_EQ(Engine.GetNumEntities(), 3);
    EXPECT_EQ(Engine.GetNumAvatars(), 2);
    EXPECT_EQ(Engine.GetNumObjects(), 1);

    SpaceEntity* FoundAvatar1 = Engine.GetAvatarByIndex(0);
    if (FoundAvatar1 == nullptr)
    {
        FAIL();
    }

    EXPECT_EQ(FoundAvatar1->GetId(), Avatar1->GetId());

    // The second avatar (the one added third) should be found in the second element.
    SpaceEntity* FoundAvatar2 = Engine.GetAvatarByIndex(1);
    if (FoundAvatar2 == nullptr)
    {
        FAIL();
    }

    EXPECT_EQ(FoundAvatar2->GetId(), Avatar3->GetId());
}

/*
    This tests the behaviour GetObjectByIndex
    by creating 2 different entities and 1 avatar and checking they can all be retrieved.
    This also tests the GetNumX functions.
*/
CSP_PUBLIC_TEST(CSPEngine, OfflineRealtimeEngineTests, GetObjectByIndex)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();

    CSPSceneDescription SceneDescription;
    OfflineRealtimeEngine Engine { SceneDescription, *SystemsManager.GetLogSystem(), *SystemsManager.GetScriptSystem() };

    const csp::common::String EntityName1 = "Entity1";
    const csp::common::String EntityName2 = "Entity2";
    const csp::common::String EntityName3 = "Entity3";

    SpaceEntity* Entity1 = nullptr;
    SpaceEntity* Entity2 = nullptr;
    SpaceEntity* Entity3 = nullptr;

    Engine.CreateEntity(EntityName1, SpaceTransform {}, nullptr, [&Entity1](SpaceEntity* NewEntity) { Entity1 = NewEntity; });
    Engine.CreateAvatar(EntityName2, nullptr, SpaceTransform {}, false, AvatarState::Idle, "", AvatarPlayMode::Default,
        [&Entity2](SpaceEntity* NewEntity) { Entity2 = NewEntity; });
    Engine.CreateEntity(EntityName3, SpaceTransform {}, nullptr, [&Entity3](SpaceEntity* NewEntity) { Entity3 = NewEntity; });

    EXPECT_EQ(Engine.GetNumEntities(), 3);
    EXPECT_EQ(Engine.GetNumAvatars(), 1);
    EXPECT_EQ(Engine.GetNumObjects(), 2);

    SpaceEntity* FoundEntity1 = Engine.GetObjectByIndex(0);
    if (FoundEntity1 == nullptr)
    {
        FAIL();
    }

    EXPECT_EQ(FoundEntity1->GetId(), Entity1->GetId());

    // The second entity (the one added third) should be found in the second element.
    SpaceEntity* FoundEntity2 = Engine.GetObjectByIndex(1);
    if (FoundEntity2 == nullptr)
    {
        FAIL();
    }

    EXPECT_EQ(FoundEntity2->GetId(), Entity3->GetId());
}

/*
    This tests the behaviour of correctly setting the ParentId and RootHierarchy entities.
    We first test that the constuctor is correctly setting these properties, and then ensure
    the properties are still correrct after additions and deletions.

*/
CSP_PUBLIC_TEST(CSPEngine, OfflineRealtimeEngineTests, ParentTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();

    CSPSceneDescription SceneDescription;
    OfflineRealtimeEngine Engine { SceneDescription, *SystemsManager.GetLogSystem(), *SystemsManager.GetScriptSystem() };

    const csp::common::String EntityName1 = "Entity1";
    const csp::common::String EntityName2 = "Entity2";
    const csp::common::String EntityName3 = "Entity3";

    SpaceEntity* Entity1 = nullptr;
    SpaceEntity* Entity2 = nullptr;
    SpaceEntity* Entity3 = nullptr;

    Engine.CreateEntity(EntityName1, SpaceTransform {}, nullptr, [&Entity1](SpaceEntity* NewEntity) { Entity1 = NewEntity; });
    Engine.CreateEntity(EntityName2, SpaceTransform {}, Entity1->GetId(), [&Entity2](SpaceEntity* NewEntity) { Entity2 = NewEntity; });
    Engine.CreateEntity(EntityName3, SpaceTransform {}, Entity2->GetId(), [&Entity3](SpaceEntity* NewEntity) { Entity3 = NewEntity; });

    EXPECT_EQ(Entity1->GetParent(), nullptr);
    ASSERT_NE(Entity2->GetParent(), nullptr);
    ASSERT_NE(Entity3->GetParent(), nullptr);

    EXPECT_EQ(Entity2->GetParent(), Entity1);
    EXPECT_EQ(Entity3->GetParent(), Entity2);

    ASSERT_EQ(Engine.GetRootHierarchyEntities()->Size(), 1);

    EXPECT_EQ((*Engine.GetRootHierarchyEntities())[0]->GetId(), Entity1->GetId());

    // Reparent the third entity to be a child of the first
    Entity3->SetParentId(Entity2->GetId());

    EXPECT_EQ(Entity1->GetParent(), nullptr);
    EXPECT_EQ(Entity2->GetParent(), Entity1);
    EXPECT_EQ(Entity3->GetParent(), Entity2);

    // Move all entities to the root.
    Entity2->RemoveParentEntity();
    Entity3->RemoveParentEntity();

    // Parents should all be null.
    EXPECT_EQ(Entity1->GetParent(), nullptr);
    EXPECT_EQ(Entity2->GetParent(), nullptr);
    EXPECT_EQ(Entity3->GetParent(), nullptr);

    // All entities should be at the root.
    EXPECT_EQ(Engine.GetRootHierarchyEntities()->Size(), 3);

    // Ensure Root hierarchy is updated if entity is moved from the root.
    Entity3->SetParentId(Entity1->GetId());

    EXPECT_EQ(Entity1->GetParent(), nullptr);
    EXPECT_EQ(Entity2->GetParent(), nullptr);
    EXPECT_EQ(Entity3->GetParent(), Entity1);

    EXPECT_EQ(Engine.GetRootHierarchyEntities()->Size(), 2);
}

/*
    This tests the behaviour of OfflineRealtimeEngine::MarkEntityForUpdate
    by verifying an entity update is queued when ProcessPendingEntityOperations is called
*/
CSP_PUBLIC_TEST(CSPEngine, OfflineRealtimeEngineTests, MarkEntityForUpdate)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();

    CSPSceneDescription SceneDescription;
    OfflineRealtimeEngine Engine { SceneDescription, *SystemsManager.GetLogSystem(), *SystemsManager.GetScriptSystem() };

    const csp::common::String EntityName = "Entity1";
    SpaceEntity* Entity = nullptr;

    Engine.CreateEntity(EntityName, SpaceTransform {}, nullptr, [&Entity](SpaceEntity* NewEntity) { Entity = NewEntity; });

    const csp::common::String NewEntityName = "NewEntity1";

    Entity->SetName(NewEntityName);

    EXPECT_EQ(Entity->GetName(), NewEntityName);
}

/*
    This is a basic integration test, showing that an empty CSPSceneDescription is correctly processed in the full offline flow.
*/
CSP_PUBLIC_TEST(CSPEngine, OfflineRealtimeEngineTests, EmptySceneDescriptionTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId, false);

    // Get checkpoint file
    auto FilePath = std::filesystem::absolute("assets/checkpoint-empty.json");

    std::ifstream Stream { FilePath.u8string().c_str() };

    if (!Stream)
    {
        FAIL();
    }

    std::stringstream SStream;
    SStream << Stream.rdbuf();

    std::string Json = SStream.str();

    systems::CSPSceneData SceneData { Json.c_str() };
    CSPSceneDescription SceneDescription { Json.c_str() };

    // Enter space from scene description
    auto RealtimeEngine = std::make_unique<csp::multiplayer::OfflineRealtimeEngine>(
        SceneDescription, *SystemsManager.GetLogSystem(), *SystemsManager.GetScriptSystem());

    // Ensure callback is called correctly with the correct number of entities.
    bool CallbackCalled = false;

    RealtimeEngine->SetEntityFetchCompleteCallback(
        [&CallbackCalled](uint32_t Count)
        {
            EXPECT_EQ(Count, 0);
            CallbackCalled = true;
        });

    auto [EnterSpaceResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, SceneData.Space.Id, RealtimeEngine.get());
    EXPECT_EQ(EnterSpaceResult.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_TRUE(CallbackCalled);

    EXPECT_EQ(RealtimeEngine->GetAllEntities()->Size(), 0);

    // Cleanup
    AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    LogOut(UserSystem);
}

/*
    This is a basic integration test, showing that a basic CSPSceneDescription with one entity is correctly processed in the full offline flow.
*/
CSP_PUBLIC_TEST(CSPEngine, OfflineRealtimeEngineTests, BasicSceneDescriptionTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId, false);

    // Get checkpoint file
    auto FilePath = std::filesystem::absolute("assets/checkpoint-basic.json");

    std::ifstream Stream { FilePath.u8string().c_str() };

    if (!Stream)
    {
        FAIL();
    }

    std::stringstream SStream;
    SStream << Stream.rdbuf();

    std::string Json = SStream.str();

    // Enter space from scene description
    systems::CSPSceneData SceneData { Json.c_str() };
    CSPSceneDescription SceneDescription { Json.c_str() };

    auto RealtimeEngine = std::make_unique<csp::multiplayer::OfflineRealtimeEngine>(
        SceneDescription, *SystemsManager.GetLogSystem(), *SystemsManager.GetScriptSystem());

    // Ensure callback is called correctly with the correct number of entities.
    bool CallbackCalled = false;

    RealtimeEngine->SetEntityFetchCompleteCallback(
        [&CallbackCalled](uint32_t Count)
        {
            EXPECT_EQ(Count, 1);
            CallbackCalled = true;
        });

    auto [EnterSpaceResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, SceneData.Space.Id, RealtimeEngine.get());
    EXPECT_EQ(EnterSpaceResult.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_TRUE(CallbackCalled);

    if (RealtimeEngine->GetAllEntities()->Size() != 1)
    {
        FAIL();
    }

    // Ensure entity has a static model component.
    SpaceEntity* Entity = (*RealtimeEngine->GetAllEntities())[0];

    EXPECT_EQ(Entity->GetName(), "Entity");
    EXPECT_EQ(Entity->GetId(), 255223);
    EXPECT_EQ(Entity->GetEntityType(), csp::multiplayer::SpaceEntityType::Object);
    EXPECT_EQ(Entity->GetIsTransferable(), true);
    EXPECT_EQ(Entity->GetIsPersistent(), true);
    EXPECT_EQ(Entity->GetPosition(), csp::common::Vector3::Zero());
    EXPECT_EQ(Entity->GetRotation(), csp::common::Vector4::Identity());
    EXPECT_EQ(Entity->GetScale(), csp::common::Vector3::One());
    EXPECT_FALSE(Entity->GetParentId().HasValue());
    EXPECT_EQ(Entity->GetOwnerId(), 0);

    if (Entity->GetComponents()->Size() != 1)
    {
        FAIL();
    }

    EXPECT_EQ(Entity->GetComponent(0)->GetComponentType(), csp::multiplayer::ComponentType::StaticModel);

    // Cleanup
    AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    LogOut(UserSystem);
}