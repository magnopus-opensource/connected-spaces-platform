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

#include "../SpaceSystemTestHelpers.h"
#include "../UserSystemTestHelpers.h"
#include "Awaitable.h"
#include "CSP/Multiplayer/Components/AnimatedModelSpaceComponent.h"
#include "CSP/Multiplayer/Components/AudioSpaceComponent.h"
#include "CSP/Multiplayer/Components/ButtonSpaceComponent.h"
#include "CSP/Multiplayer/Components/CollisionSpaceComponent.h"
#include "CSP/Multiplayer/Components/CustomSpaceComponent.h"
#include "CSP/Multiplayer/Components/FogSpaceComponent.h"
#include "CSP/Multiplayer/Components/ImageSpaceComponent.h"
#include "CSP/Multiplayer/Components/LightSpaceComponent.h"
#include "CSP/Multiplayer/Components/ReflectionSpaceComponent.h"
#include "CSP/Multiplayer/Components/ScriptSpaceComponent.h"
#include "CSP/Multiplayer/Components/StaticModelSpaceComponent.h"
#include "CSP/Multiplayer/Components/VideoPlayerSpaceComponent.h"
#include "CSP/Multiplayer/Script/EntityScript.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/SystemsManager.h"
#include "TestHelpers.h"

#include "gtest/gtest.h"

using namespace csp::multiplayer;

namespace
{

bool RequestPredicate(const csp::systems::ResultBase& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; }
} // namespace

CSP_PUBLIC_TEST(CSPEngine, ComponentTests, ApplicationOriginTest)
{
    SpaceEntity* mySpaceEntity = new SpaceEntity();
    CustomSpaceComponent myCustomComponent(csp::systems::SystemsManager::Get().GetLogSystem(), mySpaceEntity);

    csp::common::String testApplicationOrigin = "UE::CSP";
    myCustomComponent.SetApplicationOrigin(testApplicationOrigin);

    EXPECT_TRUE(myCustomComponent.GetApplicationOrigin() == testApplicationOrigin);
}

CSP_PUBLIC_TEST(CSPEngine, ComponentTests, GetRemovedPropertyAssertionTest)
{
    SpaceEntity* mySpaceEntity = new SpaceEntity();
    CustomSpaceComponent myCustomComponent(csp::systems::SystemsManager::Get().GetLogSystem(), mySpaceEntity);

    const csp::common::String propertyKey("MyPropertyKey");
    const csp::common::String myString("MyTestString");
    csp::common::ReplicatedValue testStringValue(myString);

    myCustomComponent.SetCustomProperty(propertyKey, testStringValue);
    myCustomComponent.RemoveCustomProperty(propertyKey);

    EXPECT_FALSE(myCustomComponent.HasCustomProperty(propertyKey));
}

CSP_PUBLIC_TEST(CSPEngine, ComponentTests, ReplacePropertyWithNewTypeTest)
{
    SpaceEntity* mySpaceEntity = new SpaceEntity();
    CustomSpaceComponent myCustomComponent(csp::systems::SystemsManager::Get().GetLogSystem(), mySpaceEntity);

    const csp::common::String propertyKey("MyPropertyKey");
    const csp::common::String myString("MyTestString");
    csp::common::ReplicatedValue testStringValue(myString);

    const int64_t myInt = 42;
    csp::common::ReplicatedValue testIntValue(myInt);

    myCustomComponent.SetCustomProperty(propertyKey, testStringValue);
    myCustomComponent.RemoveCustomProperty(propertyKey);
    myCustomComponent.SetCustomProperty(propertyKey, testIntValue);

    EXPECT_TRUE(myCustomComponent.GetCustomProperty(propertyKey) == testIntValue);
}

