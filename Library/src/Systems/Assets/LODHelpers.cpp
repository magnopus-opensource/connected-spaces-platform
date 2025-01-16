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

#include "Common/Algorithm.h"
#include "Debug/Logging.h"

#include <string>

namespace csp::systems
{

csp::common::String CreateLODStyleVar(int LODLevel) { return std::string("lod:" + std::to_string(LODLevel)).c_str(); }

int GetLODLevelFromStylesArray(const csp::common::Array<csp::common::String>& Styles)
{
    for (size_t i = 0; i < Styles.Size(); ++i)
    {
        std::string Style = Styles[i].c_str();
        size_t Pos = Style.find("lod:");

        if (Pos != std::string::npos)
        {
            return std::stoi(Style.substr(4, 1));
        }
    }

    return -1;
}

LODChain CreateLODChainFromAssets(const csp::common::Array<Asset>& Assets, const csp::common::String& AssetCollectionId)
{
    LODChain Chain;
    Chain.AssetCollectionId = AssetCollectionId;

    csp::common::List<LODAsset> AssetList;

    if (Assets.Size() == 1)
    {
        int LODLevel = csp::systems::GetLODLevelFromStylesArray(Assets[0].Styles);

        if (LODLevel == -1)
        {
            // As there is only 1 asset, return this as LOD 0
            LODLevel = 0;
        }

        AssetList.Append(csp::systems::LODAsset { Assets[0], LODLevel });
    }
    else
    {
        for (int i = 0; i < Assets.Size(); ++i)
        {
            const csp::systems::Asset& Asset = Assets[i];
            int LODLevel = csp::systems::GetLODLevelFromStylesArray(Asset.Styles);

            if (LODLevel != -1)
            {
                AssetList.Append(csp::systems::LODAsset { Asset, LODLevel });
            }
        }
    }

    Chain.LODAssets = csp::common::Array<LODAsset>(AssetList.Size());

    for (int i = 0; i < Chain.LODAssets.Size(); ++i)
    {
        Chain.LODAssets[i] = std::move(AssetList[i]);
    }

    csp::common::Sort(Chain.LODAssets, [](const LODAsset& Asset1, const LODAsset& Asset2) { return Asset1.Level < Asset2.Level; });

    return Chain;
}

bool ValidateNewLODLevelForChain(const LODChain& Chain, int LODLevel)
{
    // Ensure LODLevel doesnt already exist
    int Index = csp::common::FindIf(Chain.LODAssets, [LODLevel](const LODAsset& LODAsset) { return LODAsset.Level == LODLevel; });

    return Index == -1;
}
} // namespace csp::systems
