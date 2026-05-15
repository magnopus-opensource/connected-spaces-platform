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
#include "CSP/Common/SharedEnums.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Systems/Quota/QuotaSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "SpaceSystemTestHelpers.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"

#include "gtest/gtest.h"
#include <cstdint>
#include <filesystem>
#include <future>
using namespace csp::systems;

namespace
{

bool RequestPredicate(const csp::systems::ResultBase& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

CSP_PUBLIC_TEST(CSPEngine, QuotaSystemTests, TierNameEnumTesttoStringTest)
{
    EXPECT_EQ(TierNameEnumToString(csp::systems::TierNames::Basic), "basic");
    EXPECT_EQ(TierNameEnumToString(csp::systems::TierNames::Premium), "premium");
    EXPECT_EQ(TierNameEnumToString(csp::systems::TierNames::Pro), "pro");
    EXPECT_EQ(TierNameEnumToString(csp::systems::TierNames::Enterprise), "enterprise");
}

CSP_PUBLIC_TEST(CSPEngine, QuotaSystemTests, StringToTierNameEnumTestTest)
{
    EXPECT_EQ(StringToTierNameEnum("basic"), csp::systems::TierNames::Basic);
    EXPECT_EQ(StringToTierNameEnum("premium"), csp::systems::TierNames::Premium);
    EXPECT_EQ(StringToTierNameEnum("pro"), csp::systems::TierNames::Pro);
    EXPECT_EQ(StringToTierNameEnum("enterprise"), csp::systems::TierNames::Enterprise);
}

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
    EXPECT_EQ(TierFeatureEnumToString(csp::systems::TierFeatures::GoogleGenAI), "GoogleGenAI");
}

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
    EXPECT_EQ(StringToTierFeatureEnum("GoogleGenAI"), csp::systems::TierFeatures::GoogleGenAI);
}

