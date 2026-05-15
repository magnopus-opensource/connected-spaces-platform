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
#include "CSP/Multiplayer/ComponentSchema.h"
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
bool RequestPredicate(const csp::systems::ResultBase& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; }
}

/*
    Checks OfflineRealtimeEngine is returning the correct enum for GetRealtimeEngineType
*/
CSP_PUBLIC_TEST(CSPEngine, OfflineRealtimeEngineTests, GetRealtimeEngineType)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();

    CSPSceneDescription sceneDescription;
    OfflineRealtimeEngine engine { sceneDescription, *systemsManager.GetLogSystem(), *systemsManager.GetScriptSystem() };

    EXPECT_EQ(engine.GetRealtimeEngineType(), csp::common::RealtimeEngineType::Offline);
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
    auto& systemsManager = csp::systems::SystemsManager::Get();

    CSPSceneDescription sceneDescription;
    OfflineRealtimeEngine engine { sceneDescription, *systemsManager.GetLogSystem(), *systemsManager.GetScriptSystem() };

    // Create test properties for our avatar.
    const common::String testName = "TestName";
    const SpaceTransform transform { common::Vector3::One(), common::Vector4::One(), common::Vector3::Zero() };
    static constexpr const bool isVisible = false;
    static constexpr const auto state = AvatarState::Running;
    const common::String avatarId = "Id";
    static constexpr const auto playMode = AvatarPlayMode::Creator;

    SpaceEntity* createdEntity = nullptr;

    engine.CreateAvatar(testName, nullptr, transform, isVisible, state, avatarId, playMode, LocomotionModel::Grounded,
        [&createdEntity](SpaceEntity* newEntity) { createdEntity = newEntity; });

    // Callback should be called before the function ends in offline mode, so this should be set.
    if (createdEntity == nullptr)
    {
        FAIL();
    }

    // Ensure created entity is populated correctly.
    EXPECT_EQ(createdEntity->GetName(), testName);
    EXPECT_EQ(createdEntity->GetTransform(), transform);
    EXPECT_EQ(createdEntity->GetParent(), nullptr);

    // Now check our AvatarComponent which should have been created by CreateAvatar.
    if (createdEntity->GetComponents()->Size() != 1)
    {
        FAIL();
    }

    const auto* avatarComponent = static_cast<AvatarSpaceComponent*>((*createdEntity->GetComponents())[0]);

    // Ensure created avatar component is populated correctly.
    EXPECT_EQ(avatarComponent->GetIsVisible(), isVisible);
    EXPECT_EQ(avatarComponent->GetState(), AvatarState::Running);
    EXPECT_EQ(avatarComponent->GetAvatarId(), avatarId);
    EXPECT_EQ(avatarComponent->GetAvatarPlayMode(), playMode);

    // Check that our avatar is registered as an entity in the engine.
    if (engine.GetNumEntities() != 1)
    {
        FAIL();
    }

    EXPECT_EQ(engine.GetEntityByIndex(0)->GetId(), createdEntity->GetId());

    // Check our avatar is registered as an avatar in the engine.
    if (engine.GetNumAvatars() != 1)
    {
        FAIL();
    }

    EXPECT_EQ(engine.GetAvatarByIndex(0)->GetId(), createdEntity->GetId());

    // Check our avatar is NOT registered as an object in the engine.
    EXPECT_EQ(engine.GetNumObjects(), 0);
}

/*
    This tests the behaviour of OfflineRealtimeEngine::CreateEntity.
    This is very similar to the CreateAvatar test, except an avatar component isn't create by the function.
*/
CSP_PUBLIC_TEST(CSPEngine, OfflineRealtimeEngineTests, CreateEntity)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();

    CSPSceneDescription sceneDescription;
    OfflineRealtimeEngine engine { sceneDescription, *systemsManager.GetLogSystem(), *systemsManager.GetScriptSystem() };

    // Create test properties for our entity.
    const common::String testName = "TestName";
    const SpaceTransform transform { common::Vector3::One(), common::Vector4::One(), common::Vector3::Zero() };
    static constexpr const bool isVisible = false;
    static constexpr const auto state = AvatarState::Running;
    const common::String avatarId = "Id";
    static constexpr const auto playMode = AvatarPlayMode::Creator;

    SpaceEntity* createdEntity = nullptr;

    engine.CreateEntity(testName, transform, nullptr, [&createdEntity](SpaceEntity* newEntity) { createdEntity = newEntity; });

    // Callback should be called before the function ends in offline mode, so this should be set.
    if (createdEntity == nullptr)
    {
        FAIL();
    }

    // Ensure created entity is populated correctly.
    EXPECT_EQ(createdEntity->GetName(), testName);
    EXPECT_EQ(createdEntity->GetTransform(), transform);
    EXPECT_EQ(createdEntity->GetParent(), nullptr);

    // Check that our entity is registered as an entity in the engine.
    if (engine.GetNumEntities() != 1)
    {
        FAIL();
    }

    EXPECT_EQ(engine.GetEntityByIndex(0)->GetId(), createdEntity->GetId());

    // Check our entity is NOT registered as an avatar in the engine.
    EXPECT_EQ(engine.GetNumAvatars(), 0);

    // Check our entity is also registered as an object in the engine.
    EXPECT_EQ(engine.GetNumObjects(), 1);
}

