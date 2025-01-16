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

void CreateAssetCollection(csp::systems::AssetSystem* AssetSystem, const csp::common::Optional<csp::common::String>& SpaceId,
    const csp::common::Optional<csp::common::String>& ParentId, const csp::common::String& Name,
    const csp::common::Optional<csp::systems::EAssetCollectionType>& AssetCollectionType,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& Tags, csp::systems::AssetCollection& OutAssetCollection);

void DeleteAssetCollection(csp::systems::AssetSystem* AssetSystem, csp::systems::AssetCollection& AssetCollection);

void GetAssetCollections(
    csp::systems::AssetSystem* AssetSystem, csp::systems::Space& Space, csp::common::Array<csp::systems::AssetCollection>& OutAssetCollections);

void GetAssetCollectionsByIds(csp::systems::AssetSystem* AssetSystem, const csp::common::Array<csp::common::String>& Ids,
    csp::common::Array<csp::systems::AssetCollection>& OutAssetCollections);

void CreateAsset(csp::systems::AssetSystem* AssetSystem, csp::systems::AssetCollection& AssetCollection, const csp::common::String& Name,
    const csp::common::Optional<csp::common::String>& ThirdPartyPackagedAssetIdentifier,
    const csp::common::Optional<csp::systems::EThirdPartyPlatform>& ThirdPartyPlatform, csp::systems::Asset& OutAsset);

void UploadAssetData(csp::systems::AssetSystem* AssetSystem, const csp::systems::AssetCollection& AssetCollection, const csp::systems::Asset& Asset,
    const csp::systems::FileAssetDataSource& Source, csp::common::String& OutUri);

void UploadAssetData(csp::systems::AssetSystem* AssetSystem, const csp::systems::AssetCollection& AssetCollection, const csp::systems::Asset& Asset,
    const csp::systems::BufferAssetDataSource& Source, csp::common::String& OutUri);

void GetAssetById(csp::systems::AssetSystem* AssetSystem, const csp::common::String& AssetCollectionId, const csp::common::String& AssetId,
    csp::systems::Asset& OutAsset);

void DeleteAsset(csp::systems::AssetSystem* AssetSystem, csp::systems::AssetCollection& AssetCollection, csp::systems::Asset& Asset);

void UpdateAsset(csp::systems::AssetSystem* AssetSystem, csp::systems::AssetCollection& AssetCollection, csp::systems::Asset& Asset);

void GetAssetsInCollection(
    csp::systems::AssetSystem* AssetSystem, csp::systems::AssetCollection& AssetCollection, csp::common::Array<csp::systems::Asset>& OutAssets);

void GetAssetsByCollectionIds(
    csp::systems::AssetSystem* AssetSystem, const csp::common::Array<csp::common::String>& Ids, csp::common::Array<csp::systems::Asset>& OutAssets);

void UpdateAssetCollectionMetadata(csp::systems::AssetSystem* AssetSystem, csp::systems::AssetCollection& AssetCollection,
    const csp::common::Map<csp::common::String, csp::common::String>& InMetaData,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& Tags,
    csp::common::Map<csp::common::String, csp::common::String>& OutMetaData);