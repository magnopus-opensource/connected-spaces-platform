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
#pragma once

#include "CSP/Systems/Assets/Asset.h"
#include "CSP/Systems/Assets/AssetCollection.h"
#include "CSP/Systems/Sequence/SequenceSystem.h"
#include "CSP/Systems/Spaces/Space.h"
#include "CSP/Systems/Spatial/Anchor.h"

namespace csp::systems
{
/// @brief CSPSceneData which represents all data that exists for a space.
/// @details This data structure is created through the deserialization of a CSPSceneDescription Json which is retrieved externally.
/// The json file used to create this structure is also used to create a systems::CSPSceneData object.
/// The reason these are seperated is to break dependencies between our multiplayer and corer modules.
class CSP_API CSPSceneData
{
public:
    /// @brief Constructor for CSPSceneData by deserializing a SceneDescription json file.
    /// @param SceneDescriptionJson csp::common::List<csp::common::String> : The SceneDescription JSON.
    /// The specific packing of the JSON string into this list is not specific, you may pack by character
    /// or by token, so long as when naively concatenated, the original string is reproduced.
    CSPSceneData(const csp::common::List<csp::common::String>& SceneDescriptionJson);

    /// @brief The space the scene data represents.
    csp::systems::Space Space;
    /// @brief The asset collections for this scene.
    csp::common::Array<csp::systems::AssetCollection> AssetCollections;
    /// @brief The assets for this scene.
    csp::common::Array<csp::systems::Asset> Assets;
    /// @brief The sequences for this scene.
    csp::common::Array<csp::systems::Sequence> Sequences;
    /// @brief The anchors for this scene.
    csp::common::Array<csp::systems::Anchor> Anchors;
};

}
