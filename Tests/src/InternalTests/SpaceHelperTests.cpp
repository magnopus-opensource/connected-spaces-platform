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

#include "Services/ApiBase/ApiBase.h"
#include "Systems/Spaces/SpaceSystemHelpers.h"
#include "TestHelpers.h"

using namespace csp::common;
using namespace csp::services;

CSP_INTERNAL_TEST(CSPEngine, SpaceHelperTests, SpaceGetSpaceMetadataAssetCollectionNameTest)
{
    const String spaceId = "12345678";

    const String assetCollectionName = csp::systems::SpaceSystemHelpers::GetSpaceMetadataAssetCollectionName(spaceId);
    const auto assetCollectionNameList = assetCollectionName.Split('_');

    EXPECT_EQ(assetCollectionNameList.Size(), 5);

    EXPECT_EQ(assetCollectionNameList[4], spaceId);
}

CSP_INTERNAL_TEST(CSPEngine, SpaceHelperTests, GetSpaceIdFromMetadataAssetCollectionNameTest)
{
    const String spaceId = "12345678";

    const String assetCollectionName = csp::systems::SpaceSystemHelpers::GetSpaceMetadataAssetCollectionName(spaceId);

    const auto assetCollectionNameList = assetCollectionName.Split('_');

    EXPECT_EQ(assetCollectionNameList.Size(), 5);

    EXPECT_EQ(assetCollectionNameList[4], spaceId);

    const String returnedSpaceId = csp::systems::SpaceSystemHelpers::GetSpaceIdFromMetadataAssetCollectionName(assetCollectionName);

    EXPECT_EQ(returnedSpaceId, spaceId);
}

CSP_INTERNAL_TEST(CSPEngine, SpaceHelperTests, ConvertSpaceMetadataToAssetCollectionMetadataTest)
{
    const String metaData = "CSP_META_DATA";
    const String metaDataKey = csp::systems::SpaceSystemHelpers::SPACE_METADATA_KEY;

    const auto metaDataMap = csp::systems::SpaceSystemHelpers::ConvertSpaceMetadataToAssetCollectionMetadata(metaData);

    EXPECT_EQ(metaDataMap.Size(), 1);

    EXPECT_TRUE(metaDataMap.HasKey(metaDataKey));

    EXPECT_EQ(metaDataMap[metaDataKey], metaData);
}

CSP_INTERNAL_TEST(CSPEngine, SpaceHelperTests, GetSpaceThumbnailAssetCollectionNameTest)
{
    const String spaceId = "12345678";

    const String spaceThumbnailName = csp::systems::SpaceSystemHelpers::GetSpaceThumbnailAssetCollectionName(spaceId);
    const auto spaceThumbnailNameList = spaceThumbnailName.Split('_');

    EXPECT_EQ(spaceThumbnailNameList.Size(), 5);

    EXPECT_EQ(spaceThumbnailNameList[4], spaceId);
}

CSP_INTERNAL_TEST(CSPEngine, SpaceHelperTests, GetUniqueSpaceThumbnailAssetNameTest)
{
    const String spaceId = "12345678";

    const String uniqueSpaceThumbnailAssetName = csp::systems::SpaceSystemHelpers::GetUniqueSpaceThumbnailAssetName(spaceId);
    const auto uniqueSpaceThumbnailAssetNameList = uniqueSpaceThumbnailAssetName.Split('_');

    EXPECT_EQ(uniqueSpaceThumbnailAssetNameList.Size(), 3);

    EXPECT_EQ(uniqueSpaceThumbnailAssetNameList[2], spaceId);
}

CSP_INTERNAL_TEST(CSPEngine, SpaceHelperTests, GetUniqueAvatarThumbnailAssetNameTest)
{
    const String extension = "user";

    const String uniqueAvatarThumbnailAssetName = csp::systems::SpaceSystemHelpers::GetUniqueAvatarThumbnailAssetName("_" + extension);
    const auto uniqueAvatarThumbnailAssetNameList = uniqueAvatarThumbnailAssetName.Split('_');

    EXPECT_EQ(uniqueAvatarThumbnailAssetNameList.Size(), 3);

    EXPECT_EQ(uniqueAvatarThumbnailAssetNameList[2], extension);
}

