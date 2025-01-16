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
#include "CSP/Systems/Assets/Asset.h"
#include "CSP/Systems/Assets/AssetCollection.h"
#include "CSP/Systems/WebService.h"

namespace csp::services
{

class ApiResponseBase;

CSP_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
CSP_END_IGNORE

} // namespace csp::services

namespace csp::systems
{

/// @ingroup Asset System
/// @brief An LODAsset represents an asset for a singular LOD level, and contains both the data and the specified LOD level.
class CSP_API LODAsset
{
public:
    Asset Asset;
    int Level = 0;
};

/// @ingroup Asset System
/// @brief An LODChain represents a set of Asset Levels of Detail, with the intention of operating similarly to that of video game LOD systems.
/// It stores an ID for the asset collection containing the assets, and an array of LODAssets that represent the LOD structure.
class CSP_API LODChain
{
public:
    csp::common::String AssetCollectionId;
    csp::common::Array<LODAsset> LODAssets;
};

/// @ingroup Asset System
/// @brief Data class used to contain information when attempting to download LOD chain data.
class CSP_API LODChainResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class AssetSystem;

    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Retreives the LODChain from the result.
    const LODChain& GetLODChain() const;

    CSP_NO_EXPORT LODChainResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) {};

private:
    LODChainResult(void*) {};

    void SetLODChain(const LODChain& Chain);
    void SetLODChain(LODChain&& Chain);

    void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    LODChain Chain;
};

/// @brief Callback containing LOD chain data.
/// @param Result LODChainResult : result class
typedef std::function<void(const LODChainResult& Result)> LODChainResultCallback;

} // namespace csp::systems
