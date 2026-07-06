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
#include "CSP/Multiplayer/ComponentSchema.h"
#include "CSP/Multiplayer/Components/AIChatbotComponent.h"
#include "CSP/Multiplayer/Components/AnimatedModelSpaceComponent.h"
#include "CSP/Multiplayer/Components/AudioSpaceComponent.h"
#include "CSP/Multiplayer/Components/AvatarSpaceComponent.h"
#include "CSP/Multiplayer/Components/ButtonSpaceComponent.h"
#include "CSP/Multiplayer/Components/CinematicCameraSpaceComponent.h"
#include "CSP/Multiplayer/Components/CollisionSpaceComponent.h"
#include "CSP/Multiplayer/Components/ConversationSpaceComponent.h"
#include "CSP/Multiplayer/Components/CustomSpaceComponent.h"
#include "CSP/Multiplayer/Components/ECommerceSpaceComponent.h"
#include "CSP/Multiplayer/Components/ExternalLinkSpaceComponent.h"
#include "CSP/Multiplayer/Components/FiducialMarkerSpaceComponent.h"
#include "CSP/Multiplayer/Components/FogSpaceComponent.h"
#include "CSP/Multiplayer/Components/GaussianSplatSpaceComponent.h"
#include "CSP/Multiplayer/Components/HotspotSpaceComponent.h"
#include "CSP/Multiplayer/Components/ImageSpaceComponent.h"
#include "CSP/Multiplayer/Components/LightSpaceComponent.h"
#include "CSP/Multiplayer/Components/PortalSpaceComponent.h"
#include "CSP/Multiplayer/Components/ReflectionSpaceComponent.h"
#include "CSP/Multiplayer/Components/ScreenSharingSpaceComponent.h"
#include "CSP/Multiplayer/Components/ScriptSpaceComponent.h"
#include "CSP/Multiplayer/Components/SplineSpaceComponent.h"
#include "CSP/Multiplayer/Components/StaticModelSpaceComponent.h"
#include "CSP/Multiplayer/Components/TextSpaceComponent.h"
#include "CSP/Multiplayer/Components/VideoPlayerSpaceComponent.h"
#include "CSP/Multiplayer/OfflineRealtimeEngine.h"
#include "CSP/Multiplayer/OnlineRealtimeEngine.h"
#include "CSP/Multiplayer/Script/EntityScript.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/Script/ScriptSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "Common/Convert.h"
#include "TestHelpers.h"

#include "gtest/gtest.h"

#include <algorithm>
#include <limits>

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

CSP_PUBLIC_TEST(CSPEngine, ComponentTests, SchemaComponentRoundtrip)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    constexpr auto SchemaTypeId = std::numeric_limits<uint64_t>::max();

    const auto ComponentSchemas = csp::common::Array<csp::multiplayer::ComponentSchema> {
        csp::multiplayer::ComponentSchema {
            csp::multiplayer::ComponentSchema::TypeIdType { SchemaTypeId },
            "Example",
            {
                { 0, "stringProperty", "DefaultValue" },
            },
        },
    };

    auto UserId = csp::common::String {};
    const auto TestUser = CreateTestUser();
    ASSERT_FALSE(TestUser.Email.IsEmpty());

    LogIn(UserSystem, UserId, TestUser.Email, GeneratedTestAccountPassword);
    ASSERT_FALSE(UserId.IsEmpty());

    auto Space = csp::systems::Space {};
    CreateDefaultTestSpace(SpaceSystem, Space);
    ASSERT_FALSE(Space.Id.IsEmpty());

    {
        auto Engine = csp::multiplayer::OnlineRealtimeEngine {
            *SystemsManager.GetMultiplayerConnection(),
            *SystemsManager.GetLogSystem(),
            *SystemsManager.GetEventBus(),
            *SystemsManager.GetScriptSystem(),
            ComponentSchemas,
        };
        Engine.SetEntityFetchCompleteCallback([](auto) { });

        const auto [EnterResult] = AWAIT(SpaceSystem, EnterSpace, Space.Id, &Engine);
        ASSERT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        auto [Entity] = AWAIT(&Engine, CreateEntity, "SchemaEntity", csp::multiplayer::SpaceTransform {}, csp::common::Optional<uint64_t> {});
        ASSERT_NE(Entity, nullptr);

        auto* Component = Entity->AddComponentByTypeId(uint64_t { SchemaTypeId });
        ASSERT_NE(Component, nullptr);

        Component->SetProperty(0, csp::common::String { "RoundtripValue" });

        Entity->QueueUpdate();
        Engine.ProcessPendingEntityOperations();

        AWAIT(SpaceSystem, ExitSpace);
    }

    {
        auto Engine = csp::multiplayer::OnlineRealtimeEngine {
            *SystemsManager.GetMultiplayerConnection(),
            *SystemsManager.GetLogSystem(),
            *SystemsManager.GetEventBus(),
            *SystemsManager.GetScriptSystem(),
            ComponentSchemas,
        };

        auto EntitiesFetched = std::promise<void> {};
        auto EntitiesFetchedFuture = EntitiesFetched.get_future();
        Engine.SetEntityFetchCompleteCallback([&EntitiesFetched](auto) { EntitiesFetched.set_value(); });

        const auto [EnterResult] = AWAIT(SpaceSystem, EnterSpace, Space.Id, &Engine);
        ASSERT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        ASSERT_TRUE(WaitForFuture(EntitiesFetchedFuture));

        auto* Entity = Engine.FindSpaceObject("SchemaEntity");
        ASSERT_NE(Entity, nullptr);

        const auto* Components = Entity->GetComponents();
        ASSERT_EQ(Components->Size(), 1);

        const auto* SchemaComponent = Components->begin()->second;
        ASSERT_EQ(SchemaComponent->GetTypeId(), SchemaTypeId);

        const auto* Value = SchemaComponent->GetProperty(0);
        ASSERT_NE(Value, nullptr);
        EXPECT_EQ(Value->GetString(), csp::common::String { "RoundtripValue" });

        AWAIT(SpaceSystem, ExitSpace);
    }

    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}

