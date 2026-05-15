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
#include "Multiplayer/SpaceEntityStatePatcher.h"
#include "Json/JsonSerializer.h"
#include <numeric>

namespace csp::multiplayer
{
CSPSceneDescription::CSPSceneDescription(const csp::common::List<csp::common::String>& sceneDescriptionJson)

{
    // Unpack the list into a single JSON string.
    // The reason this JSON is packed into a list _at all_ is merely a wrapper generator workaround,
    // csp::common::Strings cannot be passed as heap objects, and these SceneDescriptions can be large
    // enough to blow the stack
    this->m_sceneDescriptionJson = std::accumulate(sceneDescriptionJson.begin(), sceneDescriptionJson.end(), csp::common::String {});
}

csp::common::Array<csp::multiplayer::SpaceEntity*> CSPSceneDescription::CreateEntities(
    csp::common::IRealtimeEngine& realtimeEngine, csp::common::LogSystem& logSystem, csp::common::IJSScriptRunner& remoteScriptRunner) const
{
    mcs::SceneDescription sceneDescription;
    csp::json::JsonDeserializer::Deserialize(m_sceneDescriptionJson.c_str(), sceneDescription);

    csp::common::Array<csp::multiplayer::SpaceEntity*> entities { sceneDescription.Objects.size() };

    size_t objectsIndex = 0;
    for (const auto& object : sceneDescription.Objects)
    {
        auto entity = SpaceEntityStatePatcher::NewFromObjectMessage(object, realtimeEngine, remoteScriptRunner, logSystem);

        entities[objectsIndex] = entity.release();
        objectsIndex++;
    }

    return entities;
}

}
