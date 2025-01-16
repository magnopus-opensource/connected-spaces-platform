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
#include "AssetSystemTestHelpers.h"
#include "Awaitable.h"
#include "CSP/CSPFoundation.h"
#include "CSP/Common/Array.h"
#include "CSP/Common/Optional.h"
#include "CSP/Multiplayer/Components/CustomSpaceComponent.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Systems/Spatial/AnchorSystem.h"
#include "CSP/Systems/Spatial/SpatialDataTypes.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "SpaceSystemTestHelpers.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"
#include "signalrclient/signalr_value.h"

#include "gtest/gtest.h"

namespace
{

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

void CreateAnchor(csp::systems::AnchorSystem* AnchorSystem, const csp::common::String& AssetCollectionId,
    const csp::common::Optional<csp::systems::GeoLocation>& Location, csp::systems::Anchor& OutAnchor,
    const csp::common::Optional<csp::common::Map<csp::common::String, csp::common::String>>& SpatialKeyValue,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& Tags)
{
    char UniqueThirdPartyAnchorId[256];
    SPRINTF(UniqueThirdPartyAnchorId, "OLY-UNITTEST-ID-%s", GetUniqueString().c_str());

    auto AnchorPosition = csp::systems::OlyAnchorPosition(100.0, 100.0, 100.0);
    auto AnchorRotation = csp::systems::OlyRotation(100.0, 100.0, 100.0, 100.0);
    auto AnchorLocation = Location.HasValue() ? *Location : csp::systems::GeoLocation(180.0, 90.0);
    auto AnchorTags = Tags.HasValue() ? *Tags : csp::common::Array<csp::common::String> { "Test1", "Test2" };
    auto AnchorSpatialKeyValue = SpatialKeyValue.HasValue()
        ? *SpatialKeyValue
        : csp::common::Map<csp::common::String, csp::common::String> { { "TestKey1", "TestValue1" }, { "TestKey2", "TestValue2" } };

    auto [Result] = Awaitable(&csp::systems::AnchorSystem::CreateAnchor, AnchorSystem, csp::systems::AnchorProvider::GoogleCloudAnchors,
        UniqueThirdPartyAnchorId, AssetCollectionId, AnchorLocation, AnchorPosition, AnchorRotation, AnchorSpatialKeyValue, AnchorTags)
                        .Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    if (Result.GetResultCode() == csp::systems::EResultCode::Success)
    {
        OutAnchor = Result.GetAnchor();
        std::cerr << "Anchor Created: Id=" << OutAnchor.Id << std::endl;
    }
}

void CreateAnchorInSpace(csp::systems::AnchorSystem* AnchorSystem, const csp::common::String& SpaceId, uint64_t SpaceEntityId,
    const csp::common::String& AssetCollectionId, const csp::common::Optional<csp::systems::GeoLocation>& Location, csp::systems::Anchor& OutAnchor,
    const csp::common::Optional<csp::common::Map<csp::common::String, csp::common::String>>& SpatialKeyValue,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& Tags)
{
    char UniqueThirdPartyAnchorId[256];
    SPRINTF(UniqueThirdPartyAnchorId, "OLY-UNITTEST-ID-%s", GetUniqueString().c_str());

    auto AnchorPosition = csp::systems::OlyAnchorPosition(100.0, 100.0, 100.0);
    auto AnchorRotation = csp::systems::OlyRotation(100.0, 100.0, 100.0, 100.0);
    auto AnchorLocation = Location.HasValue() ? *Location : csp::systems::GeoLocation(180.0, 90.0);
    auto AnchorTags = Tags.HasValue() ? *Tags : csp::common::Array<csp::common::String> { "Test1", "Test2" };
    auto AnchorSpatialKeyValue = SpatialKeyValue.HasValue()
        ? *SpatialKeyValue
        : csp::common::Map<csp::common::String, csp::common::String> { { "TestKey1", "TestValue1" }, { "TestKey2", "TestValue2" } };

    auto [Result] = Awaitable(&csp::systems::AnchorSystem::CreateAnchorInSpace, AnchorSystem, csp::systems::AnchorProvider::GoogleCloudAnchors,
        UniqueThirdPartyAnchorId, SpaceId, SpaceEntityId, AssetCollectionId, AnchorLocation, AnchorPosition, AnchorRotation, AnchorSpatialKeyValue,
        AnchorTags)
                        .Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    if (Result.GetResultCode() == csp::systems::EResultCode::Success)
    {
        OutAnchor = Result.GetAnchor();
        std::cerr << "Anchor Created: Id=" << OutAnchor.Id << std::endl;
    }
}

void DeleteAnchors(csp::systems::AnchorSystem* AnchorSystem, const csp::common::Array<csp::common::String>& AnchorIDs)
{
    auto [Result] = Awaitable(&csp::systems::AnchorSystem::DeleteAnchors, AnchorSystem, AnchorIDs).Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    if (Result.GetResultCode() == csp::systems::EResultCode::Success)
    {
        for (int idx = 0; idx < AnchorIDs.Size(); idx++)
        {
            std::cerr << "Anchor Deleted: "
                      << "Id=" << AnchorIDs[idx] << std::endl;
        }
    }
}

void CreateAnchorResolution(
    csp::systems::AnchorSystem* AnchorSystem, const csp::common::String& AnchorId, csp::systems::AnchorResolution& OutAnchorResolution)
{
    const bool SuccessfullyResolved = true;
    const int ResolveAttempted = 3;
    const double ResolveTime = 1000;
    const csp::common::String TestTag = "TestTag";
    csp::common::Array<csp::common::String> Tags { TestTag };

    auto [Result] = Awaitable(
        &csp::systems::AnchorSystem::CreateAnchorResolution, AnchorSystem, AnchorId, SuccessfullyResolved, ResolveAttempted, ResolveTime, Tags)
                        .Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    const auto& AnchorResolution = Result.GetAnchorResolution();

    EXPECT_FALSE(AnchorResolution.Id.IsEmpty());
    EXPECT_EQ(AnchorResolution.AnchorId, AnchorId);
    EXPECT_EQ(AnchorResolution.SuccessfullyResolved, SuccessfullyResolved);
    EXPECT_EQ(AnchorResolution.ResolveAttempted, ResolveAttempted);
    EXPECT_EQ(AnchorResolution.Tags.Size(), 1);
    EXPECT_EQ(AnchorResolution.Tags[0], TestTag);

    OutAnchorResolution = AnchorResolution;
}

} // namespace