/*
    This tests the behaviour of OfflineRealtimeEngine::DestroyEntity
    by verifying it is removed from the engine when called.
    It also verifies that the SetDestroyCallback is called correctly.
*/
CSP_PUBLIC_TEST(CSPEngine, OfflineRealtimeEngineTests, DestroyEntity)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();

    CSPSceneDescription sceneDescription;
    OfflineRealtimeEngine engine { sceneDescription, *systemsManager.GetLogSystem(), *systemsManager.GetScriptSystem() };

    engine.CreateEntity("", SpaceTransform {}, nullptr, [](SpaceEntity*) {});

    if (engine.GetNumEntities() != 1)
    {
        FAIL();
    }

    SpaceEntity* createdEntity = engine.GetEntityByIndex(0);

    bool destroyCalled = false;

    engine.DestroyEntity(createdEntity,
        [&destroyCalled](bool destroyed)
        {
            EXPECT_TRUE(destroyed);
            destroyCalled = true;
        });

    EXPECT_TRUE(destroyCalled);
    EXPECT_EQ(engine.GetNumEntities(), 0);
}

/*
    This tests the behaviour of OfflineRealtimeEngine::DestroyEntity for Avatars.
    This is similar to DestroyEntity test, except it also verifies the avatar is removed
    from the avatar container.
*/
CSP_PUBLIC_TEST(CSPEngine, OfflineRealtimeEngineTests, DestroyAvatar)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();

    CSPSceneDescription sceneDescription;
    OfflineRealtimeEngine engine { sceneDescription, *systemsManager.GetLogSystem(), *systemsManager.GetScriptSystem() };

    engine.CreateAvatar(
        "", nullptr, SpaceTransform {}, false, AvatarState::Idle, "", AvatarPlayMode::Default, LocomotionModel::Grounded, [](SpaceEntity*) { });

    if (engine.GetNumEntities() != 1)
    {
        FAIL();
    }

    SpaceEntity* createdEntity = engine.GetEntityByIndex(0);

    bool destroyCalled = false;

    engine.DestroyEntity(createdEntity,
        [&destroyCalled](bool destroyed)
        {
            EXPECT_TRUE(destroyed);
            destroyCalled = true;
        });

    EXPECT_TRUE(destroyCalled);
    EXPECT_EQ(engine.GetNumEntities(), 0);
    EXPECT_EQ(engine.GetNumAvatars(), 0);
}

/*
    This tests the behaviour AddEntityToSelectedEntities and RemoveEntityFromSelectedEntities
    by checking if the provided entity gets added and removed from the internal container.
*/
CSP_PUBLIC_TEST(CSPEngine, OfflineRealtimeEngineTests, SelectEntity)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();

    CSPSceneDescription sceneDescription;
    OfflineRealtimeEngine engine { sceneDescription, *systemsManager.GetLogSystem(), *systemsManager.GetScriptSystem() };

    SpaceEntity* createdEntity = nullptr;
    SpaceEntity* createdEntity2 = nullptr;

    engine.CreateEntity("", SpaceTransform {}, nullptr, [&createdEntity](SpaceEntity* newEntity) { createdEntity = newEntity; });
    engine.CreateEntity("", SpaceTransform {}, nullptr, [&createdEntity2](SpaceEntity* newEntity) { createdEntity2 = newEntity; });

    if (createdEntity == nullptr)
    {
        FAIL();
    }

    engine.AddEntityToSelectedEntities(createdEntity);

    if (engine.m_selectedEntities.Size() != 1)
    {
        FAIL();
    }

    EXPECT_EQ(engine.m_selectedEntities[0]->GetId(), createdEntity->GetId());

    engine.AddEntityToSelectedEntities(createdEntity2);

    if (engine.m_selectedEntities.Size() != 2)
    {
        FAIL();
    }

    EXPECT_EQ(engine.m_selectedEntities[1]->GetId(), createdEntity2->GetId());

    // Remove the second entity
    engine.RemoveEntityFromSelectedEntities(createdEntity2);

    EXPECT_EQ(engine.m_selectedEntities.Size(), 1);

    if (engine.m_selectedEntities.Size() != 1)
    {
        FAIL();
    }

    EXPECT_EQ(engine.m_selectedEntities[0]->GetId(), createdEntity->GetId());

    // Remove the first entity
    engine.RemoveEntityFromSelectedEntities(createdEntity);

    EXPECT_EQ(engine.m_selectedEntities.Size(), 0);
}

