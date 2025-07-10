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
#include "CSP/Multiplayer/SpaceEntitySystem.h"
#include "CSP/Systems/CSPSceneData.h"
#include "Multiplayer/MCS/MCSSceneDescription.h"
#include "Multiplayer/MCS/MCSTypes.h"
#include "Json/JsonSerializer.h"

#include <filesystem>
#include <fstream>

using namespace csp::multiplayer;
using namespace csp::systems;

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ObjectMessageSerializeTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

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
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    const bool TestValue = true;
    mcs::ItemComponentData ComponentValue { TestValue };

    csp::common::String SerializedValue = csp::json::JsonSerializer::Serialize(ComponentValue);

    mcs::ItemComponentData DeserializedValue {};
    csp::json::JsonDeserializer::Deserialize(SerializedValue, DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ItemComponentDataSerializeInt64Test)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    const int64_t TestValue = -10;
    mcs::ItemComponentData ComponentValue { TestValue };

    csp::common::String SerializedValue = csp::json::JsonSerializer::Serialize(ComponentValue);

    mcs::ItemComponentData DeserializedValue {};
    csp::json::JsonDeserializer::Deserialize(SerializedValue, DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ItemComponentDataSerializeUInt64Test)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    const uint64_t TestValue = 10;
    mcs::ItemComponentData ComponentValue { TestValue };

    csp::common::String SerializedValue = csp::json::JsonSerializer::Serialize(ComponentValue);

    mcs::ItemComponentData DeserializedValue {};
    csp::json::JsonDeserializer::Deserialize(SerializedValue, DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ItemComponentDataSerializeDoubleTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    const double TestValue = 10.1;
    mcs::ItemComponentData ComponentValue { TestValue };

    csp::common::String SerializedValue = csp::json::JsonSerializer::Serialize(ComponentValue);

    mcs::ItemComponentData DeserializedValue {};
    csp::json::JsonDeserializer::Deserialize(SerializedValue, DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ItemComponentDataSerializeFloatTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    const float TestValue = 10.1f;
    mcs::ItemComponentData ComponentValue { TestValue };

    csp::common::String SerializedValue = csp::json::JsonSerializer::Serialize(ComponentValue);

    mcs::ItemComponentData DeserializedValue {};
    csp::json::JsonDeserializer::Deserialize(SerializedValue, DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ItemComponentDataSerializeStringTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    const std::string TestValue = "Test";
    mcs::ItemComponentData ComponentValue { TestValue };

    csp::common::String SerializedValue = csp::json::JsonSerializer::Serialize(ComponentValue);

    mcs::ItemComponentData DeserializedValue {};
    csp::json::JsonDeserializer::Deserialize(SerializedValue, DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ItemComponentDataSerializeStringEmptyTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    const std::string TestValue = "";
    mcs::ItemComponentData ComponentValue { TestValue };

    csp::common::String SerializedValue = csp::json::JsonSerializer::Serialize(ComponentValue);

    mcs::ItemComponentData DeserializedValue {};
    csp::json::JsonDeserializer::Deserialize(SerializedValue, DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ItemComponentDataSerializeFloatVectorTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    const std::vector<float> TestValue = { 1.1f, 2.2f, 3.3f };
    mcs::ItemComponentData ComponentValue { TestValue };

    csp::common::String SerializedValue = csp::json::JsonSerializer::Serialize(ComponentValue);

    mcs::ItemComponentData DeserializedValue {};
    csp::json::JsonDeserializer::Deserialize(SerializedValue, DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ItemComponentDataSerializeFloatVectorEmptyTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    const std::vector<float> TestValue = {};
    mcs::ItemComponentData ComponentValue { TestValue };

    csp::common::String SerializedValue = csp::json::JsonSerializer::Serialize(ComponentValue);

    mcs::ItemComponentData DeserializedValue {};
    csp::json::JsonDeserializer::Deserialize(SerializedValue, DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ItemComponentDataSerializeStringMapTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

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
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    const std::map<std::string, mcs::ItemComponentData> TestValue = {};
    mcs::ItemComponentData ComponentValue { TestValue };

    csp::common::String SerializedValue = csp::json::JsonSerializer::Serialize(ComponentValue);

    mcs::ItemComponentData DeserializedValue {};
    csp::json::JsonDeserializer::Deserialize(SerializedValue, DeserializedValue);

    EXPECT_EQ(DeserializedValue, ComponentValue);
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ItemComponentDataSerializeUIntMapTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

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
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

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

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, SceneDescriptionDeserializeEmptyTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto FilePath = std::filesystem::absolute("assets/checkpoint-example-empty.json");

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
    csp::multiplayer::MultiplayerConnection Connection { LogSystem };
    csp::multiplayer::NetworkEventBus NetworkEventBus { &Connection, LogSystem };
    csp::multiplayer::SpaceEntitySystem EntitySystem(&Connection, LogSystem, NetworkEventBus, ScriptRunner);

    CSPSceneDescription SceneDescription { Json.c_str(), EntitySystem, LogSystem, ScriptRunner };
    CSPSceneData SceneData { Json.c_str() };

    EXPECT_EQ(SceneDescription.Entities.Size(), 0);
    EXPECT_EQ(SceneData.AssetCollections.Size(), 0);
    EXPECT_EQ(SceneData.Assets.Size(), 0);
    EXPECT_EQ(SceneData.Sequences.Size(), 0);
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, SceneDescriptionDeserializeTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto FilePath = std::filesystem::absolute("assets/checkpoint-example.json");

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
    csp::multiplayer::MultiplayerConnection Connection { LogSystem };
    csp::multiplayer::NetworkEventBus NetworkEventBus { &Connection, LogSystem };
    csp::multiplayer::SpaceEntitySystem EntitySystem(&Connection, LogSystem, NetworkEventBus, ScriptRunner);

    CSPSceneDescription SceneDescription { Json.c_str(), EntitySystem, LogSystem, ScriptRunner };
    CSPSceneData SceneData { Json.c_str() };

    // Test csp scene description values are correct

    // Space
    EXPECT_EQ(SceneData.Space.Id, "66c65e8d9821e1cc2b51dc0d");
    EXPECT_EQ(SceneData.Space.CreatedBy, "66a0033d6541645960bfffda");
    EXPECT_EQ(SceneData.Space.CreatedAt, "2024-08-21T21:39:25.017+00:00");
    EXPECT_EQ(SceneData.Space.OwnerId, "66a0033d6541645960bfffda");
    EXPECT_EQ(SceneData.Space.Name, "Abu Dhabi Airport");
    EXPECT_EQ(SceneData.Space.UserIds.Size(), 21);
    EXPECT_EQ(SceneData.Space.UserIds[0], "66a0033d6541645960bfffda");
    EXPECT_EQ(SceneData.Space.UserIds[20], "6823720f8f72b4d0fa153cfd");
    EXPECT_EQ(SceneData.Space.BannedUserIds.Size(), 0);
    EXPECT_EQ(SceneData.Space.ModeratorIds.Size(), 20);
    EXPECT_EQ(SceneData.Space.ModeratorIds[0], "669ac6673d223b140719c19e");
    EXPECT_EQ(SceneData.Space.ModeratorIds[19], "6823720f8f72b4d0fa153cfd");

    EXPECT_FALSE(csp::systems::HasFlag(SceneData.Space.Attributes, csp::systems::SpaceAttributes::IsDiscoverable));
    EXPECT_TRUE(csp::systems::HasFlag(SceneData.Space.Attributes, csp::systems::SpaceAttributes::RequiresInvite));

    // Entities
    EXPECT_EQ(SceneDescription.Entities.Size(), 74);

    csp::multiplayer::SpaceEntity* SpaceEntity = SceneDescription.Entities[0];
    EXPECT_EQ(SpaceEntity->GetId(), 1484);
    EXPECT_EQ(SpaceEntity->GetEntityType(), csp::multiplayer::SpaceEntityType::Object);
    EXPECT_EQ(SpaceEntity->IsTransferable, true);
    EXPECT_EQ(SpaceEntity->IsPersistent, true);
    // TODO: ownerid
    EXPECT_FALSE(SpaceEntity->ParentId.HasValue());

    // AssetCollection
    EXPECT_EQ(SceneData.AssetCollections.Size(), 88);

    csp::systems::AssetCollection AssetCollection = SceneData.AssetCollections[0];
    EXPECT_EQ(AssetCollection.GetMetadataMutable().Size(), 1);
    EXPECT_EQ(AssetCollection.GetMetadataMutable()["assetType"], csp::common::String { "staticmodel" });
    EXPECT_EQ(AssetCollection.Id, "6807d74f486cf8794b73f107");
    EXPECT_EQ(AssetCollection.Name, "Small_Kiosk_T.glb_536.4520788603719");
    EXPECT_EQ(AssetCollection.Type, csp::systems::EAssetCollectionType::DEFAULT);
    EXPECT_EQ(AssetCollection.Tags.Size(), 0);
    EXPECT_EQ(AssetCollection.PointOfInterestId, "");
    EXPECT_EQ(AssetCollection.ParentId, "");
    EXPECT_EQ(AssetCollection.SpaceId, "66c65e8d9821e1cc2b51dc0d");
    EXPECT_EQ(AssetCollection.CreatedBy, "669ac6673d223b140719c19e");
    EXPECT_EQ(AssetCollection.CreatedAt, "2025-04-22T17:52:15.369+00:00");
    EXPECT_EQ(AssetCollection.UpdatedBy, "669ac6673d223b140719c19e");
    EXPECT_EQ(AssetCollection.UpdatedAt, "2025-04-22T17:52:15.369+00:00");
    EXPECT_EQ(AssetCollection.IsUnique, false);
    EXPECT_EQ(AssetCollection.Version, "");

    // Asset
    EXPECT_EQ(SceneData.Assets.Size(), 488);

    csp::systems::Asset Asset = SceneData.Assets[0];
    EXPECT_EQ(Asset.AssetCollectionId, "6807d74f486cf8794b73f107");
    EXPECT_EQ(Asset.Id, "6807d750b618cc273309a258");
    EXPECT_EQ(Asset.FileName, "");
    EXPECT_EQ(Asset.Name, "Small_Kiosk_T_lod3.glb");
    EXPECT_EQ(Asset.LanguageCode, "en-us");
    EXPECT_EQ(Asset.Type, csp::systems::EAssetType::MODEL);
    EXPECT_EQ(Asset.Platforms.Size(), 1);
    EXPECT_EQ(Asset.Platforms[0], csp::systems::EAssetPlatform::DEFAULT);
    EXPECT_EQ(Asset.Styles.Size(), 2);
    EXPECT_EQ(Asset.Styles[0], "Default");
    EXPECT_EQ(Asset.Styles[1], "lod:3");
    EXPECT_EQ(Asset.ExternalUri, "");
    EXPECT_EQ(Asset.Uri, "");
    EXPECT_EQ(Asset.Checksum, "");
    EXPECT_EQ(Asset.Version, 0);
    EXPECT_EQ(Asset.MimeType, "");
    EXPECT_EQ(Asset.ExternalMimeType, "");
    EXPECT_EQ(Asset.ThirdPartyPackagedAssetIdentifier, "");
    EXPECT_EQ(Asset.ThirdPartyPlatformType, csp::systems::EThirdPartyPlatform::NONE);

    // Sequences
    EXPECT_EQ(SceneData.Sequences.Size(), 0);
}

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, SceneDescriptionMinimalDeserializeTest)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto FilePath = std::filesystem::absolute("assets/checkpoint-example-minimal.json");

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
    csp::multiplayer::MultiplayerConnection Connection { LogSystem };
    csp::multiplayer::NetworkEventBus NetworkEventBus { &Connection, LogSystem };
    csp::multiplayer::SpaceEntitySystem EntitySystem(&Connection, LogSystem, NetworkEventBus, ScriptRunner);

    CSPSceneDescription SceneDescription { Json.c_str(), EntitySystem, LogSystem, ScriptRunner };
    CSPSceneData SceneData { Json.c_str() };

    // Test csp scene description values are correct

    // Space
    EXPECT_EQ(SceneData.Space.Id, "66c65e8d9821e1cc2b51dc0d");
    EXPECT_EQ(SceneData.Space.CreatedBy, "66a0033d6541645960bfffda");
    EXPECT_EQ(SceneData.Space.CreatedAt, "2024-08-21T21:39:25.017+00:00");
    EXPECT_EQ(SceneData.Space.OwnerId, "66a0033d6541645960bfffda");
    EXPECT_EQ(SceneData.Space.Name, "Abu Dhabi Airport");
    EXPECT_EQ(SceneData.Space.UserIds.Size(), 0);
    EXPECT_EQ(SceneData.Space.BannedUserIds.Size(), 0);
    EXPECT_EQ(SceneData.Space.ModeratorIds.Size(), 0);

    EXPECT_FALSE(csp::systems::HasFlag(SceneData.Space.Attributes, csp::systems::SpaceAttributes::IsDiscoverable));
    EXPECT_TRUE(csp::systems::HasFlag(SceneData.Space.Attributes, csp::systems::SpaceAttributes::RequiresInvite));

    // Entities
    EXPECT_EQ(SceneDescription.Entities.Size(), 1);

    csp::multiplayer::SpaceEntity* SpaceEntity = SceneDescription.Entities[0];
    EXPECT_EQ(SpaceEntity->GetId(), 1484);
    EXPECT_EQ(SpaceEntity->GetEntityType(), csp::multiplayer::SpaceEntityType::Object);
    EXPECT_EQ(SpaceEntity->IsTransferable, true);
    EXPECT_EQ(SpaceEntity->IsPersistent, true);
    // TODO: ownerid
    EXPECT_FALSE(SpaceEntity->ParentId.HasValue());

    // AssetCollection
    EXPECT_EQ(SceneData.AssetCollections.Size(), 1);

    csp::systems::AssetCollection AssetCollection = SceneData.AssetCollections[0];
    EXPECT_EQ(AssetCollection.GetMetadataMutable().Size(), 1);
    EXPECT_EQ(AssetCollection.GetMetadataMutable()["assetType"], csp::common::String { "staticmodel" });
    EXPECT_EQ(AssetCollection.Id, "6807d74f486cf8794b73f107");
    EXPECT_EQ(AssetCollection.Name, "Small_Kiosk_T.glb_536.4520788603719");
    EXPECT_EQ(AssetCollection.Type, csp::systems::EAssetCollectionType::DEFAULT);
    EXPECT_EQ(AssetCollection.Tags.Size(), 0);
    EXPECT_EQ(AssetCollection.PointOfInterestId, "");
    EXPECT_EQ(AssetCollection.ParentId, "");
    EXPECT_EQ(AssetCollection.SpaceId, "66c65e8d9821e1cc2b51dc0d");
    EXPECT_EQ(AssetCollection.CreatedBy, "669ac6673d223b140719c19e");
    EXPECT_EQ(AssetCollection.CreatedAt, "2025-04-22T17:52:15.369+00:00");
    EXPECT_EQ(AssetCollection.UpdatedBy, "669ac6673d223b140719c19e");
    EXPECT_EQ(AssetCollection.UpdatedAt, "2025-04-22T17:52:15.369+00:00");
    EXPECT_EQ(AssetCollection.IsUnique, false);
    EXPECT_EQ(AssetCollection.Version, "");

    // Asset
    EXPECT_EQ(SceneData.Assets.Size(), 1);

    csp::systems::Asset Asset = SceneData.Assets[0];
    EXPECT_EQ(Asset.AssetCollectionId, "6807d74f486cf8794b73f107");
    EXPECT_EQ(Asset.Id, "6807d750b618cc273309a258");
    EXPECT_EQ(Asset.FileName, "");
    EXPECT_EQ(Asset.Name, "Small_Kiosk_T_lod3.glb");
    EXPECT_EQ(Asset.LanguageCode, "en-us");
    EXPECT_EQ(Asset.Type, csp::systems::EAssetType::MODEL);
    EXPECT_EQ(Asset.Platforms.Size(), 1);
    EXPECT_EQ(Asset.Platforms[0], csp::systems::EAssetPlatform::DEFAULT);
    EXPECT_EQ(Asset.Styles.Size(), 1);
    EXPECT_EQ(Asset.Styles[0], "Default");
    EXPECT_EQ(Asset.ExternalUri, "");
    EXPECT_EQ(Asset.Uri, "");
    EXPECT_EQ(Asset.Checksum, "");
    EXPECT_EQ(Asset.Version, 0);
    EXPECT_EQ(Asset.MimeType, "");
    EXPECT_EQ(Asset.ExternalMimeType, "");
    EXPECT_EQ(Asset.ThirdPartyPackagedAssetIdentifier, "");
    EXPECT_EQ(Asset.ThirdPartyPlatformType, csp::systems::EThirdPartyPlatform::NONE);

    // Sequences
    EXPECT_EQ(SceneData.Sequences.Size(), 0);
}