CSP_PUBLIC_TEST(CSPEngine, QuotaSystemTests, GetTotalSpacesOwnedByUserTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto userSystem = systemsManager.GetUserSystem();
    auto quotaSystem = systemsManager.GetQuotaSystem();

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    auto [Result] = AWAIT_PRE(quotaSystem, GetTotalSpacesOwnedByUser, RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(Result.GetFeatureLimitInfo().ActivityCount, 0);
    EXPECT_EQ(Result.GetFeatureLimitInfo().Limit, -1);
    EXPECT_EQ(Result.GetFeatureLimitInfo().FeatureName, csp::systems::TierFeatures::SpaceOwner);
}

CSP_PUBLIC_TEST(CSPEngine, QuotaSystemTests, SetUserTierTest)
{
    if (std::string(AdminAccountEmail()).empty())
    {
        GTEST_SKIP() << "Admin account email not set. This test cannot be run.";
    }

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto userSystem = systemsManager.GetUserSystem();
    auto quotaSystem = systemsManager.GetQuotaSystem();

    csp::common::String basicUserId;
    const auto basicUser = CreateTestUser();
    LogIn(userSystem, basicUserId, basicUser.Email, GeneratedTestAccountPassword);

    // All users on the test tenant are pro
    {
        std::promise<TierNames> userTierCheckPromise;
        std::future<TierNames> userTierCheckFuture = userTierCheckPromise.get_future();
        quotaSystem->GetCurrentUserTier(
            [&userTierCheckPromise](const UserTierResult& result)
            {
                if (result.GetResultCode() == EResultCode::Success)
                {
                    userTierCheckPromise.set_value(result.GetUserTierInfo().TierName);
                }
            });

        ASSERT_EQ(userTierCheckFuture.get(), TierNames::Pro);
    }

    LogOut(userSystem);

    // Login to an admin user to change the user type to basic. Only admin users can do this.
    csp::common::String adminUserId;
    // Have to be an admin use to change user tier
    LogInAsAdminUser(userSystem, adminUserId);

    {
        std::promise<csp::systems::UserTierInfo> userTierChangePromise;
        std::future<csp::systems::UserTierInfo> userTierChangeFuture = userTierChangePromise.get_future();

        quotaSystem->SetUserTier(TierNames::Basic, basicUserId,
            [&userTierChangePromise](const UserTierResult& result)
            {
                if (result.GetResultCode() == EResultCode::Success)
                {
                    userTierChangePromise.set_value(result.GetUserTierInfo());
                }
            });

        ASSERT_EQ(userTierChangeFuture.get().TierName, TierNames::Basic);
    }

    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, QuotaSystemTests, SetUserTierWhenNotAdminTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto userSystem = systemsManager.GetUserSystem();
    auto quotaSystem = systemsManager.GetQuotaSystem();

    csp::common::String basicUserId;
    const auto basicUser = CreateTestUser();
    LogIn(userSystem, basicUserId, basicUser.Email, GeneratedTestAccountPassword);

    // All users on the test tenant are pro
    {
        std::promise<TierNames> userTierCheckPromise;
        std::future<TierNames> userTierCheckFuture = userTierCheckPromise.get_future();
        quotaSystem->GetCurrentUserTier(
            [&userTierCheckPromise](const UserTierResult& result)
            {
                if (result.GetResultCode() == EResultCode::Success)
                {
                    userTierCheckPromise.set_value(result.GetUserTierInfo().TierName);
                }
            });

        ASSERT_EQ(userTierCheckFuture.get(), TierNames::Pro);
    }

    LogOut(userSystem);

    // Non admin users should not be able to change tier assignments
    csp::common::String otherUserId;
    LogInAsNewTestUser(userSystem, otherUserId);

    {
        std::promise<std::pair<EResultCode, uint16_t>> userTierChangePromise;
        std::future<std::pair<EResultCode, uint16_t>> userTierChangeFuture = userTierChangePromise.get_future();

        quotaSystem->SetUserTier(TierNames::Basic, basicUserId,
            [&userTierChangePromise](const UserTierResult& result)
            {
                if (result.GetResultCode() != EResultCode::InProgress)
                {
                    userTierChangePromise.set_value({ result.GetResultCode(), result.GetHttpResultCode() });
                }
            });

        const auto userTierChangeResult = userTierChangeFuture.get();
        ASSERT_EQ(userTierChangeResult.first, EResultCode::Failed);
        ASSERT_EQ(userTierChangeResult.second, static_cast<uint16_t>(csp::web::EResponseCodes::ResponseForbidden));
    }

    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, QuotaSystemTests, SetUserTierWithInvalidUserTest)
{
    if (std::string(AdminAccountEmail()).empty())
    {
        GTEST_SKIP() << "Admin account email not set. This test cannot be run.";
    }

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto userSystem = systemsManager.GetUserSystem();
    auto quotaSystem = systemsManager.GetQuotaSystem();

    // Login to an admin user to change the user type to basic. Only admin users can do this.
    csp::common::String adminUserId;
    // Have to be an admin use to change user tier
    LogInAsAdminUser(userSystem, adminUserId);

    std::promise<std::pair<EResultCode, uint16_t>> userTierChangePromise;
    std::future<std::pair<EResultCode, uint16_t>> userTierChangeFuture = userTierChangePromise.get_future();

    quotaSystem->SetUserTier(TierNames::Basic, "InvalidUserId",
        [&userTierChangePromise](const UserTierResult& result)
        {
            if (result.GetResultCode() != EResultCode::InProgress)
            {
                userTierChangePromise.set_value({ result.GetResultCode(), result.GetHttpResultCode() });
            }
        });

    const auto userTierChangeResult = userTierChangeFuture.get();
    ASSERT_EQ(userTierChangeResult.first, EResultCode::Failed);
    ASSERT_EQ(userTierChangeResult.second, static_cast<uint16_t>(csp::web::EResponseCodes::ResponseBadRequest));

    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, QuotaSystemTests, GetCurrentUserTierTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto userSystem = systemsManager.GetUserSystem();
    auto quotaSystem = systemsManager.GetQuotaSystem();

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    auto [Result] = AWAIT_PRE(quotaSystem, GetCurrentUserTier, RequestPredicate);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(Result.GetUserTierInfo().TierName, TierNames::Pro);
    EXPECT_EQ(Result.GetUserTierInfo().AssignToId, userId);
    EXPECT_EQ(Result.GetUserTierInfo().AssignToType, "user");
}

CSP_PUBLIC_TEST(CSPEngine, QuotaSystemTests, GetTierFeatureProgressForUser)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto userSystem = systemsManager.GetUserSystem();
    auto quotaSystem = systemsManager.GetQuotaSystem();

    csp::common::Array<TierFeatures> tierFeaturesArray = { TierFeatures::SpaceOwner, TierFeatures::ObjectCaptureUpload,
        TierFeatures::AudioVideoUpload, TierFeatures::OpenAI, TierFeatures::TicketedSpace };
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    auto [Result] = AWAIT_PRE(quotaSystem, GetTierFeatureProgressForUser, RequestPredicate, tierFeaturesArray);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(Result.GetFeaturesLimitInfo().Size(), tierFeaturesArray.Size());

    for (size_t i = 0; i < tierFeaturesArray.Size(); i++)
    {
        EXPECT_EQ(Result.GetFeaturesLimitInfo()[i].FeatureName, tierFeaturesArray[i]);
        EXPECT_EQ(Result.GetFeaturesLimitInfo()[i].Limit, -1);
        EXPECT_EQ(Result.GetFeaturesLimitInfo()[i].ActivityCount, 0);
    }
}

