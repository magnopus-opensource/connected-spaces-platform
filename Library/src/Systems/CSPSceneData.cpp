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

#include "CSP/CSPFoundation.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "Services/AggregationService/Dto.h"
#include "Services/PrototypeService/Dto.h"
#include "Services/UserService/Dto.h"
#include "Systems/MCS/MCSSceneData.h"

#include <numeric>

namespace csp::systems
{

CSPSceneData::CSPSceneData(const csp::common::List<csp::common::String>& SceneDescriptionJson)
{
    // Unpack the list into a single JSON string.
    // The reason this JSON is packed into a list _at all_ is merely a wrapper generator workaround,
    // csp::common::Strings cannot be passed as heap objects, and these SceneDescriptions can be large
    // enough to blow the stack
    csp::common::String SceneDescriptionStr = std::accumulate(SceneDescriptionJson.begin(), SceneDescriptionJson.end(), csp::common::String {});

    // TODO: Take the LogSystem as a constructor parameter instead, which is a public API change. Until then this
    // keeps the behaviour of the singleton logging macros it replaced, logging through the SystemsManager's
    // LogSystem once CSP is initialised and dropping messages otherwise.
    auto SilentLogSystem = csp::common::LogSystem {};
    SilentLogSystem.SetSystemLevel(csp::common::LogLevel::NoLogging);
    auto& LogSystem = csp::CSPFoundation::GetIsInitialised() ? *csp::systems::SystemsManager::Get().GetLogSystem() : SilentLogSystem;

    const auto SceneData = mcs::ParseSceneData(SceneDescriptionStr.c_str(), LogSystem);

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
