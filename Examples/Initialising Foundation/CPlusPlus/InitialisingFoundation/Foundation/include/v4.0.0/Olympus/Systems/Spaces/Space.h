#pragma once

#include "Olympus/Common/Array.h"
#include "Olympus/Common/Map.h"
#include "Olympus/Common/String.h"
#include "Olympus/OlympusCommon.h"
#include "Olympus/Services/WebService.h"
#include "Olympus/Systems/Spatial/SpatialDataTypes.h"
#include "Olympus/Systems/SystemsResult.h"

#include <functional>

namespace oly_multiplayer
{
class MultiplayerConnection;
}

namespace oly_services
{

OLY_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
OLY_END_IGNORE

} // namespace oly_services

namespace oly_systems
{
class AssetCollection;

enum class SpaceType
{
    Private,
    Public,
};

/// @ingroup Space System
/// @brief Data representation for a Space that maps to a CHS Lite Group
class OLY_API BasicSpace
{
public:
    oly_common::String Id;
    oly_common::String Name;
    oly_common::String Description;
    SpaceType Type;
};

/// @ingroup Space System
/// @brief Data representation of a Space which maps to a UserService Group.
class OLY_API Space : public BasicSpace
{
public:
    Space() = default;
    Space(const Space& Other) = default;

    /** @name Data Values
     *   A Space contains some basic information that define it, this is a 1:1 mapping to a UserService Group
     *
     *   @{ */
    oly_common::String CreatedBy;
    oly_common::String CreatedAt;
    oly_common::String OwnerId;
    oly_common::Array<oly_common::String> UserIds;
    oly_common::Array<oly_common::String> ModeratorIds;
    oly_common::Array<oly_common::String> BannedUserIds;
    /** @} */
};

/// @ingroup Space System
/// @brief Data representation of the geo location of a space
class OLY_API SpaceGeoLocation
{
    /** @cond DO_NOT_DOCUMENT */
    friend class SpaceSystem;
    friend class SpaceGeoLocationResult;
    friend class SpaceGeoLocationCollectionResult;
    /** @endcond */

public:
    oly_common::String SpaceId;
    oly_systems::GeoLocation Location;
    float Orientation;
    oly_common::Array<oly_systems::GeoLocation> GeoFence;

private:
    // This ID is the POI ID in the spatial data service. It is intentionally not exposed so that
    // clients cannot directly pass it to the PointOfInterestSystem. This ensures that clients
    // must go through the SpaceSystem for all operations with POIs related to SpaceGeoLocations.
    oly_common::String Id;
};

/// @ingroup Space System
/// @brief Data class used to contain information when attempting to get a space.
class OLY_API SpaceResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class SpaceSystem;

    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    OLY_END_IGNORE
    /** @endcond */

public:
    /// @brief Retrieves the space array being stored.
    /// @return Space : reference to the space
    const Space& GetSpace() const;

    /// @brief Retrieves the code associated with the space. This is typically only useful for foundation-internal functionality.
    /// @return oly_common::String : the space code
    const oly_common::String& GetSpaceCode() const;

private:
    SpaceResult(void*) {};
    SpaceResult(oly_services::EResultCode ResCode, uint16_t HttpResCode)
        : oly_services::ResultBase(ResCode, HttpResCode) {};
    OLY_NO_EXPORT SpaceResult(const oly_services::ResultBase& InResult)
        : oly_services::ResultBase(InResult.GetResultCode(), InResult.GetHttpResultCode()) {};

    void SetSpace(const Space& InSpace);

    void OnResponse(const oly_services::ApiResponseBase* ApiResponse) override;

    Space Space;

    // space group codes are used _very_ rarely by our services.
    // as a result they offer minimal value to fdn's users, and so we treat them separately
    // from the far more heavily used `Space` type
    oly_common::String SpaceCode;
};

/// @ingroup Space System
/// @brief Data class used to contain information when attempting to get an array of spaces.
class OLY_API SpacesResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    OLY_END_IGNORE
    /** @endcond */

