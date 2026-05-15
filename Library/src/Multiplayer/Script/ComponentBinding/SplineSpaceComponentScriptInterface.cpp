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

#include "Multiplayer/Script/ComponentBinding/SplineSpaceComponentScriptInterface.h"

#include "CSP/Multiplayer/Components/SplineSpaceComponent.h"
#include "CSP/Multiplayer/SpaceEntity.h"

namespace csp::multiplayer
{

csp::multiplayer::SplineSpaceComponentScriptInterface::SplineSpaceComponentScriptInterface(SplineSpaceComponent* inComponent)
    : ComponentScriptInterface(inComponent)
{
}

ComponentScriptInterface::Vector3 SplineSpaceComponentScriptInterface::GetLocationAlongSpline(float normalisedDistance)
{
    auto result = static_cast<SplineSpaceComponent*>(m_component)->GetLocationAlongSpline(normalisedDistance);

    return { result.X, result.Y, result.Z };
}

std::vector<ComponentScriptInterface::Vector3> SplineSpaceComponentScriptInterface::GetWaypoints()
{
    std::vector<Vector3> returnList;

    auto result = static_cast<SplineSpaceComponent*>(m_component)->GetWaypoints();

    for (size_t i = 0; i < result.Size(); ++i)
    {
        returnList.push_back({ result[i].X, result[i].Y, result[i].Z });
    }

    return returnList;
}

void SplineSpaceComponentScriptInterface::SetWaypoints(std::vector<Vector3> waypoints)
{
    csp::common::List<csp::common::Vector3> convertedList;

    for (size_t i = 0; i < waypoints.size(); ++i)
    {
        convertedList.Append({ waypoints[i][0], waypoints[i][1], waypoints[i][2] });
    }

    static_cast<SplineSpaceComponent*>(m_component)->SetWaypoints(convertedList);
}

} // namespace csp::multiplayer