/*
    This tests the behaviour FindSpaceEntity.
    by creating 2 different entities and one avatar, checking they can all be retrieved.
    This alo tests that Avatars are registered as Enities.
*/
CSP_PUBLIC_TEST(CSPEngine, OfflineRealtimeEngineTests, FindSpaceEntity)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();

    CSPSceneDescription sceneDescription;
    OfflineRealtimeEngine engine { sceneDescription, *systemsManager.GetLogSystem(), *systemsManager.GetScriptSystem() };

    const csp::common::String entityName1 = "Entity1";
    const csp::common::String entityName2 = "Entity2";
    const csp::common::String entityName3 = "Entity3";

    SpaceEntity* entity1 = nullptr;
    SpaceEntity* entity2 = nullptr;
    SpaceEntity* entity3 = nullptr;

    engine.CreateEntity(entityName1, SpaceTransform {}, nullptr, [&entity1](SpaceEntity* newEntity) { entity1 = newEntity; });
    engine.CreateAvatar(entityName2, nullptr, SpaceTransform {}, false, AvatarState::Idle, "", AvatarPlayMode::Default, LocomotionModel::Grounded,
        [&entity2](SpaceEntity* newEntity) { entity2 = newEntity; });
    engine.CreateEntity(entityName3, SpaceTransform {}, nullptr, [&entity3](SpaceEntity* newEntity) { entity3 = newEntity; });

    SpaceEntity* foundEntity1 = engine.FindSpaceEntity(entityName1);
    if (foundEntity1 == nullptr)
    {
        FAIL();
    }

    EXPECT_EQ(foundEntity1->GetId(), entity1->GetId());

    SpaceEntity* foundEntity2 = engine.FindSpaceEntity(entityName2);
    if (foundEntity2 == nullptr)
    {
        FAIL();
    }

    EXPECT_EQ(foundEntity2->GetId(), entity2->GetId());

    SpaceEntity* foundEntity3 = engine.FindSpaceEntity(entityName3);
    if (foundEntity3 == nullptr)
    {
        FAIL();
    }

    EXPECT_EQ(foundEntity3->GetId(), entity3->GetId());
}

/*
    This tests the behaviour FindSpaceEntityById
    by creating 2 different entities and 1 avatar and checking they can all be retrieved.
*/
CSP_PUBLIC_TEST(CSPEngine, OfflineRealtimeEngineTests, FindSpaceEntityById)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();

    CSPSceneDescription sceneDescription;
    OfflineRealtimeEngine engine { sceneDescription, *systemsManager.GetLogSystem(), *systemsManager.GetScriptSystem() };

    const csp::common::String entityName1 = "Entity1";
    const csp::common::String entityName2 = "Entity2";
    const csp::common::String entityName3 = "Entity3";

    SpaceEntity* entity1 = nullptr;
    SpaceEntity* entity2 = nullptr;
    SpaceEntity* entity3 = nullptr;

    engine.CreateEntity(entityName1, SpaceTransform {}, nullptr, [&entity1](SpaceEntity* newEntity) { entity1 = newEntity; });
    engine.CreateAvatar(entityName2, nullptr, SpaceTransform {}, false, AvatarState::Idle, "", AvatarPlayMode::Default, LocomotionModel::Grounded,
        [&entity2](SpaceEntity* newEntity) { entity2 = newEntity; });
    engine.CreateEntity(entityName3, SpaceTransform {}, nullptr, [&entity3](SpaceEntity* newEntity) { entity3 = newEntity; });

    SpaceEntity* foundEntity1 = engine.FindSpaceEntityById(entity1->GetId());
    if (foundEntity1 == nullptr)
    {
        FAIL();
    }

    EXPECT_EQ(foundEntity1->GetId(), entity1->GetId());

    SpaceEntity* foundEntity2 = engine.FindSpaceEntityById(entity2->GetId());
    if (foundEntity2 == nullptr)
    {
        FAIL();
    }

    EXPECT_EQ(foundEntity2->GetId(), entity2->GetId());

    SpaceEntity* foundEntity3 = engine.FindSpaceEntityById(entity3->GetId());
    if (foundEntity3 == nullptr)
    {
        FAIL();
    }

    EXPECT_EQ(foundEntity3->GetId(), entity3->GetId());
}

