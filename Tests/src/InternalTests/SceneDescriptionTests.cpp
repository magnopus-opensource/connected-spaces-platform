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
bool RequestPredicate(const csp::systems::ResultBase& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; }
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ObjectMessageSerializeTest)
{
    const uint64_t testId = 1;
    const uint64_t testType = 2;
    const bool testIsTransferable = true;
    const bool testIsPersistent = true;
    const uint64_t testOwnerId = 0; // TODO: Set to 3 when this is added to the test files.
    const std::optional<uint64_t> testParentId = 4;
    std::map<mcs::PropertyKeyType, mcs::ItemComponentData> testComponents;
    testComponents[0] = mcs::ItemComponentData { { 0ll } };

    mcs::ObjectMessage object { testId, testType, testIsTransferable, testIsPersistent, testOwnerId, testParentId, testComponents };

    csp::common::String serializedValue = csp::json::JsonSerializer::Serialize(object);

    mcs::ObjectMessage deserializedObject { 0, 0, false, false, 0, 0, {} };
    csp::json::JsonDeserializer::Deserialize(serializedValue.c_str(), deserializedObject);

    EXPECT_EQ(deserializedObject, object);
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ItemComponentDataSerializeBoolTest)
{
    const bool testValue = true;
    mcs::ItemComponentData componentValue { testValue };

    csp::common::String serializedValue = csp::json::JsonSerializer::Serialize(componentValue);

    mcs::ItemComponentData deserializedValue {};
    csp::json::JsonDeserializer::Deserialize(serializedValue, deserializedValue);

    EXPECT_EQ(deserializedValue, componentValue);
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ItemComponentDataSerializeInt64Test)
{
    const int64_t testValue = -10;
    mcs::ItemComponentData componentValue { testValue };

    csp::common::String serializedValue = csp::json::JsonSerializer::Serialize(componentValue);

    mcs::ItemComponentData deserializedValue {};
    csp::json::JsonDeserializer::Deserialize(serializedValue, deserializedValue);

    EXPECT_EQ(deserializedValue, componentValue);
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ItemComponentDataSerializeUInt64Test)
{
    const uint64_t testValue = 10;
    mcs::ItemComponentData componentValue { testValue };

    csp::common::String serializedValue = csp::json::JsonSerializer::Serialize(componentValue);

    mcs::ItemComponentData deserializedValue {};
    csp::json::JsonDeserializer::Deserialize(serializedValue, deserializedValue);

    EXPECT_EQ(deserializedValue, componentValue);
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ItemComponentDataSerializeDoubleTest)
{
    const double testValue = 10.1;
    mcs::ItemComponentData componentValue { testValue };

    csp::common::String serializedValue = csp::json::JsonSerializer::Serialize(componentValue);

    mcs::ItemComponentData deserializedValue {};
    csp::json::JsonDeserializer::Deserialize(serializedValue, deserializedValue);

    EXPECT_EQ(deserializedValue, componentValue);
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ItemComponentDataSerializeFloatTest)
{
    const float testValue = 10.1f;
    mcs::ItemComponentData componentValue { testValue };

    csp::common::String serializedValue = csp::json::JsonSerializer::Serialize(componentValue);

    mcs::ItemComponentData deserializedValue {};
    csp::json::JsonDeserializer::Deserialize(serializedValue, deserializedValue);

    EXPECT_EQ(deserializedValue, componentValue);
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ItemComponentDataSerializeStringTest)
{
    const std::string testValue = "Test";
    mcs::ItemComponentData componentValue { testValue };

    csp::common::String serializedValue = csp::json::JsonSerializer::Serialize(componentValue);

    mcs::ItemComponentData deserializedValue {};
    csp::json::JsonDeserializer::Deserialize(serializedValue, deserializedValue);

    EXPECT_EQ(deserializedValue, componentValue);
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ItemComponentDataSerializeStringEmptyTest)
{
    const std::string testValue = "";
    mcs::ItemComponentData componentValue { testValue };

    csp::common::String serializedValue = csp::json::JsonSerializer::Serialize(componentValue);

    mcs::ItemComponentData deserializedValue {};
    csp::json::JsonDeserializer::Deserialize(serializedValue, deserializedValue);

    EXPECT_EQ(deserializedValue, componentValue);
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ItemComponentDataSerializeFloatVectorTest)
{
    const std::vector<float> testValue = { 1.1f, 2.2f, 3.3f };
    mcs::ItemComponentData componentValue { testValue };

    csp::common::String serializedValue = csp::json::JsonSerializer::Serialize(componentValue);

    mcs::ItemComponentData deserializedValue {};
    csp::json::JsonDeserializer::Deserialize(serializedValue, deserializedValue);

    EXPECT_EQ(deserializedValue, componentValue);
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ItemComponentDataSerializeFloatVectorEmptyTest)
{
    const std::vector<float> testValue = {};
    mcs::ItemComponentData componentValue { testValue };

    csp::common::String serializedValue = csp::json::JsonSerializer::Serialize(componentValue);

    mcs::ItemComponentData deserializedValue {};
    csp::json::JsonDeserializer::Deserialize(serializedValue, deserializedValue);

    EXPECT_EQ(deserializedValue, componentValue);
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ItemComponentDataSerializeStringMapTest)
{
    const std::map<std::string, mcs::ItemComponentData> testValue
        = { { "Key1", mcs::ItemComponentData { 1.1f } }, { "Key2", mcs::ItemComponentData { std::string { "Test" } } } };
    mcs::ItemComponentData componentValue { testValue };

    csp::common::String serializedValue = csp::json::JsonSerializer::Serialize(componentValue);

    mcs::ItemComponentData deserializedValue {};
    csp::json::JsonDeserializer::Deserialize(serializedValue, deserializedValue);

    EXPECT_EQ(deserializedValue, componentValue);
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ItemComponentDataSerializeStringMapEmptyTest)
{
    const std::map<std::string, mcs::ItemComponentData> testValue = {};
    mcs::ItemComponentData componentValue { testValue };

    csp::common::String serializedValue = csp::json::JsonSerializer::Serialize(componentValue);

    mcs::ItemComponentData deserializedValue {};
    csp::json::JsonDeserializer::Deserialize(serializedValue, deserializedValue);

    EXPECT_EQ(deserializedValue, componentValue);
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ItemComponentDataSerializeUIntMapTest)
{
    const std::map<uint16_t, mcs::ItemComponentData> testValue
        = { { 0, mcs::ItemComponentData { 1.1f } }, { 1, mcs::ItemComponentData { std::string { "Test" } } } };
    mcs::ItemComponentData componentValue { testValue };

    csp::common::String serializedValue = csp::json::JsonSerializer::Serialize(componentValue);

    mcs::ItemComponentData deserializedValue {};
    csp::json::JsonDeserializer::Deserialize(serializedValue, deserializedValue);

    EXPECT_EQ(deserializedValue, componentValue);
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ItemComponentDataSerializeUIntMapEmptyTest)
{
    const std::map<uint16_t, mcs::ItemComponentData> testValue = {};
    mcs::ItemComponentData componentValue { testValue };

    csp::common::String serializedValue = csp::json::JsonSerializer::Serialize(componentValue);

    mcs::ItemComponentData deserializedValue {};
    csp::json::JsonDeserializer::Deserialize(serializedValue, deserializedValue);

    EXPECT_EQ(deserializedValue, componentValue);
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
    void SetModuleSource(csp::common::String, csp::common::String) override { }
    void ClearModuleSource(csp::common::String) override { }
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

    auto filePath = std::filesystem::absolute("assets/checkpoint-empty.json");

    std::ifstream stream { filePath.u8string().c_str() };

    if (!stream)
    {
        FAIL();
    }

    std::stringstream sStream;
    sStream << stream.rdbuf();

    std::string json = sStream.str();

    MockScriptRunner scriptRunner;
    csp::common::LogSystem logSystem;

    csp::multiplayer::OfflineRealtimeEngine realtimeEngine(logSystem, scriptRunner);

    CSPSceneDescription sceneDescription { csp::common::List<csp::common::String> { json.c_str() } };
    auto entities = sceneDescription.CreateEntities(realtimeEngine, logSystem, scriptRunner);

    CSPSceneData sceneData { csp::common::List<csp::common::String> { json.c_str() } };

    EXPECT_EQ(sceneData.Space.Id, "68addce4985d7612f76b9461");
    EXPECT_EQ(sceneData.Space.Name, "checkpoint-empty");
    EXPECT_EQ(sceneData.Space.OwnerId, "68addce0985d7612f76b945e");
    EXPECT_EQ(sceneData.Space.CreatedAt, "2025-08-26T16:12:20.701+00:00");

    if (sceneData.Space.UserIds.Size() != 1)
    {
        FAIL();
    }

    EXPECT_EQ(sceneData.Space.UserIds[0], "68addce0985d7612f76b945e");
    EXPECT_EQ(sceneData.Space.BannedUserIds.Size(), 0);
    EXPECT_EQ(sceneData.Space.ModeratorIds.Size(), 0);
    EXPECT_EQ(sceneData.Space.Tags.Size(), 0);

    EXPECT_FALSE(csp::systems::HasFlag(sceneData.Space.Attributes, csp::systems::SpaceAttributes::IsDiscoverable));
    EXPECT_TRUE(csp::systems::HasFlag(sceneData.Space.Attributes, csp::systems::SpaceAttributes::RequiresInvite));

    EXPECT_EQ(entities.Size(), 0);
    EXPECT_EQ(sceneData.AssetCollections.Size(), 0);
    EXPECT_EQ(sceneData.Assets.Size(), 0);
    EXPECT_EQ(sceneData.Sequences.Size(), 0);

    csp::CSPFoundation::Shutdown();
}

