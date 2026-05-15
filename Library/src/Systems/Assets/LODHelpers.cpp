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
#include "LODHelpers.h"

#include "Debug/Logging.h"

#include <algorithm>
#include <optional>
#include <string>

namespace csp::systems
{

csp::common::String CreateLODStyleVar(int lodLevel) { return std::string("lod:" + std::to_string(lodLevel)).c_str(); }

int GetLODLevelFromStylesArray(const csp::common::Array<csp::common::String>& styles)
{
    for (size_t i = 0; i < styles.Size(); ++i)
    {
        std::string style = styles[i].c_str();
        size_t pos = style.find("lod:");

        if (pos != std::string::npos)
        {
            return std::stoi(style.substr(4, 1));
        }
    }

    return -1;
}

LODChain CreateLODChainFromAssets(const csp::common::Array<Asset>& assets, const csp::common::String& assetCollectionId)
{
    LODChain chain;
    chain.AssetCollectionId = assetCollectionId;

    csp::common::List<LODAsset> assetList;

    if (assets.Size() == 1)
    {
        int lodLevel = csp::systems::GetLODLevelFromStylesArray(assets[0].Styles);

        if (lodLevel == -1)
        {
            // As there is only 1 asset, return this as LOD 0
            lodLevel = 0;
        }

        assetList.Append(csp::systems::LODAsset { assets[0], lodLevel });
    }
    else
    {
        for (size_t i = 0; i < assets.Size(); ++i)
        {
            const csp::systems::Asset& asset = assets[i];
            int lodLevel = csp::systems::GetLODLevelFromStylesArray(asset.Styles);

            if (lodLevel != -1)
            {
                assetList.Append(csp::systems::LODAsset { asset, lodLevel });
            }
        }
    }

    chain.LODAssets = csp::common::Array<LODAsset>(assetList.Size());

    for (size_t i = 0; i < chain.LODAssets.Size(); ++i)
    {
        chain.LODAssets[i] = std::move(assetList[i]);
    }

    std::sort(
        chain.LODAssets.begin(), chain.LODAssets.end(), [](const LODAsset& asset1, const LODAsset& asset2) { return asset1.Level < asset2.Level; });

    return chain;
}

bool ValidateNewLODLevelForChain(const LODChain& chain, int lodLevel)
{
    // Ensure LODLevel doesnt already exist
    const auto it
        = std::find_if(chain.LODAssets.begin(), chain.LODAssets.end(), [lodLevel](const LODAsset& asset) { return asset.Level == lodLevel; });
    return it == chain.LODAssets.end();
}
} // namespace csp::systems
