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

#include "CSP/CSPSceneDescription.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Multiplayer/SpaceEntitySystem.h"
#include "Multiplayer/MCS/MCSSceneDescription.h"
#include "Multiplayer/MCS/MCSTypes.h"
#include "Json/JsonSerializer.h"

#include <filesystem>
#include <fstream>

using namespace csp::multiplayer;

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ObjectMessageSerializeTest)
{
    const uint64_t TestId = 1;
    const uint64_t TestType = 2;
    const bool TestIsTransferable = true;
    const bool TestIsPersistent = true;
    const uint64_t TestOwnerId = 3;
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

CSP_INTERNAL_TEST(CSPEngine, SceneDescriptionTests, ItemComponentDataSerializeFloatVectorTest)
{
    const std::vector<float> TestValue = { 1.1f, 2.2f, 3.3f };
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

    mcs::SceneDescription DeserializedValue {};
    csp::json::JsonDeserializer::Deserialize(Json.c_str(), DeserializedValue);

    EXPECT_EQ(DeserializedValue.Objects.size(), 0);
    EXPECT_EQ(DeserializedValue.Prototypes.size(), 0);
    EXPECT_EQ(DeserializedValue.AssetDetails.size(), 0);
    EXPECT_EQ(DeserializedValue.Sequences.size(), 0);

    // Convert to csp scene description
    csp::common::LogSystem LogSystem;
    csp::multiplayer::MultiplayerConnection Connection { &LogSystem };
    csp::multiplayer::SpaceEntitySystem EntitySystem { &Connection, &LogSystem };
    csp::SceneDescription SceneDescription { DeserializedValue, EntitySystem };

    EXPECT_EQ(SceneDescription.Entities.Size(), 0);
    EXPECT_EQ(SceneDescription.AssetCollections.Size(), 0);
    EXPECT_EQ(SceneDescription.Assets.Size(), 0);
    EXPECT_EQ(SceneDescription.Sequences.Size(), 0);
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

    mcs::SceneDescription DeserializedValue {};
    csp::json::JsonDeserializer::Deserialize(Json.c_str(), DeserializedValue);

    // Test the mcs scene description values are correct

    // Group
    csp::services::generated::userservice::GroupDto Group = DeserializedValue.Group;

    EXPECT_EQ(Group.GetId(), "66c65e8d9821e1cc2b51dc0d");
    EXPECT_EQ(Group.GetCreatedBy(), "66a0033d6541645960bfffda");
    EXPECT_EQ(Group.GetCreatedAt(), "2024-08-21T21:39:25.017+00:00");
    EXPECT_EQ(Group.GetGroupOwnerId(), "66a0033d6541645960bfffda");
    EXPECT_EQ(Group.GetGroupType(), "space");
    EXPECT_EQ(Group.GetName(), "Abu Dhabi Airport");
    EXPECT_EQ(Group.GetUsers().size(), 21);
    EXPECT_EQ(Group.GetUsers()[0], "66a0033d6541645960bfffda");
    EXPECT_EQ(Group.GetUsers()[20], "6823720f8f72b4d0fa153cfd");
    EXPECT_EQ(Group.GetBannedUsers().size(), 0);
    EXPECT_EQ(Group.GetModerators().size(), 20);
    EXPECT_EQ(Group.GetModerators()[0], "669ac6673d223b140719c19e");
    EXPECT_EQ(Group.GetModerators()[19], "6823720f8f72b4d0fa153cfd");
    EXPECT_EQ(Group.GetDiscoverable(), false);
    EXPECT_EQ(Group.GetAutoModerator(), false);
    EXPECT_EQ(Group.GetRequiresInvite(), true);
    EXPECT_EQ(Group.GetArchived(), false);
    EXPECT_EQ(Group.GetTags().size(), 0);
    EXPECT_EQ(Group.GetIsCurrentUserOwner(), false);
    EXPECT_EQ(Group.GetIsCurrentUserMember(), false);
    EXPECT_EQ(Group.GetIsCurrentUserModerator(), false);
    EXPECT_EQ(Group.GetIsCurrentUserBanned(), false);

    // Objects
    EXPECT_EQ(DeserializedValue.Objects.size(), 74);
    EXPECT_EQ(DeserializedValue.Objects[0].GetId(), 1484);
    EXPECT_EQ(DeserializedValue.Objects[0].GetType(), 2);
    EXPECT_EQ(DeserializedValue.Objects[0].GetIsTransferable(), true);
    EXPECT_EQ(DeserializedValue.Objects[0].GetIsPersistent(), true);
    // TODO: ownerid
    EXPECT_EQ(DeserializedValue.Objects[0].GetParentId(), std::nullopt);
    EXPECT_EQ(DeserializedValue.Objects[0].GetComponents()->size(), 25);
    EXPECT_EQ(std::get<std::string>(DeserializedValue.Objects[0].GetComponents()->find(64511)->second.GetValue()), "unavailableGates");

    // Prototypes
    EXPECT_EQ(DeserializedValue.Prototypes.size(), 88);

    csp::services::generated::prototypeservice::PrototypeDto Prototype = DeserializedValue.Prototypes[0];
    EXPECT_EQ(Prototype.GetName(), "Small_Kiosk_T.glb_536.4520788603719");
    EXPECT_EQ(Prototype.GetTags().size(), 0);
    EXPECT_EQ(Prototype.GetMetadata().size(), 1);
    EXPECT_EQ(Prototype.GetMetadata().find("assetType")->second, csp::common::String { "staticmodel" });
    EXPECT_EQ(Prototype.GetUiStrings().size(), 0);
    EXPECT_EQ(Prototype.GetState().size(), 0);
    EXPECT_EQ(Prototype.HasPointOfInterestId(), false);
    EXPECT_EQ(Prototype.HasParentId(), false);
    EXPECT_EQ(Prototype.GetHighlander(), false);
    EXPECT_EQ(Prototype.GetType(), "Default");
    EXPECT_EQ(Prototype.GetSystemOwned(), false);
    EXPECT_EQ(Prototype.HasPrototypeOwnerId(), false);
    EXPECT_EQ(Prototype.GetGroupIds().size(), 1);
    EXPECT_EQ(Prototype.GetGroupIds()[0], "66c65e8d9821e1cc2b51dc0d");
    EXPECT_EQ(Prototype.GetReadAccess().size(), 2);
    EXPECT_EQ(Prototype.GetReadAccess()[0], "GroupId:66c65e8d9821e1cc2b51dc0d");
    EXPECT_EQ(Prototype.GetReadAccess()[1], "SystemRoleGroup:SuperUsers");
    EXPECT_EQ(Prototype.GetWriteAccess().size(), 2);
    EXPECT_EQ(Prototype.GetWriteAccess()[0], "GroupId:66c65e8d9821e1cc2b51dc0d:GroupRole:creator");
    EXPECT_EQ(Prototype.GetWriteAccess()[1], "SystemRoleGroup:SuperUsers");
    EXPECT_EQ(Prototype.HasOrganizationId(), false);
    EXPECT_EQ(Prototype.GetCreatedBy(), "669ac6673d223b140719c19e");
    EXPECT_EQ(Prototype.GetCreatedAt(), "2025-04-22T17:52:15.369+00:00");
    EXPECT_EQ(Prototype.GetUpdatedBy(), "669ac6673d223b140719c19e");
    EXPECT_EQ(Prototype.GetUpdatedAt(), "2025-04-22T17:52:15.369+00:00");
    EXPECT_EQ(Prototype.GetId(), "6807d74f486cf8794b73f107");

    // AssetDetails
    EXPECT_EQ(DeserializedValue.AssetDetails.size(), 488);
    csp::services::generated::prototypeservice::AssetDetailDto AssetDetail = DeserializedValue.AssetDetails[0];

    EXPECT_EQ(AssetDetail.GetPrototypeId(), "6807d74f486cf8794b73f107");
    EXPECT_EQ(AssetDetail.HasFileName(), false);
    EXPECT_EQ(AssetDetail.GetName(), "Small_Kiosk_T_lod3.glb");
    EXPECT_EQ(AssetDetail.GetLanguageCode(), "en-us");
    EXPECT_EQ(AssetDetail.GetSupportedPlatforms().size(), 1);
    EXPECT_EQ(AssetDetail.GetSupportedPlatforms()[0], "Default");
    EXPECT_EQ(AssetDetail.GetStyle().size(), 2);
    EXPECT_EQ(AssetDetail.GetStyle()[0], "Default");
    EXPECT_EQ(AssetDetail.GetStyle()[1], "lod:3");
    EXPECT_EQ(AssetDetail.GetAssetType(), "Model");
    EXPECT_EQ(AssetDetail.HasThirdPartyReferenceId(), false);
    EXPECT_EQ(AssetDetail.HasUri(), false);
    EXPECT_EQ(AssetDetail.HasOriginalAssetUri(), false);
    EXPECT_EQ(AssetDetail.HasChecksum(), false);
    EXPECT_EQ(AssetDetail.HasVersion(), false);
    EXPECT_EQ(AssetDetail.HasMimeType(), false);
    EXPECT_EQ(AssetDetail.HasExternalUri(), false);
    EXPECT_EQ(AssetDetail.HasExternalMimeType(), false);
    EXPECT_EQ(AssetDetail.GetTags().size(), 1);
    EXPECT_EQ(AssetDetail.GetTags()[0], "lod:3");
    EXPECT_EQ(AssetDetail.GetSizeInBytes(), 0);
    EXPECT_EQ(AssetDetail.GetId(), "6807d750b618cc273309a258");

    // Sequences
    EXPECT_EQ(DeserializedValue.Sequences.size(), 0);

    // Convert to csp scene description
    csp::common::LogSystem LogSystem;
    csp::multiplayer::MultiplayerConnection Connection { &LogSystem };
    csp::multiplayer::SpaceEntitySystem EntitySystem { &Connection, &LogSystem };
    csp::SceneDescription SceneDescription { DeserializedValue, EntitySystem };

    // Test csp scene description values are correct

    // Space
    EXPECT_EQ(SceneDescription.Space.Id, "66c65e8d9821e1cc2b51dc0d");
    EXPECT_EQ(SceneDescription.Space.CreatedBy, "66a0033d6541645960bfffda");
    EXPECT_EQ(SceneDescription.Space.CreatedAt, "2024-08-21T21:39:25.017+00:00");
    EXPECT_EQ(SceneDescription.Space.OwnerId, "66a0033d6541645960bfffda");
    EXPECT_EQ(SceneDescription.Space.Name, "Abu Dhabi Airport");
    EXPECT_EQ(SceneDescription.Space.UserIds.Size(), 21);
    EXPECT_EQ(SceneDescription.Space.UserIds[0], "66a0033d6541645960bfffda");
    EXPECT_EQ(SceneDescription.Space.UserIds[20], "6823720f8f72b4d0fa153cfd");
    EXPECT_EQ(SceneDescription.Space.BannedUserIds.Size(), 0);
    EXPECT_EQ(SceneDescription.Space.ModeratorIds.Size(), 20);
    EXPECT_EQ(SceneDescription.Space.ModeratorIds[0], "669ac6673d223b140719c19e");
    EXPECT_EQ(SceneDescription.Space.ModeratorIds[19], "6823720f8f72b4d0fa153cfd");

    EXPECT_FALSE(csp::systems::HasFlag(SceneDescription.Space.Attributes, csp::systems::SpaceAttributes::IsDiscoverable));
    EXPECT_TRUE(csp::systems::HasFlag(SceneDescription.Space.Attributes, csp::systems::SpaceAttributes::RequiresInvite));

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
    EXPECT_EQ(SceneDescription.AssetCollections.Size(), 88);

    csp::systems::AssetCollection AssetCollection = SceneDescription.AssetCollections[0];
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
    EXPECT_EQ(SceneDescription.Assets.Size(), 488);

    csp::systems::Asset Asset = SceneDescription.Assets[0];
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
    EXPECT_EQ(SceneDescription.Sequences.Size(), 0);
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

    mcs::SceneDescription DeserializedValue {};
    csp::json::JsonDeserializer::Deserialize(Json.c_str(), DeserializedValue);

    // Test the mcs scene description values are correct

    // Group
    csp::services::generated::userservice::GroupDto Group = DeserializedValue.Group;

    EXPECT_EQ(Group.GetId(), "66c65e8d9821e1cc2b51dc0d");
    EXPECT_EQ(Group.GetCreatedBy(), "66a0033d6541645960bfffda");
    EXPECT_EQ(Group.GetCreatedAt(), "2024-08-21T21:39:25.017+00:00");
    EXPECT_EQ(Group.GetGroupOwnerId(), "66a0033d6541645960bfffda");
    EXPECT_EQ(Group.GetGroupType(), "space");
    EXPECT_EQ(Group.GetName(), "Abu Dhabi Airport");
    EXPECT_EQ(Group.GetUsers().size(), 0);
    EXPECT_EQ(Group.GetBannedUsers().size(), 0);
    EXPECT_EQ(Group.GetModerators().size(), 0);
    EXPECT_EQ(Group.GetDiscoverable(), false);
    EXPECT_EQ(Group.GetAutoModerator(), false);
    EXPECT_EQ(Group.GetRequiresInvite(), true);
    EXPECT_EQ(Group.GetArchived(), false);
    EXPECT_EQ(Group.GetTags().size(), 0);
    EXPECT_EQ(Group.GetIsCurrentUserOwner(), false);
    EXPECT_EQ(Group.GetIsCurrentUserMember(), false);
    EXPECT_EQ(Group.GetIsCurrentUserModerator(), false);
    EXPECT_EQ(Group.GetIsCurrentUserBanned(), false);

    // Objects
    EXPECT_EQ(DeserializedValue.Objects.size(), 1);
    EXPECT_EQ(DeserializedValue.Objects[0].GetId(), 1484);
    EXPECT_EQ(DeserializedValue.Objects[0].GetType(), 2);
    EXPECT_EQ(DeserializedValue.Objects[0].GetIsTransferable(), true);
    EXPECT_EQ(DeserializedValue.Objects[0].GetIsPersistent(), true);
    EXPECT_EQ(DeserializedValue.Objects[0].GetComponents()->size(), 1);
    // TODO: ownerid
    EXPECT_EQ(DeserializedValue.Objects[0].GetParentId(), std::nullopt);
    EXPECT_EQ(DeserializedValue.Objects[0].GetComponents()->size(), 1);

    // Prototypes
    EXPECT_EQ(DeserializedValue.Prototypes.size(), 1);

    csp::services::generated::prototypeservice::PrototypeDto Prototype = DeserializedValue.Prototypes[0];
    EXPECT_EQ(Prototype.GetName(), "Small_Kiosk_T.glb_536.4520788603719");
    EXPECT_EQ(Prototype.GetTags().size(), 0);
    EXPECT_EQ(Prototype.GetMetadata().size(), 1);
    EXPECT_EQ(Prototype.GetMetadata().find("assetType")->second, csp::common::String { "staticmodel" });
    EXPECT_EQ(Prototype.GetUiStrings().size(), 0);
    EXPECT_EQ(Prototype.GetState().size(), 0);
    EXPECT_EQ(Prototype.HasPointOfInterestId(), false);
    EXPECT_EQ(Prototype.HasParentId(), false);
    EXPECT_EQ(Prototype.GetHighlander(), false);
    EXPECT_EQ(Prototype.GetType(), "Default");
    EXPECT_EQ(Prototype.GetSystemOwned(), false);
    EXPECT_EQ(Prototype.HasPrototypeOwnerId(), false);
    EXPECT_EQ(Prototype.GetGroupIds().size(), 1);
    EXPECT_EQ(Prototype.GetGroupIds()[0], "66c65e8d9821e1cc2b51dc0d");
    EXPECT_EQ(Prototype.GetReadAccess().size(), 1);
    EXPECT_EQ(Prototype.GetReadAccess()[0], "GroupId:66c65e8d9821e1cc2b51dc0d");
    EXPECT_EQ(Prototype.GetWriteAccess().size(), 1);
    EXPECT_EQ(Prototype.GetWriteAccess()[0], "GroupId:66c65e8d9821e1cc2b51dc0d:GroupRole:creator");
    EXPECT_EQ(Prototype.HasOrganizationId(), false);
    EXPECT_EQ(Prototype.GetCreatedBy(), "669ac6673d223b140719c19e");
    EXPECT_EQ(Prototype.GetCreatedAt(), "2025-04-22T17:52:15.369+00:00");
    EXPECT_EQ(Prototype.GetUpdatedBy(), "669ac6673d223b140719c19e");
    EXPECT_EQ(Prototype.GetUpdatedAt(), "2025-04-22T17:52:15.369+00:00");
    EXPECT_EQ(Prototype.GetId(), "6807d74f486cf8794b73f107");

    // AssetDetails
    EXPECT_EQ(DeserializedValue.AssetDetails.size(), 1);
    csp::services::generated::prototypeservice::AssetDetailDto AssetDetail = DeserializedValue.AssetDetails[0];

    EXPECT_EQ(AssetDetail.GetPrototypeId(), "6807d74f486cf8794b73f107");
    EXPECT_EQ(AssetDetail.HasFileName(), false);
    EXPECT_EQ(AssetDetail.GetName(), "Small_Kiosk_T_lod3.glb");
    EXPECT_EQ(AssetDetail.GetLanguageCode(), "en-us");
    EXPECT_EQ(AssetDetail.GetSupportedPlatforms().size(), 1);
    EXPECT_EQ(AssetDetail.GetSupportedPlatforms()[0], "Default");
    EXPECT_EQ(AssetDetail.GetStyle().size(), 1);
    EXPECT_EQ(AssetDetail.GetStyle()[0], "Default");
    EXPECT_EQ(AssetDetail.GetAssetType(), "Model");
    EXPECT_EQ(AssetDetail.HasThirdPartyReferenceId(), false);
    EXPECT_EQ(AssetDetail.HasUri(), false);
    EXPECT_EQ(AssetDetail.HasOriginalAssetUri(), false);
    EXPECT_EQ(AssetDetail.HasChecksum(), false);
    EXPECT_EQ(AssetDetail.HasVersion(), false);
    EXPECT_EQ(AssetDetail.HasMimeType(), false);
    EXPECT_EQ(AssetDetail.HasExternalUri(), false);
    EXPECT_EQ(AssetDetail.HasExternalMimeType(), false);
    EXPECT_EQ(AssetDetail.GetTags().size(), 1);
    EXPECT_EQ(AssetDetail.GetTags()[0], "lod:3");
    EXPECT_EQ(AssetDetail.GetSizeInBytes(), 0);
    EXPECT_EQ(AssetDetail.GetId(), "6807d750b618cc273309a258");

    // Sequences
    EXPECT_EQ(DeserializedValue.Sequences.size(), 0);

    // Convert to csp scene description
    csp::common::LogSystem LogSystem;
    csp::multiplayer::MultiplayerConnection Connection { &LogSystem };
    csp::multiplayer::SpaceEntitySystem EntitySystem { &Connection, &LogSystem };
    csp::SceneDescription SceneDescription { DeserializedValue, EntitySystem };

    // Test csp scene description values are correct

    // Space
    EXPECT_EQ(SceneDescription.Space.Id, "66c65e8d9821e1cc2b51dc0d");
    EXPECT_EQ(SceneDescription.Space.CreatedBy, "66a0033d6541645960bfffda");
    EXPECT_EQ(SceneDescription.Space.CreatedAt, "2024-08-21T21:39:25.017+00:00");
    EXPECT_EQ(SceneDescription.Space.OwnerId, "66a0033d6541645960bfffda");
    EXPECT_EQ(SceneDescription.Space.Name, "Abu Dhabi Airport");
    EXPECT_EQ(SceneDescription.Space.UserIds.Size(), 0);
    EXPECT_EQ(SceneDescription.Space.BannedUserIds.Size(), 0);
    EXPECT_EQ(SceneDescription.Space.ModeratorIds.Size(), 0);

    EXPECT_FALSE(csp::systems::HasFlag(SceneDescription.Space.Attributes, csp::systems::SpaceAttributes::IsDiscoverable));
    EXPECT_TRUE(csp::systems::HasFlag(SceneDescription.Space.Attributes, csp::systems::SpaceAttributes::RequiresInvite));

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
    EXPECT_EQ(SceneDescription.AssetCollections.Size(), 1);

    csp::systems::AssetCollection AssetCollection = SceneDescription.AssetCollections[0];
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
    EXPECT_EQ(SceneDescription.Assets.Size(), 1);

    csp::systems::Asset Asset = SceneDescription.Assets[0];
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
    EXPECT_EQ(SceneDescription.Sequences.Size(), 0);
}
