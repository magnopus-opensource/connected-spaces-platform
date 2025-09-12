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

#include "TestHelpers.h"
#include "gtest/gtest.h"

#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Multiplayer/CSPSceneDescription.h"
#include "CSP/Multiplayer/Components/StaticModelSpaceComponent.h"
#include "CSP/Multiplayer/OfflineRealtimeEngine.h"
#include "CSP/Systems/Assets/AssetSystem.h"
#include "CSP/Systems/CSPSceneData.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "Multiplayer/MCS/MCSSceneDescription.h"
#include "Multiplayer/MCS/MCSTypes.h"
#include "PublicAPITests/UserSystemTestHelpers.h"
#include "Json/JsonSerializer.h"

#include <filesystem>
#include <fstream>

using namespace csp::multiplayer;
using namespace csp::systems;

namespace
{
bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ObjectMessageSerializeTest)
{
    const uint64_t TestId = 1;
    const uint64_t TestType = 2;
    const bool TestIsTransferable = true;
    const bool TestIsPersistent = true;
    const uint64_t TestOwnerId = 0; // TODO: Set to 3 when this is added to the test files.
    const std::optional<uint64_t> TestParentId = 4;
    std::map<mcs::PropertyKeyType, mcs::ItemComponentData> TestComponents;
    TestComponents[0] = mcs::ItemComponentData { { 0ll } };

    mcs::ObjectMessage Object { TestId, TestType, TestIsTransferable, TestIsPersistent, TestOwnerId, TestParentId, TestComponents };

    csp::common::String SerializedValue = csp::json::JsonSerializer::Serialize(Object);

    mcs::ObjectMessage DeserializedObject { 0, 0, false, false, 0, 0, {} };
    csp::json::JsonDeserializer::Deserialize(SerializedValue.c_str(), DeserializedObject);

    EXPECT_EQ(DeserializedObject, Object);
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ItemComponentDataSerializeBoolTest)
{
    const bool TestValue = true;
    mcs::ItemComponentData ComponentValue { TestValue };

    csp::common::String SerializedValue = csp::json::JsonSerializer::Serialize(ComponentValue);

    mcs::ItemComponentData DeserializedValue {};
    csp::json::JsonDeserializer::Deserialize(SerializedValue, DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ItemComponentDataSerializeInt64Test)
{
    const int64_t TestValue = -10;
    mcs::ItemComponentData ComponentValue { TestValue };

    csp::common::String SerializedValue = csp::json::JsonSerializer::Serialize(ComponentValue);

    mcs::ItemComponentData DeserializedValue {};
    csp::json::JsonDeserializer::Deserialize(SerializedValue, DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ItemComponentDataSerializeUInt64Test)
{
    const uint64_t TestValue = 10;
    mcs::ItemComponentData ComponentValue { TestValue };

    csp::common::String SerializedValue = csp::json::JsonSerializer::Serialize(ComponentValue);

    mcs::ItemComponentData DeserializedValue {};
    csp::json::JsonDeserializer::Deserialize(SerializedValue, DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ItemComponentDataSerializeDoubleTest)
{
    const double TestValue = 10.1;
    mcs::ItemComponentData ComponentValue { TestValue };

    csp::common::String SerializedValue = csp::json::JsonSerializer::Serialize(ComponentValue);

    mcs::ItemComponentData DeserializedValue {};
    csp::json::JsonDeserializer::Deserialize(SerializedValue, DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ItemComponentDataSerializeFloatTest)
{
    const float TestValue = 10.1f;
    mcs::ItemComponentData ComponentValue { TestValue };

    csp::common::String SerializedValue = csp::json::JsonSerializer::Serialize(ComponentValue);

    mcs::ItemComponentData DeserializedValue {};
    csp::json::JsonDeserializer::Deserialize(SerializedValue, DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ItemComponentDataSerializeStringTest)
{
    const std::string TestValue = "Test";
    mcs::ItemComponentData ComponentValue { TestValue };

    csp::common::String SerializedValue = csp::json::JsonSerializer::Serialize(ComponentValue);

    mcs::ItemComponentData DeserializedValue {};
    csp::json::JsonDeserializer::Deserialize(SerializedValue, DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ItemComponentDataSerializeStringEmptyTest)
{
    const std::string TestValue = "";
    mcs::ItemComponentData ComponentValue { TestValue };

    csp::common::String SerializedValue = csp::json::JsonSerializer::Serialize(ComponentValue);

    mcs::ItemComponentData DeserializedValue {};
    csp::json::JsonDeserializer::Deserialize(SerializedValue, DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ItemComponentDataSerializeFloatVectorTest)
{
    const std::vector<float> TestValue = { 1.1f, 2.2f, 3.3f };
    mcs::ItemComponentData ComponentValue { TestValue };

    csp::common::String SerializedValue = csp::json::JsonSerializer::Serialize(ComponentValue);

    mcs::ItemComponentData DeserializedValue {};
    csp::json::JsonDeserializer::Deserialize(SerializedValue, DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ItemComponentDataSerializeFloatVectorEmptyTest)
{
    const std::vector<float> TestValue = {};
    mcs::ItemComponentData ComponentValue { TestValue };

    csp::common::String SerializedValue = csp::json::JsonSerializer::Serialize(ComponentValue);

    mcs::ItemComponentData DeserializedValue {};
    csp::json::JsonDeserializer::Deserialize(SerializedValue, DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ItemComponentDataSerializeStringMapTest)
{
    const std::map<std::string, mcs::ItemComponentData> TestValue
        = { { "Key1", mcs::ItemComponentData { 1.1f } }, { "Key2", mcs::ItemComponentData { std::string { "Test" } } } };
    mcs::ItemComponentData ComponentValue { TestValue };

    csp::common::String SerializedValue = csp::json::JsonSerializer::Serialize(ComponentValue);

    mcs::ItemComponentData DeserializedValue {};
    csp::json::JsonDeserializer::Deserialize(SerializedValue, DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ItemComponentDataSerializeStringMapEmptyTest)
{
    const std::map<std::string, mcs::ItemComponentData> TestValue = {};
    mcs::ItemComponentData ComponentValue { TestValue };

    csp::common::String SerializedValue = csp::json::JsonSerializer::Serialize(ComponentValue);

    mcs::ItemComponentData DeserializedValue {};
    csp::json::JsonDeserializer::Deserialize(SerializedValue, DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ItemComponentDataSerializeUIntMapTest)
{
    const std::map<uint16_t, mcs::ItemComponentData> TestValue
        = { { 0, mcs::ItemComponentData { 1.1f } }, { 1, mcs::ItemComponentData { std::string { "Test" } } } };
    mcs::ItemComponentData ComponentValue { TestValue };

    csp::common::String SerializedValue = csp::json::JsonSerializer::Serialize(ComponentValue);

    mcs::ItemComponentData DeserializedValue {};
    csp::json::JsonDeserializer::Deserialize(SerializedValue, DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ItemComponentDataSerializeUIntMapEmptyTest)
{
    const std::map<uint16_t, mcs::ItemComponentData> TestValue = {};
    mcs::ItemComponentData ComponentValue { TestValue };

    csp::common::String SerializedValue = csp::json::JsonSerializer::Serialize(ComponentValue);

    mcs::ItemComponentData DeserializedValue {};
    csp::json::JsonDeserializer::Deserialize(SerializedValue, DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}

class MockScriptRunner : public csp::common::IJSScriptRunner
{
    bool RunScript(int64_t, const csp::common::String&) override { return false; }
    void RegisterScriptBinding(csp::common::IScriptBinding*) override { }
    void UnregisterScriptBinding(csp::common::IScriptBinding*) override { }
    bool BindContext(int64_t) override { return false; }
    bool ResetContext(int64_t) override { return false; }
    void* GetContext(int64_t) override { return nullptr; }
    void* GetModule(int64_t, const csp::common::String&) override { return nullptr; }
    bool CreateContext(int64_t) override { return false; }
    bool DestroyContext(int64_t) override { return false; }
    void SetModuleSource(csp::common::String ModuleUrl, csp::common::String) override { }
    void ClearModuleSource(csp::common::String) override { }
};

class TestAuthContext : public csp::common::IAuthContext
{
public:
    const csp::common::LoginState& GetLoginState() const override { return State; }
    void RefreshToken(std::function<void(bool)> Success) override { Success(true); }

private:
    csp::common::LoginState State;
};

/*
    CSPSceneDescription / CSPSceneData tests from a generated checkpoint file.

    The checkpoint files for these tests were generated using the following steps:
        - Create a space using a test in the OKO_TESTS test tenant
        - Log into an admin account on swagger for the OKO_TESTS tenant
        - Call spaces/{spaceId}/export using swagger to generate an export id
        - Call spaces/{spaceId}/checkpoints to generate a checkpoint asset
        - Create another test downloading the asset that is generated from the checkpoints call
        - Copy the downloaded checkpoint json into a file

    If we wanted to create a large amount of these, we should probably write a test to automate this process.
*/

// Tests SceneData/SceneDescription can be parsed from an empty checkpoint file.
CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, SceneDescriptionDeserializeEmptyTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto FilePath = std::filesystem::absolute("assets/checkpoint-empty.json");

    std::ifstream Stream { FilePath.u8string().c_str() };

    if (!Stream)
    {
        FAIL();
    }

    std::stringstream SStream;
    SStream << Stream.rdbuf();

    std::string Json = SStream.str();

    MockScriptRunner ScriptRunner;
    csp::common::LogSystem LogSystem;

    csp::multiplayer::OfflineRealtimeEngine RealtimeEngine(LogSystem, ScriptRunner);

    CSPSceneDescription SceneDescription { csp::common::List<csp::common::String> { Json.c_str() } };
    auto Entities = SceneDescription.CreateEntities(RealtimeEngine, LogSystem, ScriptRunner);

    CSPSceneData SceneData { csp::common::List<csp::common::String> { Json.c_str() } };

    EXPECT_EQ(SceneData.Space.Id, "68addce4985d7612f76b9461");
    EXPECT_EQ(SceneData.Space.Name, "checkpoint-empty");
    EXPECT_EQ(SceneData.Space.OwnerId, "68addce0985d7612f76b945e");
    EXPECT_EQ(SceneData.Space.CreatedAt, "2025-08-26T16:12:20.701+00:00");

    if (SceneData.Space.UserIds.Size() != 1)
    {
        FAIL();
    }

    EXPECT_EQ(SceneData.Space.UserIds[0], "68addce0985d7612f76b945e");
    EXPECT_EQ(SceneData.Space.BannedUserIds.Size(), 0);
    EXPECT_EQ(SceneData.Space.ModeratorIds.Size(), 0);
    EXPECT_EQ(SceneData.Space.Tags.Size(), 0);

    EXPECT_FALSE(csp::systems::HasFlag(SceneData.Space.Attributes, csp::systems::SpaceAttributes::IsDiscoverable));
    EXPECT_TRUE(csp::systems::HasFlag(SceneData.Space.Attributes, csp::systems::SpaceAttributes::RequiresInvite));

    EXPECT_EQ(Entities.Size(), 0);
    EXPECT_EQ(SceneData.AssetCollections.Size(), 0);
    EXPECT_EQ(SceneData.Assets.Size(), 0);
    EXPECT_EQ(SceneData.Sequences.Size(), 0);

    csp::CSPFoundation::Shutdown();
}

// Tests SceneData/SceneDescription can be parsed from an basic checkpoint file.
// This file contains one of every item exposed by the scene description, except anchors.
CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, SceneDescriptionDeserializeBasicTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto FilePath = std::filesystem::absolute("assets/checkpoint-basic.json");

    std::ifstream Stream { FilePath.u8string().c_str() };

    if (!Stream)
    {
        FAIL();
    }

    std::stringstream SStream;
    SStream << Stream.rdbuf();

    std::string Json = SStream.str();

    MockScriptRunner ScriptRunner;
    csp::common::LogSystem LogSystem;

    csp::multiplayer::OfflineRealtimeEngine RealtimeEngine(LogSystem, ScriptRunner);

    CSPSceneDescription SceneDescription { csp::common::List<csp::common::String> { Json.c_str() } };
    auto Entities = SceneDescription.CreateEntities(RealtimeEngine, LogSystem, ScriptRunner);

    CSPSceneData SceneData { csp::common::List<csp::common::String> { Json.c_str() } };

    EXPECT_EQ(SceneData.Space.Id, "68af162f015bb6793cacf4a2");
    EXPECT_EQ(SceneData.Space.Name, "checkpoint-basic");

    // Ensure arrays are the size we expect before continuing.
    if (Entities.Size() != 1)
    {
        FAIL();
    }

    if (SceneData.AssetCollections.Size() != 1)
    {
        FAIL();
    }

    if (SceneData.Assets.Size() != 1)
    {
        FAIL();
    }

    if (SceneData.Sequences.Size() != 1)
    {
        FAIL();
    }

    // Check entity is parsed correctly.
    csp::multiplayer::SpaceEntity* Entity = Entities[0];
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

    if (Entity->GetComponent(0)->GetComponentType() != csp::multiplayer::ComponentType::StaticModel)
    {
        FAIL();
    }

    auto StaticModelComponent = static_cast<csp::multiplayer::StaticModelSpaceComponent*>(Entity->GetComponent(0));
    EXPECT_EQ(StaticModelComponent->GetExternalResourceAssetCollectionId(), "TestAssetCollectionId");

    // Test asset collection is parsed correctly.
    csp::systems::AssetCollection Collection = SceneData.AssetCollections[0];
    EXPECT_EQ(Collection.Name, "BasicCheckpointAssetCollection2");
    EXPECT_EQ(Collection.SpaceId, "68af162f015bb6793cacf4a2");

    if (Collection.Tags.Size() != 1)
    {
        FAIL();
    }

    EXPECT_EQ(Collection.GetMetadataMutable().Size(), 0);
    EXPECT_EQ(Collection.Id, "68af1633e321a47fd460550e");
    EXPECT_EQ(Collection.Type, csp::systems::EAssetCollectionType::DEFAULT);
    EXPECT_EQ(Collection.Tags[0], "origin-68af1633e321a47fd460550e");
    EXPECT_EQ(Collection.PointOfInterestId, "");
    EXPECT_EQ(Collection.CreatedBy, "68af162b626ccc0c332bd60d");
    EXPECT_EQ(Collection.CreatedAt, "2025-08-27T14:29:07.329+00:00");
    EXPECT_EQ(Collection.UpdatedBy, "68af162b626ccc0c332bd60d");
    EXPECT_EQ(Collection.UpdatedAt, "2025-08-27T14:29:07.329+00:00");
    EXPECT_EQ(Collection.IsUnique, false);
    EXPECT_EQ(Collection.Version, "");

    // Test asset is parsed correctly
    const csp::systems::Asset& Asset = SceneData.Assets[0];
    EXPECT_EQ(Asset.Name, "BasicCheckpointAsset2");
    EXPECT_EQ(Asset.AssetCollectionId, Collection.Id);

    // Test sequence is parsed correctly
    // We use * as this gets encoded, so we want to ensure the sequence is correctly decoded.
    const csp::systems::Sequence& Sequence = SceneData.Sequences[0];
    EXPECT_EQ(Sequence.Key, "*BasicCheckpointSequence2*");

    if (Sequence.Items.Size() != 3)
    {
        FAIL();
    }

    EXPECT_EQ(Sequence.Items[0], "1");
    EXPECT_EQ(Sequence.Items[1], "2");
    EXPECT_EQ(Sequence.Items[2], "3");

    csp::CSPFoundation::Shutdown();
}

// The same test as above, but test that when we split the input, everything still works
// The interface that forces us to pass a split array rather than a string is a wrapper gen constraint
// rather than the true form of the, but lets still test it
// WARNING, this isn't the best way to be doing this, just put the whole string as the first element of the list,
// we support this behaviour because we are forced by the wrapper gen to provide this non-optimal interface expression.
CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, SceneDescriptionDeserializeBasicSplitInputTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto FilePath = std::filesystem::absolute("assets/checkpoint-basic.json");

    std::ifstream Stream { FilePath.u8string().c_str() };

    if (!Stream)
    {
        FAIL();
    }

    std::stringstream SStream;
    SStream << Stream.rdbuf();

    // Build our line array
    csp::common::List<csp::common::String> JsonChars;
    std::string line;
    while (std::getline(SStream, line))
    {
        JsonChars.Append(csp::common::String(line.c_str()));
    }

    MockScriptRunner ScriptRunner;
    csp::common::LogSystem LogSystem;

    csp::multiplayer::OfflineRealtimeEngine RealtimeEngine(LogSystem, ScriptRunner);

    CSPSceneDescription SceneDescription { JsonChars };
    auto Entities = SceneDescription.CreateEntities(RealtimeEngine, LogSystem, ScriptRunner);

    CSPSceneData SceneData { JsonChars };

    // Just do a minimal check, we don't need to fully validate everything here, we're just checking the string concatanation works.
    EXPECT_EQ(SceneData.Space.Id, "68af162f015bb6793cacf4a2");
    EXPECT_EQ(SceneData.Space.Name, "checkpoint-basic");
}

// Tests that a material parsed from scene data is valid
CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, SceneDescriptionDeserializeMaterialTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto FilePath = std::filesystem::absolute("assets/checkpoint-material.json");

    std::ifstream Stream { FilePath.u8string().c_str() };

    if (!Stream)
    {
        FAIL();
    }

    std::stringstream SStream;
    SStream << Stream.rdbuf();

    std::string Json = SStream.str();

    CSPSceneData SceneData { csp::common::List<csp::common::String> { Json.c_str() } };
    EXPECT_EQ(SceneData.Space.Name, "checkpoint-material");

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* AssetSystem = SystemsManager.GetAssetSystem();

    if (SceneData.AssetCollections.Size() != 1)
    {
        FAIL();
    }

    if (SceneData.Assets.Size() != 1)
    {
        FAIL();
    }

    const csp::systems::AssetCollection& Collection = SceneData.AssetCollections[0];
    const csp::systems::Asset& Asset = SceneData.Assets[0];

    auto [MaterialResult] = AWAIT_PRE(AssetSystem, GetMaterialFromUri, RequestPredicate, Collection, Asset.Id, Asset.Uri);
    EXPECT_EQ(MaterialResult.GetResultCode(), csp::systems::EResultCode::Success);

    const csp::systems::Material* Material = MaterialResult.GetMaterial();

    EXPECT_EQ(Material->GetName(), "Material");
    EXPECT_EQ(Material->GetShaderType(), csp::systems::EShaderType::Standard);
    EXPECT_EQ(Material->GetMaterialCollectionId(), Collection.Id);
    EXPECT_EQ(Material->GetMaterialId(), Asset.Id);

    csp::CSPFoundation::Shutdown();
}