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

#include "CSP/Multiplayer/CSPSceneDescription.h"
#include "Multiplayer/MCS/MCSSceneDescription.h"
#include "Multiplayer/MCS/MCSTypes.h"
#include "Json/JsonSerializer.h"

namespace csp::multiplayer
{
CSPSceneDescription::CSPSceneDescription(const csp::common::String& SceneDescriptionJson)
    : SceneDescriptionJson { SceneDescriptionJson }
{
}

csp::common::Array<csp::multiplayer::SpaceEntity*> CSPSceneDescription::CreateEntities(
    csp::common::IRealtimeEngine& RealtimeEngine, csp::common::LogSystem& LogSystem, csp::common::IJSScriptRunner& RemoteScriptRunner) const
{
    mcs::SceneDescription SceneDescription;
    csp::json::JsonDeserializer::Deserialize(SceneDescriptionJson.c_str(), SceneDescription);

    csp::common::Array<csp::multiplayer::SpaceEntity*> Entities { SceneDescription.Objects.size() };

    size_t ObjectsIndex = 0;
    for (const auto& Object : SceneDescription.Objects)
    {
        auto* Entity = new multiplayer::SpaceEntity(&RealtimeEngine, RemoteScriptRunner, &LogSystem);
        Entity->FromObjectMessage(Object);

        Entities[ObjectsIndex] = Entity;
        ObjectsIndex++;
    }

    return Entities;
}

}
