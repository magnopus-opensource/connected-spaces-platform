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

#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Common/Array.h"
#include "CSP/Common/CancellationToken.h"
#include "CSP/Common/NetworkEventData.h"
#include "CSP/Common/Optional.h"
#include "CSP/Multiplayer/NetworkEventBus.h"
#include "CSP/Systems/Assets/AlphaVideoMaterial.h"
#include "CSP/Systems/Assets/Asset.h"
#include "CSP/Systems/Assets/AssetCollection.h"
#include "CSP/Systems/Assets/GLTFMaterial.h"
#include "CSP/Systems/Assets/LOD.h"
#include "CSP/Systems/Spaces/Space.h"
#include "CSP/Systems/SystemBase.h"

namespace async
{
CSP_START_IGNORE
template <typename T> class event_task;
template <typename T> class task;
CSP_END_IGNORE
}

namespace csp::services
{

class ApiBase;

} // namespace csp::services

namespace csp::web
{

class RemoteFileManager;

} // namespace csp::web

namespace csp::multiplayer
{

class NetworkEventBus;

} // namespace csp::multiplayer

namespace csp::systems
{

/// @ingroup Asset System
/// @brief Public facing system that allows uploading/downloading and creation of assets.
class CSP_API AssetSystem : public SystemBase
{
    CSP_START_IGNORE
    /** @cond DO_NOT_DOCUMENT */
    friend class SystemsManager;
    /** @endcond */
    CSP_END_IGNORE

public:
    /// @brief Creates an asset collection.
    /// @param Space Space : optional space to associate the asset collection with
    /// @param ParentAssetCollectionId csp::common::String : optional parent asset collection Id
    /// @param AssetCollectionName csp::common::String : name of the asset collection
    /// @param Metadata csp::common::StringMap : optional metadata info to be associated with the asset collection
    /// @param Type EAssetCollectionType : type of the new asset collection
    /// @param Tags csp::common::Array<csp::common::String> : optional array of strings to add to the asset collection as tags
    /// @param Callback AssetCollectionResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void CreateAssetCollection(const csp::common::Optional<csp::common::String>& spaceId,
        const csp::common::Optional<csp::common::String>& parentAssetCollectionId, const csp::common::String& assetCollectionName,
        const csp::common::Optional<csp::common::Map<csp::common::String, csp::common::String>>& metadata, const EAssetCollectionType type,
        const csp::common::Optional<csp::common::Array<csp::common::String>>& tags, AssetCollectionResultCallback callback);

    CSP_NO_EXPORT async::task<AssetCollectionResult> CreateAssetCollection(const csp::common::Optional<csp::common::String>& spaceId,
        const csp::common::Optional<csp::common::String>& parentAssetCollectionId, const csp::common::String& assetCollectionName,
        const csp::common::Optional<csp::common::Map<csp::common::String, csp::common::String>>& metadata, const EAssetCollectionType type,
        const csp::common::Optional<csp::common::Array<csp::common::String>>& tags);

    /// @brief Deletes a given asset collection.
    /// @param AssetCollection AssectCollection : asset collection to delete
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void DeleteAssetCollection(const AssetCollection& assetCollection, NullResultCallback callback);
    CSP_NO_EXPORT async::task<NullResult> DeleteAssetCollection(const AssetCollection& assetCollection);

    /// @brief Deletes a given array of asset collections.
    /// @param AssetCollections csp::common::Array<AssetCollection> : The array of asset collections to delete
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void DeleteMultipleAssetCollections(csp::common::Array<AssetCollection>& assetCollections, NullResultCallback callback);

    /// @brief Copies an array of asset collections to another space. Note that all source asset collections must belong to the same space.
    /// @param SourceAssetCollections csp::common::Array<AssetCollection> : The array of asset collections to copy. They must all belong to the same
    /// space.
    /// @param DestSpaceId  const csp::common::String : The unique identifier of the space to copy these asset collections to.
    /// @param CopyAsync const csp::common::Optional<bool> : Whether to instruct the services to perform the copy of the asset collections
    /// asynchronously.
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void CopyAssetCollectionsToSpace(csp::common::Array<AssetCollection>& sourceAssetCollections,
        const csp::common::String& destSpaceId, bool copyAsync, AssetCollectionsResultCallback callback);

    /// @brief Finds an asset collection by its Id.
    /// @param AssetCollectionId csp::common::String : asset collection to delete
    /// @param Callback AssetCollectionResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetAssetCollectionById(const csp::common::String& assetCollectionId, AssetCollectionResultCallback callback);
    CSP_NO_EXPORT async::task<AssetCollectionResult> GetAssetCollectionById(const csp::common::String& assetCollectionId);

