#include "CSP/Multiplayer/Component/ComponentRegistry.h"
#include "CSP/Multiplayer/Component/ComponentBuilder.h"

#include "Awaitable.h"
#include "CSP/CSPFoundation.h"
#include "CSP/Common/ReplicatedValue.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/OnlineRealtimeEngine.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/Spaces/Space.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"

#include "SpaceSystemTestHelpers.h"
#include "UserSystemTestHelpers.h"

#include "TestHelpers.h"
#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

namespace
{
bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }
}

CSP_PUBLIC_TEST(CSPEngine, ComponentTests, ComponentDeserializeTest)
{
    auto FilePath = std::filesystem::absolute("assets/components-test.json");

    std::ifstream Stream { FilePath.u8string().c_str() };

    if (!Stream)
    {
        FAIL();
    }

    std::stringstream SStream;
    SStream << Stream.rdbuf();

    std::string Json = SStream.str();

    csp::multiplayer::ComponentRegistry Regstry;
    Regstry.RegisterComponents(Json.c_str());

    const auto& Templates = Regstry.GetTemplates();

    if (Templates.Size() != 1)
    {
        FAIL();
    }

    const auto& Template = Templates[0];

    EXPECT_EQ(Template.Name, "Test1");
    EXPECT_EQ(Template.Id, "1");
    EXPECT_EQ(Template.Category, "Category1");
    EXPECT_EQ(Template.Description, "Description1");

    if (Template.Properties.Size() != 8)
    {
        FAIL();
    }

    EXPECT_EQ(Template.Properties[0].Name, "boolProperty");
    EXPECT_EQ(Template.Properties[0].Value.GetReplicatedValueType(), csp::common::ReplicatedValueType::Boolean);
    EXPECT_EQ(Template.Properties[0].Value.GetBool(), false);

    EXPECT_EQ(Template.Properties[1].Name, "intProperty");
    EXPECT_EQ(Template.Properties[1].Value.GetReplicatedValueType(), csp::common::ReplicatedValueType::Integer);
    EXPECT_EQ(Template.Properties[1].Value.GetInt(), 0);

    EXPECT_EQ(Template.Properties[2].Name, "floatProperty");
    EXPECT_EQ(Template.Properties[2].Value.GetReplicatedValueType(), csp::common::ReplicatedValueType::Float);
    EXPECT_EQ(Template.Properties[2].Value.GetFloat(), 0.0);

    EXPECT_EQ(Template.Properties[3].Name, "stringProperty");
    EXPECT_EQ(Template.Properties[3].Value.GetReplicatedValueType(), csp::common::ReplicatedValueType::String);
    EXPECT_EQ(Template.Properties[3].Value.GetString(), "");

    EXPECT_EQ(Template.Properties[4].Name, "vec2Property");
    EXPECT_EQ(Template.Properties[4].Value.GetReplicatedValueType(), csp::common::ReplicatedValueType::Vector2);
    EXPECT_EQ(Template.Properties[4].Value.GetVector2(), csp::common::Vector2::Zero());

    EXPECT_EQ(Template.Properties[5].Name, "vec3Property");
    EXPECT_EQ(Template.Properties[5].Value.GetReplicatedValueType(), csp::common::ReplicatedValueType::Vector3);
    EXPECT_EQ(Template.Properties[5].Value.GetVector3(), csp::common::Vector3::Zero());

    EXPECT_EQ(Template.Properties[6].Name, "vec4Property");
    EXPECT_EQ(Template.Properties[6].Value.GetReplicatedValueType(), csp::common::ReplicatedValueType::Vector4);
    EXPECT_EQ(Template.Properties[6].Value.GetVector4(), csp::common::Vector4::Zero());

    EXPECT_EQ(Template.Properties[7].Name, "stringMapProperty");
    EXPECT_EQ(Template.Properties[7].Value.GetReplicatedValueType(), csp::common::ReplicatedValueType::StringMap);
    EXPECT_EQ(Template.Properties[7].Value.GetStringMap().Size(), 1);
    EXPECT_EQ(Template.Properties[7].Value.GetStringMap().HasKey("key1"), true);
}

