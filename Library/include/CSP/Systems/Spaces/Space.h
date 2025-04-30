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
#include "CSP/Systems/Spatial/SpatialDataTypes.h"
#include "CSP/Systems/SystemsResult.h"
#include "CSP/Systems/WebService.h"

#include <functional>

namespace csp::multiplayer
{
class MultiplayerConnection;
}

namespace csp::services
{

CSP_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
CSP_END_IGNORE

} // namespace csp::services

namespace csp::systems
{

class AssetCollection;

enum class CSP_FLAGS SpaceAttributes : uint8_t
{
    None = 0,
    IsDiscoverable = 1,
    RequiresInvite = 2,

    /// @brief Non-discoverable, no invite required
    Unlisted = None,
    /// @brief Discoverable, no invite required
    Public = IsDiscoverable,
    /// @brief Non-discoverable, invite required
    Private = RequiresInvite,
    /// @brief Discoverable, invite required
    Gated = IsDiscoverable | RequiresInvite
};

CSP_START_IGNORE

inline constexpr SpaceAttributes operator|(SpaceAttributes lhs, SpaceAttributes rhs)
{
    using T = std::underlying_type_t<SpaceAttributes>;

    return static_cast<SpaceAttributes>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

inline constexpr SpaceAttributes& operator|=(SpaceAttributes& lhs, SpaceAttributes rhs)
{
    lhs = lhs | rhs;

    return lhs;
}

inline constexpr SpaceAttributes operator&(SpaceAttributes lhs, SpaceAttributes rhs)
{
    using T = std::underlying_type_t<SpaceAttributes>;

    return static_cast<SpaceAttributes>(static_cast<T>(lhs) & static_cast<T>(rhs));
}

template <typename T> inline constexpr bool HasFlag(T value, T flag) { return (value & flag) == flag; }

CSP_END_IGNORE

/// @ingroup Space System
/// @brief Data representation for a Space that maps to a Magnopus Connected Services 'Lite Group'
class CSP_API BasicSpace
{
public:
    BasicSpace()
        : Attributes(SpaceAttributes::None) {};

    csp::common::String Id;
    csp::common::String Name;
    csp::common::String Description;
    SpaceAttributes Attributes;
    csp::common::Array<csp::common::String> Tags;
};

/// @ingroup Space System
/// @brief Data representation of a Space which maps to a UserService Group.
class CSP_API Space : public BasicSpace
{
public:
    Space() = default;
    Space(const Space& Other) = default;

    /** @name Data Values
     *   A Space contains some basic information that define it, this is a 1:1 mapping to a UserService Group
     *
     *   @{ */
    csp::common::String CreatedBy;
    csp::common::String CreatedAt;
    csp::common::String OwnerId;
    csp::common::Array<csp::common::String> UserIds;
    csp::common::Array<csp::common::String> ModeratorIds;
    csp::common::Array<csp::common::String> BannedUserIds;
    /** @} */

    /// @brief Whether or not the user is "known" to the space. That being defined by whether the userID is contained in the UserIds, ModeratorIds or
    /// is the Creator. Banned users do not count as known.
    /// @return Whether or not the user is known to the space
    CSP_NO_EXPORT [[nodiscard]] bool UserIsKnownToSpace(const csp::common::String UserId) const;
};

/// @ingroup Space System
/// @brief Data representation of the geo location of a space
class CSP_API SpaceGeoLocation
{
    /** @cond DO_NOT_DOCUMENT */
    friend class SpaceSystem;
    friend class SpaceGeoLocationResult;
    friend class SpaceGeoLocationCollectionResult;
    /** @endcond */

public:
    csp::common::String SpaceId;
    csp::systems::GeoLocation Location;
    float Orientation;
    csp::common::Array<csp::systems::GeoLocation> GeoFence;

private:
    // This ID is the POI ID in the spatial data service. It is intentionally not exposed so that
    // clients cannot directly pass it to the PointOfInterestSystem. This ensures that clients
    // must go through the SpaceSystem for all operations with POIs related to SpaceGeoLocations.
    csp::common::String Id;
};

/// @ingroup Space System
/// @brief Data class used to contain information when attempting to get a space.
class CSP_API SpaceResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class SpaceSystem;

    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Retrieves the space array being stored.
    /// @return Space : reference to the space
    const Space& GetSpace() const;

    /// @brief Retrieves the code associated with the space. This is typically only useful for internal Connected Spaces Platform functionality.
    /// @return csp::common::String : the space code
    const csp::common::String& GetSpaceCode() const;

    CSP_NO_EXPORT SpaceResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};

private:
    SpaceResult(void*) {};
    CSP_NO_EXPORT SpaceResult(const csp::systems::ResultBase& InResult)
        : csp::systems::ResultBase(InResult.GetResultCode(), InResult.GetHttpResultCode()) {};

