#pragma once

#include "Olympus/Common/Array.h"
#include "Olympus/Common/Optional.h"
#include "Olympus/Common/String.h"
#include "Olympus/Multiplayer/SpaceEntity.h"
#include "Olympus/OlympusCommon.h"
#include "Olympus/Systems/Spaces/Space.h"
#include "Olympus/Systems/Spatial/Anchor.h"
#include "Olympus/Systems/SystemBase.h"
#include "Olympus/Systems/SystemsResult.h"

namespace oly_web
{

class WebClient;

}

namespace oly_systems
{

/// @ingroup Anchor System
/// @brief Public facing system that allows interfacing with CHS's concept of an Anchor.
/// Offers methods for creating and deleting Anchors.
class OLY_API OLY_NO_DISPOSE AnchorSystem : public SystemBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class SystemsManager;
    /** @endcond */

public:
    ~AnchorSystem();

    /** @name Asynchronous Calls
     *   These are methods that perform WebClient calls and therefore operate asynchronously and require a callback to be passed for a completion
     * result
     *
     *   @{ */

    /// @brief Creates a new Anchor.
    /// @param ThirdPartyAnchorProvider oly_systems::AnchorProvider : the 3rd party used for the new anchor
    /// @param ThirdPartyAnchorId oly_common::String : The ID of the new anchor in the 3rd party system
    /// @param AssetCollectionId : ID of the asset collection the new anchor is associated with
    /// @param Location oly_systems::GeoLocation: the geographical location of the new Anchor
    /// @param Position oly_systems::OlyAnchorPosition: the virtual position inside a space of the new Anchor
    /// @param Rotation oly_systems::OlyAnchorRotation: the rotation of the new Anchor
    /// @param SpatialKeyValue oly_common::StringMap : optional searchable SpatialKeyValue info to be associated with the Anchor
    /// Either the value or the key can be search for.
    /// @param Tags oly_common::Array<oly_common::String> : optional array of strings to add to the Anchor as tags
    /// @param Callback AnchorResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void CreateAnchor(oly_systems::AnchorProvider ThirdPartyAnchorProvider, const oly_common::String& ThirdPartyAnchorId,
        const oly_common::String& AssetCollectionId, const oly_systems::GeoLocation& Location, const oly_systems::OlyAnchorPosition& Position,
        const oly_systems::OlyRotation& Rotation,
        const oly_common::Optional<oly_common::Map<oly_common::String, oly_common::String>>& SpatialKeyValue,
        const oly_common::Optional<oly_common::Array<oly_common::String>>& Tags, AnchorResultCallback Callback);

    /// @brief Creates a new Anchor in a space.
    /// @param ThirdPartyAnchorProvider oly_systems::AnchorProvider : the 3rd party used for the new anchor
    /// @param ThirdPartyAnchorId oly_common::String : The ID of the new anchor in the 3rd party system
    /// @param SpaceId oly_common::String : The space that the new anchor is associated with
    /// @param SpaceEntityId uint64_t : The multiplayer object the new anchor is associated with
    /// @param AssetCollectionId : ID of the asset collection the new anchor is associated with
    /// @param Location oly_systems::GeoLocation: the geographical location of the new Anchor
    /// @param Position oly_systems::OlyAnchorPosition: the virtual position inside a space of the new Anchor
    /// @param Rotation oly_systems::OlyAnchorRotation: the rotation of the new Anchor
    /// @param SpatialKeyValue oly_common::StringMap : optional searchable SpatialKeyValue info to be associated with the Anchor
    /// Either the value or the key can be search for.
    /// @param Tags oly_common::Array<oly_common::String> : optional array of strings to add to the Anchor as tags
    /// @param Callback AnchorResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void CreateAnchorInSpace(oly_systems::AnchorProvider ThirdPartyAnchorProvider, const oly_common::String& ThirdPartyAnchorId,
        const oly_common::String& SpaceId, uint64_t SpaceEntityId, const oly_common::String& AssetCollectionId,
        const oly_systems::GeoLocation& Location, const oly_systems::OlyAnchorPosition& Position, const oly_systems::OlyRotation& Rotation,
        const oly_common::Optional<oly_common::Map<oly_common::String, oly_common::String>>& SpatialKeyValue,
        const oly_common::Optional<oly_common::Array<oly_common::String>>& Tags, AnchorResultCallback Callback);

    /// @brief Deletes a list of Anchors
    /// @param AnchorIds oly_common::Array<oly_common::String> : List of Anchor IDs to be deleted
    /// @param Callback NullResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void DeleteAnchors(const oly_common::Array<oly_common::String>& AnchorIds, NullResultCallback Callback);

    /// @brief Retrieves an array with all the Anchors that are located inside the circular area defined by the parameters
    /// @param OriginLocation oly_systems::GeoLocation : latitude and longitude coordinates of the circular area origin
    /// @param AreaRadius double : radius from the circular area origin in meters
    /// @param SpatialKeys oly_common::Array<oly_common::String> : optional searchable SpatialKeys info associated with the Anchor
    /// @param SpatialValues oly_common::Array<oly_common::String> : optional searchable SpatialValues associated with the Anchor
    /// @param Tags oly_common::Array<oly_common::String> : optional searchable Tags info associated with the Anchor
    /// @param AllTags bool : optional flag to determine of all tags must be present in Anchor
    /// @param SpaceIds oly_common::Array<oly_common::String> : optional array of ids of spaces to search within
    /// @param Skip int : optional Number of result entries that will be skipped from the result.
    /// @param Limit int : optional Maximum number of result entries to be retrieved. for all available result entries pass
    /// @param Callback AnchorCollectionResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void GetAnchorsInArea(const oly_systems::GeoLocation& OriginLocation, const double AreaRadius,
        const oly_common::Optional<oly_common::Array<oly_common::String>>& SpatialKeys,
        const oly_common::Optional<oly_common::Array<oly_common::String>>& SpatialValues,
        const oly_common::Optional<oly_common::Array<oly_common::String>>& Tags, const oly_common::Optional<bool>& AllTags,
        const oly_common::Optional<oly_common::Array<oly_common::String>>& SpaceIds, const oly_common::Optional<int>& Skip,
        const oly_common::Optional<int>& Limit, AnchorCollectionResultCallback Callback);

    /// @brief Retrieves an array with all the Anchors that are located inside the given space
    /// @param SpaceId oly_common::String : id of the space to search within
    /// @param Skip int : optional Number of result entries that will be skipped from the result.
    /// @param Limit int : optional Maximum number of result entries to be retrieved. for all available result entries pass
    /// @param Callback AnchorCollectionResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void GetAnchorsInSpace(const oly_common::String& SpaceId, const oly_common::Optional<int>& Skip,
        const oly_common::Optional<int>& Limit, AnchorCollectionResultCallback Callback);

    /// @brief Retrieves a list of Anchors that belong to the given AssetCollection
    /// @param AssetCollectionId oly_common::String : id of the AssetCollection to filter by
    /// @param Callback AnchorCollectionResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void GetAnchorsByAssetCollectionId(const oly_common::String& AssetCollectionId, const oly_common::Optional<int>& Skip,
        const oly_common::Optional<int>& Limit, AnchorCollectionResultCallback Callback);

    /// @brief Creates a new AnchorResolution.
    /// @param AnchorId oly_common::String : Anchor Id to associate AnchorResolution with
    /// @param SuccessfullyResolved bool :  Successfully resolved value for an anchor
    /// @param ResolveAttempted int : Number of resolve attempted for an anchor
    /// @param ResolveTime double : Resolve time of anchor in seconds
    /// @param Tags oly_common::Array<oly_common::String> : optional searchable Tags info associated with the AnchorResolution
    /// @param Callback AnchorResolutionResultCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void CreateAnchorResolution(const oly_common::String& AnchorId, bool SuccessfullyResolved, int ResolveAttempted,
        double ResolveTime, const oly_common::Array<oly_common::String>& Tags, AnchorResolutionResultCallback Callback);

private:
    AnchorSystem(); // This constructor is only provided to appease the wrapper generator and should not be used
    OLY_NO_EXPORT AnchorSystem(oly_web::WebClient* InWebClient);

    oly_services::ApiBase* AnchorsAPI;
};

} // namespace oly_systems
