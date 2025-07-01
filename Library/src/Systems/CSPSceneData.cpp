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

#include "CSP/Systems/CSPSceneData.h"
#include "CSP/Systems/MCS/MCSSceneData.h"
#include "Services/AggregationService/Dto.h"
#include "Services/PrototypeService/Dto.h"
#include "Services/UserService/Dto.h"
#include "Json/JsonSerializer.h"

namespace csp::systems
{

CSPSceneData::CSPSceneData(const csp::common::String& SceneDescriptionJson)
{
    mcs::SceneData SceneData;
    csp::json::JsonDeserializer::Deserialize(SceneDescriptionJson.c_str(), SceneData);

    csp::systems::GroupDtoToSpace(SceneData.Group, Space);

    AssetCollections = csp::common::Array<csp::systems::AssetCollection>(SceneData.Prototypes.size());

    size_t PrototypesIndex = 0;
    for (const auto& Prototype : SceneData.Prototypes)
    {
        csp::systems::PrototypeDtoToAssetCollection(Prototype, AssetCollections[PrototypesIndex]);
        PrototypesIndex++;
    }

    Assets = csp::common::Array<csp::systems::Asset>(SceneData.AssetDetails.size());

    size_t AssetDetailIndex = 0;
    for (const auto& AssetDetail : SceneData.AssetDetails)
    {
        csp::systems::AssetDetailDtoToAsset(AssetDetail, Assets[AssetDetailIndex]);
        AssetDetailIndex++;
    }

    Sequences = csp::common::Array<csp::systems::Sequence>(SceneData.Sequences.size());

    size_t SequenceIndex = 0;
    for (const auto& Sequence : SceneData.Sequences)
    {
        csp::systems::SequenceDtoToSequence(Sequence, Sequences[SequenceIndex]);
        SequenceIndex++;
    }
}
}
