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
#include "CSP/Multiplayer/Components/SplineSpaceComponent.h"

#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Multiplayer/Script/ComponentBinding/SplineSpaceComponentScriptInterface.h"
#include "tinysplinecxx.h"

namespace csp::multiplayer
{
SplineSpaceComponent::SplineSpaceComponent(SpaceEntity* Parent)
    : ComponentBase(ComponentType::Spline, Parent)
{
    Properties[static_cast<uint32_t>(SplinePropertyKeys::Waypoints)] = 0.f;

    SetScriptInterface(CSP_NEW SplineSpaceComponentScriptInterface(this));
}

csp::common::Vector3 SplineSpaceComponent::GetLocationAlongSpline(float NormalisedDistance)
{
    csp::common::List<csp::common::Vector3> ListPoints = GetWaypoints();
    if (ListPoints.Size() != 0)
    {
        std::vector<tinyspline::real> Internalpoints;

        for (int i = 0; i < ListPoints.Size(); ++i)
        {
            Internalpoints.push_back(static_cast<double>(ListPoints[i].X));
            Internalpoints.push_back(static_cast<double>(ListPoints[i].Y));
            Internalpoints.push_back(static_cast<double>(ListPoints[i].Z));
        }

        tinyspline::BSpline spline = tinyspline::BSpline::interpolateCubicNatural(Internalpoints, 3);
        const std::vector<tinyspline::real> SplineResult = spline.eval(NormalisedDistance).result();

        return { (float)SplineResult[0], (float)SplineResult[1], (float)SplineResult[2] };
    }
    else
    {
        CSP_LOG_ERROR_MSG("Waypoints not Set.");

        return {};
    }
};

csp::common::List<csp::common::Vector3> SplineSpaceComponent::GetWaypoints() const
{
    csp::common::List<csp::common::Vector3> returnList;

    const auto& RepVal = GetIntegerProperty(static_cast<uint32_t>(SplinePropertyKeys::Waypoints));

    for (int i = 0; i < RepVal; ++i)
    {
        returnList.Append(GetVector3Property(static_cast<uint32_t>((static_cast<int>(SplinePropertyKeys::Waypoints) + 1) + i)));
    }

    return returnList;
}

void SplineSpaceComponent::SetWaypoints(const csp::common::List<csp::common::Vector3>& Waypoints)
{
    SetProperty(static_cast<uint32_t>(SplinePropertyKeys::Waypoints), (int64_t)Waypoints.Size());

    for (int i = 0; i < Waypoints.Size(); ++i)
    {
        SetProperty(static_cast<uint32_t>((static_cast<int>(SplinePropertyKeys::Waypoints) + 1) + i), Waypoints[i]);
    }
}
} // namespace csp::multiplayer
