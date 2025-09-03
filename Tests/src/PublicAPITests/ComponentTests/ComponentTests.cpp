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

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }
} // namespace

CSP_PUBLIC_TEST(CSPEngine, ComponentTests, ApplicationOriginTest)
{
    SpaceEntity* MySpaceEntity = new SpaceEntity();
    CustomSpaceComponent MyCustomComponent(csp::systems::SystemsManager::Get().GetLogSystem(), MySpaceEntity);

    csp::common::String TestApplicationOrigin = "UE::CSP";
    MyCustomComponent.SetApplicationOrigin(TestApplicationOrigin);

    EXPECT_TRUE(MyCustomComponent.GetApplicationOrigin() == TestApplicationOrigin);
}

CSP_PUBLIC_TEST(CSPEngine, ComponentTests, GetRemovedPropertyAssertionTest)
{
    SpaceEntity* MySpaceEntity = new SpaceEntity();
    CustomSpaceComponent MyCustomComponent(csp::systems::SystemsManager::Get().GetLogSystem(), MySpaceEntity);

    const csp::common::String PropertyKey("MyPropertyKey");
    const csp::common::String MyString("MyTestString");
    csp::common::ReplicatedValue TestStringValue(MyString);

    MyCustomComponent.SetCustomProperty(PropertyKey, TestStringValue);
    MyCustomComponent.RemoveCustomProperty(PropertyKey);

    EXPECT_FALSE(MyCustomComponent.HasCustomProperty(PropertyKey));
}

CSP_PUBLIC_TEST(CSPEngine, ComponentTests, ReplacePropertyWithNewTypeTest)
{
    SpaceEntity* MySpaceEntity = new SpaceEntity();
    CustomSpaceComponent MyCustomComponent(csp::systems::SystemsManager::Get().GetLogSystem(), MySpaceEntity);

    const csp::common::String PropertyKey("MyPropertyKey");
    const csp::common::String MyString("MyTestString");
    csp::common::ReplicatedValue TestStringValue(MyString);

    const int64_t MyInt = 42;
    csp::common::ReplicatedValue TestIntValue(MyInt);

    MyCustomComponent.SetCustomProperty(PropertyKey, TestStringValue);
    MyCustomComponent.RemoveCustomProperty(PropertyKey);
    MyCustomComponent.SetCustomProperty(PropertyKey, TestIntValue);

    EXPECT_TRUE(MyCustomComponent.GetCustomProperty(PropertyKey) == TestIntValue);
}

CSP_PUBLIC_TEST(CSPEngine, ComponentTests, GetKeysPropertyAssertionTest)
{
    SpaceEntity* MySpaceEntity = new SpaceEntity();
    CustomSpaceComponent MyCustomComponent(csp::systems::SystemsManager::Get().GetLogSystem(), MySpaceEntity);

    EXPECT_EQ(MyCustomComponent.GetCustomPropertyKeys().Size(), 0);

    const csp::common::String PropertyKey1("MyPropertyKey1");
    const csp::common::String MyString1("MyTestString1");
    const csp::common::String PropertyKey2("MyPropertyKey2");
    const csp::common::String MyString2("MyTestString2");
    csp::common::ReplicatedValue TestStringValue1(MyString1);
    csp::common::ReplicatedValue TestStringValue2(MyString2);

    MyCustomComponent.SetCustomProperty(PropertyKey1, TestStringValue1);

    EXPECT_EQ(MyCustomComponent.GetCustomPropertyKeys().Size(), 1);

    MyCustomComponent.SetCustomProperty(PropertyKey1, TestStringValue1);

    EXPECT_EQ(MyCustomComponent.GetCustomPropertyKeys().Size(), 1);
    EXPECT_TRUE(MyCustomComponent.GetCustomPropertyKeys().Contains(PropertyKey1));

    MyCustomComponent.RemoveCustomProperty(PropertyKey1);

    EXPECT_EQ(MyCustomComponent.GetCustomPropertyKeys().Size(), 0);
    EXPECT_FALSE(MyCustomComponent.HasCustomProperty(PropertyKey1));

    MyCustomComponent.SetCustomProperty(PropertyKey1, TestStringValue1);
    MyCustomComponent.SetCustomProperty(PropertyKey2, TestStringValue2);

    EXPECT_EQ(MyCustomComponent.GetCustomPropertyKeys().Size(), 2);

    MyCustomComponent.SetCustomProperty(PropertyKey1, TestStringValue1);
    MyCustomComponent.SetCustomProperty(PropertyKey2, TestStringValue2);

    EXPECT_EQ(MyCustomComponent.GetCustomPropertyKeys().Size(), 2);
    EXPECT_TRUE(MyCustomComponent.GetCustomPropertyKeys().Contains(PropertyKey1));
    EXPECT_TRUE(MyCustomComponent.GetCustomPropertyKeys().Contains(PropertyKey2));

    MyCustomComponent.RemoveCustomProperty(PropertyKey1);

    EXPECT_EQ(MyCustomComponent.GetCustomPropertyKeys().Size(), 1);
    EXPECT_TRUE(MyCustomComponent.GetCustomPropertyKeys().Contains(PropertyKey2));
}

