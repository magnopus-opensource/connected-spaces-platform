/*
 * Copyright 2025 Magnopus LLC

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

#include "CSP/Systems/CSPSceneData.h"
#include "CSP/Systems/MCS/MCSSceneData.h"
#include "Services/AggregationService/Dto.h"
#include "Services/PrototypeService/Dto.h"
#include "Services/UserService/Dto.h"
#include "Json/JsonSerializer.h"

#include <numeric>

namespace csp::systems
{

CSPSceneData::CSPSceneData(const csp::common::List<csp::common::String>& sceneDescriptionJson)
{
    // Unpack the list into a single JSON string.
    // The reason this JSON is packed into a list _at all_ is merely a wrapper generator workaround,
    // csp::common::Strings cannot be passed as heap objects, and these SceneDescriptions can be large
    // enough to blow the stack
    csp::common::String sceneDescriptionStr = std::accumulate(sceneDescriptionJson.begin(), sceneDescriptionJson.end(), csp::common::String {});

    mcs::SceneData sceneData;
    csp::json::JsonDeserializer::Deserialize(sceneDescriptionStr.c_str(), sceneData);

    csp::systems::GroupDtoToSpace(sceneData.Group, Space);

    AssetCollections = csp::common::Array<csp::systems::AssetCollection>(sceneData.Prototypes.size());

    size_t prototypesIndex = 0;
    for (const auto& prototype : sceneData.Prototypes)
    {
        csp::systems::PrototypeDtoToAssetCollection(prototype, AssetCollections[prototypesIndex]);
        prototypesIndex++;
    }

    Assets = csp::common::Array<csp::systems::Asset>(sceneData.AssetDetails.size());

    size_t assetDetailIndex = 0;
    for (const auto& assetDetail : sceneData.AssetDetails)
    {
        csp::systems::AssetDetailDtoToAsset(assetDetail, Assets[assetDetailIndex]);
        assetDetailIndex++;
    }

    Sequences = csp::common::Array<csp::systems::Sequence>(sceneData.Sequences.size());

    size_t sequenceIndex = 0;
    for (const auto& sequence : sceneData.Sequences)
    {
        csp::systems::SequenceDtoToSequence(sequence, Sequences[sequenceIndex]);
        sequenceIndex++;
    }
}
}
