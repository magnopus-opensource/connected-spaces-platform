#pragma once

#include "Olympus/Common/Array.h"
#include "Olympus/Common/CancellationToken.h"
#include "Olympus/Common/Optional.h"
#include "Olympus/OlympusCommon.h"
#include "Olympus/Systems/Assets/Asset.h"
#include "Olympus/Systems/Assets/AssetCollection.h"
#include "Olympus/Systems/Assets/LOD.h"
#include "Olympus/Systems/Spaces/Space.h"
#include "Olympus/Systems/SystemBase.h"

namespace oly_services
{

class ApiBase;

} // namespace oly_services

namespace oly_web
{

class RemoteFileManager;

}

namespace oly_systems
{

/// @ingroup Asset System
/// @brief Public facing system that allows uploading/downloading and creation of assets.
class OLY_API OLY_NO_DISPOSE AssetSystem : public SystemBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class SystemsManager;
    /** @endcond */

public:
    ~AssetSystem();

    /// @brief Creates an asset collection.
    /// @param Space Space : optional space to associate the asset collection with
    /// @param ParentAssetCollectionId oly_common::String : optional parent asset collection Id
    /// @param AssetCollectionName oly_common::String : name of the asset collection
    /// @param Metadata oly_common::StringMap : optional metadata info to be associated with the asset collection
    /// @param Type EAssetCollectionType : type of the new asset collection
    /// @param Tags oly_common::Array<oly_common::String> : optional array of strings to add to the asset collection as tags
    /// @param Callback AssetCollectionResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void CreateAssetCollection(const oly_common::Optional<oly_common::String>& SpaceId,
        const oly_common::Optional<oly_common::String>& ParentAssetCollectionId, const oly_common::String& AssetCollectionName,
        const oly_common::Optional<oly_common::Map<oly_common::String, oly_common::String>>& Metadata, const EAssetCollectionType Type,
        const oly_common::Optional<oly_common::Array<oly_common::String>>& Tags, AssetCollectionResultCallback Callback);

    /// @brief Deletes a given asset collection.
    /// @param AssetCollection AssectCollection : asset collection to delete
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void DeleteAssetCollection(const AssetCollection& AssetCollection, NullResultCallback Callback);

    /// @brief Finds an asset collection by its Id.
    /// @param AssetCollectionId oly_common::String : asset collection to delete
    /// @param Callback AssetCollectionResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void GetAssetCollectionById(const oly_common::String& AssetCollectionId, AssetCollectionResultCallback Callback);

    /// @brief Finds an asset collection by its Name.
    /// @param AssetCollectionName oly_common::String : name of the asset collection to be retrieved
    /// @param Callback AssetCollectionResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void GetAssetCollectionByName(const oly_common::String& AssetCollectionName, AssetCollectionResultCallback Callback);

    /// @brief Finds a collection of asset collections by their Ids.
    /// @param AssetCollectionIds oly_common::Array<oly_common::String> : an array of Ids to search for
    /// @param Callback AssetCollectionResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void GetAssetCollectionsByIds(
        const oly_common::Array<oly_common::String>& AssetCollectionIds, AssetCollectionsResultCallback Callback);

    /// @brief Retrieves asset collections based on the specified search criteria.
    /// Results pagination is supported through the use of ResultsSkipNumber and ResultsMaxNumber.
    /// @param Space Space : optional space to get asset collections associated with it
    /// @param AssetCollectionParentId oly_common::String : optional asset collection parent id to get asset collections associated with it
    /// @param AssetCollectionType EAssetCollectionType : type of the asset collection
    /// @param AssetCollectionTags oly_common::Array<oly_common::String : optional array of strings representing asset collection tags
    /// @param AssetCollectionNames oly_common::Optional<oly_common::Array<oly_common::String : optional array of strings representing asset
    /// collection names
    /// @param ResultsSkipNumber int : optional param representing the number of result entries that will be skipped from the result. For no skip pass
    /// nullptr.
    /// @param ResultsMaxNumber int : optional param representing the maximum number of result entries to be retrieved. For all available result
    /// entries pass nullptr.
    /// @param Callback AssetCollectionsResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void GetAssetCollectionsByCriteria(const oly_common::Optional<oly_common::String>& SpaceId,
        const oly_common::Optional<oly_common::String>& AssetCollectionParentId,
        const oly_common::Optional<EAssetCollectionType>& AssetCollectionType,
        const oly_common::Optional<oly_common::Array<oly_common::String>>& AssetCollectionTags,
        const oly_common::Optional<oly_common::Array<oly_common::String>>& AssetCollectionNames, const oly_common::Optional<int>& ResultsSkipNumber,
        const oly_common::Optional<int>& ResultsMaxNumber, AssetCollectionsResultCallback Callback);

    /// @brief Updates the Metadata field of an Asset Collection
    /// @param AssetCollection AssetCollection : asset collection to be updated
    /// @param NewMetadata oly_common::StringMap : the new metadata information that will replace the previous
    /// @param Callback AssetCollectionResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void UpdateAssetCollectionMetadata(const AssetCollection& AssetCollection,
        const oly_common::Map<oly_common::String, oly_common::String>& NewMetadata, AssetCollectionResultCallback Callback);

    /// @brief Creates a new asset.
    /// @param AssetCollection AssetCollection : the parent collection for the asset to be associated with
    /// @param Name oly_common::String : name of the asset collection
    /// @param ThirdPartyPackagedAssetIdentifier oly_common::String : optional id to a third party packaged asset identifier
    /// @param ThirdPartyPlatform EThirdPartyPlatform : optional enum class for Third Part Platform
    /// @param Type oly_systems::EAssetType : type of the asset
    /// @param Callback AssetResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void CreateAsset(const AssetCollection& AssetCollection, const oly_common::String& Name,
        const oly_common::Optional<oly_common::String>& ThirdPartyPackagedAssetIdentifier,
        const oly_common::Optional<oly_systems::EThirdPartyPlatform>& ThirdPartyPlatform, EAssetType Type, AssetResultCallback Callback);

    /// @brief Update a given asset.
    /// @param Asset Asset : asset to update
    /// @param Callback AssetResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void UpdateAsset(const Asset& Asset, AssetResultCallback Callback);

    /// @brief Deletes a given asset.
    /// @param AssetCollection AssetCollection : the parent collection that the asset is associated with
    /// @param Asset Asset : asset to delete
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void DeleteAsset(const AssetCollection& AssetCollection, const Asset& Asset, NullResultCallback Callback);

    /// @brief Retrieves all assets in a given asset collection.
    /// @param AssetCollection AssetCollection : collection to get all assets from
    /// @param Callback AssetsResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void GetAssetsInCollection(const AssetCollection& AssetCollection, AssetsResultCallback Callback);

    /// @brief Retrieves the asset specified by the Id
    /// @param AssetCollectionId oly_common::String : the Id of the asset collection containing the asset
    /// @param AssetId oly_common::String : the Id of the asset to retrieve
    /// @param Callback AssetResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void GetAssetById(const oly_common::String& AssetCollectionId, const oly_common::String& AssetId, AssetResultCallback Callback);

    /// @brief Retrieves all assets that belong to the asset collections with the give Ids.
    /// @param AssetCollectionIds oly_common::Array<oly_common::String> : collection of asset collection Ids get all assets from
    /// @param Callback AssetsResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void GetAssetsByCollectionIds(const oly_common::Array<oly_common::String>& AssetCollectionIds, AssetsResultCallback Callback);

    /// @brief Retrieves assets based on the specified search criteria.
    /// @param AssetCollectionIds oly_common::Array<oly_common::String> : the asset collection Ids that will be used as search criteria. Note that you
    /// have to pass at least one Id.
    /// @param AssetIds oly_common::Array<oly_common::String> : optional array of strings representing asset ids
    /// @param AssetNames oly_common::Array<oly_common::String> : optional array of strings representing asset names
    /// @param AssetTypes oly_common::Array<EAssetType> : optional array of asset types
    /// @param Callback AssetsResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void GetAssetsByCriteria(const oly_common::Array<oly_common::String>& AssetCollectionIds,
        const oly_common::Optional<oly_common::Array<oly_common::String>>& AssetIds,
        const oly_common::Optional<oly_common::Array<oly_common::String>>& AssetNames,
        const oly_common::Optional<oly_common::Array<EAssetType>>& AssetTypes, AssetsResultCallback Callback);

    /// @brief Uploads data for the given asset to CHS from the given source.
    /// @param AssetCollection AssetCollection : collection the asset is associated to
    /// @param Asset Asset : asset to upload data for
    /// @param AssetDataSource AssetDataSource : asset data to upload
    /// AssetDataSource is an interface. A derived class must be passed.
    /// @param Callback UriResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT_WITH_PROGRESS void UploadAssetData(
        const AssetCollection& AssetCollection, const Asset& Asset, const AssetDataSource& AssetDataSource, UriResultCallback Callback);

    /// @brief Uploads data for the given asset to CHS from the given source, taking a CancellationToken to allow cancelling the request.
    /// @param AssetCollection AssetCollection : collection the asset is associated to
    /// @param Asset Asset : asset to upload data for
    /// @param AssetDataSource AssetDataSource : asset data to upload
    /// AssetDataSource is an interface. A derived class must be passed.
    /// @param CancellationToken oly_Common::CancellationToken : token for cancelling upload
    /// @param Callback UriResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT_WITH_PROGRESS void UploadAssetDataEx(const AssetCollection& AssetCollection, const Asset& Asset,
        const AssetDataSource& AssetDataSource, oly_common::CancellationToken& CancellationToken, UriResultCallback Callback);

    /// @brief Downloads data for a given Asset from CHS.
    /// @param Asset Asset : asset to download data for
    /// @param Callback DataResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void DownloadAssetData(const Asset& Asset, AssetDataResultCallback Callback);

    /// @brief Downloads data for a given Asset from CHS, taking a CancellationToken to allow cancelling the request.
    /// @param Asset Asset : asset to download data for
    /// @param CancellationToken oly_Common::CancellationToken : token for cancelling download
    /// @param Callback AssetDataResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void DownloadAssetDataEx(const Asset& Asset, oly_common::CancellationToken& CancellationToken, AssetDataResultCallback Callback);

    /// @brief Get the size of the data associated with an Asset.
    /// @param Asset Asset : asset to get data size for
    /// @param Callback UInt64ResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void GetAssetDataSize(const Asset& Asset, UInt64ResultCallback Callback);

    /// @brief Gets a LOD chain within the given AssetCollection.
    /// @param AssetCollection AssetCollection : AssetCollection which contains the LOD chain.
    /// @param Callback LODChainResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void GetLODChain(const AssetCollection& AssetCollection, LODChainResultCallback Callback);

    /// @brief Registers an asset to the LOD chain
    /// @param AssetCollection AssetCollection : AssetCollection which contains the LOD chain.
    /// @param Asset Asset : Asset to register as LOD
    /// @param Asset int : LOD level for Asset to be registered to
    /// @param Callback AssetResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT_WITH_PROGRESS void RegisterAssetToLODChain(
        const AssetCollection& AssetCollection, const Asset& Asset, int LODLevel, AssetResultCallback Callback);

private:
    AssetSystem(); // This constructor is only provided to appease the wrapper generator and should not be used
    OLY_NO_EXPORT AssetSystem(oly_web::WebClient* InWebClient);

    oly_services::ApiBase* PrototypeAPI;
    oly_services::ApiBase* AssetDetailAPI;

    oly_web::RemoteFileManager* FileManager;
};

} // namespace oly_systems