    /// @brief Finds an asset collection by its Name.
    /// @param AssetCollectionName csp::common::String : name of the asset collection to be retrieved
    /// @param Callback AssetCollectionResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetAssetCollectionByName(const csp::common::String& assetCollectionName, AssetCollectionResultCallback callback);

    /// @brief Retrieves asset collections based on the specified search criteria.
    /// These parameters filter the search criteria using the intersection of the provided parameters.
    /// Results pagination is supported through the use of ResultsSkipNumber and ResultsMaxNumber.
    /// @param AssetCollectionIds : Search for asset collections with these ids.
    /// @param ParentId : Search for asset collections with this parent id.
    /// @param Names : Search for asset collections with these names.
    /// @param Types : Search for asset collections of these types.
    /// @param Tags : Search for asset collections with these user provided tags.
    /// @param SpaceIds : Search for asset collections associated with the provided spaces.
    /// @param ResultsSkipNumber : The number of result entries that will be skipped from the result. For no skip pass
    /// nullptr.
    /// @param ResultsMaxNumber : The maximum number of result entries to be retrieved. For all available results pass nullptr.
    /// @param Callback : Callback when asynchronous task finishes.
    CSP_ASYNC_RESULT void FindAssetCollections(const csp::common::Optional<csp::common::Array<csp::common::String>>& assetCollectionIds,
        const csp::common::Optional<csp::common::String>& parentId, const csp::common::Optional<csp::common::Array<csp::common::String>>& names,
        const csp::common::Optional<csp::common::Array<EAssetCollectionType>>& types,
        const csp::common::Optional<csp::common::Array<csp::common::String>>& tags,
        const csp::common::Optional<csp::common::Array<csp::common::String>>& spaceIds, const csp::common::Optional<int>& resultsSkipNumber,
        const csp::common::Optional<int>& resultsMaxNumber, AssetCollectionsResultCallback callback);

    CSP_NO_EXPORT async::task<AssetCollectionsResult> FindAssetCollections(const csp::common::Optional<csp::common::Array<csp::common::String>>& ids,
        const csp::common::Optional<csp::common::String>& parentId, const csp::common::Optional<csp::common::Array<csp::common::String>>& names,
        const csp::common::Optional<csp::common::Array<EAssetCollectionType>>& types,
        const csp::common::Optional<csp::common::Array<csp::common::String>>& tags,
        const csp::common::Optional<csp::common::Array<csp::common::String>>& spaceIds, const csp::common::Optional<int>& resultsSkipNumber,
        const csp::common::Optional<int>& resultsMaxNumber);

