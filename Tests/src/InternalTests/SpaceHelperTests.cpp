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

#include "CSP/Systems/Assets/AssetCollection.h"
#include "signalrclient/signalr_value.h"

#if !defined(SKIP_INTERNAL_TESTS) || defined(RUN_SPACE_HELPER_TESTS)

#include "Services/ApiBase/ApiBase.h"
#include "Systems/Spaces/SpaceSystemHelpers.h"
#include "TestHelpers.h"

using namespace csp::common;
using namespace csp::services;

CSP_INTERNAL_TEST(CSPEngine, SpaceHelperTests, SpaceGetSpaceMetadataAssetCollectionNameTest)
{
    const String SpaceId = "12345678";

    const String AssetCollectionName = csp::systems::SpaceSystemHelpers::GetSpaceMetadataAssetCollectionName(SpaceId);
    const auto AssetCollectionNameList = AssetCollectionName.Split('_');

    EXPECT_EQ(AssetCollectionNameList.Size(), 5);

    EXPECT_EQ(AssetCollectionNameList[4], SpaceId);
}

CSP_INTERNAL_TEST(CSPEngine, SpaceHelperTests, GetSpaceIdFromMetadataAssetCollectionNameTest)
{
    const String SpaceId = "12345678";

    const String AssetCollectionName = csp::systems::SpaceSystemHelpers::GetSpaceMetadataAssetCollectionName(SpaceId);

    const auto AssetCollectionNameList = AssetCollectionName.Split('_');

    EXPECT_EQ(AssetCollectionNameList.Size(), 5);

    EXPECT_EQ(AssetCollectionNameList[4], SpaceId);

    const String ReturnedSpaceId = csp::systems::SpaceSystemHelpers::GetSpaceIdFromMetadataAssetCollectionName(AssetCollectionName);

    EXPECT_EQ(ReturnedSpaceId, SpaceId);
}

CSP_INTERNAL_TEST(CSPEngine, SpaceHelperTests, ConvertSpaceMetadataToAssetCollectionMetadataTest)
{
    const String MetaData = "CSP_META_DATA";
    const String MetaDataKey = csp::systems::SpaceSystemHelpers::SPACE_METADATA_KEY;

    const auto MetaDataMap = csp::systems::SpaceSystemHelpers::ConvertSpaceMetadataToAssetCollectionMetadata(MetaData);

    EXPECT_EQ(MetaDataMap.Size(), 1);

    EXPECT_TRUE(MetaDataMap.HasKey(MetaDataKey));

    EXPECT_EQ(MetaDataMap[MetaDataKey], MetaData);
}

CSP_INTERNAL_TEST(CSPEngine, SpaceHelperTests, GetSpaceThumbnailAssetCollectionNameTest)
{
    const String SpaceId = "12345678";

    const String SpaceThumbnailName = csp::systems::SpaceSystemHelpers::GetSpaceThumbnailAssetCollectionName(SpaceId);
    const auto SpaceThumbnailNameList = SpaceThumbnailName.Split('_');

    EXPECT_EQ(SpaceThumbnailNameList.Size(), 5);

    EXPECT_EQ(SpaceThumbnailNameList[4], SpaceId);
}

CSP_INTERNAL_TEST(CSPEngine, SpaceHelperTests, GetUniqueSpaceThumbnailAssetNameTest)
{
    const String SpaceId = "12345678";

    const String UniqueSpaceThumbnailAssetName = csp::systems::SpaceSystemHelpers::GetUniqueSpaceThumbnailAssetName(SpaceId);
    const auto UniqueSpaceThumbnailAssetNameList = UniqueSpaceThumbnailAssetName.Split('_');

    EXPECT_EQ(UniqueSpaceThumbnailAssetNameList.Size(), 3);

    EXPECT_EQ(UniqueSpaceThumbnailAssetNameList[2], SpaceId);
}

CSP_INTERNAL_TEST(CSPEngine, SpaceHelperTests, GetUniqueAvatarThumbnailAssetNameTest)
{
    const String Extension = "user";

    const String UniqueAvatarThumbnailAssetName = csp::systems::SpaceSystemHelpers::GetUniqueAvatarThumbnailAssetName("_" + Extension);
    const auto UniqueAvatarThumbnailAssetNameList = UniqueAvatarThumbnailAssetName.Split('_');

    EXPECT_EQ(UniqueAvatarThumbnailAssetNameList.Size(), 3);

    EXPECT_EQ(UniqueAvatarThumbnailAssetNameList[2], Extension);
}