CSP_PUBLIC_TEST(CSPEngine, ComponentTests, ARVisibleTest)
{
    SpaceEntity* MySpaceEntity = new SpaceEntity();

    csp::common::LogSystem* LogSystem = csp::systems::SystemsManager::Get().GetLogSystem();

    std::vector<ComponentBase*> Components { new AnimatedModelSpaceComponent(LogSystem, MySpaceEntity),
        new ButtonSpaceComponent(LogSystem, MySpaceEntity), new ImageSpaceComponent(LogSystem, MySpaceEntity),
        new LightSpaceComponent(LogSystem, MySpaceEntity), new StaticModelSpaceComponent(LogSystem, MySpaceEntity),
        new VideoPlayerSpaceComponent(LogSystem, MySpaceEntity) };

    for (auto Component : Components)
    {
        auto* VisibleComponent = dynamic_cast<IVisibleComponent*>(Component);

        EXPECT_TRUE(VisibleComponent->GetIsARVisible());
    }

    for (auto Component : Components)
    {
        auto* VisibleComponent = dynamic_cast<IVisibleComponent*>(Component);
        VisibleComponent->SetIsARVisible(false);

        EXPECT_FALSE(VisibleComponent->GetIsARVisible());

        delete Component;
    }
}

CSP_PUBLIC_TEST(CSPEngine, ComponentTests, VirtualVisibleTest)
{
    SpaceEntity* MySpaceEntity = new SpaceEntity();

    csp::common::LogSystem* LogSystem = csp::systems::SystemsManager::Get().GetLogSystem();

    std::vector<ComponentBase*> Components { new AnimatedModelSpaceComponent(LogSystem, MySpaceEntity),
        new ButtonSpaceComponent(LogSystem, MySpaceEntity), new ImageSpaceComponent(LogSystem, MySpaceEntity),
        new LightSpaceComponent(LogSystem, MySpaceEntity), new StaticModelSpaceComponent(LogSystem, MySpaceEntity),
        new VideoPlayerSpaceComponent(LogSystem, MySpaceEntity) };

    for (auto Component : Components)
    {
        auto* VisibleComponent = dynamic_cast<IVisibleComponent*>(Component);

        EXPECT_TRUE(VisibleComponent->GetIsVirtualVisible());
    }

    for (auto Component : Components)
    {
        auto* VisibleComponent = dynamic_cast<IVisibleComponent*>(Component);
        VisibleComponent->SetIsVirtualVisible(false);

        EXPECT_FALSE(VisibleComponent->GetIsVirtualVisible());

        delete Component;
    }
}

CSP_PUBLIC_TEST(CSPEngine, ComponentTests, ThirdPartyComponentRefTest)
{
    SpaceEntity* MySpaceEntity = new SpaceEntity();

    csp::common::LogSystem* LogSystem = csp::systems::SystemsManager::Get().GetLogSystem();

    std::vector<ComponentBase*> Components { new AnimatedModelSpaceComponent(LogSystem, MySpaceEntity),
        new AudioSpaceComponent(LogSystem, MySpaceEntity), new CollisionSpaceComponent(LogSystem, MySpaceEntity),
        new FogSpaceComponent(LogSystem, MySpaceEntity), new LightSpaceComponent(LogSystem, MySpaceEntity),
        new ReflectionSpaceComponent(LogSystem, MySpaceEntity), new StaticModelSpaceComponent(LogSystem, MySpaceEntity) };

    for (auto Component : Components)
    {
        auto* ThirdPartyComponentRef = dynamic_cast<IThirdPartyComponentRef*>(Component);

        EXPECT_EQ(ThirdPartyComponentRef->GetThirdPartyComponentRef(), "");
    }

    for (auto Component : Components)
    {
        auto* ThirdPartyComponentRef = dynamic_cast<IThirdPartyComponentRef*>(Component);
        ThirdPartyComponentRef->SetThirdPartyComponentRef("ComponentRef");

        EXPECT_EQ(ThirdPartyComponentRef->GetThirdPartyComponentRef(), "ComponentRef");

        delete Component;
    }
}

CSP_PUBLIC_TEST(CSPEngine, ComponentTests, ComponentBaseTest)
{
    SpaceEntity* MySpaceEntity = new SpaceEntity();
    CustomSpaceComponent MyCustomComponent(csp::systems::SystemsManager::Get().GetLogSystem(), MySpaceEntity);

    EXPECT_EQ(MyCustomComponent.GetComponentName(), "");

    MyCustomComponent.SetComponentName("ComponentName");

    EXPECT_EQ(MyCustomComponent.GetComponentName(), "ComponentName");
}

CSP_PUBLIC_TEST(CSPEngine, ComponentTests, ComponentBaseScriptTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    RealtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

    // Create object to represent the custom
    csp::common::String ObjectName = "Object 1";
    SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

    // Create custom component
    auto* CustomComponent = (CustomSpaceComponent*)CreatedObject->AddComponent(ComponentType::Custom);
    // Create script component
    auto* ScriptComponent = (ScriptSpaceComponent*)CreatedObject->AddComponent(ComponentType::ScriptData);
    CreatedObject->QueueUpdate();
    RealtimeEngine->ProcessPendingEntityOperations();

    // Setup script
    std::string CustomScriptText = R"xx(
	
		var custom = ThisEntity.getCustomComponents()[0];
		custom.name = "ComponentName";
    )xx";

    EXPECT_EQ(CustomComponent->GetComponentName(), "");

    ScriptComponent->SetScriptSource(CustomScriptText.c_str());
    CreatedObject->GetScript().Invoke();
    const bool ScriptHasErrors = CreatedObject->GetScript().HasError();
    EXPECT_FALSE(ScriptHasErrors);
    RealtimeEngine->ProcessPendingEntityOperations();

    // Ensure values are set correctly
    EXPECT_EQ(CustomComponent->GetComponentName(), "ComponentName");

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
