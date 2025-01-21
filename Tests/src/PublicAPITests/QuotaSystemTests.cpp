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
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Systems/Quota/QuotaSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "SpaceSystemTestHelpers.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"

#include "gtest/gtest.h"
#include <filesystem>
using namespace csp::systems;

namespace
{

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

#if RUN_ALL_UNIT_TESTS || RUN_QUOTASYSTEM_TESTS || RUN_QUOTASYSTEM_QUERY_TEST
CSP_PUBLIC_TEST(CSPEngine, QuotaSystemTests, TierNameEnumTesttoStringTest)
{
    EXPECT_EQ(TierNameEnumToString(csp::systems::TierNames::Basic), "basic");
    EXPECT_EQ(TierNameEnumToString(csp::systems::TierNames::Premium), "premium");
    EXPECT_EQ(TierNameEnumToString(csp::systems::TierNames::Pro), "pro");
    EXPECT_EQ(TierNameEnumToString(csp::systems::TierNames::Enterprise), "enterprise");
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_QUOTASYSTEM_TESTS || RUN_QUOTASYSTEM_QUERY_TEST
CSP_PUBLIC_TEST(CSPEngine, QuotaSystemTests, StringToTierNameEnumTestTest)
{
    EXPECT_EQ(StringToTierNameEnum("basic"), csp::systems::TierNames::Basic);
    EXPECT_EQ(StringToTierNameEnum("premium"), csp::systems::TierNames::Premium);
    EXPECT_EQ(StringToTierNameEnum("pro"), csp::systems::TierNames::Pro);
    EXPECT_EQ(StringToTierNameEnum("enterprise"), csp::systems::TierNames::Enterprise);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_QUOTASYSTEM_TESTS || RUN_QUOTASYSTEM_QUERY_TEST
CSP_PUBLIC_TEST(CSPEngine, QuotaSystemTests, TierFeatureEnumTesttoStringTest)
{
    EXPECT_EQ(TierFeatureEnumToString(csp::systems::TierFeatures::Agora), "Agora");
    EXPECT_EQ(TierFeatureEnumToString(csp::systems::TierFeatures::AudioVideoUpload), "AudioVideoUpload");
    EXPECT_EQ(TierFeatureEnumToString(csp::systems::TierFeatures::ObjectCaptureUpload), "ObjectCaptureUpload");
    EXPECT_EQ(TierFeatureEnumToString(csp::systems::TierFeatures::OpenAI), "OpenAI");
    EXPECT_EQ(TierFeatureEnumToString(csp::systems::TierFeatures::ScopeConcurrentUsers), "ScopeConcurrentUsers");
    EXPECT_EQ(TierFeatureEnumToString(csp::systems::TierFeatures::Shopify), "Shopify");
    EXPECT_EQ(TierFeatureEnumToString(csp::systems::TierFeatures::SpaceOwner), "SpaceOwner");
    EXPECT_EQ(TierFeatureEnumToString(csp::systems::TierFeatures::TicketedSpace), "TicketedSpace");
    EXPECT_EQ(TierFeatureEnumToString(csp::systems::TierFeatures::TotalUploadSizeInKilobytes), "TotalUploadSizeInKilobytes");
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_QUOTASYSTEM_TESTS || RUN_QUOTASYSTEM_STRINGTOTIERFEATUREENUM_TEST
CSP_PUBLIC_TEST(CSPEngine, QuotaSystemTests, StringToTierFeatureEnumTestTest)
{
    EXPECT_EQ(StringToTierFeatureEnum("Agora"), csp::systems::TierFeatures::Agora);
    EXPECT_EQ(StringToTierFeatureEnum("AudioVideoUpload"), csp::systems::TierFeatures::AudioVideoUpload);
    EXPECT_EQ(StringToTierFeatureEnum("ObjectCaptureUpload"), csp::systems::TierFeatures::ObjectCaptureUpload);
    EXPECT_EQ(StringToTierFeatureEnum("OpenAI"), csp::systems::TierFeatures::OpenAI);
    EXPECT_EQ(StringToTierFeatureEnum("ScopeConcurrentUsers"), csp::systems::TierFeatures::ScopeConcurrentUsers);
    EXPECT_EQ(StringToTierFeatureEnum("Shopify"), csp::systems::TierFeatures::Shopify);
    EXPECT_EQ(StringToTierFeatureEnum("SpaceOwner"), csp::systems::TierFeatures::SpaceOwner);
    EXPECT_EQ(StringToTierFeatureEnum("TicketedSpace"), csp::systems::TierFeatures::TicketedSpace);
    EXPECT_EQ(StringToTierFeatureEnum("TotalUploadSizeInKilobytes"), csp::systems::TierFeatures::TotalUploadSizeInKilobytes);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_QUOTASYSTEM_TESTS || RUN_QUOTASYSTEM_GETTOTALSPACESOWNEDBYUSER_TEST
CSP_PUBLIC_TEST(CSPEngine, QuotaSystemTests, GetTotalSpacesOwnedByUserTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto UserSystem = SystemsManager.GetUserSystem();
    auto QuotaSystem = SystemsManager.GetQuotaSystem();

    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    auto [Result] = AWAIT_PRE(QuotaSystem, GetTotalSpacesOwnedByUser, RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(Result.GetFeatureLimitInfo().ActivityCount, 0);
    EXPECT_EQ(Result.GetFeatureLimitInfo().Limit, -1);
    EXPECT_EQ(Result.GetFeatureLimitInfo().FeatureName, csp::systems::TierFeatures::SpaceOwner);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_QUOTASYSTEM_TESTS || RUN_QUOTASYSTEM_GETCURRENTUSERTIER_TEST
CSP_PUBLIC_TEST(CSPEngine, QuotaSystemTests, GetCurrentUserTierTest)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto UserSystem = SystemsManager.GetUserSystem();
    auto QuotaSystem = SystemsManager.GetQuotaSystem();

    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    auto [Result] = AWAIT_PRE(QuotaSystem, GetCurrentUserTier, RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(Result.GetUserTierInfo().TierName, TierNames::Pro);
    EXPECT_EQ(Result.GetUserTierInfo().AssignToId, UserId);
    EXPECT_EQ(Result.GetUserTierInfo().AssignToType, "user");
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_QUOTASYSTEM_TESTS || RUN_QUOTASYSTEM_GETTIERFEATUREPROGRESSFORUSER_TEST
CSP_PUBLIC_TEST(CSPEngine, QuotaSystemTests, GetTierFeatureProgressForUser)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto UserSystem = SystemsManager.GetUserSystem();
    auto QuotaSystem = SystemsManager.GetQuotaSystem();

    csp::common::Array<TierFeatures> TierFeaturesArray = { TierFeatures::SpaceOwner, TierFeatures::ObjectCaptureUpload,
        TierFeatures::AudioVideoUpload, TierFeatures::OpenAI, TierFeatures::TicketedSpace };
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    auto [Result] = AWAIT_PRE(QuotaSystem, GetTierFeatureProgressForUser, RequestPredicate, TierFeaturesArray);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(Result.GetFeaturesLimitInfo().Size(), TierFeaturesArray.Size());

    for (int i = 0; i < TierFeaturesArray.Size(); i++)
    {
        EXPECT_EQ(Result.GetFeaturesLimitInfo()[i].FeatureName, TierFeaturesArray[i]);
        EXPECT_EQ(Result.GetFeaturesLimitInfo()[i].Limit, -1);
        EXPECT_EQ(Result.GetFeaturesLimitInfo()[i].ActivityCount, 0);
    }
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_QUOTASYSTEM_TESTS || RUN_QUOTASYSTEM_GETTIERFEATUREPROGRESSFORSPACE_TEST
CSP_PUBLIC_TEST(CSPEngine, QuotaSystemTests, GetTierFeatureProgressForSpace)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto UserSystem = SystemsManager.GetUserSystem();
    auto QuotaSystem = SystemsManager.GetQuotaSystem();
    auto SpaceSystem = SystemsManager.GetSpaceSystem();

    csp::common::Array<TierFeatures> TierFeaturesArray = { TierFeatures::SpaceOwner, TierFeatures::ObjectCaptureUpload,
        TierFeatures::AudioVideoUpload, TierFeatures::OpenAI, TierFeatures::TicketedSpace };

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    ::Space Space;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    auto [Result] = AWAIT_PRE(QuotaSystem, GetTierFeatureProgressForSpace, RequestPredicate, Space.Id, TierFeaturesArray);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(Result.GetFeaturesLimitInfo().Size(), TierFeaturesArray.Size());

    for (int i = 0; i < TierFeaturesArray.Size(); i++)
    {
        EXPECT_EQ(Result.GetFeaturesLimitInfo()[i].FeatureName, TierFeaturesArray[i]);
        EXPECT_EQ(Result.GetFeaturesLimitInfo()[i].Limit, -1);
        EXPECT_EQ(Result.GetFeaturesLimitInfo()[i].ActivityCount, 0);
    }

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_QUOTASYSTEM_TESTS || RUN_QUOTASYSTEM_GETTIERFEATUREQUOTA_TEST
CSP_PUBLIC_TEST(CSPEngine, QuotaSystemTests, GetTierFeatureQuota)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto UserSystem = SystemsManager.GetUserSystem();
    auto QuotaSystem = SystemsManager.GetQuotaSystem();

    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // test quota queries for basic tier
    {
        auto [Result] = AWAIT_PRE(QuotaSystem, GetTierFeatureQuota, RequestPredicate, TierNames::Basic, TierFeatures::OpenAI);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetFeatureQuotaInfo().FeatureName, TierFeatures::OpenAI);
        EXPECT_EQ(Result.GetFeatureQuotaInfo().TierName, TierNames::Basic);
    }

    // test quota queries for pro tier
    {
        auto [Result] = AWAIT_PRE(QuotaSystem, GetTierFeatureQuota, RequestPredicate, TierNames::Pro, TierFeatures::SpaceOwner);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetFeatureQuotaInfo().FeatureName, TierFeatures::SpaceOwner);
        EXPECT_EQ(Result.GetFeatureQuotaInfo().TierName, TierNames::Pro);
    }
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_QUOTASYSTEM_TESTS || RUN_QUOTASYSTEM_GETTIERFEATURESQUOTA_TEST
CSP_PUBLIC_TEST(CSPEngine, QuotaSystemTests, GetTierFeaturesQuota)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto UserSystem = SystemsManager.GetUserSystem();
    auto QuotaSystem = SystemsManager.GetQuotaSystem();

    csp::common::Array<FeatureQuotaInfo> ExpectedInfoArray
        = { FeatureQuotaInfo(TierFeatures::SpaceOwner, TierNames::Basic, 3, PeriodEnum::Total, true),
              FeatureQuotaInfo(TierFeatures::ScopeConcurrentUsers, TierNames::Basic, 5, PeriodEnum::Hours24, true),
              FeatureQuotaInfo(TierFeatures::ObjectCaptureUpload, TierNames::Basic, 5, PeriodEnum::CalendarMonth, false),
              FeatureQuotaInfo(TierFeatures::AudioVideoUpload, TierNames::Basic, -1, PeriodEnum::Total, false),
              FeatureQuotaInfo(TierFeatures::TotalUploadSizeInKilobytes, TierNames::Basic, 10000000, PeriodEnum::Total, true),
              FeatureQuotaInfo(TierFeatures::Agora, TierNames::Basic, 0, PeriodEnum::Total, false),
              FeatureQuotaInfo(TierFeatures::OpenAI, TierNames::Basic, 0, PeriodEnum::Total, false),
              FeatureQuotaInfo(TierFeatures::Shopify, TierNames::Basic, 0, PeriodEnum::Total, false),
              FeatureQuotaInfo(TierFeatures::TicketedSpace, TierNames::Basic, 0, PeriodEnum::Total, false) };

    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    auto [Result] = AWAIT_PRE(QuotaSystem, GetTierFeaturesQuota, RequestPredicate, TierNames::Basic);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(Result.GetFeaturesQuotaInfo().Size(), ExpectedInfoArray.Size());

    for (int i = 0; i < Result.GetFeaturesQuotaInfo().Size(); i++)
    {
        EXPECT_EQ(Result.GetFeaturesQuotaInfo()[i].FeatureName, ExpectedInfoArray[i].FeatureName);
        EXPECT_EQ(Result.GetFeaturesQuotaInfo()[i].TierName, ExpectedInfoArray[i].TierName);
        EXPECT_EQ(Result.GetFeaturesQuotaInfo()[i].Limit, ExpectedInfoArray[i].Limit);
        EXPECT_EQ(Result.GetFeaturesQuotaInfo()[i].Period, ExpectedInfoArray[i].Period);
    }
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_QUOTASYSTEM_TESTS || RUN_QUOTASYSTEM_GETCONCURRENTUSERSINSPACE_TEST
CSP_PUBLIC_TEST(CSPEngine, QuotaSystemTests, GetConcurrentUsersInSpace)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto UserSystem = SystemsManager.GetUserSystem();
    auto QuotaSystem = SystemsManager.GetQuotaSystem();
    auto SpaceSystem = SystemsManager.GetSpaceSystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();

    csp::common::Array<TierFeatures> TierFeaturesArray = { TierFeatures::SpaceOwner, TierFeatures::ObjectCaptureUpload,
        TierFeatures::AudioVideoUpload, TierFeatures::OpenAI, TierFeatures::TicketedSpace };

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-REWIND";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    ::Space Space;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    auto [Result] = AWAIT_PRE(QuotaSystem, GetConcurrentUsersInSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    EXPECT_EQ(Result.GetFeatureLimitInfo().FeatureName, TierFeatures::ScopeConcurrentUsers);
    EXPECT_EQ(Result.GetFeatureLimitInfo().ActivityCount, 0);
    EXPECT_EQ(Result.GetFeatureLimitInfo().Limit, 50);

    // Enter space
    auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto [Result1] = AWAIT_PRE(QuotaSystem, GetConcurrentUsersInSpace, RequestPredicate, Space.Id);

    EXPECT_EQ(Result1.GetResultCode(), csp::systems::EResultCode::Success);

    EXPECT_EQ(Result1.GetFeatureLimitInfo().FeatureName, TierFeatures::ScopeConcurrentUsers);
    EXPECT_EQ(Result1.GetFeatureLimitInfo().ActivityCount, 0);
    EXPECT_EQ(Result1.GetFeatureLimitInfo().Limit, 50);

    auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_QUOTASYSTEM_TESTS || RUN_QUOTASYSTEM_GETTOTALSPACESIZEINKILOBYTES_TEST
CSP_PUBLIC_TEST(CSPEngine, QuotaSystemTests, GetTotalSpaceSizeinKilobytes)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto UserSystem = SystemsManager.GetUserSystem();
    auto QuotaSystem = SystemsManager.GetQuotaSystem();
    auto SpaceSystem = SystemsManager.GetSpaceSystem();
    auto AssetSystem = SystemsManager.GetAssetSystem();

    const char* TestSpaceName = "OLY-UNITTEST-SPACE-CSP";
    const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-CSP";
    const char* TestAssetCollectionName = "OLY-UNITTEST-ASSETCOLLECTION-CSP";
    const char* TestAssetName = "OLY-UNITTEST-ASSET-CSP";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    csp::common::String UserId;

    // Log in
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    ::Space Space;
    CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    char UniqueAssetCollectionName[256];
    SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, Space.Id.c_str());

    char UniqueAssetName[256];
    SPRINTF(UniqueAssetName, "%s-%s", TestAssetName, Space.Id.c_str());

    auto [Result] = AWAIT_PRE(QuotaSystem, GetTotalSpaceSizeInKilobytes, RequestPredicate, Space.Id);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(Result.GetFeatureLimitInfo().FeatureName, TierFeatures::TotalUploadSizeInKilobytes);
    EXPECT_EQ(Result.GetFeatureLimitInfo().ActivityCount, 0);
    EXPECT_EQ(Result.GetFeatureLimitInfo().Limit, 10000000);

    // Create asset collection
    csp::systems::AssetCollection AssetCollection;
    CreateAssetCollection(AssetSystem, Space.Id, nullptr, UniqueAssetCollectionName, nullptr, nullptr, AssetCollection);

    // Create asset
    csp::systems::Asset Asset;
    CreateAsset(AssetSystem, AssetCollection, UniqueAssetName, nullptr, nullptr, Asset);
    auto& InitialAssetId = Asset.Id;

    // Upload data
    auto FilePath = std::filesystem::absolute("assets/testkb.json");
    uintmax_t UpdateFileSize = std::filesystem::file_size(FilePath);
    csp::systems::FileAssetDataSource Source;
    Source.FilePath = FilePath.u8string().c_str();

    Source.SetMimeType("application/json");

    printf("Uploading asset data...\n");

    csp::common::String Uri;
    UploadAssetData(AssetSystem, AssetCollection, Asset, Source, Uri);

    csp::systems::Asset RemoteAsset;
    GetAssetById(AssetSystem, AssetCollection.Id, Asset.Id, RemoteAsset);

    EXPECT_EQ(InitialAssetId, RemoteAsset.Id);

    Asset.Uri = Uri;

    auto [DownloadedResult] = AWAIT_PRE(AssetSystem, DownloadAssetData, RequestPredicate, Asset);

    EXPECT_EQ(DownloadedResult.GetResultCode(), csp::systems::EResultCode::Success);

    EXPECT_EQ(DownloadedResult.GetDataLength(), UpdateFileSize);

    auto [UpdatedSizeResult] = AWAIT_PRE(QuotaSystem, GetTotalSpaceSizeInKilobytes, RequestPredicate, Space.Id);

    EXPECT_EQ(UpdatedSizeResult.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(UpdatedSizeResult.GetFeatureLimitInfo().FeatureName, TierFeatures::TotalUploadSizeInKilobytes);
    EXPECT_EQ(UpdatedSizeResult.GetFeatureLimitInfo().ActivityCount, UpdateFileSize / 1000);
    EXPECT_EQ(UpdatedSizeResult.GetFeatureLimitInfo().Limit, 10000000);

    // Delete asset
    DeleteAsset(AssetSystem, AssetCollection, Asset);

    // Delete asset collection
    DeleteAssetCollection(AssetSystem, AssetCollection);

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);
}
#endif