CSP_PUBLIC_TEST(CSPEngine, QuotaSystemTests, GetTierFeatureProgressForSpace)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto userSystem = systemsManager.GetUserSystem();
    auto quotaSystem = systemsManager.GetQuotaSystem();
    auto spaceSystem = systemsManager.GetSpaceSystem();

    csp::common::Array<TierFeatures> tierFeaturesArray = { TierFeatures::SpaceOwner, TierFeatures::ObjectCaptureUpload,
        TierFeatures::AudioVideoUpload, TierFeatures::OpenAI, TierFeatures::TicketedSpace };

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    ::Space space;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    auto [Result] = AWAIT_PRE(quotaSystem, GetTierFeatureProgressForSpace, RequestPredicate, space.Id, tierFeaturesArray);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(Result.GetFeaturesLimitInfo().Size(), tierFeaturesArray.Size());

    for (size_t i = 0; i < tierFeaturesArray.Size(); i++)
    {
        EXPECT_EQ(Result.GetFeaturesLimitInfo()[i].FeatureName, tierFeaturesArray[i]);
        EXPECT_EQ(Result.GetFeaturesLimitInfo()[i].Limit, -1);
        EXPECT_EQ(Result.GetFeaturesLimitInfo()[i].ActivityCount, 0);
    }

    // Delete space
    DeleteSpace(spaceSystem, space.Id);
}

CSP_PUBLIC_TEST(CSPEngine, QuotaSystemTests, GetTierFeatureQuota)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto userSystem = systemsManager.GetUserSystem();
    auto quotaSystem = systemsManager.GetQuotaSystem();

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // test quota queries for basic tier
    {
        auto [Result] = AWAIT_PRE(quotaSystem, GetTierFeatureQuota, RequestPredicate, TierNames::Basic, TierFeatures::OpenAI);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetFeatureQuotaInfo().FeatureName, TierFeatures::OpenAI);
        EXPECT_EQ(Result.GetFeatureQuotaInfo().TierName, TierNames::Basic);
    }

    // test quota queries for pro tier
    {
        auto [Result] = AWAIT_PRE(quotaSystem, GetTierFeatureQuota, RequestPredicate, TierNames::Pro, TierFeatures::SpaceOwner);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
        EXPECT_EQ(Result.GetFeatureQuotaInfo().FeatureName, TierFeatures::SpaceOwner);
        EXPECT_EQ(Result.GetFeatureQuotaInfo().TierName, TierNames::Pro);
    }
}

