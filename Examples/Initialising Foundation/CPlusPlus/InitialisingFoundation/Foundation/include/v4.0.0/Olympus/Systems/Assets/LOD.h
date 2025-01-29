#pragma once

#include "Olympus/Common/Array.h"
#include "Olympus/Common/Map.h"
#include "Olympus/Common/String.h"
#include "Olympus/OlympusCommon.h"
#include "Olympus/Services/WebService.h"
#include "Olympus/Systems/Assets/Asset.h"
#include "Olympus/Systems/Assets/AssetCollection.h"

namespace oly_services
{

class ApiResponseBase;

OLY_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
OLY_END_IGNORE

} // namespace oly_services

namespace oly_systems
{

/// @ingroup Asset System
class OLY_API LODAsset
{
public:
    Asset Asset;
    int Level = 0;
};

class OLY_API LODChain
{
public:
    oly_common::String AssetCollectionId;
    oly_common::Array<LODAsset> LODAssets;
};

/// @ingroup Asset System
/// @brief Data class used to contain information when attempting to download LOD chain data.
class OLY_API LODChainResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class AssetSystem;

    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    OLY_END_IGNORE
    /** @endcond */

public:
    const LODChain& GetLODChain() const;

protected:
private:
    LODChainResult(void*) {};
    LODChainResult(oly_services::EResultCode ResCode, uint16_t HttpResCode)
        : oly_services::ResultBase(ResCode, HttpResCode) {};
    OLY_NO_EXPORT LODChainResult(const oly_services::ResultBase& InResult)
        : oly_services::ResultBase(InResult.GetResultCode(), InResult.GetHttpResultCode()) {};

    void SetLODChain(const LODChain& Chain);
    void SetLODChain(LODChain&& Chain);

    void OnResponse(const oly_services::ApiResponseBase* ApiResponse) override;

    LODChain Chain;
};

/// @brief Callback containing LOD chain data.
/// @param Result LODChainResult : result class
typedef std::function<void(const LODChainResult& Result)> LODChainResultCallback;

} // namespace oly_systems
