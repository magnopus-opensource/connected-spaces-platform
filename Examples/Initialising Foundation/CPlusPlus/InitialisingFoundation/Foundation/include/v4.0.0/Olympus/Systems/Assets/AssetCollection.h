#pragma once

#include "Olympus/Common/Array.h"
#include "Olympus/Common/Map.h"
#include "Olympus/Common/String.h"
#include "Olympus/OlympusCommon.h"
#include "Olympus/Services/WebService.h"

#include <functional>

namespace oly_services
{

class ApiResponseBase;

OLY_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
OLY_END_IGNORE

} // namespace oly_services

namespace oly_systems
{

enum class EAssetCollectionType
{
    DEFAULT,
    FOUNDATION_INTERNAL,
    COMMENT_CONTAINER,
    COMMENT,
    SPACE_THUMBNAIL
};

/// @ingroup Asset System
/// @brief Data representation of an asset collection which maps to a PrototypeService::Prototype.
class OLY_API AssetCollection
{
public:
    AssetCollection();
    AssetCollection(const AssetCollection& Other);
    ~AssetCollection();
    AssetCollection& operator=(const AssetCollection& Other);

    /// @brief Retrieves a mutable version of the asset collection's metadata. To be used when it is necessary to mutate the asset collection's
    /// metadata.
    /// @return AssetCollection : mutable reference
    oly_common::Map<oly_common::String, oly_common::String>& GetMetadataMutable();

    /// @brief Retrieves an immutable version of the asset collection's metadata. To be used with const references to asset collections.
    /// @return AssetCollection : immutable reference
    const oly_common::Map<oly_common::String, oly_common::String>& GetMetadataImmutable() const;

    oly_common::String Id;
    oly_common::String Name;
    EAssetCollectionType Type;
    oly_common::Array<oly_common::String> Tags;
    oly_common::String PointOfInterestId;
    oly_common::String ParentId;
    oly_common::Array<oly_common::String> SpaceIds;
    oly_common::String CreatedBy;
    oly_common::String CreatedAt;
    oly_common::String UpdatedBy;
    oly_common::String UpdatedAt;
    bool IsUnique;

    // NOTE: Why is this here?
    oly_common::String Version;

private:
    // Metadata is managed via a private pointer with public accessors because it has been found that oly_common::Map's
    // internal STL map's destructor invokes client application's deallocators when they override 'delete'
    // in their codebase.
    oly_common::Map<oly_common::String, oly_common::String>* Metadata;
};

/// @ingroup Asset System
/// @brief Data class used to contain information when creating an asset collection.
class OLY_API AssetCollectionResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    OLY_END_IGNORE
    /** @endcond */

public:
    /// @brief Retrieves the asset collection result.
    /// @return AssetCollection : const ref of asset collection class
    AssetCollection& GetAssetCollection();

    /// @brief Retrieves the asset collection result.
    /// @return AssetCollection : const ref of asset collection class
    const AssetCollection& GetAssetCollection() const;

private:
    AssetCollectionResult(void*) {};

    void OnResponse(const oly_services::ApiResponseBase* ApiResponse) override;

    AssetCollection AssetCollection;
};

/// @ingroup Asset System
/// @brief Data class used to contain information when attempting to get an array of asset collections.
class OLY_API AssetCollectionsResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class AssetSystem;

    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    OLY_END_IGNORE
    /** @endcond */

public:
    /// @brief Creates an invalid AssetCollectionsResult instance that can be used to notify the user of an error.
    /// @return AssetCollectionsResult : invalid AssetCollectionsResult instance
    OLY_NO_EXPORT static AssetCollectionsResult Invalid();

    /// @brief Retrieves the asset collection array being stored as a pointer.
    /// @return oly_common::Array<AssetCollection> : pointer to asset collection array
    oly_common::Array<AssetCollection>& GetAssetCollections();

    /// @brief Retrieves the asset collection array being stored as a pointer.
    /// @return oly_common::Array<AssetCollection> : pointer to asset collection array
    const oly_common::Array<AssetCollection>& GetAssetCollections() const;

    /// @brief Retrieves the async operation total number of result asset collections.
    /// If the async operation was using pagination this count number represents the sum of asset collection sizes from every page.
    /// If the async operation is not using pagination this count number will be equal to the AssetCollections array size.
    /// @return uint64_t : count number as described above
    uint64_t GetTotalCount() const;

private:
    AssetCollectionsResult(void*) {};
    AssetCollectionsResult(oly_services::EResultCode ResCode, uint16_t HttpResCode)
        : oly_services::ResultBase(ResCode, HttpResCode) {};

    void OnResponse(const oly_services::ApiResponseBase* ApiResponse) override;

    void FillResultTotalCount(const oly_common::String& JsonContent);

    oly_common::Array<AssetCollection> AssetCollections;
    uint64_t ResultTotalCount = 0;
};

/// @brief Callback containing asset collection.
/// @param Result AssetCollectionResult : result class
typedef std::function<void(const AssetCollectionResult& Result)> AssetCollectionResultCallback;

/// @brief Callback containing array of asset collections.
/// @param Result AssetCollectionsResult : result class
typedef std::function<void(const AssetCollectionsResult& Result)> AssetCollectionsResultCallback;

} // namespace oly_systems
