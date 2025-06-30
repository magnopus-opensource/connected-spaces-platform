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

#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Multiplayer/SpaceEntitySystem.h"
#include "CSP/Systems/Assets/Asset.h"
#include "CSP/Systems/Assets/AssetCollection.h"
#include "CSP/Systems/Sequence/SequenceSystem.h"
#include "CSP/Systems/Spaces/Space.h"

namespace csp::multiplayer::mcs
{
class SceneDescription;
}

namespace csp
{

class SceneDescription
{
public:
    SceneDescription(const csp::multiplayer::mcs::SceneDescription& MCSSceneDescription, csp::multiplayer::SpaceEntitySystem& EntitySystem);

    csp::systems::Space Space;
    csp::common::Array<csp::multiplayer::SpaceEntity*> Entities;
    csp::common::Array<csp::systems::AssetCollection> AssetCollections;
    csp::common::Array<csp::systems::Asset> Assets;
    csp::common::Array<csp::systems::Sequence> Sequences;
};

}