CSP_PUBLIC_TEST(CSPEngine, QuotaSystemTests, GetTierFeaturesQuota)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto userSystem = systemsManager.GetUserSystem();
    auto quotaSystem = systemsManager.GetQuotaSystem();

    csp::common::Array<TierFeatures> tierFeaturesArray = { TierFeatures::SpaceOwner, TierFeatures::ScopeConcurrentUsers,
        TierFeatures::ObjectCaptureUpload, TierFeatures::AudioVideoUpload, TierFeatures::TotalUploadSizeInKilobytes, TierFeatures::Agora,
        TierFeatures::OpenAI, TierFeatures::GoogleGenAI, TierFeatures::Shopify, TierFeatures::TicketedSpace };

    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    auto [Result] = AWAIT_PRE(quotaSystem, GetTierFeaturesQuota, RequestPredicate, TierNames::Basic);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    EXPECT_EQ(Result.GetFeaturesQuotaInfo().Size(), tierFeaturesArray.Size());

    for (size_t i = 0; i < Result.GetFeaturesQuotaInfo().Size(); i++)
    {
        EXPECT_EQ(Result.GetFeaturesQuotaInfo()[i].FeatureName, tierFeaturesArray[i]);
    }
}

CSP_PUBLIC_TEST(CSPEngine, QuotaSystemTests, GetConcurrentUsersInSpace)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto userSystem = systemsManager.GetUserSystem();
    auto quotaSystem = systemsManager.GetQuotaSystem();
    auto spaceSystem = systemsManager.GetSpaceSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    ::Space space;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    auto [Result] = AWAIT_PRE(quotaSystem, GetConcurrentUsersInSpace, RequestPredicate, space.Id);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

    EXPECT_EQ(Result.GetFeatureLimitInfo().FeatureName, TierFeatures::ScopeConcurrentUsers);
    EXPECT_EQ(Result.GetFeatureLimitInfo().ActivityCount, 0);
    EXPECT_EQ(Result.GetFeatureLimitInfo().Limit, 50);

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
    realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

    // Enter space
    auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

    EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

    auto [Result1] = AWAIT_PRE(quotaSystem, GetConcurrentUsersInSpace, RequestPredicate, space.Id);

    EXPECT_EQ(Result1.GetResultCode(), csp::systems::EResultCode::Success);

    EXPECT_EQ(Result1.GetFeatureLimitInfo().FeatureName, TierFeatures::ScopeConcurrentUsers);
    EXPECT_EQ(Result1.GetFeatureLimitInfo().ActivityCount, 0);
    EXPECT_EQ(Result1.GetFeatureLimitInfo().Limit, 50);

    auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}

