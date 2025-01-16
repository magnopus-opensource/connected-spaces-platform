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
#include "CSP/Systems/WebService.h"

namespace csp::services
{

CSP_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
CSP_END_IGNORE

} // namespace csp::services

namespace csp::systems
{

enum class AnchorProvider
{
    GoogleCloudAnchors = 0
};

/// @ingroup Anchor System
/// @brief a data structure specifically for anchor position vector handling.
class CSP_API OlyAnchorPosition
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
class CSP_API Anchor
{
public:
    Anchor() {};

    /** @name Data Values
     *   An Anchor contains some basic information that define it
     *
     *   @{ */
    csp::common::String Id;
    AnchorProvider ThirdPartyAnchorProvider;
    csp::common::String ThirdPartyAnchorId;
    csp::common::String CreatedBy;
    csp::common::String CreatedAt;
    csp::common::String SpaceId;
    uint64_t SpaceEntityId;
    csp::common::String AssetCollectionId;
    GeoLocation Location;
    OlyAnchorPosition Position;
    OlyRotation Rotation;
    csp::common::Array<csp::common::String> Tags;
    csp::common::Map<csp::common::String, csp::common::String> SpatialKeyValue;
    /** @} */
};

/// @ingroup Anchor System
/// @brief Data class used to contain information after creating or retrieving an Anchor.
class CSP_API AnchorResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    Anchor& GetAnchor();
    const Anchor& GetAnchor() const;

private:
    AnchorResult(void*) {};

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    Anchor Anchor;
};

/// @ingroup Anchor System
/// @brief Data class used to contain information when attempting to get an array of Anchors.
class CSP_API AnchorCollectionResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Retrieves the Anchors array being stored.
    /// @return csp::common::Array<Anchor> : reference to Anchors array
    csp::common::Array<Anchor>& GetAnchors();

    /// @brief Retrieves the Anchors array being stored.
    /// @return csp::common::Array<Anchor> : reference to Anchors array
    const csp::common::Array<Anchor>& GetAnchors() const;

private:
    AnchorCollectionResult(void*) {};

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    csp::common::Array<Anchor> Anchors;
};

/// @brief Callback containing an Anchor and enum result used when creating or retrieving an Anchor.
/// @param Result POIResult : data class containing information on task result/progress
typedef std::function<void(const AnchorResult& Result)> AnchorResultCallback;

/// @brief Callback containing an array of Anchors and enum result used when retrieving an Anchors collection.
/// @param Result AnchorCollectionResult : data class containing information on task result/progress
typedef std::function<void(const AnchorCollectionResult& Result)> AnchorCollectionResultCallback;

/// @ingroup Anchor System
/// @brief Data representation of an AnchorResolution
class CSP_API AnchorResolution
{
public:
    csp::common::String Id;
    csp::common::String AnchorId;
    bool SuccessfullyResolved;
    int ResolveAttempted;
    double ResolveTime;
    csp::common::Array<csp::common::String> Tags;
};

/// @ingroup Anchor System
/// @brief Data class used to contain information after creating or retrieving an AnchorResolution.
class CSP_API AnchorResolutionResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
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

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    AnchorResolution AnchorResolution;
};

/// @ingroup Anchor System
/// @brief Data class used to contain information when attempting to get an array of AnchorResolutions.
class CSP_API AnchorResolutionCollectionResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Retrieves the AnchorResolutions array being stored.
    /// @return csp::common::Array<AnchorResolution> : reference to AnchorResolution array
    csp::common::Array<AnchorResolution>& GetAnchorResolutions();

    /// @brief Retrieves the AnchorResolution array being stored.
    /// @return csp::common::Array<AnchorResolution> : reference to AnchorResolution array
    const csp::common::Array<AnchorResolution>& GetAnchorResolutions() const;

private:
    AnchorResolutionCollectionResult(void*) {};

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    csp::common::Array<AnchorResolution> AnchorResolutions;
};

/// @brief Callback containing an AnchorResolution result used when creating or retrieving an AnchorResolution.
/// @param Result POIResult : data class containing information on task result/progress
typedef std::function<void(const AnchorResolutionResult& Result)> AnchorResolutionResultCallback;

/// @brief Callback containing an array of AnchorResolutions result used when retrieving an AnchorResolutions collection.
/// @param Result AnchorResolutionCollectionResult : data class containing information on task result/progress
typedef std::function<void(const AnchorResolutionCollectionResult& Result)> AnchorResolutionCollectionResultCallback;

} // namespace csp::systems
