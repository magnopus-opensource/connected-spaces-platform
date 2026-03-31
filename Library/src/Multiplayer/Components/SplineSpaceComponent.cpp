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
#include "CSP/Common/Systems/Log/LogSystem.h"

#include "Multiplayer/Component/Schema.h"
#include "Multiplayer/Script/ComponentBinding/SplineSpaceComponentScriptInterface.h"

#include "tinyspline.h"

namespace csp::multiplayer
{

namespace 
{
    struct TsSpline
    {
        tsBSpline s = ts_bspline_init();
        ~TsSpline() { ts_bspline_free(&s); }
    };

    struct TsNet
    {
        tsDeBoorNet n = ts_deboornet_init();
        ~TsNet() { ts_deboornet_free(&n); }
    };
}

const auto Schema = ComponentBase::ComponentSchema {
    ComponentType::Spline,
    std::vector<ComponentBase::ComponentSchema::Property> {
        {
            static_cast<ComponentBase::PropertyKey>(SplinePropertyKeys::Waypoints),
            0.f,
        },
    },
};

SplineSpaceComponent::SplineSpaceComponent(csp::common::LogSystem* LogSystem, SpaceEntity* Parent)
    : ComponentBase(Schema, LogSystem, Parent)
{
    SetScriptInterface(new SplineSpaceComponentScriptInterface(this));
}

csp::common::Vector3 SplineSpaceComponent::GetLocationAlongSpline(float NormalisedDistance)
{
    csp::common::List<csp::common::Vector3> ListPoints = GetWaypoints();
    if (ListPoints.Size() != 0)
    {
        std::vector<tsReal> Internalpoints;

        for (size_t i = 0; i < ListPoints.Size(); ++i)
        {
            Internalpoints.push_back(static_cast<double>(ListPoints[i].X));
            Internalpoints.push_back(static_cast<double>(ListPoints[i].Y));
            Internalpoints.push_back(static_cast<double>(ListPoints[i].Z));
        }

        std::vector<tsReal> internalPoints;
        internalPoints.reserve(ListPoints.Size() * 3);

        for (size_t i = 0; i < ListPoints.Size(); ++i)
        {
            internalPoints.push_back(static_cast<tsReal>(ListPoints[i].X));
            internalPoints.push_back(static_cast<tsReal>(ListPoints[i].Y));
            internalPoints.push_back(static_cast<tsReal>(ListPoints[i].Z));
        }

        constexpr size_t Dimension = 3;

        tsStatus Status {};
        TsSpline Spline;

        tsError Err = ts_bspline_interpolate_cubic_natural(internalPoints.data(), internalPoints.size() / Dimension, Dimension, &Spline.s, &Status);

        if (Err != TS_SUCCESS)
        {
            if (LogSystem != nullptr)
            {
                LogSystem->LogMsg(csp::common::LogLevel::Error, (std::string{ "SplineSpaceComponent::GetLocationAlongSpline spline error: "} + Status.message).c_str());
            }

            return {};
        }

        TsNet Net;
        Err = ts_bspline_eval(&Spline.s, NormalisedDistance, &Net.n, &Status);

        if (Err != TS_SUCCESS)
        {
            if (LogSystem != nullptr)
            {
                LogSystem->LogMsg(csp::common::LogLevel::Error,
                    (std::string { "SplineSpaceComponent::GetLocationAlongSpline spline error: " } + Status.message).c_str());
            }

            return {};
        }

        const tsReal* ptr = ts_deboornet_result_ptr(&Net.n);

        std::vector<tsReal> SplineResult(ptr, ptr + ts_deboornet_dimension(&Net.n));
        return { static_cast<float>(SplineResult[0]), static_cast<float>(SplineResult[1]), static_cast<float>(SplineResult[2]) };
    }
    else
    {
        if (LogSystem != nullptr)
        {
            LogSystem->LogMsg(csp::common::LogLevel::Error, "Waypoints not Set.");
        }

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

    for (size_t i = 0; i < Waypoints.Size(); ++i)
    {
        SetProperty(static_cast<uint32_t>((static_cast<int>(SplinePropertyKeys::Waypoints) + 1) + i), Waypoints[i]);
    }
}
} // namespace csp::multiplayer
