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
#include "CSP/Common/Optional.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/Spaces/Space.h"
#include "CSP/Systems/Spatial/Anchor.h"
#include "CSP/Systems/SystemBase.h"
#include "CSP/Systems/SystemsResult.h"

namespace csp::web
{

class WebClient;

} // namespace csp::web

namespace csp::systems
{

/// @ingroup Anchor System
/// @brief Public facing system that allows interfacing with Magnopus Connected Services' concept of an Anchor.
/// Offers methods for creating and deleting Anchors.
class CSP_API AnchorSystem : public SystemBase
{
    CSP_START_IGNORE
    /** @cond DO_NOT_DOCUMENT */
    friend class SystemsManager;
    /** @endcond */
    CSP_END_IGNORE

public:
    /** @name Asynchronous Calls
     *   These are methods that perform WebClient calls and therefore operate asynchronously and require a callback to be passed for a completion
     * result
     *
     *   @{ */

    /// @brief Creates a new Anchor.
    /// @param ThirdPartyAnchorProvider csp::systems::AnchorProvider : the 3rd party used for the new anchor
    /// @param ThirdPartyAnchorId csp::common::String : The ID of the new anchor in the 3rd party system
    /// @param AssetCollectionId : ID of the asset collection the new anchor is associated with
    /// @param Location csp::systems::GeoLocation: the geographical location of the new Anchor
    /// @param Position csp::systems::OlyAnchorPosition: the virtual position inside a space of the new Anchor
    /// @param Rotation csp::systems::OlyAnchorRotation: the rotation of the new Anchor
    /// @param SpatialKeyValue csp::common::StringMap : optional searchable SpatialKeyValue info to be associated with the Anchor
    /// Either the value or the key can be search for.
    /// @param Tags csp::common::Array<csp::common::String> : optional array of strings to add to the Anchor as tags
    /// @param Callback AnchorResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void CreateAnchor(csp::systems::AnchorProvider thirdPartyAnchorProvider, const csp::common::String& thirdPartyAnchorId,
        const csp::common::String& assetCollectionId, const csp::systems::GeoLocation& location, const csp::systems::OlyAnchorPosition& position,
        const csp::systems::OlyRotation& rotation,
        const csp::common::Optional<csp::common::Map<csp::common::String, csp::common::String>>& spatialKeyValue,
        const csp::common::Optional<csp::common::Array<csp::common::String>>& tags, AnchorResultCallback callback);