CSP_PUBLIC_TEST(CSPEngine, ComponentTests, GetKeysPropertyAssertionTest)
{
    SpaceEntity* mySpaceEntity = new SpaceEntity();
    CustomSpaceComponent myCustomComponent(csp::systems::SystemsManager::Get().GetLogSystem(), mySpaceEntity);

    EXPECT_EQ(myCustomComponent.GetCustomPropertyKeys().Size(), 0);

    const csp::common::String propertyKey1("MyPropertyKey1");
    const csp::common::String myString1("MyTestString1");
    const csp::common::String propertyKey2("MyPropertyKey2");
    const csp::common::String myString2("MyTestString2");
    csp::common::ReplicatedValue testStringValue1(myString1);
    csp::common::ReplicatedValue testStringValue2(myString2);

    myCustomComponent.SetCustomProperty(propertyKey1, testStringValue1);

    EXPECT_EQ(myCustomComponent.GetCustomPropertyKeys().Size(), 1);

    myCustomComponent.SetCustomProperty(propertyKey1, testStringValue1);

    EXPECT_EQ(myCustomComponent.GetCustomPropertyKeys().Size(), 1);
    EXPECT_TRUE(myCustomComponent.GetCustomPropertyKeys().Contains(propertyKey1));

    myCustomComponent.RemoveCustomProperty(propertyKey1);

    EXPECT_EQ(myCustomComponent.GetCustomPropertyKeys().Size(), 0);
    EXPECT_FALSE(myCustomComponent.HasCustomProperty(propertyKey1));

    myCustomComponent.SetCustomProperty(propertyKey1, testStringValue1);
    myCustomComponent.SetCustomProperty(propertyKey2, testStringValue2);

    EXPECT_EQ(myCustomComponent.GetCustomPropertyKeys().Size(), 2);

    myCustomComponent.SetCustomProperty(propertyKey1, testStringValue1);
    myCustomComponent.SetCustomProperty(propertyKey2, testStringValue2);

    EXPECT_EQ(myCustomComponent.GetCustomPropertyKeys().Size(), 2);
    EXPECT_TRUE(myCustomComponent.GetCustomPropertyKeys().Contains(propertyKey1));
    EXPECT_TRUE(myCustomComponent.GetCustomPropertyKeys().Contains(propertyKey2));

    myCustomComponent.RemoveCustomProperty(propertyKey1);

    EXPECT_EQ(myCustomComponent.GetCustomPropertyKeys().Size(), 1);
    EXPECT_TRUE(myCustomComponent.GetCustomPropertyKeys().Contains(propertyKey2));
}

CSP_PUBLIC_TEST(CSPEngine, ComponentTests, ARVisibleTest)
{
    SpaceEntity* mySpaceEntity = new SpaceEntity();

    csp::common::LogSystem* logSystem = csp::systems::SystemsManager::Get().GetLogSystem();

    std::vector<ComponentBase*> components { new AnimatedModelSpaceComponent(logSystem, mySpaceEntity),
        new ButtonSpaceComponent(logSystem, mySpaceEntity), new ImageSpaceComponent(logSystem, mySpaceEntity),
        new LightSpaceComponent(logSystem, mySpaceEntity), new StaticModelSpaceComponent(logSystem, mySpaceEntity),
        new VideoPlayerSpaceComponent(logSystem, mySpaceEntity) };

    for (auto component : components)
    {
        auto* visibleComponent = dynamic_cast<IVisibleComponent*>(component);

        EXPECT_TRUE(visibleComponent->GetIsARVisible());
    }

    for (auto component : components)
    {
        auto* visibleComponent = dynamic_cast<IVisibleComponent*>(component);
        visibleComponent->SetIsARVisible(false);

        EXPECT_FALSE(visibleComponent->GetIsARVisible());

        delete component;
    }
}