    void SetSpace(const Space& InSpace);

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    Space Space;

    // space group codes are used _very_ rarely by our services.
    // as a result they offer minimal value to fdn's users, and so we treat them separately
    // from the far more heavily used `Space` type
    csp::common::String SpaceCode;
};

/// @ingroup Space System
/// @brief Data class used to contain information when attempting to get an array of spaces.
class CSP_API SpacesResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Retrieves the spaces array being stored.
    /// @return csp::common::Array<Space> : pointer to spaces array
    csp::common::Array<Space>& GetSpaces();

    /// @brief Retrieves the spaces array being stored.
    /// @return csp::common::Array<Space> : pointer to spaces array
    const csp::common::Array<Space>& GetSpaces() const;

    CSP_NO_EXPORT SpacesResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};

private:
    SpacesResult(void*) {};

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    csp::common::Array<Space> Spaces;
};

/// @ingroup Space System
/// @brief Data class used to contain information when attempting to update the Space details.
class CSP_API BasicSpaceResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    BasicSpace& GetSpace();
    const BasicSpace& GetSpace() const;

private:
    BasicSpaceResult(void*) {};

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    BasicSpace Space;
};

/// @ingroup Space System
/// @brief Data class used to contain information when attempting to get an array of spaces.
class CSP_API BasicSpacesResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Retrieves the spaces array being stored.
    /// @return csp::common::Array<Space> : pointer to spaces array
    csp::common::Array<BasicSpace>& GetSpaces();

    /// @brief Retrieves the spaces array being stored.
    /// @return csp::common::Array<Space> : pointer to spaces array
    const csp::common::Array<BasicSpace>& GetSpaces() const;

    /// @brief Retrieves the async operation total number of result spaces.
    /// If the async operation was using pagination this count number represents the sum of space sizes from every page.
    /// If the async operation is not using pagination this count number will be equal to the spaces array size.
    /// @return uint64_t : count number as described above
    uint64_t GetTotalCount() const;

private:
    BasicSpacesResult(void*) {};

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    void FillResultTotalCount(const csp::common::String& JsonContent);

    csp::common::Array<BasicSpace> Spaces;
    uint64_t ResultTotalCount = 0;
};

/// @ingroup Space System
/// @brief @brief Data class used to contain information when attempting to retrieve the Space metadata information.
class CSP_API SpaceMetadataResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class SpaceSystem;

    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    const csp::common::Map<csp::common::String, csp::common::String>& GetMetadata() const;

    CSP_NO_EXPORT
    SpaceMetadataResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};

private:
    SpaceMetadataResult(void*) {};

    SpaceMetadataResult() {};

    void SetMetadata(const csp::common::Map<csp::common::String, csp::common::String>& MetadataAssetCollection);

    csp::common::Map<csp::common::String, csp::common::String> Metadata;
};

/// @ingroup Space System
/// @brief @brief Data class used to contain information when attempting to retrieve multiple Spaces metadata information.
class CSP_API SpacesMetadataResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class SpaceSystem;

    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    const csp::common::Map<csp::common::String, csp::common::Map<csp::common::String, csp::common::String>>& GetMetadata() const;
    const csp::common::Map<csp::common::String, csp::common::Array<csp::common::String>>& GetTags() const;

