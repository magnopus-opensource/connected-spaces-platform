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

#include "CSP/Common/Interfaces/IRealtimeEngine.h"
#include "CSP/Multiplayer/SpaceEntity.h"

namespace csp::multiplayer
{
/// @brief CSPSceneDescription which represents all entities that exists for a scene.
/// @details This data structure is created through the deserialization of a CSPSceneDescription Json which is retrieved externally.
/// The json file used to create this structure is also used to create a systems::CSPSceneData object.
/// The reason these are seperated is to break dependencies between our multiplayer and corer modules.
class CSP_API CSPSceneDescription
{
public:
    /// @brief Constructor for CSPSceneDescription by deserializing a SceneDescription json file.
    /// The specific packing of the JSON string into this list is not specific, you may pack by character
    /// or by token, so long as when naively concatenated, the original string is reproduced.
    CSPSceneDescription(const csp::common::List<csp::common::String>& SceneDescriptionJson);

    CSPSceneDescription() { }

    /// @brief Generates an array of entities from the SceneDescription Json
    /// This function exists because the construction of SpaceEntites relies on a RealtimeEngine, and the OfflineRealtimeEngine requires a
    /// CSPSceneDescription for construction.
    /// @param RealtimeEngine csp::common::IRealtimeEngine& : The RealtimeEngine for this session.
    /// @param LogSystem csp::common::LogSystem& : The SpaceEntitySystem for this session.
    /// @param RemoteScriptRunner csp::common::IJSScriptRunner& : The ScriptRunner for this session.
    CSP_NO_EXPORT csp::common::Array<csp::multiplayer::SpaceEntity*> CreateEntities(
        csp::common::IRealtimeEngine& RealtimeEngine, csp::common::LogSystem& LogSystem, csp::common::IJSScriptRunner& RemoteScriptRunner) const;

private:
    csp::common::String SceneDescriptionJson;
};

}