/*
    This tests the behaviour FindSpaceAvatar
    by creating 2 avatars and one entity and checking they can all be retrieved.
*/
CSP_PUBLIC_TEST(CSPEngine, OfflineRealtimeEngineTests, FindSpaceAvatar)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();

    CSPSceneDescription sceneDescription;
    OfflineRealtimeEngine engine { sceneDescription, *systemsManager.GetLogSystem(), *systemsManager.GetScriptSystem() };

    const csp::common::String avatarName1 = "Avatar1";
    const csp::common::String entityName1 = "Avatar2";
    const csp::common::String avatarName3 = "Avatar3";

    SpaceEntity* avatar1 = nullptr;
    SpaceEntity* entity1 = nullptr;
    SpaceEntity* avatar3 = nullptr;

    engine.CreateAvatar(avatarName1, nullptr, SpaceTransform {}, false, AvatarState::Idle, "", AvatarPlayMode::Default, LocomotionModel::Grounded,
        [&avatar1](SpaceEntity* newEntity) { avatar1 = newEntity; });
    engine.CreateEntity(entityName1, SpaceTransform {}, nullptr, [&entity1](SpaceEntity* newEntity) { entity1 = newEntity; });
    engine.CreateAvatar(avatarName3, nullptr, SpaceTransform {}, false, AvatarState::Idle, "", AvatarPlayMode::Default, LocomotionModel::Grounded,
        [&avatar3](SpaceEntity* newEntity) { avatar3 = newEntity; });

    SpaceEntity* foundAvatar1 = engine.FindSpaceEntity(avatarName1);
    if (foundAvatar1 == nullptr)
    {
        FAIL();
    }

    EXPECT_EQ(foundAvatar1->GetId(), avatar1->GetId());

    // Avatar should still be found using FindSpaceEntity
    SpaceEntity* foundEntity1 = engine.FindSpaceEntity(entityName1);
    if (foundEntity1 == nullptr)
    {
        FAIL();
    }

    EXPECT_EQ(foundEntity1->GetId(), entity1->GetId());

    SpaceEntity* foundAvatar3 = engine.FindSpaceEntity(avatarName3);
    if (foundAvatar3 == nullptr)
    {
        FAIL();
    }

    EXPECT_EQ(foundAvatar3->GetId(), avatar3->GetId());
}

/*
    This tests the behaviour FindSpaceObject by  creating 2 different entities and 1 avatar,
    checking ONLY the entities can be retieved from this function.
*/
CSP_PUBLIC_TEST(CSPEngine, OfflineRealtimeEngineTests, FindSpaceObject)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();

    CSPSceneDescription sceneDescription;
    OfflineRealtimeEngine engine { sceneDescription, *systemsManager.GetLogSystem(), *systemsManager.GetScriptSystem() };

    const csp::common::String entityName1 = "Entity1";
    const csp::common::String entityName2 = "Entity2";
    const csp::common::String entityName3 = "Entity3";

    SpaceEntity* entity1 = nullptr;
    SpaceEntity* entity2 = nullptr;
    SpaceEntity* entity3 = nullptr;

    engine.CreateEntity(entityName1, SpaceTransform {}, nullptr, [&entity1](SpaceEntity* newEntity) { entity1 = newEntity; });
    engine.CreateAvatar(entityName2, nullptr, SpaceTransform {}, false, AvatarState::Idle, "", AvatarPlayMode::Default, LocomotionModel::Grounded,
        [&entity2](SpaceEntity* newEntity) { entity2 = newEntity; });
    engine.CreateEntity(entityName3, SpaceTransform {}, nullptr, [&entity3](SpaceEntity* newEntity) { entity3 = newEntity; });

    SpaceEntity* foundEntity1 = engine.FindSpaceObject(entity1->GetName());
    if (foundEntity1 == nullptr)
    {
        FAIL();
    }

    EXPECT_EQ(foundEntity1->GetName(), entity1->GetName());

    // Our avatar should not be found.
    SpaceEntity* foundEntity2 = engine.FindSpaceObject(entity2->GetName());
    EXPECT_EQ(foundEntity2, nullptr);

    SpaceEntity* foundEntity3 = engine.FindSpaceObject(entity3->GetName());
    if (foundEntity3 == nullptr)
    {
        FAIL();
    }

    EXPECT_EQ(foundEntity3->GetName(), entity3->GetName());
}