CSP_PUBLIC_TEST(CSPEngine, QuotaSystemTests, GetTotalSpaceSizeinKilobytes)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto userSystem = systemsManager.GetUserSystem();
    auto quotaSystem = systemsManager.GetQuotaSystem();
    auto spaceSystem = systemsManager.GetSpaceSystem();
    auto assetSystem = systemsManager.GetAssetSystem();

    const char* testSpaceName = "CSP-UNITTEST-SPACE-CSP";
    const char* testSpaceDescription = "CSP-UNITTEST-SPACEDESC-CSP";
    const char* testAssetCollectionName = "CSP-UNITTEST-ASSETCOLLECTION-CSP";
    const char* testAssetName = "CSP-UNITTEST-ASSET-CSP";

    char uniqueSpaceName[256];
    SPRINTF(uniqueSpaceName, "%s-%s", testSpaceName, GetUniqueString().c_str());

    csp::common::String userId;

    // Log in
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    ::Space space;
    CreateSpace(spaceSystem, uniqueSpaceName, testSpaceDescription, SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, space);

    char uniqueAssetCollectionName[256];
    SPRINTF(uniqueAssetCollectionName, "%s-%s", testAssetCollectionName, space.Id.c_str());

    char uniqueAssetName[256];
    SPRINTF(uniqueAssetName, "%s-%s", testAssetName, space.Id.c_str());

    auto [Result] = AWAIT_PRE(quotaSystem, GetTotalSpaceSizeInKilobytes, RequestPredicate, space.Id);

    EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(Result.GetFeatureLimitInfo().FeatureName, TierFeatures::TotalUploadSizeInKilobytes);
    EXPECT_EQ(Result.GetFeatureLimitInfo().ActivityCount, 0);
    EXPECT_EQ(Result.GetFeatureLimitInfo().Limit, 10000000);

    // Create asset collection
    csp::systems::AssetCollection assetCollection;
    CreateAssetCollection(assetSystem, space.Id, nullptr, uniqueAssetCollectionName, nullptr, nullptr, assetCollection);

    // Create asset
    csp::systems::Asset asset;
    CreateAsset(assetSystem, assetCollection, uniqueAssetName, nullptr, nullptr, asset);
    auto& initialAssetId = asset.Id;

    // Upload data
    auto filePath = std::filesystem::absolute("assets/testkb.json");
    uintmax_t updateFileSize = std::filesystem::file_size(filePath);
    csp::systems::FileAssetDataSource source;
    source.FilePath = filePath.u8string().c_str();

    source.SetMimeType("application/json");

    printf("Uploading asset data...\n");

    csp::common::String uri;
    UploadAssetData(assetSystem, assetCollection, asset, source, uri);

    csp::systems::Asset remoteAsset;
    GetAssetById(assetSystem, assetCollection.Id, asset.Id, remoteAsset);

    EXPECT_EQ(initialAssetId, remoteAsset.Id);

    asset.Uri = uri;

    auto [DownloadedResult] = AWAIT_PRE(assetSystem, DownloadAssetData, RequestPredicate, asset);

    EXPECT_EQ(DownloadedResult.GetResultCode(), csp::systems::EResultCode::Success);

    EXPECT_EQ(DownloadedResult.GetDataLength(), updateFileSize);

    auto [UpdatedSizeResult] = AWAIT_PRE(quotaSystem, GetTotalSpaceSizeInKilobytes, RequestPredicate, space.Id);

    EXPECT_EQ(UpdatedSizeResult.GetResultCode(), csp::systems::EResultCode::Success);
    EXPECT_EQ(UpdatedSizeResult.GetFeatureLimitInfo().FeatureName, TierFeatures::TotalUploadSizeInKilobytes);
    EXPECT_EQ(UpdatedSizeResult.GetFeatureLimitInfo().ActivityCount, updateFileSize / 1000);
    EXPECT_EQ(UpdatedSizeResult.GetFeatureLimitInfo().Limit, 10000000);

    // Delete asset
    DeleteAsset(assetSystem, assetCollection, asset);

    // Delete asset collection
    DeleteAssetCollection(assetSystem, assetCollection);

    // Delete space
    DeleteSpace(spaceSystem, space.Id);
}

CSP_PUBLIC_TEST(CSPEngine, QuotaSystemTests, FailWhenNotLoggedInTest)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto quotaSystem = systemsManager.GetQuotaSystem();

    {
        auto [Result] = AWAIT_PRE(quotaSystem, GetTotalSpacesOwnedByUser, RequestPredicate);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
        EXPECT_EQ(Result.GetHttpResultCode(), 0);
    }

    {
        auto [Result] = AWAIT_PRE(quotaSystem, GetTierFeatureProgressForUser, RequestPredicate, {});

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
        EXPECT_EQ(Result.GetHttpResultCode(), 0);
    }

    {
        auto [Result] = AWAIT_PRE(quotaSystem, GetCurrentUserTier, RequestPredicate);

        EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Failed);
        EXPECT_EQ(Result.GetHttpResultCode(), 0);
    }
}