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
#pragma once

#include "CSP/Common/String.h"
#include "CSP/Systems/Assets/AssetSystem.h"

void CreateAssetCollection(csp::systems::AssetSystem* assetSystem, const csp::common::Optional<csp::common::String>& spaceId,
    const csp::common::Optional<csp::common::String>& parentId, const csp::common::String& name,
    const csp::common::Optional<csp::systems::EAssetCollectionType>& assetCollectionType,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& tags, csp::systems::AssetCollection& outAssetCollection);

void DeleteAssetCollection(csp::systems::AssetSystem* assetSystem, csp::systems::AssetCollection& assetCollection);

void GetAssetCollections(
    csp::systems::AssetSystem* assetSystem, csp::systems::Space& space, csp::common::Array<csp::systems::AssetCollection>& outAssetCollections);

void GetAssetCollectionsByIds(csp::systems::AssetSystem* assetSystem, const csp::common::Array<csp::common::String>& ids,
    csp::common::Array<csp::systems::AssetCollection>& outAssetCollections);

void CreateAsset(csp::systems::AssetSystem* assetSystem, csp::systems::AssetCollection& assetCollection, const csp::common::String& name,
    const csp::common::Optional<csp::common::String>& thirdPartyPackagedAssetIdentifier,
    const csp::common::Optional<csp::systems::EThirdPartyPlatform>& thirdPartyPlatform, csp::systems::Asset& outAsset);

void UploadAssetData(csp::systems::AssetSystem* assetSystem, const csp::systems::AssetCollection& assetCollection, const csp::systems::Asset& asset,
    const csp::systems::FileAssetDataSource& source, csp::common::String& outUri);

void UploadAssetData(csp::systems::AssetSystem* assetSystem, const csp::systems::AssetCollection& assetCollection, const csp::systems::Asset& asset,
    const csp::systems::BufferAssetDataSource& source, csp::common::String& outUri);

void GetAssetById(csp::systems::AssetSystem* assetSystem, const csp::common::String& assetCollectionId, const csp::common::String& assetId,
    csp::systems::Asset& outAsset);

void DeleteAsset(csp::systems::AssetSystem* assetSystem, csp::systems::AssetCollection& assetCollection, csp::systems::Asset& asset);

void UpdateAsset(csp::systems::AssetSystem* assetSystem, csp::systems::AssetCollection& assetCollection, csp::systems::Asset& asset);

void GetAssetsInCollection(
    csp::systems::AssetSystem* assetSystem, csp::systems::AssetCollection& assetCollection, csp::common::Array<csp::systems::Asset>& outAssets);

void GetAssetsByCollectionIds(
    csp::systems::AssetSystem* assetSystem, const csp::common::Array<csp::common::String>& ids, csp::common::Array<csp::systems::Asset>& outAssets);

void UpdateAssetCollectionMetadata(csp::systems::AssetSystem* assetSystem, csp::systems::AssetCollection& assetCollection,
    const csp::common::Map<csp::common::String, csp::common::String>& inMetaData,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& tags,
    csp::common::Map<csp::common::String, csp::common::String>& outMetaData);