/*
    This tests the behaviour GetEntityByIndex
    by creating 2 different entities and 1 avatar and checking they can all be retrieved.
    This also tests the GetNumX functions.
*/
CSP_PUBLIC_TEST(CSPEngine, OfflineRealtimeEngineTests, GetEntityByIndex)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();

    CSPSceneDescription sceneDescription;
    OfflineRealtimeEngine engine { sceneDescription, *systemsManager.GetLogSystem(), *systemsManager.GetScriptSystem() };

    const csp::common::String entityName1 = "Entity1";
    const csp::common::String entityName2 = "Entity2";
    const csp::common::String entityName3 = "Entity3";

    SpaceEntity* entity1 = nullptr;
    SpaceEntity* entity2 = nullptr;
    SpaceEntity* entity3 = nullptr;

    engine.CreateEntity(entityName1, SpaceTransform {}, nullptr, [&entity1](SpaceEntity* newEntity) { entity1 = newEntity; });
    engine.CreateAvatar(entityName2, nullptr, SpaceTransform {}, false, AvatarState::Idle, "", AvatarPlayMode::Default, LocomotionModel::Grounded,
        [&entity2](SpaceEntity* newEntity) { entity2 = newEntity; });
    engine.CreateEntity(entityName3, SpaceTransform {}, nullptr, [&entity3](SpaceEntity* newEntity) { entity3 = newEntity; });

    EXPECT_EQ(engine.GetNumEntities(), 3);
    EXPECT_EQ(engine.GetNumAvatars(), 1);
    EXPECT_EQ(engine.GetNumObjects(), 2);

    SpaceEntity* foundEntity1 = engine.GetEntityByIndex(0);
    if (foundEntity1 == nullptr)
    {
        FAIL();
    }

    EXPECT_EQ(foundEntity1->GetId(), entity1->GetId());

    SpaceEntity* foundEntity2 = engine.GetEntityByIndex(1);
    if (foundEntity2 == nullptr)
    {
        FAIL();
    }

    EXPECT_EQ(foundEntity2->GetId(), entity2->GetId());

    SpaceEntity* foundEntity3 = engine.GetEntityByIndex(2);
    if (foundEntity3 == nullptr)
    {
        FAIL();
    }

    EXPECT_EQ(foundEntity3->GetId(), entity3->GetId());
}

/*
    This tests the behaviour GetAvatarByIndex
    by creating 2 avatars and one entity and checking they can all be retrieved.
    This also tests the GetNumX functions.
*/
CSP_PUBLIC_TEST(CSPEngine, OfflineRealtimeEngineTests, GetAvatarByIndex)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();

    CSPSceneDescription sceneDescription;
    OfflineRealtimeEngine engine { sceneDescription, *systemsManager.GetLogSystem(), *systemsManager.GetScriptSystem() };

    const csp::common::String avatarName1 = "Avatar1";
    const csp::common::String avatarName2 = "Avatar2";
    const csp::common::String avatarName3 = "Avatar3";

    SpaceEntity* avatar1 = nullptr;
    SpaceEntity* avatar2 = nullptr;
    SpaceEntity* avatar3 = nullptr;

    engine.CreateAvatar(avatarName1, nullptr, SpaceTransform {}, false, AvatarState::Idle, "", AvatarPlayMode::Default, LocomotionModel::Grounded,
        [&avatar1](SpaceEntity* newEntity) { avatar1 = newEntity; });
    engine.CreateEntity(avatarName2, SpaceTransform {}, nullptr, [&avatar2](SpaceEntity* newEntity) { avatar2 = newEntity; });
    engine.CreateAvatar(avatarName3, nullptr, SpaceTransform {}, false, AvatarState::Idle, "", AvatarPlayMode::Default, LocomotionModel::Grounded,
        [&avatar3](SpaceEntity* newEntity) { avatar3 = newEntity; });

    EXPECT_EQ(engine.GetNumEntities(), 3);
    EXPECT_EQ(engine.GetNumAvatars(), 2);
    EXPECT_EQ(engine.GetNumObjects(), 1);

    SpaceEntity* foundAvatar1 = engine.GetAvatarByIndex(0);
    if (foundAvatar1 == nullptr)
    {
        FAIL();
    }

    EXPECT_EQ(foundAvatar1->GetId(), avatar1->GetId());

    // The second avatar (the one added third) should be found in the second element.
    SpaceEntity* foundAvatar2 = engine.GetAvatarByIndex(1);
    if (foundAvatar2 == nullptr)
    {
        FAIL();
    }

    EXPECT_EQ(foundAvatar2->GetId(), avatar3->GetId());
}