CSP_PUBLIC_TEST(CSPEngine, ComponentTests, ComponentBuilderTest)
{
    auto FilePath = std::filesystem::absolute("assets/components-test.json");

    std::ifstream Stream { FilePath.u8string().c_str() };

    if (!Stream)
    {
        FAIL();
    }

    std::stringstream SStream;
    SStream << Stream.rdbuf();

    std::string Json = SStream.str();

    csp::multiplayer::ComponentRegistry Regstry;
    Regstry.RegisterComponents(Json.c_str());

    csp::multiplayer::Component Component = csp::multiplayer::CreateComponent(Regstry, "TestComponent1", 0, nullptr, nullptr);

    const auto& Properties = *Component.GetProperties();

    if (Properties.Size() != 8)
    {
        FAIL();
    }

    EXPECT_EQ(Properties["boolProperty"].GetReplicatedValueType(), csp::common::ReplicatedValueType::Boolean);
    EXPECT_EQ(Properties["boolProperty"].GetBool(), false);

    EXPECT_EQ(Properties["intProperty"].GetReplicatedValueType(), csp::common::ReplicatedValueType::Integer);
    EXPECT_EQ(Properties["intProperty"].GetInt(), 0);

    EXPECT_EQ(Properties["floatProperty"].GetReplicatedValueType(), csp::common::ReplicatedValueType::Float);
    EXPECT_EQ(Properties["floatProperty"].GetFloat(), 0.0);

    EXPECT_EQ(Properties["stringProperty"].GetReplicatedValueType(), csp::common::ReplicatedValueType::String);
    EXPECT_EQ(Properties["stringProperty"].GetString(), "");

    EXPECT_EQ(Properties["vec2Property"].GetReplicatedValueType(), csp::common::ReplicatedValueType::Vector2);
    EXPECT_EQ(Properties["vec2Property"].GetVector2(), csp::common::Vector2::Zero());

    EXPECT_EQ(Properties["vec3Property"].GetReplicatedValueType(), csp::common::ReplicatedValueType::Vector3);
    EXPECT_EQ(Properties["vec3Property"].GetVector3(), csp::common::Vector3::Zero());

    EXPECT_EQ(Properties["vec4Property"].GetReplicatedValueType(), csp::common::ReplicatedValueType::Vector4);
    EXPECT_EQ(Properties["vec4Property"].GetVector4(), csp::common::Vector4::Zero());

    EXPECT_EQ(Properties["stringMapProperty"].GetReplicatedValueType(), csp::common::ReplicatedValueType::StringMap);
    EXPECT_EQ(Properties["stringMapProperty"].GetStringMap().Size(), 1);
    EXPECT_EQ(Properties["stringMapProperty"].GetStringMap().HasKey("key1"), true);
}

CSP_PUBLIC_TEST(CSPEngine, ComponentTests, EntityComponentTest)
{
    auto FilePath = std::filesystem::absolute("assets/components-test.json");

    std::ifstream Stream { FilePath.u8string().c_str() };

    if (!Stream)
    {
        FAIL();
    }

    std::stringstream SStream;
    SStream << Stream.rdbuf();

    std::string Json = SStream.str();

    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-MAG";

    char UniqueAssetCollectionName[256];
    SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateDefaultTestSpace(SpaceSystem, Space);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
    RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) { });
    RealtimeEngine->RegisterComponents(Json.c_str());

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    RealtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) { });

    auto Entity = CreateTestObject(RealtimeEngine.get());
    Entity->AddComponent2("TestComponent1");

    auto Component = Entity->GetComponent2(0);

    const auto& Properties = *Component->GetProperties();

    EXPECT_EQ(Properties["boolProperty"].GetReplicatedValueType(), csp::common::ReplicatedValueType::Boolean);
    EXPECT_EQ(Properties["boolProperty"].GetBool(), false);

    EXPECT_EQ(Properties["intProperty"].GetReplicatedValueType(), csp::common::ReplicatedValueType::Integer);
    EXPECT_EQ(Properties["intProperty"].GetInt(), 0);

    EXPECT_EQ(Properties["floatProperty"].GetReplicatedValueType(), csp::common::ReplicatedValueType::Float);
    EXPECT_EQ(Properties["floatProperty"].GetFloat(), 0.0);

    EXPECT_EQ(Properties["stringProperty"].GetReplicatedValueType(), csp::common::ReplicatedValueType::String);
    EXPECT_EQ(Properties["stringProperty"].GetString(), "");

    EXPECT_EQ(Properties["vec2Property"].GetReplicatedValueType(), csp::common::ReplicatedValueType::Vector2);
    EXPECT_EQ(Properties["vec2Property"].GetVector2(), csp::common::Vector2::Zero());

    EXPECT_EQ(Properties["vec3Property"].GetReplicatedValueType(), csp::common::ReplicatedValueType::Vector3);
    EXPECT_EQ(Properties["vec3Property"].GetVector3(), csp::common::Vector3::Zero());

    EXPECT_EQ(Properties["vec4Property"].GetReplicatedValueType(), csp::common::ReplicatedValueType::Vector4);
    EXPECT_EQ(Properties["vec4Property"].GetVector4(), csp::common::Vector4::Zero());

    EXPECT_EQ(Properties["stringMapProperty"].GetReplicatedValueType(), csp::common::ReplicatedValueType::StringMap);
    EXPECT_EQ(Properties["stringMapProperty"].GetStringMap().Size(), 1);
    EXPECT_EQ(Properties["stringMapProperty"].GetStringMap().HasKey("key1"), true);

}