    /// @brief Creates a new Anchor in a space.
    /// @param ThirdPartyAnchorProvider csp::systems::AnchorProvider : the 3rd party used for the new anchor
    /// @param ThirdPartyAnchorId csp::common::String : The ID of the new anchor in the 3rd party system
    /// @param SpaceId csp::common::String : The space that the new anchor is associated with
    /// @param SpaceEntityId uint64_t : The multiplayer object the new anchor is associated with
    /// @param AssetCollectionId : ID of the asset collection the new anchor is associated with
    /// @param Location csp::systems::GeoLocation: the geographical location of the new Anchor
    /// @param Position csp::systems::OlyAnchorPosition: the virtual position inside a space of the new Anchor
    /// @param Rotation csp::systems::OlyAnchorRotation: the rotation of the new Anchor
    /// @param SpatialKeyValue csp::common::StringMap : optional searchable SpatialKeyValue info to be associated with the Anchor
    /// Either the value or the key can be search for.
    /// @param Tags csp::common::Array<csp::common::String> : optional array of strings to add to the Anchor as tags
    /// @param Callback AnchorResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void CreateAnchorInSpace(csp::systems::AnchorProvider thirdPartyAnchorProvider, const csp::common::String& thirdPartyAnchorId,
        const csp::common::String& spaceId, uint64_t spaceEntityId, const csp::common::String& assetCollectionId,
        const csp::systems::GeoLocation& location, const csp::systems::OlyAnchorPosition& position, const csp::systems::OlyRotation& rotation,
        const csp::common::Optional<csp::common::Map<csp::common::String, csp::common::String>>& spatialKeyValue,
        const csp::common::Optional<csp::common::Array<csp::common::String>>& tags, AnchorResultCallback callback);

    /// @brief Deletes a list of Anchors
    /// @param AnchorIds csp::common::Array<csp::common::String> : List of Anchor IDs to be deleted
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void DeleteAnchors(const csp::common::Array<csp::common::String>& anchorIds, NullResultCallback callback);

    /// @brief Retrieves an array with all the Anchors that are located inside the circular area defined by the parameters
    /// @param OriginLocation csp::systems::GeoLocation : latitude and longitude coordinates of the circular area origin
    /// @param AreaRadius double : radius from the circular area origin in meters
    /// @param SpatialKeys csp::common::Array<csp::common::String> : optional searchable SpatialKeys info associated with the Anchor
    /// @param SpatialValues csp::common::Array<csp::common::String> : optional searchable SpatialValues associated with the Anchor
    /// @param Tags csp::common::Array<csp::common::String> : optional searchable Tags info associated with the Anchor
    /// @param AllTags bool : optional flag to determine of all tags must be present in Anchor
    /// @param SpaceIds csp::common::Array<csp::common::String> : optional array of ids of spaces to search within
    /// @param Skip int : optional Number of result entries that will be skipped from the result.
    /// @param Limit int : optional Maximum number of result entries to be retrieved. for all available result entries pass
    /// @param Callback AnchorCollectionResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetAnchorsInArea(const csp::systems::GeoLocation& originLocation, const double areaRadius,
        const csp::common::Optional<csp::common::Array<csp::common::String>>& spatialKeys,
        const csp::common::Optional<csp::common::Array<csp::common::String>>& spatialValues,
        const csp::common::Optional<csp::common::Array<csp::common::String>>& tags, const csp::common::Optional<bool>& allTags,
        const csp::common::Optional<csp::common::Array<csp::common::String>>& spaceIds, const csp::common::Optional<int>& skip,
        const csp::common::Optional<int>& limit, AnchorCollectionResultCallback callback);

    /// @brief Retrieves an array with all the Anchors that are located inside the given space
    /// @param SpaceId csp::common::String : id of the space to search within
    /// @param Skip int : optional Number of result entries that will be skipped from the result.
    /// @param Limit int : optional Maximum number of result entries to be retrieved. for all available result entries pass
    /// @param Callback AnchorCollectionResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetAnchorsInSpace(const csp::common::String& spaceId, const csp::common::Optional<int>& skip,
        const csp::common::Optional<int>& limit, AnchorCollectionResultCallback callback);

    /// @brief Retrieves a list of Anchors that belong to the given AssetCollection
    /// @param AssetCollectionId csp::common::String : id of the AssetCollection to filter by
    /// @param Callback AnchorCollectionResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetAnchorsByAssetCollectionId(const csp::common::String& assetCollectionId, const csp::common::Optional<int>& skip,
        const csp::common::Optional<int>& limit, AnchorCollectionResultCallback callback);

    /// @brief Creates a new AnchorResolution.
    /// @param AnchorId csp::common::String : Anchor Id to associate AnchorResolution with
    /// @param SuccessfullyResolved bool :  Successfully resolved value for an anchor
    /// @param ResolveAttempted int : Number of resolve attempted for an anchor
    /// @param ResolveTime double : Resolve time of anchor in seconds
    /// @param Tags csp::common::Array<csp::common::String> : optional searchable Tags info associated with the AnchorResolution
    /// @param Callback AnchorResolutionResultCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void CreateAnchorResolution(const csp::common::String& anchorId, bool successfullyResolved, int resolveAttempted,
        double resolveTime, const csp::common::Array<csp::common::String>& tags, AnchorResolutionResultCallback callback);

private:
    AnchorSystem(); // This constructor is only provided to appease the wrapper generator and should not be used
    CSP_NO_EXPORT AnchorSystem(csp::web::WebClient* inWebClient, csp::common::LogSystem& logSystem);
    ~AnchorSystem();

    csp::services::ApiBase* m_anchorsApi;
};

} // namespace csp::systems
