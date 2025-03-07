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
#include "CSP/Common/Map.h"
#include "CSP/Common/String.h"
#include "CSP/Systems/WebService.h"

#include <functional>

namespace csp::services
{

class ApiResponseBase;

CSP_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
CSP_END_IGNORE

} // namespace csp::services

namespace csp::systems
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
class CSP_API AssetCollection
{
public:
    AssetCollection();
    AssetCollection(const AssetCollection& Other);
    ~AssetCollection();
    AssetCollection& operator=(const AssetCollection& Other);

    /// @brief Retrieves a mutable version of the asset collection's metadata. To be used when it is necessary to mutate the asset collection's
    /// metadata.
    /// @return AssetCollection : mutable reference
    csp::common::Map<csp::common::String, csp::common::String>& GetMetadataMutable();

    /// @brief Retrieves an immutable version of the asset collection's metadata. To be used with const references to asset collections.
    /// @return AssetCollection : immutable reference
    const csp::common::Map<csp::common::String, csp::common::String>& GetMetadataImmutable() const;

    // The unique identifier of this asset collection.
    csp::common::String Id;

    // The unique name of this asset collection.
    csp::common::String Name;

    // The type of this asset collection.
    EAssetCollectionType Type;

    // The set of tag strings that have been associated with this asset collection. Note that asset collections can be searched by tag.
    csp::common::Array<csp::common::String> Tags;

    // The unique identifier of the POI this asset collection relates to. Empty if it does not relate to a POI.
    csp::common::String PointOfInterestId;

    // The unique ID of the asset collection that is a parent to this asset collection. Empty if there is no parent relationship.
    csp::common::String ParentId;

    // Where the asset collection belongs to a space, the unique ID of the space that this asset collection belongs to. Empty otherwise.
    csp::common::String SpaceId;

    // The unique ID of the user who created the asset collection.
    csp::common::String CreatedBy;

    // The UTC string representing when this asset collection was created.
    csp::common::String CreatedAt;

    // The unique ID of the user who last updated the asset collection.
    csp::common::String UpdatedBy;

    // The UTC string representing when this asset collection was last updated.
    csp::common::String UpdatedAt;
    bool IsUnique;

    // NOTE: Why is this here?
    csp::common::String Version;

private:
    // Metadata is managed via a private pointer with public accessors because it has been found that csp::common::Map's
    // internal STL map's destructor invokes client application's deallocators when they override 'delete'
    // in their codebase.
    csp::common::Map<csp::common::String, csp::common::String>* Metadata;
};

/// @ingroup Asset System
/// @brief Data class used to contain information when creating an asset collection.
class CSP_API AssetCollectionResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
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

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    AssetCollection AssetCollection;
};

/// @ingroup Asset System
/// @brief Data class used to contain information when attempting to get an array of asset collections.
class CSP_API AssetCollectionsResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class AssetSystem;

    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Retrieves the asset collection array being stored as a pointer.
    /// @return csp::common::Array<AssetCollection> : pointer to asset collection array
    csp::common::Array<AssetCollection>& GetAssetCollections();

    /// @brief Retrieves the asset collection array being stored as a pointer.
    /// @return csp::common::Array<AssetCollection> : pointer to asset collection array
    const csp::common::Array<AssetCollection>& GetAssetCollections() const;

    /// @brief Retrieves the async operation total number of result asset collections.
    /// If the async operation was using pagination this count number represents the sum of asset collection sizes from every page.
    /// If the async operation is not using pagination this count number will be equal to the AssetCollections array size.
    /// @return uint64_t : count number as described above
    uint64_t GetTotalCount() const;

    CSP_NO_EXPORT AssetCollectionsResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};

private:
    AssetCollectionsResult(void*) {};

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    void FillResultTotalCount(const csp::common::String& JsonContent);

    csp::common::Array<AssetCollection> AssetCollections;
    uint64_t ResultTotalCount = 0;
};

/// @brief Callback containing asset collection.
/// @param Result AssetCollectionResult : result class
typedef std::function<void(const AssetCollectionResult& Result)> AssetCollectionResultCallback;

/// @brief Callback containing array of asset collections.
/// @param Result AssetCollectionsResult : result class
typedef std::function<void(const AssetCollectionsResult& Result)> AssetCollectionsResultCallback;

} // namespace csp::systems