// Tests SceneData/SceneDescription can be parsed from an basic checkpoint file.
// This file contains one of every item exposed by the scene description, except anchors.
CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, SceneDescriptionDeserializeBasicTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto filePath = std::filesystem::absolute("assets/checkpoint-basic.json");

    std::ifstream stream { filePath.u8string().c_str() };

    if (!stream)
    {
        FAIL();
    }

    std::stringstream sStream;
    sStream << stream.rdbuf();

    std::string json = sStream.str();

    MockScriptRunner scriptRunner;
    csp::common::LogSystem logSystem;

    csp::multiplayer::OfflineRealtimeEngine realtimeEngine(logSystem, scriptRunner);

    CSPSceneDescription sceneDescription { csp::common::List<csp::common::String> { json.c_str() } };
    auto entities = sceneDescription.CreateEntities(realtimeEngine, logSystem, scriptRunner);

    CSPSceneData sceneData { csp::common::List<csp::common::String> { json.c_str() } };

    EXPECT_EQ(sceneData.Space.Id, "68af162f015bb6793cacf4a2");
    EXPECT_EQ(sceneData.Space.Name, "checkpoint-basic");

    // Ensure arrays are the size we expect before continuing.
    if (entities.Size() != 1)
    {
        FAIL();
    }

    if (sceneData.AssetCollections.Size() != 1)
    {
        FAIL();
    }

    if (sceneData.Assets.Size() != 1)
    {
        FAIL();
    }

    if (sceneData.Sequences.Size() != 1)
    {
        FAIL();
    }

    // Check entity is parsed correctly.
    csp::multiplayer::SpaceEntity* entity = entities[0];
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

    if (entity->GetComponent(0)->GetComponentType() != csp::multiplayer::ComponentType::StaticModel)
    {
        FAIL();
    }

    auto staticModelComponent = static_cast<csp::multiplayer::StaticModelSpaceComponent*>(entity->GetComponent(0));
    EXPECT_EQ(staticModelComponent->GetExternalResourceAssetCollectionId(), "TestAssetCollectionId");

    // Test asset collection is parsed correctly.
    csp::systems::AssetCollection collection = sceneData.AssetCollections[0];
    EXPECT_EQ(collection.Name, "BasicCheckpointAssetCollection2");
    EXPECT_EQ(collection.SpaceId, "68af162f015bb6793cacf4a2");

    if (collection.Tags.Size() != 1)
    {
        FAIL();
    }

    EXPECT_EQ(collection.GetMetadataMutable().Size(), 0);
    EXPECT_EQ(collection.Id, "68af1633e321a47fd460550e");
    EXPECT_EQ(collection.Type, csp::systems::EAssetCollectionType::DEFAULT);
    EXPECT_EQ(collection.Tags[0], "origin-68af1633e321a47fd460550e");
    EXPECT_EQ(collection.PointOfInterestId, "");
    EXPECT_EQ(collection.CreatedBy, "68af162b626ccc0c332bd60d");
    EXPECT_EQ(collection.CreatedAt, "2025-08-27T14:29:07.329+00:00");
    EXPECT_EQ(collection.UpdatedBy, "68af162b626ccc0c332bd60d");
    EXPECT_EQ(collection.UpdatedAt, "2025-08-27T14:29:07.329+00:00");
    EXPECT_EQ(collection.IsUnique, false);
    EXPECT_EQ(collection.Version, "");

    // Test asset is parsed correctly
    const csp::systems::Asset& asset = sceneData.Assets[0];
    EXPECT_EQ(asset.Name, "BasicCheckpointAsset2");
    EXPECT_EQ(asset.AssetCollectionId, collection.Id);

    // Test sequence is parsed correctly
    // We use * as this gets encoded, so we want to ensure the sequence is correctly decoded.
    const csp::systems::Sequence& sequence = sceneData.Sequences[0];
    EXPECT_EQ(sequence.Key, "*BasicCheckpointSequence2*");

    if (sequence.Items.Size() != 3)
    {
        FAIL();
    }

    EXPECT_EQ(sequence.Items[0], "1");
    EXPECT_EQ(sequence.Items[1], "2");
    EXPECT_EQ(sequence.Items[2], "3");

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

    auto filePath = std::filesystem::absolute("assets/checkpoint-basic.json");

    std::ifstream stream { filePath.u8string().c_str() };

    if (!stream)
    {
        FAIL();
    }

    std::stringstream sStream;
    sStream << stream.rdbuf();

    // Build our line array
    csp::common::List<csp::common::String> jsonChars;
    std::string line;
    while (std::getline(sStream, line))
    {
        jsonChars.Append(csp::common::String(line.c_str()));
    }

    MockScriptRunner scriptRunner;
    csp::common::LogSystem logSystem;

    csp::multiplayer::OfflineRealtimeEngine realtimeEngine(logSystem, scriptRunner);

    CSPSceneDescription sceneDescription { jsonChars };
    auto entities = sceneDescription.CreateEntities(realtimeEngine, logSystem, scriptRunner);

    CSPSceneData sceneData { jsonChars };

    // Just do a minimal check, we don't need to fully validate everything here, we're just checking the string concatanation works.
    EXPECT_EQ(sceneData.Space.Id, "68af162f015bb6793cacf4a2");
    EXPECT_EQ(sceneData.Space.Name, "checkpoint-basic");

    csp::CSPFoundation::Shutdown();
}