private:
    SpacesMetadataResult(void*) {};
    SpacesMetadataResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};
    SpacesMetadataResult() {};

    void SetMetadata(const csp::common::Map<csp::common::String, csp::common::Map<csp::common::String, csp::common::String>>& InMetadata);
    void SetTags(const csp::common::Map<csp::common::String, csp::common::Array<csp::common::String>>& InTags);

    csp::common::Map<csp::common::String, csp::common::Map<csp::common::String, csp::common::String>> Metadata;
    csp::common::Map<csp::common::String, csp::common::Array<csp::common::String>> Tags;
};

/// @ingroup Space System
/// @brief Data class used to contain the obfuscated email addresses of the users that have not yet accepted the space invites
class CSP_API PendingInvitesResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Retrieves the stored obfuscated email addresses
    /// @return csp::common::Array<csp::common::String> : reference to the emails array
    csp::common::Array<csp::common::String>& GetPendingInvitesEmails();

    /// @brief Retrieves the stored obfuscated email addresses
    /// @return csp::common::Array<csp::common::String> : reference to the emails array
    const csp::common::Array<csp::common::String>& GetPendingInvitesEmails() const;

private:
    PendingInvitesResult(void*) {};

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    csp::common::Array<csp::common::String> PendingInvitesEmailAddresses;
};

/// @ingroup Space System
/// @brief Data class used to contain the ids of the users that have accepted the space invites
class CSP_API AcceptedInvitesResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Retrieves the stored user ids
    /// @return csp::common::Array<csp::common::String> : reference to the user ids array
    csp::common::Array<csp::common::String>& GetAcceptedInvitesUserIds();

    /// @brief Retrieves the stored user ids
    /// @return csp::common::Array<csp::common::String> : reference to the user ids array
    const csp::common::Array<csp::common::String>& GetAcceptedInvitesUserIds() const;

private:
    AcceptedInvitesResult(void*) {};

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    csp::common::Array<csp::common::String> AcceptedInvitesUserIds;
};

/// @ingroup Space System
/// @brief Data class used to contain the outcome of space geo location operations.
/// The result can be successful and still return no geo location if one does not exist.
class CSP_API SpaceGeoLocationResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class SpaceSystem;
    friend class PointOfInterestSystem;

    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Utility to check if a geo location actually exists for the space
    /// @return bool : true if GetSpaceGeoLocation will return a valid geo location for the space, false otherwise
    const bool HasSpaceGeoLocation() const;

    /// @brief Returns the geo location of the space if one exists
    /// @return SpaceGeoLocation : Geo location of the space
    const SpaceGeoLocation& GetSpaceGeoLocation() const;

    CSP_NO_EXPORT SpaceGeoLocationResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};

private:
    SpaceGeoLocationResult(void*) {};

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    bool HasGeoLocation = false;
    SpaceGeoLocation GeoLocation;
};

/// @ingroup Space System
/// @brief Collection result to be used only by the PointOfInterestSystem
class SpaceGeoLocationCollectionResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class PointOfInterestSystem;

    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

private:
    SpaceGeoLocationCollectionResult(void*) {};
    SpaceGeoLocationCollectionResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    csp::common::Array<SpaceGeoLocation> GeoLocations;
};

typedef std::function<void(const SpaceResult& Result)> SpaceResultCallback;
typedef std::function<void(const SpacesResult& Result)> SpacesResultCallback;

typedef std::function<void(const BasicSpaceResult& Result)> BasicSpaceResultCallback;
typedef std::function<void(const BasicSpacesResult& Result)> BasicSpacesResultCallback;

typedef std::function<void(const SpaceMetadataResult& Result)> SpaceMetadataResultCallback;
typedef std::function<void(const SpacesMetadataResult& Result)> SpacesMetadataResultCallback;

typedef std::function<void(const PendingInvitesResult& Result)> PendingInvitesResultCallback;
typedef std::function<void(const AcceptedInvitesResult& Result)> AcceptedInvitesResultCallback;

typedef std::function<void(const SpaceGeoLocationResult& Result)> SpaceGeoLocationResultCallback;
typedef std::function<void(const SpaceGeoLocationCollectionResult& Result)> SpaceGeoLocationCollectionResultCallback;

typedef std::function<void(bool Result)> BoolCallback;

} // namespace csp::systems
