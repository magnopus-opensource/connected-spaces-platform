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
#include "CSP/Multiplayer/EventBus.h"
#include "CSP/Multiplayer/EventParameters.h"
#include "CSP/Systems/Assets/Asset.h"
#include "CSP/Systems/Assets/AssetCollection.h"
#include "CSP/Systems/Assets/GLTFMaterial.h"
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

} // namespace csp::web

namespace csp::multiplayer
{

class EventBus;

} // namespace csp::multiplayer

namespace csp::memory
{

CSP_START_IGNORE
template <typename T> void Delete(T* Ptr);
CSP_END_IGNORE

} // namespace csp::memory

namespace csp::systems
{

/// @ingroup Asset System
/// @brief Public facing system that allows uploading/downloading and creation of assets.
class CSP_API AssetSystem : public SystemBase
{
    CSP_START_IGNORE
    /** @cond DO_NOT_DOCUMENT */
    friend class SystemsManager;
    friend void csp::memory::Delete<AssetSystem>(AssetSystem* Ptr);
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
    CSP_ASYNC_RESULT void CreateAssetCollection(const csp::common::Optional<csp::common::String>& SpaceId,
        const csp::common::Optional<csp::common::String>& ParentAssetCollectionId, const csp::common::String& AssetCollectionName,
        const csp::common::Optional<csp::common::Map<csp::common::String, csp::common::String>>& Metadata, const EAssetCollectionType Type,
        const csp::common::Optional<csp::common::Array<csp::common::String>>& Tags, AssetCollectionResultCallback Callback);

    /// @brief Deletes a given asset collection.
    /// @param AssetCollection AssectCollection : asset collection to delete
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void DeleteAssetCollection(const AssetCollection& AssetCollection, NullResultCallback Callback);

    /// @brief Copies an array of asset collections to another space. Note that all source asset collections must belong to the same space.
    /// @param SourceAssetCollections csp::common::Array<AssetCollection> : The array of asset collections to copy. They must all belong to the same
    /// space.
    /// @param DestSpaceId  const csp::common::String : The unique identifier of the space to copy these asset collections to.
    /// @param CopyAsync const csp::common::Optional<bool> : Whether to instruct the services to perform the copy of the asset collections
    /// asynchronously.
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void CopyAssetCollectionsToSpace(csp::common::Array<AssetCollection>& SourceAssetCollections,
        const csp::common::String& DestSpaceId, bool CopyAsync, AssetCollectionsResultCallback Callback);

    /// @brief Finds an asset collection by its Id.
    /// @param AssetCollectionId csp::common::String : asset collection to delete
    /// @param Callback AssetCollectionResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetAssetCollectionById(const csp::common::String& AssetCollectionId, AssetCollectionResultCallback Callback);

    /// @brief Finds an asset collection by its Name.
    /// @param AssetCollectionName csp::common::String : name of the asset collection to be retrieved
    /// @param Callback AssetCollectionResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetAssetCollectionByName(const csp::common::String& AssetCollectionName, AssetCollectionResultCallback Callback);

    /// @brief Retrieves asset collections based on the specified search criteria.
    /// Results pagination is supported through the use of ResultsSkipNumber and ResultsMaxNumber.
    /// @param Space Space : optional space to get asset collections associated with it
    /// @param AssetCollectionParentId csp::common::String : optional asset collection parent id to get asset collections associated with it
    /// @param AssetCollectionType EAssetCollectionType : type of the asset collection
    /// @param AssetCollectionTags csp::common::Array<csp::common::String> : optional array of strings representing asset collection tags
    /// @param AssetCollectionNames csp::common::Optional<csp::common::Array<csp::common::String>> : optional array of strings representing asset
    /// collection names
    /// @param ResultsSkipNumber int : optional param representing the number of result entries that will be skipped from the result. For no skip pass
    /// nullptr.
    /// @param ResultsMaxNumber int : optional param representing the maximum number of result entries to be retrieved. For all available result
    /// entries pass nullptr.
    /// @param Callback AssetCollectionsResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void FindAssetCollections(const csp::common::Optional<csp::common::Array<csp::common::String>>& Ids,
        const csp::common::Optional<csp::common::String>& ParentId, const csp::common::Optional<csp::common::Array<csp::common::String>>& Names,
        const csp::common::Optional<csp::common::Array<EAssetCollectionType>>& Types,
        const csp::common::Optional<csp::common::Array<csp::common::String>>& Tags,
        const csp::common::Optional<csp::common::Array<csp::common::String>>& SpaceIds, const csp::common::Optional<int>& ResultsSkipNumber,
        const csp::common::Optional<int>& ResultsMaxNumber, AssetCollectionsResultCallback Callback);