// Tests that a material parsed from scene data is valid
CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, SceneDescriptionDeserializeMaterialTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto filePath = std::filesystem::absolute("assets/checkpoint-material.json");

    std::ifstream stream { filePath.u8string().c_str() };

    if (!stream)
    {
        FAIL();
    }

    std::stringstream sStream;
    sStream << stream.rdbuf();

    std::string json = sStream.str();

    CSPSceneData sceneData { csp::common::List<csp::common::String> { json.c_str() } };
    EXPECT_EQ(sceneData.Space.Name, "checkpoint-material");

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* assetSystem = systemsManager.GetAssetSystem();

    if (sceneData.AssetCollections.Size() != 1)
    {
        FAIL();
    }

    if (sceneData.Assets.Size() != 1)
    {
        FAIL();
    }

    const csp::systems::AssetCollection& collection = sceneData.AssetCollections[0];
    const csp::systems::Asset& asset = sceneData.Assets[0];

    auto [MaterialResult] = AWAIT_PRE(assetSystem, GetMaterialFromUri, RequestPredicate, collection, asset.Id, asset.Uri);
    EXPECT_EQ(MaterialResult.GetResultCode(), csp::systems::EResultCode::Success);

    const csp::systems::Material* material = MaterialResult.GetMaterial();

    EXPECT_EQ(material->GetName(), "Material");
    EXPECT_EQ(material->GetShaderType(), csp::systems::EShaderType::Standard);
    EXPECT_EQ(material->GetMaterialCollectionId(), collection.Id);
    EXPECT_EQ(material->GetMaterialId(), asset.Id);

    csp::CSPFoundation::Shutdown();
}