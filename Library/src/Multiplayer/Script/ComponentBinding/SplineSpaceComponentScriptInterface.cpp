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

csp::multiplayer::SplineSpaceComponentScriptInterface::SplineSpaceComponentScriptInterface(SplineSpaceComponent* InComponent)
    : ComponentScriptInterface(InComponent)
{
}

ComponentScriptInterface::Vector3 SplineSpaceComponentScriptInterface::GetLocationAlongSpline(float NormalisedDistance)
{
    auto Result = static_cast<SplineSpaceComponent*>(Component)->GetLocationAlongSpline(NormalisedDistance);

    return { Result.X, Result.Y, Result.Z };
}

std::vector<ComponentScriptInterface::Vector3> SplineSpaceComponentScriptInterface::GetWaypoints()
{
    std::vector<Vector3> ReturnList;

    auto Result = static_cast<SplineSpaceComponent*>(Component)->GetWaypoints();

    for (int i = 0; i < Result.Size(); ++i)
    {
        ReturnList.push_back({ Result[i].X, Result[i].Y, Result[i].Z });
    }

    return ReturnList;
}

void SplineSpaceComponentScriptInterface::SetWaypoints(std::vector<Vector3> Waypoints)
{
    csp::common::List<csp::common::Vector3> ConvertedList;

    for (int i = 0; i < Waypoints.size(); ++i)
    {
        ConvertedList.Append({ Waypoints[i][0], Waypoints[i][1], Waypoints[i][2] });
    }

    static_cast<SplineSpaceComponent*>(Component)->SetWaypoints(ConvertedList);
}

} // namespace csp::multiplayer