/*
    This tests the behaviour GetObjectByIndex
    by creating 2 different entities and 1 avatar and checking they can all be retrieved.
    This also tests the GetNumX functions.
*/
CSP_PUBLIC_TEST(CSPEngine, OfflineRealtimeEngineTests, GetObjectByIndex)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();

    CSPSceneDescription sceneDescription;
    OfflineRealtimeEngine engine { sceneDescription, *systemsManager.GetLogSystem(), *systemsManager.GetScriptSystem() };

    const csp::common::String entityName1 = "Entity1";
    const csp::common::String entityName2 = "Entity2";
    const csp::common::String entityName3 = "Entity3";

    SpaceEntity* entity1 = nullptr;
    SpaceEntity* entity2 = nullptr;
    SpaceEntity* entity3 = nullptr;

    engine.CreateEntity(entityName1, SpaceTransform {}, nullptr, [&entity1](SpaceEntity* newEntity) { entity1 = newEntity; });
    engine.CreateAvatar(entityName2, nullptr, SpaceTransform {}, false, AvatarState::Idle, "", AvatarPlayMode::Default, LocomotionModel::Grounded,
        [&entity2](SpaceEntity* newEntity) { entity2 = newEntity; });
    engine.CreateEntity(entityName3, SpaceTransform {}, nullptr, [&entity3](SpaceEntity* newEntity) { entity3 = newEntity; });

    EXPECT_EQ(engine.GetNumEntities(), 3);
    EXPECT_EQ(engine.GetNumAvatars(), 1);
    EXPECT_EQ(engine.GetNumObjects(), 2);

    SpaceEntity* foundEntity1 = engine.GetObjectByIndex(0);
    if (foundEntity1 == nullptr)
    {
        FAIL();
    }

    EXPECT_EQ(foundEntity1->GetId(), entity1->GetId());

    // The second entity (the one added third) should be found in the second element.
    SpaceEntity* foundEntity2 = engine.GetObjectByIndex(1);
    if (foundEntity2 == nullptr)
    {
        FAIL();
    }

    EXPECT_EQ(foundEntity2->GetId(), entity3->GetId());
}

/*
    Test that when we load a scene description with parents, the parents are hooked up automatically
*/
CSP_PUBLIC_TEST(CSPEngine, OfflineRealtimeEngineTests, ParentLoadTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();

    auto filePath = std::filesystem::absolute("assets/checkpoint-parents.json");
    std::ifstream stream { filePath.u8string().c_str() };

    if (!stream)
    {
        FAIL();
    }

    std::stringstream sStream;
    sStream << stream.rdbuf();

    std::string json = sStream.str();

    CSPSceneDescription sceneDescription { csp::common::List<csp::common::String> { json.c_str() } };
    OfflineRealtimeEngine engine { sceneDescription, *systemsManager.GetLogSystem(), *systemsManager.GetScriptSystem() };

    const csp::common::List<csp::multiplayer::SpaceEntity*>* allEntities = engine.GetAllEntities();

    // Should be 3 entities
    ASSERT_EQ(allEntities->Size(), 3);
    const csp::multiplayer::SpaceEntity* entity1 = (*allEntities)[0]; // id: 1510, parent: 1511
    const csp::multiplayer::SpaceEntity* entity2 = (*allEntities)[1]; // id: 1511, parent: 1512
    const csp::multiplayer::SpaceEntity* entity3 = (*allEntities)[2]; // id: 1512, parent: empty

    ASSERT_EQ(entity1->GetId(), 1510);
    ASSERT_EQ((*entity1->GetParentId()), 1511);
    ASSERT_EQ(entity1->GetParentEntity(), entity2);

    ASSERT_EQ(entity2->GetId(), 1511);
    ASSERT_EQ((*entity2->GetParentId()), 1512);
    ASSERT_EQ(entity2->GetParentEntity(), entity3);

    ASSERT_EQ(entity3->GetId(), 1512);
    ASSERT_EQ(entity3->GetParentId().HasValue(), false);
    ASSERT_EQ(entity3->GetParentEntity(), nullptr);
}

