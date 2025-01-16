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

#include "CSP/CSPCommon.h"
#include "CSP/Common/Array.h"
#include "CSP/Common/CancellationToken.h"
#include "CSP/Common/Optional.h"
#include "CSP/Systems/Assets/Asset.h"
#include "CSP/Systems/Assets/AssetCollection.h"
#include "CSP/Systems/Assets/LOD.h"
#include "CSP/Systems/Spaces/Space.h"
#include "CSP/Systems/SystemBase.h"

namespace csp::services
{

class ApiBase;

} // namespace csp::services

namespace csp::web
{

class RemoteFileManager;

}

namespace csp::systems
{

/// @ingroup Asset System
/// @brief Public facing system that allows uploading/downloading and creation of assets.
class CSP_API CSP_NO_DISPOSE AssetSystem : public SystemBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class SystemsManager;
    /** @endcond */

public:
    ~AssetSystem();

    /// @brief Creates an asset collection.
    /// @param Space Space : optional space to associate the asset collection with
    /// @param ParentAssetCollectionId csp::common::String : optional parent asset collection Id
    /// @param AssetCollectionName csp::common::String : name of the asset collection
    /// @param Metadata csp::common::StringMap : optional metadata info to be associated with the asset collection
    /// @param Type EAssetCollectionType : type of the new asset collection
    /// @param Tags csp::common::Array<csp::common::String> : optional array of strings to add to the asset collection as tags
    /// @param Callback AssetCollectionResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void CreateAssetCollection(const csp::common::Optional<csp::common::String>& SpaceId,
        const csp::common::Optional<csp::common::String>& ParentAssetCollectionId, const csp::common::String& AssetCollectionName,
        const csp::common::Optional<csp::common::Map<csp::common::String, csp::common::String>>& Metadata, const EAssetCollectionType Type,
        const csp::common::Optional<csp::common::Array<csp::common::String>>& Tags, AssetCollectionResultCallback Callback);

    /// @brief Deletes a given asset collection.
    /// @param AssetCollection AssectCollection : asset collection to delete
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void DeleteAssetCollection(const AssetCollection& AssetCollection, NullResultCallback Callback);

    /// @brief Finds an asset collection by its Id.
    /// @param AssetCollectionId csp::common::String : asset collection to delete
    /// @param Callback AssetCollectionResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetAssetCollectionById(const csp::common::String& AssetCollectionId, AssetCollectionResultCallback Callback);

    /// @brief Finds an asset collection by its Name.
    /// @param AssetCollectionName csp::common::String : name of the asset collection to be retrieved
    /// @param Callback AssetCollectionResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetAssetCollectionByName(const csp::common::String& AssetCollectionName, AssetCollectionResultCallback Callback);

    /// @brief Finds a collection of asset collections by their Ids.
    /// @param AssetCollectionIds csp::common::Array<csp::common::String> : an array of Ids to search for
    /// @param Callback AssetCollectionResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetAssetCollectionsByIds(
        const csp::common::Array<csp::common::String>& AssetCollectionIds, AssetCollectionsResultCallback Callback);

    /// @brief Retrieves asset collections based on the specified search criteria.
    /// Results pagination is supported through the use of ResultsSkipNumber and ResultsMaxNumber.
    /// @param Space Space : optional space to get asset collections associated with it
    /// @param AssetCollectionParentId csp::common::String : optional asset collection parent id to get asset collections associated with it
    /// @param AssetCollectionType EAssetCollectionType : type of the asset collection
    /// @param AssetCollectionTags csp::common::Array<csp::common::String : optional array of strings representing asset collection tags
    /// @param AssetCollectionNames csp::common::Optional<csp::common::Array<csp::common::String : optional array of strings representing asset
    /// collection names
    /// @param ResultsSkipNumber int : optional param representing the number of result entries that will be skipped from the result. For no skip pass
    /// nullptr.
    /// @param ResultsMaxNumber int : optional param representing the maximum number of result entries to be retrieved. For all available result
    /// entries pass nullptr.
    /// @param Callback AssetCollectionsResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetAssetCollectionsByCriteria(const csp::common::Optional<csp::common::String>& SpaceId,
        const csp::common::Optional<csp::common::String>& AssetCollectionParentId,
        const csp::common::Optional<EAssetCollectionType>& AssetCollectionType,
        const csp::common::Optional<csp::common::Array<csp::common::String>>& AssetCollectionTags,
        const csp::common::Optional<csp::common::Array<csp::common::String>>& AssetCollectionNames,
        const csp::common::Optional<int>& ResultsSkipNumber, const csp::common::Optional<int>& ResultsMaxNumber,
        AssetCollectionsResultCallback Callback);

    /// @brief Updates the Metadata field of an Asset Collection
    /// @param AssetCollection AssetCollection : asset collection to be updated
    /// @param NewMetadata csp::common::StringMap : the new metadata information that will replace the previous
    /// @param Callback AssetCollectionResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void UpdateAssetCollectionMetadata(const AssetCollection& AssetCollection,
        const csp::common::Map<csp::common::String, csp::common::String>& NewMetadata, AssetCollectionResultCallback Callback);

    /// @brief Creates a new asset.
    /// @param AssetCollection AssetCollection : the parent collection for the asset to be associated with
    /// @param Name csp::common::String : name of the asset collection
    /// @param ThirdPartyPackagedAssetIdentifier csp::common::String : optional id to a third party packaged asset identifier
    /// @param ThirdPartyPlatform EThirdPartyPlatform : optional enum class for Third Part Platform
    /// @param Type csp::systems::EAssetType : type of the asset
    /// @param Callback AssetResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void CreateAsset(const AssetCollection& AssetCollection, const csp::common::String& Name,
        const csp::common::Optional<csp::common::String>& ThirdPartyPackagedAssetIdentifier,
        const csp::common::Optional<csp::systems::EThirdPartyPlatform>& ThirdPartyPlatform, EAssetType Type, AssetResultCallback Callback);

    /// @brief Update a given asset.
    /// @param Asset Asset : asset to update
    /// @param Callback AssetResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void UpdateAsset(const Asset& Asset, AssetResultCallback Callback);

    /// @brief Deletes a given asset.
    /// @param AssetCollection AssetCollection : the parent collection that the asset is associated with
    /// @param Asset Asset : asset to delete
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void DeleteAsset(const AssetCollection& AssetCollection, const Asset& Asset, NullResultCallback Callback);

    /// @brief Retrieves all assets in a given asset collection.
    /// @param AssetCollection AssetCollection : collection to get all assets from
    /// @param Callback AssetsResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetAssetsInCollection(const AssetCollection& AssetCollection, AssetsResultCallback Callback);

    /// @brief Retrieves the asset specified by the Id
    /// @param AssetCollectionId csp::common::String : the Id of the asset collection containing the asset
    /// @param AssetId csp::common::String : the Id of the asset to retrieve
    /// @param Callback AssetResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetAssetById(
        const csp::common::String& AssetCollectionId, const csp::common::String& AssetId, AssetResultCallback Callback);

    /// @brief Retrieves all assets that belong to the asset collections with the give Ids.
    /// @param AssetCollectionIds csp::common::Array<csp::common::String> : collection of asset collection Ids get all assets from
    /// @param Callback AssetsResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetAssetsByCollectionIds(const csp::common::Array<csp::common::String>& AssetCollectionIds, AssetsResultCallback Callback);

    /// @brief Retrieves assets based on the specified search criteria.
    /// @param AssetCollectionIds csp::common::Array<csp::common::String> : the asset collection Ids that will be used as search criteria. Note that
    /// you have to pass at least one Id.
    /// @param AssetIds csp::common::Array<csp::common::String> : optional array of strings representing asset ids
    /// @param AssetNames csp::common::Array<csp::common::String> : optional array of strings representing asset names
    /// @param AssetTypes csp::common::Array<EAssetType> : optional array of asset types
    /// @param Callback AssetsResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetAssetsByCriteria(const csp::common::Array<csp::common::String>& AssetCollectionIds,
        const csp::common::Optional<csp::common::Array<csp::common::String>>& AssetIds,
        const csp::common::Optional<csp::common::Array<csp::common::String>>& AssetNames,
        const csp::common::Optional<csp::common::Array<EAssetType>>& AssetTypes, AssetsResultCallback Callback);

    /// @brief Uploads data for the given asset to CHS from the given source.
    /// @param AssetCollection AssetCollection : collection the asset is associated to
    /// @param Asset Asset : asset to upload data for
    /// @param AssetDataSource AssetDataSource : asset data to upload
    /// AssetDataSource is an interface. A derived class must be passed.
    /// @param Callback UriResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT_WITH_PROGRESS void UploadAssetData(
        const AssetCollection& AssetCollection, const Asset& Asset, const AssetDataSource& AssetDataSource, UriResultCallback Callback);

    /// @brief Uploads data for the given asset to CHS from the given source, taking a CancellationToken to allow cancelling the request.
    /// @param AssetCollection AssetCollection : collection the asset is associated to
    /// @param Asset Asset : asset to upload data for
    /// @param AssetDataSource AssetDataSource : asset data to upload
    /// AssetDataSource is an interface. A derived class must be passed.
    /// @param CancellationToken csp::common::CancellationToken : token for cancelling upload
    /// @param Callback UriResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT_WITH_PROGRESS void UploadAssetDataEx(const AssetCollection& AssetCollection, const Asset& Asset,
        const AssetDataSource& AssetDataSource, csp::common::CancellationToken& CancellationToken, UriResultCallback Callback);

    /// @brief Downloads data for a given Asset from CHS.
    /// @param Asset Asset : asset to download data for
    /// @param Callback DataResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void DownloadAssetData(const Asset& Asset, AssetDataResultCallback Callback);

    /// @brief Downloads data for a given Asset from CHS, taking a CancellationToken to allow cancelling the request.
    /// @param Asset Asset : asset to download data for
    /// @param CancellationToken csp::common::CancellationToken : token for cancelling download
    /// @param Callback AssetDataResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void DownloadAssetDataEx(
        const Asset& Asset, csp::common::CancellationToken& CancellationToken, AssetDataResultCallback Callback);

    /// @brief Get the size of the data associated with an Asset.
    /// @param Asset Asset : asset to get data size for
    /// @param Callback UInt64ResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetAssetDataSize(const Asset& Asset, UInt64ResultCallback Callback);

    /// @brief Gets a LOD chain within the given AssetCollection.
    /// @param AssetCollection AssetCollection : AssetCollection which contains the LOD chain.
    /// @param Callback LODChainResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetLODChain(const AssetCollection& AssetCollection, LODChainResultCallback Callback);

    /// @brief Registers an asset to the LOD chain
    /// @param AssetCollection AssetCollection : AssetCollection which contains the LOD chain.
    /// @param Asset Asset : Asset to register as LOD
    /// @param Asset int : LOD level for Asset to be registered to
    /// @param Callback AssetResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT_WITH_PROGRESS void RegisterAssetToLODChain(
        const AssetCollection& AssetCollection, const Asset& Asset, int LODLevel, AssetResultCallback Callback);

private:
    AssetSystem(); // This constructor is only provided to appease the wrapper generator and should not be used
    CSP_NO_EXPORT AssetSystem(csp::web::WebClient* InWebClient);

    csp::services::ApiBase* PrototypeAPI;
    csp::services::ApiBase* AssetDetailAPI;

    csp::web::RemoteFileManager* FileManager;
};

} // namespace csp::systems