CSP_INTERNAL_TEST(CSPEngine, SpaceHelperTests, GetAssetFileExtensionTest)
{
    String AssetFileExtension;

    const String PngMimeType = "image/png";
    const String PngExtension = ".png";

    AssetFileExtension = csp::systems::SpaceSystemHelpers::GetAssetFileExtension(PngMimeType);

    EXPECT_EQ(AssetFileExtension, PngExtension);

    const String JpegMimeType = "image/jpeg";
    const String JpegExtension = ".jpeg";

    AssetFileExtension = csp::systems::SpaceSystemHelpers::GetAssetFileExtension(JpegMimeType);

    EXPECT_EQ(AssetFileExtension, JpegExtension);

    const String GifMimeType = "image/gif";
    const String GifExtension = ".gif";

    AssetFileExtension = csp::systems::SpaceSystemHelpers::GetAssetFileExtension(GifMimeType);

    EXPECT_EQ(AssetFileExtension, GifExtension);

    const String ApngMimeType = "image/apng";
    const String ApngExtension = ".apng";

    AssetFileExtension = csp::systems::SpaceSystemHelpers::GetAssetFileExtension(ApngMimeType);

    EXPECT_EQ(AssetFileExtension, ApngExtension);

    const String AvifMimeType = "image/avif";
    const String AvifExtension = ".avif";

    AssetFileExtension = csp::systems::SpaceSystemHelpers::GetAssetFileExtension(AvifMimeType);

    EXPECT_EQ(AssetFileExtension, AvifExtension);

    const String SvgxmlMimeType = "image/svg+xml";
    const String SvgxmlExtension = ".svg";

    AssetFileExtension = csp::systems::SpaceSystemHelpers::GetAssetFileExtension(SvgxmlMimeType);

    EXPECT_EQ(AssetFileExtension, SvgxmlExtension);
}

CSP_INTERNAL_TEST(CSPEngine, SpaceHelperTests, IdCheckValidTest)
{
    const String UserId = "123";
    const Array<String> UserIds = { "123", "456", "789" };

    bool ValidityCheck = csp::systems::SpaceSystemHelpers::IdCheck(UserId, UserIds);

    EXPECT_TRUE(ValidityCheck);
}

CSP_INTERNAL_TEST(CSPEngine, SpaceHelperTests, IdCheckInValidTest)
{
    const String InvalidUserId = "101";
    const Array<String> UserIds = { "123", "456", "789" };

    bool ValidityCheck = csp::systems::SpaceSystemHelpers::IdCheck(InvalidUserId, UserIds);

    EXPECT_FALSE(ValidityCheck);
}

CSP_INTERNAL_TEST(CSPEngine, SpaceHelperTests, ConvertJsonMetadataToMapMetadataNotObjectTest)
{
    const String MetaSiteData = "Void";
    const String MetaDataSiteKey = "site";
    const String MetaDataMultiplayerVersionKey = "multiplayerVersion";
    const String MetaDataMultiplayerVersionData = "3";
    const String NotJsonObject = "{[\"testdata\"]}";

    Map<String, String> NotJsonObjectMetaDataMap;

    csp::systems::SpaceSystemHelpers::ConvertJsonMetadataToMapMetadata(NotJsonObject, NotJsonObjectMetaDataMap);

    ASSERT_EQ(NotJsonObjectMetaDataMap.Size(), 2);

    EXPECT_TRUE(NotJsonObjectMetaDataMap.HasKey(MetaDataSiteKey));

    EXPECT_EQ(NotJsonObjectMetaDataMap[MetaDataSiteKey], MetaSiteData);

    EXPECT_TRUE(NotJsonObjectMetaDataMap.HasKey(MetaDataMultiplayerVersionKey));

    EXPECT_EQ(NotJsonObjectMetaDataMap[MetaDataMultiplayerVersionKey], MetaDataMultiplayerVersionData);
}

CSP_INTERNAL_TEST(CSPEngine, SpaceHelperTests, ConvertJsonMetadataToMapMetadataObjectTest)
{
    const String MetaSiteData = "ObjectVoid";
    const String MetaDataSiteKey = "ObjectSite";
    const String JsonObject = "{\"" + MetaDataSiteKey + "\" :\"" + MetaSiteData + "\"}";

    Map<String, String> ObjectMetaDataMap;

    csp::systems::SpaceSystemHelpers::ConvertJsonMetadataToMapMetadata(JsonObject, ObjectMetaDataMap);

    ASSERT_EQ(ObjectMetaDataMap.Size(), 1);

    EXPECT_TRUE(ObjectMetaDataMap.HasKey(MetaDataSiteKey));

    EXPECT_EQ(ObjectMetaDataMap[MetaDataSiteKey], MetaSiteData);
}

#endif