/*
    This tests the behaviour of correctly setting the ParentId and RootHierarchy entities.
    We first test that the constuctor is correctly setting these properties, and then ensure
    the properties are still correrct after additions and deletions.
*/
CSP_PUBLIC_TEST(CSPEngine, OfflineRealtimeEngineTests, ParentTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();

    CSPSceneDescription sceneDescription;
    OfflineRealtimeEngine engine { sceneDescription, *systemsManager.GetLogSystem(), *systemsManager.GetScriptSystem() };

    const csp::common::String entityName1 = "Entity1";
    const csp::common::String entityName2 = "Entity2";
    const csp::common::String entityName3 = "Entity3";

    SpaceEntity* entity1 = nullptr;
    SpaceEntity* entity2 = nullptr;
    SpaceEntity* entity3 = nullptr;

    engine.CreateEntity(entityName1, SpaceTransform {}, nullptr, [&entity1](SpaceEntity* newEntity) { entity1 = newEntity; });
    engine.CreateEntity(entityName2, SpaceTransform {}, entity1->GetId(), [&entity2](SpaceEntity* newEntity) { entity2 = newEntity; });
    engine.CreateEntity(entityName3, SpaceTransform {}, entity2->GetId(), [&entity3](SpaceEntity* newEntity) { entity3 = newEntity; });

    EXPECT_EQ(entity1->GetParent(), nullptr);
    ASSERT_NE(entity2->GetParent(), nullptr);
    ASSERT_NE(entity3->GetParent(), nullptr);

    EXPECT_EQ(entity2->GetParent(), entity1);
    EXPECT_EQ(entity3->GetParent(), entity2);

    ASSERT_EQ(engine.GetRootHierarchyEntities()->Size(), 1);

    EXPECT_EQ((*engine.GetRootHierarchyEntities())[0]->GetId(), entity1->GetId());

    // Reparent the third entity to be a child of the first
    entity3->SetParentId(entity2->GetId());

    EXPECT_EQ(entity1->GetParent(), nullptr);
    EXPECT_EQ(entity2->GetParent(), entity1);
    EXPECT_EQ(entity3->GetParent(), entity2);

    // Move all entities to the root.
    entity2->RemoveParentEntity();
    entity3->RemoveParentEntity();

    // Parents should all be null.
    EXPECT_EQ(entity1->GetParent(), nullptr);
    EXPECT_EQ(entity2->GetParent(), nullptr);
    EXPECT_EQ(entity3->GetParent(), nullptr);

    // All entities should be at the root.
    EXPECT_EQ(engine.GetRootHierarchyEntities()->Size(), 3);

    // Ensure Root hierarchy is updated if entity is moved from the root.
    entity3->SetParentId(entity1->GetId());

    EXPECT_EQ(entity1->GetParent(), nullptr);
    EXPECT_EQ(entity2->GetParent(), nullptr);
    EXPECT_EQ(entity3->GetParent(), entity1);

    EXPECT_EQ(engine.GetRootHierarchyEntities()->Size(), 2);
}

/*
    This tests the behaviour of OfflineRealtimeEngine::MarkEntityForUpdate
    by verifying an entity update is queued when ProcessPendingEntityOperations is called
*/
CSP_PUBLIC_TEST(CSPEngine, OfflineRealtimeEngineTests, MarkEntityForUpdate)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();

    CSPSceneDescription sceneDescription;
    OfflineRealtimeEngine engine { sceneDescription, *systemsManager.GetLogSystem(), *systemsManager.GetScriptSystem() };

    const csp::common::String entityName = "Entity1";
    SpaceEntity* entity = nullptr;

    engine.CreateEntity(entityName, SpaceTransform {}, nullptr, [&entity](SpaceEntity* newEntity) { entity = newEntity; });

    const csp::common::String newEntityName = "NewEntity1";

    entity->SetName(newEntityName);

    EXPECT_EQ(entity->GetName(), newEntityName);
}

/*
    This is a basic integration test, showing that an empty CSPSceneDescription is correctly processed in the full offline flow.
*/
CSP_PUBLIC_TEST(CSPEngine, OfflineRealtimeEngineTests, EmptySceneDescriptionTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId, false);

    // Get checkpoint file
    auto filePath = std::filesystem::absolute("assets/checkpoint-empty.json");

    std::ifstream stream { filePath.u8string().c_str() };

    if (!stream)
    {
        FAIL();
    }

    std::stringstream sStream;
    sStream << stream.rdbuf();

    std::string json = sStream.str();

    systems::CSPSceneData sceneData { csp::common::List<csp::common::String> { json.c_str() } };
    CSPSceneDescription sceneDescription { csp::common::List<csp::common::String> { json.c_str() } };

    // Enter space from scene description
    auto realtimeEngine = std::make_unique<csp::multiplayer::OfflineRealtimeEngine>(
        sceneDescription, *systemsManager.GetLogSystem(), *systemsManager.GetScriptSystem());

    // Ensure callback is called correctly with the correct number of entities.
    bool callbackCalled = false;

    realtimeEngine->SetEntityFetchCompleteCallback(
        [&callbackCalled](uint32_t count)
        {
            EXPECT_EQ(count, 0);
            callbackCalled = true;
        });

    auto [EnterSpaceResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, sceneData.Space.Id, realtimeEngine.get());
    EXPECT_EQ(EnterSpaceResult.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_TRUE(callbackCalled);

    EXPECT_EQ(realtimeEngine->GetAllEntities()->Size(), 0);

    // Cleanup
    AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    LogOut(userSystem);
}