CSP_PUBLIC_TEST(CSPEngine, ComponentTests, UpdatedLegacySchemaExposesExtraProperty)
{
    const auto WithExtraProperty = [](const csp::multiplayer::ComponentSchema& Original) -> csp::multiplayer::ComponentSchema
    {
        const auto NextPropertyKey = [](const auto& Properties) -> uint16_t
        {
            const auto Max
                = std::max_element(Properties.begin(), Properties.end(), [](const auto& Left, const auto& Right) { return Left.Key < Right.Key; });

            return Max == Properties.end() ? uint16_t { 0 } : static_cast<uint16_t>(Max->Key + 1);
        };

        auto Properties = csp::common::Convert(Original.Properties);
        Properties.push_back({
            NextPropertyKey(Properties),
            "extraProperty",
            csp::common::String { "ExtraDefault" },
        });

        return {
            Original.TypeId,
            Original.Name,
            csp::common::Convert(Properties),
        };
    };

    const auto AllUpdated = std::vector<csp::multiplayer::ComponentSchema> {
        WithExtraProperty(csp::multiplayer::StaticModelSpaceComponent::GetSchema()),
        WithExtraProperty(csp::multiplayer::AnimatedModelSpaceComponent::GetSchema()),
        WithExtraProperty(csp::multiplayer::VideoPlayerSpaceComponent::GetSchema()),
        WithExtraProperty(csp::multiplayer::ImageSpaceComponent::GetSchema()),
        WithExtraProperty(csp::multiplayer::ExternalLinkSpaceComponent::GetSchema()),
        WithExtraProperty(csp::multiplayer::AvatarSpaceComponent::GetSchema()),
        WithExtraProperty(csp::multiplayer::LightSpaceComponent::GetSchema()),
        WithExtraProperty(csp::multiplayer::ScriptSpaceComponent::GetSchema()),
        WithExtraProperty(csp::multiplayer::ButtonSpaceComponent::GetSchema()),
        WithExtraProperty(csp::multiplayer::CustomSpaceComponent::GetSchema()),
        WithExtraProperty(csp::multiplayer::PortalSpaceComponent::GetSchema()),
        WithExtraProperty(csp::multiplayer::ConversationSpaceComponent::GetSchema()),
        WithExtraProperty(csp::multiplayer::AudioSpaceComponent::GetSchema()),
        WithExtraProperty(csp::multiplayer::SplineSpaceComponent::GetSchema()),
        WithExtraProperty(csp::multiplayer::CollisionSpaceComponent::GetSchema()),
        WithExtraProperty(csp::multiplayer::ReflectionSpaceComponent::GetSchema()),
        WithExtraProperty(csp::multiplayer::FogSpaceComponent::GetSchema()),
        WithExtraProperty(csp::multiplayer::ECommerceSpaceComponent::GetSchema()),
        WithExtraProperty(csp::multiplayer::CinematicCameraSpaceComponent::GetSchema()),
        WithExtraProperty(csp::multiplayer::FiducialMarkerSpaceComponent::GetSchema()),
        WithExtraProperty(csp::multiplayer::GaussianSplatSpaceComponent::GetSchema()),
        WithExtraProperty(csp::multiplayer::TextSpaceComponent::GetSchema()),
        WithExtraProperty(csp::multiplayer::HotspotSpaceComponent::GetSchema()),
        WithExtraProperty(csp::multiplayer::ScreenSharingSpaceComponent::GetSchema()),
        WithExtraProperty(csp::multiplayer::AIChatbotSpaceComponent::GetSchema()),
    };

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto Engine = csp::multiplayer::OfflineRealtimeEngine {
        *SystemsManager.GetLogSystem(),
        *SystemsManager.GetScriptSystem(),
        csp::common::Convert(AllUpdated),
    };

    auto* Entity = std::get<0>(AWAIT(&Engine, CreateEntity, "Test Entity", csp::multiplayer::SpaceTransform {}, csp::common::Optional<uint64_t> {}));
    ASSERT_NE(Entity, nullptr);

    for (const auto& Schema : AllUpdated)
    {
        auto* Component = Entity->AddComponent(static_cast<csp::multiplayer::ComponentType>(Schema.TypeId));
        ASSERT_NE(Component, nullptr) << Schema.Name.c_str();

        const auto ExtraKey = Schema.Properties[Schema.Properties.Size() - 1].Key;
        const auto* Value = Component->GetProperty(ExtraKey);
        ASSERT_NE(Value, nullptr) << Schema.Name.c_str();

        EXPECT_EQ(Value->GetString(), "ExtraDefault") << Schema.Name.c_str();
    }
}