#if RUN_ALL_UNIT_TESTS || RUN_ANCHORSYSTEM_TESTS || RUN_ANCHORSYSTEM_CREATE_ANCHOR_TEST
CSP_PUBLIC_TEST(CSPEngine, AnchorSystemTests, CreateAnchorTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* AnchorSystem = SystemsManager.GetAnchorSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
    const char* TestAssetCollectionName = "OLY-UNITTEST-ASSET-COLLECTION-REWIND";

    char UniqueAssetCollectionName[256];
    SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    csp::systems::AssetCollection AssetCollection;
    CreateAssetCollection(
        AssetSystem, nullptr, nullptr, UniqueAssetCollectionName, csp::systems::EAssetCollectionType::DEFAULT, nullptr, AssetCollection);

    csp::systems::Anchor Anchor;
    CreateAnchor(AnchorSystem, AssetCollection.Id, nullptr, Anchor, nullptr, nullptr);
    csp::common::Array<csp::common::String> CreatedAnchorIds(1);
    CreatedAnchorIds[0] = Anchor.Id;

    EXPECT_EQ(Anchor.ThirdPartyAnchorProvider, csp::systems::AnchorProvider::GoogleCloudAnchors);
    EXPECT_TRUE(Anchor.SpaceId.IsEmpty());
    EXPECT_EQ(Anchor.AssetCollectionId, AssetCollection.Id);

    DeleteAnchors(AnchorSystem, CreatedAnchorIds);
    DeleteAssetCollection(AssetSystem, AssetCollection);
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ANCHORSYSTEM_TESTS || RUN_ANCHORSYSTEM_CREATE_ANCHOR_IN_SPACE_TEST
CSP_PUBLIC_TEST(CSPEngine, AnchorSystemTests, CreateAnchorInSpaceTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* AnchorSystem = SystemsManager.GetAnchorSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
    const char* TestAssetCollectionName = "OLY-UNITTEST-ASSET-COLLECTION-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());
    char UniqueAssetCollectionName[256];
    SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    csp::common::String ObjectName = "Object 1";
    csp::multiplayer::SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

    csp::systems::AssetCollection AssetCollection;
    CreateAssetCollection(
        AssetSystem, Space.Id, nullptr, UniqueAssetCollectionName, csp::systems::EAssetCollectionType::DEFAULT, nullptr, AssetCollection);

    csp::systems::Anchor Anchor;
    CreateAnchorInSpace(AnchorSystem, Space.Id, CreatedObject->GetId(), AssetCollection.Id, nullptr, Anchor, nullptr, nullptr);
    csp::common::Array<csp::common::String> CreatedAnchorIds(1);
    CreatedAnchorIds[0] = Anchor.Id;

    EXPECT_EQ(Anchor.ThirdPartyAnchorProvider, csp::systems::AnchorProvider::GoogleCloudAnchors);
    EXPECT_EQ(Anchor.SpaceId, Space.Id);
    EXPECT_EQ(Anchor.SpaceEntityId, CreatedObject->GetId());
    EXPECT_EQ(Anchor.AssetCollectionId, AssetCollection.Id);

    DeleteAnchors(AnchorSystem, CreatedAnchorIds);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    DeleteAssetCollection(AssetSystem, AssetCollection);
    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ANCHORSYSTEM_TESTS || RUN_ANCHORSYSTEM_DELETE_MULTIPLE_ANCHORS_TEST
CSP_PUBLIC_TEST(CSPEngine, AnchorSystemTests, DeleteMultipleAnchorsTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* AnchorSystem = SystemsManager.GetAnchorSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
    const char* TestAssetCollectionName = "OLY-UNITTEST-ASSET-COLLECTION-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());
    char UniqueAssetCollectionName1[256];
    SPRINTF(UniqueAssetCollectionName1, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());
    char UniqueAssetCollectionName2[256];
    SPRINTF(UniqueAssetCollectionName2, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    csp::multiplayer::SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };

    csp::common::String ObjectName1 = "Object 1";
    auto [CreatedObject1] = AWAIT(EntitySystem, CreateObject, ObjectName1, ObjectTransform);
    csp::common::String ObjectName2 = "Object 2";
    auto [CreatedObject2] = AWAIT(EntitySystem, CreateObject, ObjectName2, ObjectTransform);

    csp::systems::AssetCollection AssetCollection1;
    CreateAssetCollection(
        AssetSystem, Space.Id, nullptr, UniqueAssetCollectionName1, csp::systems::EAssetCollectionType::DEFAULT, nullptr, AssetCollection1);
    csp::systems::AssetCollection AssetCollection2;
    CreateAssetCollection(
        AssetSystem, Space.Id, nullptr, UniqueAssetCollectionName2, csp::systems::EAssetCollectionType::DEFAULT, nullptr, AssetCollection2);

    csp::common::Array<csp::common::String> CreatedAnchorIds(2);
    csp::systems::Anchor Anchor;

    CreateAnchorInSpace(AnchorSystem, Space.Id, CreatedObject1->GetId(), AssetCollection1.Id, nullptr, Anchor, nullptr, nullptr);
    CreatedAnchorIds[0] = Anchor.Id;

    CreateAnchorInSpace(AnchorSystem, Space.Id, CreatedObject2->GetId(), AssetCollection2.Id, nullptr, Anchor, nullptr, nullptr);
    CreatedAnchorIds[1] = Anchor.Id;

    auto [PreDeleteGetResult]
        = Awaitable(&csp::systems::AnchorSystem::GetAnchorsInSpace, AnchorSystem, Space.Id, nullptr, nullptr).Await(RequestPredicate);
    EXPECT_EQ(PreDeleteGetResult.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(PreDeleteGetResult.GetAnchors().Size(), 2);

    DeleteAnchors(AnchorSystem, CreatedAnchorIds);

    auto [PostDeleteGetResult]
        = Awaitable(&csp::systems::AnchorSystem::GetAnchorsInSpace, AnchorSystem, Space.Id, nullptr, nullptr).Await(RequestPredicate);
    EXPECT_EQ(PostDeleteGetResult.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(PostDeleteGetResult.GetAnchors().Size(), 0);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    DeleteAssetCollection(AssetSystem, AssetCollection1);
    DeleteAssetCollection(AssetSystem, AssetCollection2);
    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ANCHORSYSTEM_TESTS || RUN_ANCHORSYSTEM_GET_ANCHORS_INSIDE_CIRCULAR_AREA_TEST
CSP_PUBLIC_TEST(CSPEngine, AnchorSystemTests, GetAnchorsInsideCircularAreaTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* AnchorSystem = SystemsManager.GetAnchorSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
    const char* TestAssetCollectionName = "OLY-UNITTEST-ASSET-COLLECTION-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());
    char UniqueAssetCollectionName[256];
    SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    csp::systems::Space Space;
    csp::common::Array<csp::common::String> SpaceId(1);
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);
    SpaceId[0] = Space.Id;

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    csp::common::String ObjectName = "Object 1";
    csp::multiplayer::SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

    csp::systems::AssetCollection AssetCollection;
    CreateAssetCollection(
        AssetSystem, Space.Id, nullptr, UniqueAssetCollectionName, csp::systems::EAssetCollectionType::DEFAULT, nullptr, AssetCollection);

    csp::systems::GeoLocation AnchorLocation;
    AnchorLocation.Latitude = 45.0;
    AnchorLocation.Longitude = 160.0;

    csp::systems::Anchor Anchor;
    CreateAnchorInSpace(AnchorSystem, Space.Id, CreatedObject->GetId(), AssetCollection.Id, AnchorLocation, Anchor, nullptr, nullptr);

    // Search for the newly created Anchor inside a circular area
    csp::common::Array<csp::systems::Anchor> AnchorCollection;

    csp::systems::GeoLocation SearchLocationOrigin;
    SearchLocationOrigin.Latitude = 44.0;
    SearchLocationOrigin.Longitude = 160.0;
    double SearchRadius = 130000.0;
    const csp::common::Array<csp::common::String> Tags = { "Test1", "Test2" };
    const csp::common::Array<csp::common::String> SpacialKeys = { "TestKey1", "TestKey2" };
    const csp::common::Array<csp::common::String> SpacialValues = { "TestValue1", "TestValue2" };

    auto [Result] = Awaitable(&csp::systems::AnchorSystem::GetAnchorsInArea, AnchorSystem, SearchLocationOrigin, SearchRadius, SpacialKeys,
        SpacialValues, Tags, true, SpaceId, nullptr, nullptr)
                        .Await(RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    if (Result.GetResultCode() == csp::systems::EResultCode::Success)
    {
        const auto& ResultAnchors = Result.GetAnchors();
        AnchorCollection = csp::common::Array<csp::systems::Anchor>(ResultAnchors.Size());

        for (int idx = 0; idx < ResultAnchors.Size(); ++idx)
        {
            AnchorCollection[idx] = ResultAnchors[idx];
        }

        // we should have at least the Anchor we've created
        EXPECT_GT(AnchorCollection.Size(), 0);

        bool AnchorFound = false;

        for (size_t idx = 0; idx < AnchorCollection.Size(); ++idx)
        {
            if (AnchorCollection[idx].Id == Anchor.Id)
            {
                auto ReturnSpatialKeyValue = AnchorCollection[idx].SpatialKeyValue;
                EXPECT_EQ(ReturnSpatialKeyValue.Size(), SpacialValues.Size());

                for (size_t i = 0; i < ReturnSpatialKeyValue.Size(); ++i)
                {
                    EXPECT_TRUE(ReturnSpatialKeyValue.HasKey(SpacialKeys[i]));
                    EXPECT_EQ(ReturnSpatialKeyValue[SpacialKeys[i]], SpacialValues[i]);
                }

                auto ReturnTags = AnchorCollection[idx].Tags;
                EXPECT_EQ(ReturnTags.Size(), Tags.Size());

                for (size_t i = 0; i < ReturnTags.Size(); ++i)
                {
                    EXPECT_EQ(ReturnTags[i], Tags[i]);
                }

                AnchorFound = true;
                break;
            }
        }

        EXPECT_TRUE(AnchorFound);
    }
    else
    {
        std::cerr << "GetAnchorsInArea failed with ResultCode: " << (int)Result.GetResultCode() << " HttpResultCode: " << Result.GetHttpResultCode()
                  << std::endl;
    }

    csp::common::Array<csp::common::String> CreatedAnchorIds(1);
    CreatedAnchorIds[0] = Anchor.Id;
    DeleteAnchors(AnchorSystem, CreatedAnchorIds);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    DeleteAssetCollection(AssetSystem, AssetCollection);
    DeleteSpace(SpaceSystem, Space.Id);

    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ANCHORSYSTEM_TESTS || RUN_ANCHORSYSTEM_GETANCHORSINSPACE_TEST
CSP_PUBLIC_TEST(CSPEngine, AnchorSystemTests, GetAnchorsInSpaceTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* AnchorSystem = SystemsManager.GetAnchorSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
    const char* TestAssetCollectionName = "OLY-UNITTEST-ASSET-COLLECTION-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());
    char UniqueAssetCollectionName1[256];
    SPRINTF(UniqueAssetCollectionName1, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());
    char UniqueAssetCollectionName2[256];
    SPRINTF(UniqueAssetCollectionName2, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    csp::multiplayer::SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };

    csp::common::String ObjectName1 = "Object 1";
    auto [CreatedObject1] = AWAIT(EntitySystem, CreateObject, ObjectName1, ObjectTransform);
    csp::common::String ObjectName2 = "Object 2";
    auto [CreatedObject2] = AWAIT(EntitySystem, CreateObject, ObjectName2, ObjectTransform);

    csp::systems::AssetCollection AssetCollection1;
    CreateAssetCollection(
        AssetSystem, Space.Id, nullptr, UniqueAssetCollectionName1, csp::systems::EAssetCollectionType::DEFAULT, nullptr, AssetCollection1);
    csp::systems::AssetCollection AssetCollection2;
    CreateAssetCollection(
        AssetSystem, Space.Id, nullptr, UniqueAssetCollectionName2, csp::systems::EAssetCollectionType::DEFAULT, nullptr, AssetCollection2);

    csp::common::Array<csp::common::String> CreatedAnchorIds(2);
    csp::systems::Anchor Anchor1;
    CreateAnchorInSpace(AnchorSystem, Space.Id, CreatedObject1->GetId(), AssetCollection1.Id, nullptr, Anchor1, nullptr, nullptr);
    CreatedAnchorIds[0] = Anchor1.Id;

    csp::systems::Anchor Anchor2;
    CreateAnchorInSpace(AnchorSystem, Space.Id, CreatedObject2->GetId(), AssetCollection2.Id, nullptr, Anchor2, nullptr, nullptr);
    CreatedAnchorIds[1] = Anchor2.Id;

    auto [Result] = Awaitable(&csp::systems::AnchorSystem::GetAnchorsInSpace, AnchorSystem, Space.Id, nullptr, nullptr).Await(RequestPredicate);
    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    auto Anchors = Result.GetAnchors();
    EXPECT_EQ(Anchors.Size(), 2);

    int AnchorsFound = 0;
    for (size_t i = 0; i < Anchors.Size(); ++i)
    {
        EXPECT_EQ(Anchors[i].SpaceId, Space.Id);
        if (Anchors[i].ThirdPartyAnchorId == Anchor1.ThirdPartyAnchorId || Anchors[i].ThirdPartyAnchorId == Anchor2.ThirdPartyAnchorId)
        {
            ++AnchorsFound;
        }
    }
    EXPECT_EQ(AnchorsFound, 2);

    DeleteAnchors(AnchorSystem, CreatedAnchorIds);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    DeleteAssetCollection(AssetSystem, AssetCollection1);
    DeleteAssetCollection(AssetSystem, AssetCollection2);
    DeleteSpace(SpaceSystem, Space.Id);
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ANCHORSYSTEM_TESTS || RUN_ANCHORSYSTEM_GETANCHORSBYASSETCOLLECTIONID_TEST
CSP_PUBLIC_TEST(CSPEngine, AnchorSystemTests, GetAnchorsByAssetCollectionIdTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* AnchorSystem = SystemsManager.GetAnchorSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", "OLY-UNITTEST-SPACE-REWIND", GetUniqueString().c_str());
    char UniqueAssetCollectionName[256];
    SPRINTF(UniqueAssetCollectionName, "%s-%s", "OLY-UNITTEST-ASSET-COLLECTION-REWIND", GetUniqueString().c_str());

    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    csp::systems::AssetCollection AssetCollection;
    CreateAssetCollection(
        AssetSystem, nullptr, nullptr, UniqueAssetCollectionName, csp::systems::EAssetCollectionType::DEFAULT, nullptr, AssetCollection);

    csp::systems::Anchor Anchor1;
    CreateAnchor(AnchorSystem, AssetCollection.Id, nullptr, Anchor1, nullptr, nullptr);

    csp::systems::Anchor Anchor2;
    CreateAnchor(AnchorSystem, AssetCollection.Id, nullptr, Anchor2, nullptr, nullptr);

    // Get and validate anchors
    {
        auto [Result] = AWAIT_PRE(AnchorSystem, GetAnchorsByAssetCollectionId, RequestPredicate, AssetCollection.Id, nullptr, nullptr);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

        const auto& Anchors = Result.GetAnchors();

        EXPECT_EQ(Anchors.Size(), 2);

        bool Found1 = false, Found2 = false;

        for (auto i = 0; i < Anchors.Size(); ++i)
        {
            if (Anchors[i].Id == Anchor1.Id)
            {
                Found1 = true;
            }
            else if (Anchors[i].Id == Anchor2.Id)
            {
                Found2 = true;
            }
        }

        EXPECT_TRUE(Found1 && Found2);
    }

    DeleteAnchors(AnchorSystem, { Anchor1.Id, Anchor2.Id });
    DeleteAssetCollection(AssetSystem, AssetCollection);
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ANCHORSYSTEM_TESTS || RUN_ANCHORSYSTEM_CREATE_ANCHOR_RESOLUTION_TEST
CSP_PUBLIC_TEST(CSPEngine, AnchorSystemTests, CreateAnchorResolutionTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* AnchorSystem = SystemsManager.GetAnchorSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* AssetSystem = SystemsManager.GetAssetSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();
    auto* EntitySystem = SystemsManager.GetSpaceEntitySystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";
    const char* TestAssetCollectionName = "OLY-UNITTEST-ASSET-COLLECTION-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());
    char UniqueAssetCollectionName[256];
    SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    EntitySystem->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* Entity) {});

    csp::common::String ObjectName = "Object 1";
    csp::multiplayer::SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(EntitySystem, CreateObject, ObjectName, ObjectTransform);

    csp::systems::AssetCollection AssetCollection;
    CreateAssetCollection(
        AssetSystem, Space.Id, nullptr, UniqueAssetCollectionName, csp::systems::EAssetCollectionType::DEFAULT, nullptr, AssetCollection);

    // Create Anchor
    csp::systems::Anchor Anchor;
    CreateAnchorInSpace(AnchorSystem, Space.Id, CreatedObject->GetId(), AssetCollection.Id, nullptr, Anchor, nullptr, nullptr);
    csp::common::Array<csp::common::String> CreatedAnchorIds(1);
    CreatedAnchorIds[0] = Anchor.Id;

    // Create AnchorResolution
    csp::systems::AnchorResolution AnchorResolution;
    CreateAnchorResolution(AnchorSystem, Anchor.Id, AnchorResolution);

    // Cleanup
    DeleteAnchors(AnchorSystem, CreatedAnchorIds);
    DeleteAssetCollection(AssetSystem, AssetCollection);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    DeleteSpace(SpaceSystem, Space.Id);

    LogOut(UserSystem);
}
#endif