    /// @brief Updates the Metadata field of an Asset Collection
    /// @param AssetCollection AssetCollection : asset collection to be updated
    /// @param NewMetadata csp::common::StringMap : the new metadata information that will replace the previous
    /// @param Tags csp::common::Array<csp::common::String : optional array of strings to replace the tags
    /// @param Callback AssetCollectionResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void UpdateAssetCollectionMetadata(const AssetCollection& assetCollection,
        const csp::common::Map<csp::common::String, csp::common::String>& newMetadata,
        const csp::common::Optional<csp::common::Array<csp::common::String>>& tags, AssetCollectionResultCallback callback);

    CSP_NO_EXPORT async::task<AssetCollectionResult> UpdateAssetCollectionMetadata(const AssetCollection& assetCollection,
        const csp::common::Map<csp::common::String, csp::common::String>& newMetadata,
        const csp::common::Optional<csp::common::Array<csp::common::String>>& tags);

    /// @brief Retrieves the number asset collections based on the specified search criteria.
    /// @param ParentId csp::common::String : optional asset collection parent id to get asset collections associated with it
    /// @param Names csp::common::Optional<csp::common::Array<csp::common::String>> : optional array of asset collection names
    /// @param Types EAssetCollectionType : type of the asset collection
    /// @param Tags csp::common::Array<csp::common::String> : optional array of asset collection tags
    /// collection names
    /// @param SpaceIds csp::common::Array<csp::common::String> : optional space ids to get asset collections associated with them
    /// @param Callback AssetCollectionCountResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetAssetCollectionCount(const csp::common::Optional<csp::common::Array<csp::common::String>>& ids,
        const csp::common::Optional<csp::common::String>& parentId, const csp::common::Optional<csp::common::Array<csp::common::String>>& names,
        const csp::common::Optional<csp::common::Array<EAssetCollectionType>>& types,
        const csp::common::Optional<csp::common::Array<csp::common::String>>& tags,
        const csp::common::Optional<csp::common::Array<csp::common::String>>& spaceIds, csp::systems::AssetCollectionCountResultCallback callback);

    /// @brief Creates a new asset.
    /// @param AssetCollection AssetCollection : the parent collection for the asset to be associated with
    /// @param Name csp::common::String : name of the asset collection
    /// @param ThirdPartyPackagedAssetIdentifier csp::common::String : optional id to a third party packaged asset identifier
    /// @param ThirdPartyPlatform EThirdPartyPlatform : optional enum class for Third Part Platform
    /// @param Type csp::systems::EAssetType : type of the asset
    /// @param Callback AssetResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void CreateAsset(const AssetCollection& assetCollection, const csp::common::String& name,
        const csp::common::Optional<csp::common::String>& thirdPartyPackagedAssetIdentifier,
        const csp::common::Optional<csp::systems::EThirdPartyPlatform>& thirdPartyPlatform, EAssetType type, AssetResultCallback callback);

    CSP_NO_EXPORT async::task<AssetResult> CreateAsset(const AssetCollection& assetCollection, const csp::common::String& name,
        const csp::common::Optional<csp::common::String>& thirdPartyPackagedAssetIdentifier,
        const csp::common::Optional<csp::systems::EThirdPartyPlatform>& thirdPartyPlatform, EAssetType type);

    /// @brief Update a given asset.
    /// @param Asset Asset : asset to update
    /// @param Callback AssetResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void UpdateAsset(const Asset& asset, AssetResultCallback callback);

    /// @brief Deletes a given asset.
    /// @param AssetCollection AssetCollection : the parent collection that the asset is associated with
    /// @param Asset Asset : asset to delete
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void DeleteAsset(const AssetCollection& assetCollection, const Asset& asset, NullResultCallback callback);
    CSP_NO_EXPORT async::task<NullResult> DeleteAsset(const AssetCollection& assetCollection, const Asset& asset);

    /// @brief Retrieves all assets in a given asset collection.
    /// @param AssetCollection AssetCollection : collection to get all assets from
    /// @param Callback AssetsResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetAssetsInCollection(const AssetCollection& assetCollection, AssetsResultCallback callback);

    /// @brief Retrieves the asset specified by the Id
    /// @param AssetCollectionId csp::common::String : the Id of the asset collection containing the asset
    /// @param AssetId csp::common::String : the Id of the asset to retrieve
    /// @param Callback AssetResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetAssetById(
        const csp::common::String& assetCollectionId, const csp::common::String& assetId, AssetResultCallback callback);

    /// @brief Retrieves all assets that belong to the asset collections with the give Ids.
    /// @param AssetCollectionIds csp::common::Array<csp::common::String> : collection of asset collection Ids get all assets from
    /// @param Callback AssetsResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetAssetsByCollectionIds(const csp::common::Array<csp::common::String>& assetCollectionIds, AssetsResultCallback callback);

    /// @brief Retrieves assets based on the specified search criteria.
    /// @param AssetCollectionIds csp::common::Array<csp::common::String> : the asset collection Ids that will be used as search criteria. Note that
    /// you have to pass at least one Id.
    /// @param AssetIds csp::common::Array<csp::common::String> : optional array of strings representing asset ids
    /// @param AssetNames csp::common::Array<csp::common::String> : optional array of strings representing asset names
    /// @param AssetTypes csp::common::Array<EAssetType> : optional array of asset types
    /// @param Callback AssetsResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetAssetsByCriteria(const csp::common::Array<csp::common::String>& assetCollectionIds,
        const csp::common::Optional<csp::common::Array<csp::common::String>>& assetIds,
        const csp::common::Optional<csp::common::Array<csp::common::String>>& assetNames,
        const csp::common::Optional<csp::common::Array<EAssetType>>& assetTypes, AssetsResultCallback callback);

    CSP_NO_EXPORT async::task<AssetsResult> GetAssetsByCriteria(const csp::common::Array<csp::common::String>& assetCollectionIds,
        const csp::common::Optional<csp::common::Array<csp::common::String>>& assetIds,
        const csp::common::Optional<csp::common::Array<csp::common::String>>& assetNames,
        const csp::common::Optional<csp::common::Array<EAssetType>>& assetTypes);

    /// @brief Uploads data for the given asset to CHS from the given source.
    /// @param AssetCollection AssetCollection : collection the asset is associated to
    /// @param Asset Asset : asset to upload data for
    /// @param AssetDataSource AssetDataSource : asset data to upload
    /// AssetDataSource is an interface. A derived class must be passed.
    /// @param Callback UriResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT_WITH_PROGRESS void UploadAssetData(
        const AssetCollection& assetCollection, const Asset& asset, const AssetDataSource& assetDataSource, UriResultCallback callback);

    /// @brief Uploads data for the given asset to CHS from the given source, taking a CancellationToken to allow cancelling the request.
    /// @param AssetCollection AssetCollection : collection the asset is associated to
    /// @param Asset Asset : asset to upload data for
    /// @param AssetDataSource AssetDataSource : asset data to upload
    /// AssetDataSource is an interface. A derived class must be passed.
    /// @param CancellationToken csp::common::CancellationToken : token for cancelling upload
    /// @param Callback UriResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT_WITH_PROGRESS void UploadAssetDataEx(const AssetCollection& assetCollection, const Asset& asset,
        const AssetDataSource& assetDataSource, csp::common::CancellationToken& cancellationToken, UriResultCallback callback);

    CSP_NO_EXPORT async::task<UriResult> UploadAssetDataEx(const AssetCollection& assetCollection, const Asset& asset,
        const AssetDataSource& assetDataSource, csp::common::CancellationToken& cancellationToken);

    /// @brief Downloads data for a given Asset from CHS.
    /// @param Asset Asset : asset to download data for
    /// @param Callback DataResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void DownloadAssetData(const Asset& asset, AssetDataResultCallback callback);

    /// @brief Downloads data for a given Asset from CHS, taking a CancellationToken to allow cancelling the request.
    /// @param Asset Asset : asset to download data for
    /// @param CancellationToken csp::common::CancellationToken : token for cancelling download
    /// @param Callback AssetDataResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void DownloadAssetDataEx(
        const Asset& asset, csp::common::CancellationToken& cancellationToken, AssetDataResultCallback callback);

    /// @brief Get the size of the data associated with an Asset.
    /// @param Asset Asset : asset to get data size for
    /// @param Callback UInt64ResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetAssetDataSize(const Asset& asset, UInt64ResultCallback callback);

    /// @brief Gets a LOD chain within the given AssetCollection.
    /// @param AssetCollection AssetCollection : AssetCollection which contains the LOD chain.
    /// @param Callback LODChainResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetLODChain(const AssetCollection& assetCollection, LODChainResultCallback callback);

    /// @brief Registers an asset to the LOD chain
    /// @param AssetCollection AssetCollection : AssetCollection which contains the LOD chain.
    /// @param Asset Asset : Asset to register as LOD
    /// @param Asset int : LOD level for Asset to be registered to
    /// @param Callback AssetResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT_WITH_PROGRESS void RegisterAssetToLODChain(
        const AssetCollection& assetCollection, const Asset& asset, int lodLevel, AssetResultCallback callback);

    /// @brief Creates a new material backed by an AssetCollection/Asset.
    /// @param Name const csp::common::String& : The name of the new material.
    /// @param ShaderType const csp::systems::EShaderType : The type of shader model the material is associated with.
    /// @param SpaceId const csp::common::String& : The space id this material is associated with.
    /// @param Metadata csp::common::Map<csp::common::String, csp::common::String>& : The metadata to be associated with the created material.
    /// @param AssetTags csp::common::Array<csp::common::String>& : Tags to be associated with the created material.
    /// @param Callback MaterialResultCallback : Callback when asynchronous task finishes.
    CSP_ASYNC_RESULT void CreateMaterial(const csp::common::String& name, const csp::systems::EShaderType shaderType,
        const csp::common::String& spaceId, csp::common::Map<csp::common::String, csp::common::String>& metadata,
        const csp::common::Array<csp::common::String>& assetTags, MaterialResultCallback callback);

    /// @brief Updates an existing material's properties.
    /// The material should be retrieved through GetMaterials or GetMaterial.
    /// If the material doesn't exist, EResultCode::Failed will be returned.
    /// If the material hasn't changed, EResultCode::Success will still be returned.
    /// @param Material const Material& : The material to update
    /// @param Callback NullResultCallback : Callback when asynchronous task finishes.
    CSP_ASYNC_RESULT void UpdateMaterial(const Material& material, NullResultCallback callback);

    /// @brief Deletes a given material.
    /// The material should be retrieved through GetMaterials or GetMaterial.
    /// @param Material const Material& : The material to delete
    /// @param Callback NullResultCallback : Callback when asynchronous task finishes.
    CSP_ASYNC_RESULT void DeleteMaterial(const Material& material, NullResultCallback callback);

    /// @brief Gets all materials associated with the given space.
    /// @param SpaceId const csp::common::String& : The space id the material is associated with.
    /// @param Callback MaterialsResultCallback : Callback when asynchronous task finishes.
    CSP_ASYNC_RESULT void GetMaterials(const csp::common::String& spaceId, MaterialsResultCallback callback);

    /// @brief Gets a material using its AssetCollection and Asset Id.
    /// @details This is slower than GetMaterialFromUri, as it needs to first retrieve the internal AssetCollection and Asset.
    /// @param AssetCollectionId const csp::common::String& : The asset collection id this material is associated with.
    /// @param AssetId const csp::common::String& : The asset id this material is associated with.
    /// @param Callback MaterialResultCallback : Callback when asynchronous task finishes.
    CSP_ASYNC_RESULT void GetMaterial(
        const csp::common::String& assetCollectionId, const csp::common::String& assetId, MaterialResultCallback callback);

    /// @brief Gets a material using it's Uri.
    /// @details This function does not retrieve the AssetCollection or Asset remotely. It downloads the material directly from the uri.
    /// This should be used in an offline context where The AssetCollection and Asset has been parsed from a SceneData file.
    /// @param AssetCollection const csp::system::AssetCollection& : The asset collection this material is associated with.
    /// @param AssetId const csp::common::String& : The asset id this material is associated with.
    /// @param Uri const csp::common::String& : The uri this material was uploaded to.
    /// @param Callback MaterialResultCallback : Callback when asynchronous task finishes.
    CSP_ASYNC_RESULT void GetMaterialFromUri(const csp::systems::AssetCollection& assetCollection, const csp::common::String& assetId,
        const csp::common::String& uri, MaterialResultCallback callback);

    // The callback for receiving asset detail changes, contains an AssetDetailBlobParams with the details.
    typedef std::function<void(const csp::common::AssetDetailBlobChangedNetworkEventData&)> AssetDetailBlobChangedCallbackHandler;

    // Callback to receive material changes, contains a MaterialChangedParams with the details.
    typedef std::function<void(const csp::common::MaterialChangedParams&)> MaterialChangedCallbackHandler;

    /// @brief Sets a callback for an asset changed event. Triggered when assets, such as textures or meshes, are modified
    /// @param Callback AssetDetailBlobChangedCallbackHandler: Callback to receive data for the asset that has been changed.
    CSP_EVENT void SetAssetDetailBlobChangedCallback(AssetDetailBlobChangedCallbackHandler callback);

    /// @brief Sets a callback for a material changed event.
    /// @param Callback MaterialChangedCallbackHandler: Callback to receive data for the material that has been changed.
    CSP_EVENT void SetMaterialChangedCallback(MaterialChangedCallbackHandler callback);

    /// @brief Registers the system to listen for the named event.
    void RegisterSystemCallback() override;
    /// @brief Deserialises the event values of the system.
    /// @param EventValues std::vector<signalr::value> : event values to deserialise
    CSP_NO_EXPORT void OnAssetDetailBlobChangedEvent(const csp::common::NetworkEventData& networkEventData);