    /// @brief Updates the Metadata field of an Asset Collection
    /// @param AssetCollection AssetCollection : asset collection to be updated
    /// @param NewMetadata csp::common::StringMap : the new metadata information that will replace the previous
    /// @param Tags csp::common::Array<csp::common::String : optional array of strings to replace the tags
    /// @param Callback AssetCollectionResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void UpdateAssetCollectionMetadata(const AssetCollection& AssetCollection,
        const csp::common::Map<csp::common::String, csp::common::String>& NewMetadata,
        const csp::common::Optional<csp::common::Array<csp::common::String>>& Tags, AssetCollectionResultCallback Callback);

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

    /// @brief Creates a new material backed by an AssetCollection/Asset.
    /// @param Name const csp::common::String& : The name of the new material.
    /// @param SpaceId const csp::common::String& : The space id this material is associated with.
    /// @param Metadata csp::common::Map<csp::common::String, csp::common::String>& : The metadata to be associated with the created material.
    /// @param AssetTags csp::common::Array<csp::common::String>& : Tags to be associated with the created material.
    /// @param Callback GLTFMaterialResultCallback : Callback when asynchronous task finishes.
    CSP_ASYNC_RESULT void CreateMaterial(const csp::common::String& Name, const csp::common::String& SpaceId,
        const csp::common::Map<csp::common::String, csp::common::String>& Metadata, const csp::
        common::Array<csp::common::String>& AssetTags,
        GLTFMaterialResultCallback Callback);

    /// @brief Updates an existing material's properties.
    /// The material should be retrieved through GetMaterials or GetMaterial.
    /// If the material doesn't exist, EResultCode::Failed will be returned.
    /// If the material hasn't changed, EResultCode::Success will still be returned.
    /// @param Material const GLTFMaterial& : The material to update
    /// @param Callback NullResultCallback : Callback when asynchronous task finishes.
    CSP_ASYNC_RESULT void UpdateMaterial(const GLTFMaterial& Material, NullResultCallback Callback);

    /// @brief Deletes a given material.
    /// The material should be retrieved through GetMaterials or GetMaterial.
    /// @param Material const GLTFMaterial& : The material to delete
    /// @param Callback NullResultCallback : Callback when asynchronous task finishes.
    CSP_ASYNC_RESULT void DeleteMaterial(const GLTFMaterial& Material, NullResultCallback Callback);

    /// @brief Gets all materials associated with the given space.
    /// @param SpaceId const csp::common::String& : The space id the material is associated with.
    /// @param Callback GLTFMaterialsResultCallback : Callback when asynchronous task finishes.
    CSP_ASYNC_RESULT void GetMaterials(const csp::common::String& SpaceId, GLTFMaterialsResultCallback Callback);

    /// @brief Gets a material using its AssetCollection and Asset Id.
    /// @param AssetCollectionId const csp::common::String& : The asset collection id this material is associated with.
    /// @param AssetId const csp::common::String& : The asset id this material is associated with.
    /// @param Callback GLTFMaterialResultCallback : Callback when asynchronous task finishes.
    CSP_ASYNC_RESULT void GetMaterial(
        const csp::common::String& AssetCollectionId, const csp::common::String& AssetId, GLTFMaterialResultCallback Callback);

    // The callback for receiving asset detail changes, contains an AssetDetailBlobParams with the details.
    typedef std::function<void(const csp::multiplayer::AssetDetailBlobParams&)> AssetDetailBlobChangedCallbackHandler;

    // Callback to receive material changes, contains a MaterialChangedParams with the details.
    typedef std::function<void(const csp::multiplayer::MaterialChangedParams&)> MaterialChangedCallbackHandler;

    /// @brief Sets a callback for an asset changed event. Triggered when assets, such as textures or meshes, are modified
    /// @param Callback AssetDetailBlobChangedCallbackHandler: Callback to receive data for the asset that has been changed.
    CSP_EVENT void SetAssetDetailBlobChangedCallback(AssetDetailBlobChangedCallbackHandler Callback);

    /// @brief Sets a callback for a material changed event.
    /// @param Callback MaterialChangedCallbackHandler: Callback to receive data for the material that has been changed.
    CSP_EVENT void SetMaterialChangedCallback(MaterialChangedCallbackHandler Callback);

    /// @brief Registers the system to listen for the named event.
    void RegisterSystemCallback() override;
    /// @brief Deregisters the system from listening for the named event.
    void DeregisterSystemCallback() override;
    /// @brief Deserialises the event values of the system.
    /// @param EventValues std::vector<signalr::value> : event values to deserialise
    CSP_NO_EXPORT void OnEvent(const std::vector<signalr::value>& EventValues) override;

private:
    AssetSystem(); // This constructor is only provided to appease the wrapper generator and should not be used
    CSP_NO_EXPORT AssetSystem(csp::web::WebClient* InWebClient, csp::multiplayer::EventBus* InEventBus);
    ~AssetSystem();

    CSP_ASYNC_RESULT void DeleteAssetCollectionById(const csp::common::String& AssetCollectionId, NullResultCallback Callback);
    CSP_ASYNC_RESULT void DeleteAssetById(
        const csp::common::String& AsseCollectiontId, const csp::common::String& AssetId, NullResultCallback Callback);

    csp::services::ApiBase* PrototypeAPI;
    csp::services::ApiBase* AssetDetailAPI;

    csp::web::RemoteFileManager* FileManager;

    AssetDetailBlobChangedCallbackHandler AssetDetailBlobChangedCallback;
    MaterialChangedCallbackHandler MaterialChangedCallback;
};

} // namespace csp::systems