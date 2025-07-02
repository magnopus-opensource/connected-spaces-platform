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
#pragma once

#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Multiplayer/SpaceEntitySystem.h"

namespace csp::multiplayer
{
/// @brief CSPSceneDescription which represents all entities that exists for a scene.
/// @details This data structure is created through the deserialization of a CSPSceneDescription Json which is retrieved externally.
/// To retrieve the scene data, a CSPSceneData should be created using the same json file.
class CSPSceneDescription
{
public:
    /// @brief Constructor for CSPSceneDescription by deserializing a SceneDescription json file.
    /// @param SceneDescriptionJson csp::common::String : The SceneDescription to deserialize.
    /// @param EntitySystem csp::multiplayer::SpaceEntitySystem& : The SpaceEntitySystem for this session.
    /// @param LogSystem csp::common::LogSystem& : The SpaceEntitySystem for this session.
    /// @param RemoteScriptRunner csp::common::IJSScriptRunner& : The ScriptRunner for this session.
    CSPSceneDescription(const csp::common::String& SceneDescriptionJson, csp::multiplayer::SpaceEntitySystem& EntitySystem,
        csp::common::LogSystem& LogSystem, csp::common::IJSScriptRunner& RemoteScriptRunner);

    /// @brief The Entities that exist for this scene.
    csp::common::Array<csp::multiplayer::SpaceEntity*> Entities;
};

}