CSP_PUBLIC_TEST(CSPEngine, ComponentTests, VirtualVisibleTest)
{
    SpaceEntity* mySpaceEntity = new SpaceEntity();

    csp::common::LogSystem* logSystem = csp::systems::SystemsManager::Get().GetLogSystem();

    std::vector<ComponentBase*> components { new AnimatedModelSpaceComponent(logSystem, mySpaceEntity),
        new ButtonSpaceComponent(logSystem, mySpaceEntity), new ImageSpaceComponent(logSystem, mySpaceEntity),
        new LightSpaceComponent(logSystem, mySpaceEntity), new StaticModelSpaceComponent(logSystem, mySpaceEntity),
        new VideoPlayerSpaceComponent(logSystem, mySpaceEntity) };

    for (auto component : components)
    {
        auto* visibleComponent = dynamic_cast<IVisibleComponent*>(component);

        EXPECT_TRUE(visibleComponent->GetIsVirtualVisible());
    }

    for (auto component : components)
    {
        auto* visibleComponent = dynamic_cast<IVisibleComponent*>(component);
        visibleComponent->SetIsVirtualVisible(false);

        EXPECT_FALSE(visibleComponent->GetIsVirtualVisible());

        delete component;
    }
}

CSP_PUBLIC_TEST(CSPEngine, ComponentTests, ThirdPartyComponentRefTest)
{
    SpaceEntity* mySpaceEntity = new SpaceEntity();

    csp::common::LogSystem* logSystem = csp::systems::SystemsManager::Get().GetLogSystem();

    std::vector<ComponentBase*> components { new AnimatedModelSpaceComponent(logSystem, mySpaceEntity),
        new AudioSpaceComponent(logSystem, mySpaceEntity), new CollisionSpaceComponent(logSystem, mySpaceEntity),
        new FogSpaceComponent(logSystem, mySpaceEntity), new LightSpaceComponent(logSystem, mySpaceEntity),
        new ReflectionSpaceComponent(logSystem, mySpaceEntity), new StaticModelSpaceComponent(logSystem, mySpaceEntity) };

    for (auto component : components)
    {
        auto* thirdPartyComponentRef = dynamic_cast<IThirdPartyComponentRef*>(component);

        EXPECT_EQ(thirdPartyComponentRef->GetThirdPartyComponentRef(), "");
    }

    for (auto component : components)
    {
        auto* thirdPartyComponentRef = dynamic_cast<IThirdPartyComponentRef*>(component);
        thirdPartyComponentRef->SetThirdPartyComponentRef("ComponentRef");

        EXPECT_EQ(thirdPartyComponentRef->GetThirdPartyComponentRef(), "ComponentRef");

        delete component;
    }
}

CSP_PUBLIC_TEST(CSPEngine, ComponentTests, ComponentBaseTest)
{
    SpaceEntity* mySpaceEntity = new SpaceEntity();
    CustomSpaceComponent myCustomComponent(csp::systems::SystemsManager::Get().GetLogSystem(), mySpaceEntity);

    EXPECT_EQ(myCustomComponent.GetComponentName(), "");

    myCustomComponent.SetComponentName("ComponentName");

    EXPECT_EQ(myCustomComponent.GetComponentName(), "ComponentName");
}

CSP_PUBLIC_TEST(CSPEngine, ComponentTests, ComponentBaseScriptTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateSpace(
        spaceSystem, uniqueSpaceName, testSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    realtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    // Create object to represent the custom
    csp::common::String objectName = "Object 1";
    SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

    // Create custom component
    auto* customComponent = (CustomSpaceComponent*)CreatedObject->AddComponent(ComponentType::Custom);
    // Create script component
    auto* scriptComponent = (ScriptSpaceComponent*)CreatedObject->AddComponent(ComponentType::ScriptData);
    CreatedObject->QueueUpdate();
    realtimeEngine->ProcessPendingEntityOperations();

    // Setup script
    std::string customScriptText = R"xx(
	
		var custom = ThisEntity.getCustomComponents()[0];
		custom.name = "ComponentName";
    )xx";

    EXPECT_EQ(customComponent->GetComponentName(), "");

    scriptComponent->SetScriptSource(customScriptText.c_str());
    CreatedObject->GetScript().Invoke();
    const bool scriptHasErrors = CreatedObject->GetScript().HasError();
    EXPECT_FALSE(scriptHasErrors);
    realtimeEngine->ProcessPendingEntityOperations();

    // Ensure values are set correctly
    EXPECT_EQ(customComponent->GetComponentName(), "ComponentName");

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}