public:
    /// @brief Retrieves the spaces array being stored.
    /// @return oly_common::Array<Space> : pointer to spaces array
    oly_common::Array<Space>& GetSpaces();

    /// @brief Retrieves the spaces array being stored.
    /// @return oly_common::Array<Space> : pointer to spaces array
    const oly_common::Array<Space>& GetSpaces() const;

    OLY_NO_EXPORT static SpacesResult Invalid();

private:
    SpacesResult(void*) {};
    SpacesResult(oly_services::EResultCode ResCode, uint16_t HttpResCode)
        : oly_services::ResultBase(ResCode, HttpResCode) {};

    void OnResponse(const oly_services::ApiResponseBase* ApiResponse) override;

    oly_common::Array<Space> Spaces;
};

/// @ingroup Space System
/// @brief Data class used to contain information when attempting to update the Space details.
class OLY_API BasicSpaceResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    OLY_END_IGNORE
    /** @endcond */

public:
    BasicSpace& GetSpace();
    const BasicSpace& GetSpace() const;

private:
    BasicSpaceResult(void*) {};

    void OnResponse(const oly_services::ApiResponseBase* ApiResponse) override;

    BasicSpace Space;
};

/// @ingroup Space System
/// @brief Data class used to contain information when attempting to get an array of spaces.
class OLY_API BasicSpacesResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    OLY_END_IGNORE
    /** @endcond */

public:
    /// @brief Retrieves the spaces array being stored.
    /// @return oly_common::Array<Space> : pointer to spaces array
    oly_common::Array<BasicSpace>& GetSpaces();

    /// @brief Retrieves the spaces array being stored.
    /// @return oly_common::Array<Space> : pointer to spaces array
    const oly_common::Array<BasicSpace>& GetSpaces() const;

    /// @brief Retrieves the async operation total number of result spaces.
    /// If the async operation was using pagination this count number represents the sum of space sizes from every page.
    /// If the async operation is not using pagination this count number will be equal to the spaces array size.
    /// @return uint64_t : count number as described above
    uint64_t GetTotalCount() const;

private:
    BasicSpacesResult(void*) {};

    void OnResponse(const oly_services::ApiResponseBase* ApiResponse) override;

    void FillResultTotalCount(const oly_common::String& JsonContent);

    oly_common::Array<BasicSpace> Spaces;
    uint64_t ResultTotalCount = 0;
};

/// @ingroup Space System
/// @brief @brief Data class used to contain information when attempting to retrieve the Space metadata information.
class OLY_API SpaceMetadataResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class SpaceSystem;

    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    OLY_END_IGNORE
    /** @endcond */

public:
    const oly_common::Map<oly_common::String, oly_common::String>& GetMetadata() const;

private:
    SpaceMetadataResult(void*) {};
    SpaceMetadataResult(oly_services::EResultCode ResCode, uint16_t HttpResCode)
        : oly_services::ResultBase(ResCode, HttpResCode) {};
    SpaceMetadataResult() {};

    void SetMetadata(const oly_common::Map<oly_common::String, oly_common::String>& MetadataAssetCollection);

    oly_common::Map<oly_common::String, oly_common::String> Metadata;
};

/// @ingroup Space System
/// @brief @brief Data class used to contain information when attempting to retrieve multiple Spaces metadata information.
class OLY_API SpacesMetadataResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class SpaceSystem;

    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    OLY_END_IGNORE
    /** @endcond */

public:
    const oly_common::Map<oly_common::String, oly_common::Map<oly_common::String, oly_common::String>>& GetMetadata() const;

private:
    SpacesMetadataResult(void*) {};
    SpacesMetadataResult(oly_services::EResultCode ResCode, uint16_t HttpResCode)
        : oly_services::ResultBase(ResCode, HttpResCode) {};
    SpacesMetadataResult() {};

    void SetMetadata(const oly_common::Map<oly_common::String, oly_common::Map<oly_common::String, oly_common::String>>& InMetadata);

    oly_common::Map<oly_common::String, oly_common::Map<oly_common::String, oly_common::String>> Metadata;
};

/// @ingroup Space System
/// @brief Data class used to contain information when attempting to Enter a Space.
class OLY_API EnterSpaceResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class SpaceSystem;

    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    OLY_END_IGNORE
    /** @endcond */

public:
    oly_multiplayer::MultiplayerConnection* GetConnection() const;