CSP_INTERNAL_TEST(CSPEngine, SpaceHelperTests, GetAssetFileExtensionTest)
{
    String assetFileExtension;

    const String pngMimeType = "image/png";
    const String pngExtension = ".png";

    assetFileExtension = csp::systems::SpaceSystemHelpers::GetAssetFileExtension(pngMimeType);

    EXPECT_EQ(assetFileExtension, pngExtension);

    const String jpegMimeType = "image/jpeg";
    const String jpegExtension = ".jpeg";

    assetFileExtension = csp::systems::SpaceSystemHelpers::GetAssetFileExtension(jpegMimeType);

    EXPECT_EQ(assetFileExtension, jpegExtension);

    const String gifMimeType = "image/gif";
    const String gifExtension = ".gif";

    assetFileExtension = csp::systems::SpaceSystemHelpers::GetAssetFileExtension(gifMimeType);

    EXPECT_EQ(assetFileExtension, gifExtension);

    const String apngMimeType = "image/apng";
    const String apngExtension = ".apng";

    assetFileExtension = csp::systems::SpaceSystemHelpers::GetAssetFileExtension(apngMimeType);

    EXPECT_EQ(assetFileExtension, apngExtension);

    const String avifMimeType = "image/avif";
    const String avifExtension = ".avif";

    assetFileExtension = csp::systems::SpaceSystemHelpers::GetAssetFileExtension(avifMimeType);

    EXPECT_EQ(assetFileExtension, avifExtension);

    const String svgxmlMimeType = "image/svg+xml";
    const String svgxmlExtension = ".svg";

    assetFileExtension = csp::systems::SpaceSystemHelpers::GetAssetFileExtension(svgxmlMimeType);

    EXPECT_EQ(assetFileExtension, svgxmlExtension);
}

CSP_INTERNAL_TEST(CSPEngine, SpaceHelperTests, IdCheckValidTest)
{
    const String userId = "123";
    const Array<String> userIds = { "123", "456", "789" };

    bool validityCheck = csp::systems::SpaceSystemHelpers::IdCheck(userId, userIds);

    EXPECT_TRUE(validityCheck);
}

CSP_INTERNAL_TEST(CSPEngine, SpaceHelperTests, IdCheckInValidTest)
{
    const String invalidUserId = "101";
    const Array<String> userIds = { "123", "456", "789" };

    bool validityCheck = csp::systems::SpaceSystemHelpers::IdCheck(invalidUserId, userIds);

    EXPECT_FALSE(validityCheck);
}

CSP_INTERNAL_TEST(CSPEngine, SpaceHelperTests, ConvertJsonMetadataToMapMetadataNotObjectTest)
{
    const String metaSiteData = "Void";
    const String metaDataSiteKey = "site";
    const String metaDataMultiplayerVersionKey = "multiplayerVersion";
    const String metaDataMultiplayerVersionData = "3";
    const String notJsonObject = "{[\"testdata\"]}";

    Map<String, String> notJsonObjectMetaDataMap;

    csp::systems::SpaceSystemHelpers::ConvertJsonMetadataToMapMetadata(notJsonObject, notJsonObjectMetaDataMap);

    ASSERT_EQ(notJsonObjectMetaDataMap.Size(), 2);

    EXPECT_TRUE(notJsonObjectMetaDataMap.HasKey(metaDataSiteKey));

    EXPECT_EQ(notJsonObjectMetaDataMap[metaDataSiteKey], metaSiteData);

    EXPECT_TRUE(notJsonObjectMetaDataMap.HasKey(metaDataMultiplayerVersionKey));

    EXPECT_EQ(notJsonObjectMetaDataMap[metaDataMultiplayerVersionKey], metaDataMultiplayerVersionData);
}

CSP_INTERNAL_TEST(CSPEngine, SpaceHelperTests, ConvertJsonMetadataToMapMetadataObjectTest)
{
    const String metaSiteData = "ObjectVoid";
    const String metaDataSiteKey = "ObjectSite";
    const String jsonObject = "{\"" + metaDataSiteKey + "\" :\"" + metaSiteData + "\"}";

    Map<String, String> objectMetaDataMap;

    csp::systems::SpaceSystemHelpers::ConvertJsonMetadataToMapMetadata(jsonObject, objectMetaDataMap);

    ASSERT_EQ(objectMetaDataMap.Size(), 1);

    EXPECT_TRUE(objectMetaDataMap.HasKey(metaDataSiteKey));

    EXPECT_EQ(objectMetaDataMap[metaDataSiteKey], metaSiteData);
}
