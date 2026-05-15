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
#include "CSP/Common/fmt_Formatters.h"

#include "CSP/Multiplayer/ComponentSchema.h"
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

const auto Schema = ComponentSchema {
    static_cast<ComponentSchema::TypeIdType>(ComponentType::Spline),
    "Spline",
    csp::common::Array<ComponentProperty> {
        {
            static_cast<ComponentProperty::KeyType>(SplinePropertyKeys::Waypoints),
            {}, // not exposed to scripting via an auto-generated property (has a legacy manual getter function)
            0.f,
        },
    },
};

const ComponentSchema& SplineSpaceComponent::GetSchema() { return Schema; }

SplineSpaceComponent::SplineSpaceComponent(csp::common::LogSystem* logSystem, SpaceEntity* parent)
    : ComponentBase(Schema, logSystem, parent)
{
    SetScriptInterface(new SplineSpaceComponentScriptInterface(this));
}

csp::common::Vector3 SplineSpaceComponent::GetLocationAlongSpline(float normalisedDistance)
{
    csp::common::List<csp::common::Vector3> listPoints = GetWaypoints();
    if (listPoints.Size() != 0)
    {
        std::vector<tsReal> internalPoints;
        internalPoints.reserve(listPoints.Size() * 3);

        for (size_t i = 0; i < listPoints.Size(); ++i)
        {
            internalPoints.push_back(static_cast<double>(listPoints[i].X));
            internalPoints.push_back(static_cast<double>(listPoints[i].Y));
            internalPoints.push_back(static_cast<double>(listPoints[i].Z));
        }

        constexpr size_t dimension = 3;

        tsStatus status {};
        TsSpline spline;

        tsError err = ts_bspline_interpolate_cubic_natural(internalPoints.data(), internalPoints.size() / dimension, dimension, &spline.s, &status);

        if (err != TS_SUCCESS)
        {
            if (m_logSystem != nullptr)
            {
                m_logSystem->LogMsg(csp::common::LogLevel::Error,
                    fmt::format("SplineSpaceComponent::GetLocationAlongSpline spline error: {}", status.message).c_str());
            }

            return {};
        }

        TsNet net;
        err = ts_bspline_eval(&spline.s, normalisedDistance, &net.n, &status);

        if (err != TS_SUCCESS)
        {
            if (m_logSystem != nullptr)
            {
                m_logSystem->LogMsg(csp::common::LogLevel::Error,
                    fmt::format("SplineSpaceComponent::GetLocationAlongSpline spline error: {}", status.message).c_str());
            }

            return {};
        }

        const tsReal* ptr = ts_deboornet_result_ptr(&net.n);

        std::vector<tsReal> splineResult(ptr, ptr + ts_deboornet_dimension(&net.n));
        return { static_cast<float>(splineResult[0]), static_cast<float>(splineResult[1]), static_cast<float>(splineResult[2]) };
    }
    else
    {
        if (m_logSystem != nullptr)
        {
            m_logSystem->LogMsg(csp::common::LogLevel::Error, "Waypoints not Set.");
        }

        return {};
    }
};

csp::common::List<csp::common::Vector3> SplineSpaceComponent::GetWaypoints() const
{
    csp::common::List<csp::common::Vector3> returnList;

    const auto& repVal = GetIntegerProperty(static_cast<uint32_t>(SplinePropertyKeys::Waypoints));

    for (int i = 0; i < repVal; ++i)
    {
        returnList.Append(GetVector3Property(static_cast<uint32_t>((static_cast<int>(SplinePropertyKeys::Waypoints) + 1) + i)));
    }

    return returnList;
}

void SplineSpaceComponent::SetWaypoints(const csp::common::List<csp::common::Vector3>& waypoints)
{
    SetProperty(static_cast<uint32_t>(SplinePropertyKeys::Waypoints), (int64_t)waypoints.Size());

    for (size_t i = 0; i < waypoints.Size(); ++i)
    {
        SetProperty(static_cast<uint32_t>((static_cast<int>(SplinePropertyKeys::Waypoints) + 1) + i), waypoints[i]);
    }
}
} // namespace csp::multiplayer