private:
    AssetSystem(); // This constructor is only provided to appease the wrapper generator and should not be used
    CSP_NO_EXPORT AssetSystem(csp::web::WebClient* webClient, csp::multiplayer::NetworkEventBus& eventBus,
        const csp::common::IAuthContext& inAuthContext, common::LogSystem& logSystem);
    ~AssetSystem();

    CSP_ASYNC_RESULT void DeleteAssetCollectionById(const csp::common::String& assetCollectionId, NullResultCallback callback);
    CSP_ASYNC_RESULT void DeleteAssetById(
        const csp::common::String& asseCollectiontId, const csp::common::String& assetId, NullResultCallback callback);

    async::task<MaterialResult> DownloadMaterial(
        const AssetCollection& assetCollection, const csp::common::String& assetId, const csp::common::String& uri);

    std::function<async::task<MaterialsResult>(const AssetsResult&)> DownloadAllMaterials(
        const csp::common::Array<AssetCollection>& assetCollections);

    csp::services::ApiBase* m_prototypeApi;
    csp::services::ApiBase* m_assetDetailApi;

    csp::web::RemoteFileManager* m_fileManager;

    AssetDetailBlobChangedCallbackHandler m_assetDetailBlobChangedCallback;
    MaterialChangedCallbackHandler m_materialChangedCallback;
};

} // namespace csp::systems