/*
    This is a basic integration test, showing that a basic CSPSceneDescription with one entity is correctly processed in the full offline flow.
*/
CSP_PUBLIC_TEST(CSPEngine, OfflineRealtimeEngineTests, BasicSceneDescriptionTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId, false);

    // Get checkpoint file
    auto filePath = std::filesystem::absolute("assets/checkpoint-basic.json");

    std::ifstream stream { filePath.u8string().c_str() };

    if (!stream)
    {
        FAIL();
    }

    std::stringstream sStream;
    sStream << stream.rdbuf();

    std::string json = sStream.str();

    // Enter space from scene description{
    systems::CSPSceneData sceneData { csp::common::List<csp::common::String> { json.c_str() } };
    CSPSceneDescription sceneDescription { csp::common::List<csp::common::String> { json.c_str() } };

    auto realtimeEngine = std::make_unique<csp::multiplayer::OfflineRealtimeEngine>(
        sceneDescription, *systemsManager.GetLogSystem(), *systemsManager.GetScriptSystem());

    // Ensure callback is called correctly with the correct number of entities.
    bool callbackCalled = false;

    realtimeEngine->SetEntityFetchCompleteCallback(
        [&callbackCalled](uint32_t count)
        {
            EXPECT_EQ(count, 1);
            callbackCalled = true;
        });

    auto [EnterSpaceResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, sceneData.Space.Id, realtimeEngine.get());
    EXPECT_EQ(EnterSpaceResult.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_TRUE(callbackCalled);

    if (realtimeEngine->GetAllEntities()->Size() != 1)
    {
        FAIL();
    }

    // Ensure entity has a static model component.
    SpaceEntity* entity = (*realtimeEngine->GetAllEntities())[0];

    EXPECT_EQ(entity->GetName(), "Entity");
    EXPECT_EQ(entity->GetId(), 255223);
    EXPECT_EQ(entity->GetEntityType(), csp::multiplayer::SpaceEntityType::Object);
    EXPECT_EQ(entity->GetIsTransferable(), true);
    EXPECT_EQ(entity->GetIsPersistent(), true);
    EXPECT_EQ(entity->GetPosition(), csp::common::Vector3::Zero());
    EXPECT_EQ(entity->GetRotation(), csp::common::Vector4::Identity());
    EXPECT_EQ(entity->GetScale(), csp::common::Vector3::One());
    EXPECT_FALSE(entity->GetParentId().HasValue());
    EXPECT_EQ(entity->GetOwnerId(), 0);

    if (entity->GetComponents()->Size() != 1)
    {
        FAIL();
    }

    EXPECT_EQ(entity->GetComponent(0)->GetComponentType(), csp::multiplayer::ComponentType::StaticModel);

    // Cleanup
    AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    LogOut(userSystem);
}

/*
    Ensures IsEntityModifiable works under the correct conditions.
    Check OfflineRealtimeEngine::IsEntityModifiable docs for details.
*/
CSP_PUBLIC_TEST(CSPEngine, OfflineRealtimeEngineTests, IsModifiableTest) 
{
    auto& systemsManager = csp::systems::SystemsManager::Get();

    CSPSceneDescription sceneDescription;
    OfflineRealtimeEngine engine { sceneDescription, *systemsManager.GetLogSystem(), *systemsManager.GetScriptSystem() };

    const csp::common::String entityName = "Entity1";

    SpaceEntity* entity = nullptr;
    engine.CreateEntity(entityName, SpaceTransform {}, nullptr, [&entity](SpaceEntity* newEntity) { entity = newEntity; });

    // Entity should be modifiable when first created as it is not locked by default.
    EXPECT_EQ(engine.IsEntityModifiable(entity), ModifiableStatus::Modifiable);

    entity->Lock();

    // Entity should not be modifiable now it is locked.
    EXPECT_EQ(engine.IsEntityModifiable(entity), ModifiableStatus::EntityLocked);

    entity->Unlock();

    // Entity should be modifiable again.
    EXPECT_EQ(engine.IsEntityModifiable(entity), ModifiableStatus::Modifiable);
}

CSP_PUBLIC_TEST(CSPEngine, OfflineRealtimeEngineTests, ConstructWithComponentSchema) 
{
    auto& systemsManager = csp::systems::SystemsManager::Get();

    const auto exampleSchemaId = ComponentSchema::TypeIdType{666};

    const auto components = csp::common::Array<csp::multiplayer::ComponentSchema> {
        {
            exampleSchemaId,
            "Example",
            csp::common::Array<ComponentProperty> {
                {
                    ComponentProperty::KeyType { 42 },
                    "value",
                    "DefaultValue",
                },
            },
        },
    };

    const auto engine = OfflineRealtimeEngine { *systemsManager.GetLogSystem(), *systemsManager.GetScriptSystem(), components };

    EXPECT_TRUE(engine.GetComponentSchemaRegistry()->HasKey(exampleSchemaId));
}
