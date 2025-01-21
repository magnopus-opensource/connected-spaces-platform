#pragma once

#include "Olympus/Common/Array.h"
#include "Olympus/Common/Map.h"
#include "Olympus/Common/String.h"
#include "Olympus/OlympusCommon.h"
#include "Olympus/Services/WebService.h"
#include "Olympus/Systems/Spatial/SpatialDataTypes.h"

namespace oly_services
{

OLY_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
OLY_END_IGNORE

} // namespace oly_services

namespace oly_systems
{

enum class AnchorProvider
{
    GoogleCloudAnchors = 0
};

class OLY_API OlyAnchorPosition
{
public:
    OlyAnchorPosition();
    OlyAnchorPosition(double InX, double InY, double InZ)
        : X(InX)
        , Y(InY)
        , Z(InZ) {};

    double X;
    double Y;
    double Z;
};

/// @ingroup Anchor System
/// @brief Data representation of an Anchor
class OLY_API Anchor
{
public:
    Anchor() {};

    /** @name Data Values
     *   An Anchor contains some basic information that define it
     *
     *   @{ */
    oly_common::String Id;
    AnchorProvider ThirdPartyAnchorProvider;
    oly_common::String ThirdPartyAnchorId;
    oly_common::String CreatedBy;
    oly_common::String CreatedAt;
    oly_common::String SpaceId;
    uint64_t SpaceEntityId;
    oly_common::String AssetCollectionId;
    GeoLocation Location;
    OlyAnchorPosition Position;
    OlyRotation Rotation;
    oly_common::Array<oly_common::String> Tags;
    oly_common::Map<oly_common::String, oly_common::String> SpatialKeyValue;
    /** @} */
};

/// @ingroup Anchor System
/// @brief Data class used to contain information after creating or retrieving an Anchor.
class OLY_API AnchorResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    OLY_END_IGNORE
    /** @endcond */

public:
    Anchor& GetAnchor();
    const Anchor& GetAnchor() const;

private:
    AnchorResult(void*) {};

    void OnResponse(const oly_services::ApiResponseBase* ApiResponse) override;

    Anchor Anchor;
};

/// @ingroup Anchor System
/// @brief Data class used to contain information when attempting to get an array of Anchors.
class OLY_API AnchorCollectionResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    OLY_END_IGNORE
    /** @endcond */

public:
    /// @brief Retrieves the Anchors array being stored.
    /// @return oly_common::Array<Anchor> : reference to Anchors array
    oly_common::Array<Anchor>& GetAnchors();

    /// @brief Retrieves the Anchors array being stored.
    /// @return oly_common::Array<Anchor> : reference to Anchors array
    const oly_common::Array<Anchor>& GetAnchors() const;

private:
    AnchorCollectionResult(void*) {};

    void OnResponse(const oly_services::ApiResponseBase* ApiResponse) override;

    oly_common::Array<Anchor> Anchors;
};

/// @brief Callback containing an Anchor and enum result used when creating or retrieving an Anchor.
/// @param Result POIResult : data class containing information on task result/progress
typedef std::function<void(const AnchorResult& Result)> AnchorResultCallback;

/// @brief Callback containing an array of Anchors and enum result used when retrieving an Anchors collection.
/// @param Result AnchorCollectionResult : data class containing information on task result/progress
typedef std::function<void(const AnchorCollectionResult& Result)> AnchorCollectionResultCallback;

/// @ingroup Anchor System
/// @brief Data representation of an AnchorResolution
class OLY_API AnchorResolution
{
public:
    oly_common::String Id;
    oly_common::String AnchorId;
    bool SuccessfullyResolved;
    int ResolveAttempted;
    double ResolveTime;
    oly_common::Array<oly_common::String> Tags;
};

/// @ingroup Anchor System
/// @brief Data class used to contain information after creating or retrieving an AnchorResolution.
class OLY_API AnchorResolutionResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    OLY_END_IGNORE
    /** @endcond */

public:
    /// @brief Retrieves the AnchorResolution being stored.
    /// @return AnchorResolution : reference to AnchorResolution
    AnchorResolution& GetAnchorResolution();

    /// @brief Retrieves the AnchorResolution being stored.
    /// @return AnchorResolution : reference to AnchorResolution array
    const AnchorResolution& GetAnchorResolution() const;

private:
    AnchorResolutionResult(void*) {};

    void OnResponse(const oly_services::ApiResponseBase* ApiResponse) override;

    AnchorResolution AnchorResolution;
};

/// @ingroup Anchor System
/// @brief Data class used to contain information when attempting to get an array of AnchorResolutions.
class OLY_API AnchorResolutionCollectionResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    OLY_END_IGNORE
    /** @endcond */

public:
    /// @brief Retrieves the AnchorResolutions array being stored.
    /// @return oly_common::Array<AnchorResolution> : reference to AnchorResolution array
    oly_common::Array<AnchorResolution>& GetAnchorResolutions();

    /// @brief Retrieves the AnchorResolution array being stored.
    /// @return oly_common::Array<AnchorResolution> : reference to AnchorResolution array
    const oly_common::Array<AnchorResolution>& GetAnchorResolutions() const;

private:
    AnchorResolutionCollectionResult(void*) {};

    void OnResponse(const oly_services::ApiResponseBase* ApiResponse) override;

    oly_common::Array<AnchorResolution> AnchorResolutions;
};

/// @brief Callback containing an AnchorResolution result used when creating or retrieving an AnchorResolution.
/// @param Result POIResult : data class containing information on task result/progress
typedef std::function<void(const AnchorResolutionResult& Result)> AnchorResolutionResultCallback;

/// @brief Callback containing an array of AnchorResolutions result used when retrieving an AnchorResolutions collection.
/// @param Result AnchorResolutionCollectionResult : data class containing information on task result/progress
typedef std::function<void(const AnchorResolutionCollectionResult& Result)> AnchorResolutionCollectionResultCallback;

} // namespace oly_systems
