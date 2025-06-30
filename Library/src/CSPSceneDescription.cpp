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

#include "CSP/CSPSceneDescription.h"
#include "Multiplayer/MCS/MCSSceneDescription.h"
#include "Multiplayer/MCS/MCSTypes.h"
#include "Services/AggregationService/Dto.h"
#include "Services/PrototypeService/Dto.h"
#include "Services/UserService/Dto.h"

namespace csp
{
SceneDescription::SceneDescription(const csp::multiplayer::mcs::SceneDescription& MCSSceneDescription,
    csp::multiplayer::SpaceEntitySystem& EntitySystem, csp::common::LogSystem& LogSystem, csp::common::IJSScriptRunner& RemoteScriptRunner)
{
    csp::systems::GroupDtoToSpace(MCSSceneDescription.Group, Space);

    Entities = csp::common::Array<csp::multiplayer::SpaceEntity*>(MCSSceneDescription.Objects.size());

    size_t ObjectsIndex = 0;
    for (const auto& Object : MCSSceneDescription.Objects)
    {
        auto* Entity = new multiplayer::SpaceEntity(&EntitySystem, RemoteScriptRunner, &LogSystem);
        Entity->FromObjectMessage(Object);

        EntitySystem.AddEntity(Entity);
        Entities[ObjectsIndex] = Entity;
        ObjectsIndex++;
    }

    AssetCollections = csp::common::Array<csp::systems::AssetCollection>(MCSSceneDescription.Prototypes.size());

    size_t PrototypesIndex = 0;
    for (const auto& Prototype : MCSSceneDescription.Prototypes)
    {
        csp::systems::PrototypeDtoToAssetCollection(Prototype, AssetCollections[PrototypesIndex]);
        PrototypesIndex++;
    }

    Assets = csp::common::Array<csp::systems::Asset>(MCSSceneDescription.AssetDetails.size());

    size_t AssetDetailIndex = 0;
    for (const auto& AssetDetail : MCSSceneDescription.AssetDetails)
    {
        csp::systems::AssetDetailDtoToAsset(AssetDetail, Assets[AssetDetailIndex]);
        AssetDetailIndex++;
    }

    Sequences = csp::common::Array<csp::systems::Sequence>(MCSSceneDescription.Sequences.size());

    size_t SequenceIndex = 0;
    for (const auto& Sequence : MCSSceneDescription.Sequences)
    {
        csp::systems::SequenceDtoToSequence(Sequence, Sequences[SequenceIndex]);
        SequenceIndex++;
    }
}
}