private:
    EnterSpaceResult(void*) {};
    EnterSpaceResult(oly_services::EResultCode ResCode, uint16_t HttpResCode)
        : oly_services::ResultBase(ResCode, HttpResCode) {};
    EnterSpaceResult() {};

    oly_multiplayer::MultiplayerConnection* Connection;
    void SetConnection(oly_multiplayer::MultiplayerConnection* IncommingConnection);
    bool EnterResponse;
};

/// @ingroup Space System
/// @brief Data class used to contain the obfuscated email addresses of the users that have not yet accepted the space invites
class OLY_API PendingInvitesResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    OLY_END_IGNORE
    /** @endcond */

public:
    /// @brief Retrieves the stored obfuscated email addresses
    /// @return oly_common::Array<oly_common::String> : reference to the emails array
    oly_common::Array<oly_common::String>& GetPendingInvitesEmails();

    /// @brief Retrieves the stored obfuscated email addresses
    /// @return oly_common::Array<oly_common::String> : reference to the emails array
    const oly_common::Array<oly_common::String>& GetPendingInvitesEmails() const;

private:
    PendingInvitesResult(void*) {};

    void OnResponse(const oly_services::ApiResponseBase* ApiResponse) override;

    oly_common::Array<oly_common::String> PendingInvitesEmailAddresses;
};

/// @ingroup Space System
/// @brief Data class used to contain the outcome of space geo location operations.
/// The result can be successful and still return no geo location if one does not exist.
class OLY_API SpaceGeoLocationResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class SpaceSystem;
    friend class PointOfInterestSystem;

    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    OLY_END_IGNORE
    /** @endcond */

public:
    /// @brief Utility to check if a geo location actually exists for the space
    /// @return bool : true if GetSpaceGeoLocation will return a valid geo location for the space, false otherwise
    const bool HasSpaceGeoLocation() const;

    /// @brief Returns the geo location of the space if one exists
    /// @return SpaceGeoLocation : Geo location of the space
    const SpaceGeoLocation& GetSpaceGeoLocation() const;

    OLY_NO_EXPORT static SpaceGeoLocationResult Invalid();

private:
    SpaceGeoLocationResult(void*) {};
    SpaceGeoLocationResult(oly_services::EResultCode ResCode, uint16_t HttpResCode)
        : oly_services::ResultBase(ResCode, HttpResCode) {};

    void OnResponse(const oly_services::ApiResponseBase* ApiResponse) override;

    bool HasGeoLocation = false;
    SpaceGeoLocation GeoLocation;
};

/// @ingroup Space System
/// @brief Collection result to be used only by the PointOfInterestSystem
class SpaceGeoLocationCollectionResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class PointOfInterestSystem;

    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    OLY_END_IGNORE
    /** @endcond */

private:
    SpaceGeoLocationCollectionResult(void*) {};
    SpaceGeoLocationCollectionResult(oly_services::EResultCode ResCode, uint16_t HttpResCode)
        : oly_services::ResultBase(ResCode, HttpResCode) {};

    void OnResponse(const oly_services::ApiResponseBase* ApiResponse) override;

    oly_common::Array<SpaceGeoLocation> GeoLocations;
};

typedef std::function<void(const SpaceResult& Result)> SpaceResultCallback;
typedef std::function<void(const SpacesResult& Result)> SpacesResultCallback;

typedef std::function<void(const BasicSpaceResult& Result)> BasicSpaceResultCallback;
typedef std::function<void(const BasicSpacesResult& Result)> BasicSpacesResultCallback;

typedef std::function<void(const SpaceMetadataResult& Result)> SpaceMetadataResultCallback;
typedef std::function<void(const SpacesMetadataResult& Result)> SpacesMetadataResultCallback;

typedef std::function<void(const PendingInvitesResult& Result)> PendingInvitesResultCallback;

typedef std::function<void(const EnterSpaceResult& Result)> EnterSpaceResultCallback;

typedef std::function<void(const SpaceGeoLocationResult& Result)> SpaceGeoLocationResultCallback;
typedef std::function<void(const SpaceGeoLocationCollectionResult& Result)> SpaceGeoLocationCollectionResultCallback;

typedef std::function<void(bool Result)> BoolCallback;

} // namespace oly_systems
