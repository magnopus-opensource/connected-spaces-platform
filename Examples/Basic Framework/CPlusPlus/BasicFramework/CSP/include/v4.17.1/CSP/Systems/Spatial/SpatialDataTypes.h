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
#include "CSP/CSPCommon.h"

namespace csp::systems
{

/// @brief Data structure representing a Geo-spatial location, handling lonitude and latitude, with a validation function
class CSP_API GeoLocation
{
public:
    GeoLocation()
        : Longitude(0.0)
        , Latitude(0.0) {};
    GeoLocation(double InLongitude, double InLatitude)
        : Longitude(InLongitude)
        , Latitude(InLatitude) {};

    double Longitude;
    double Latitude;

    /// @brief Validates if the set data for longitude and latitude is a valid location.
    /// @return a boolean represnting validity of the location.
    bool IsValid() const;

    bool operator==(const GeoLocation& Other) const;
    bool operator!=(const GeoLocation& Other) const;
};

/// @brief Data structure representing rotation as defined by Connected Spaces Platform, in the format (X,Y,Z,W) as a quaternion with double values.
class CSP_API OlyRotation
{
public:
    OlyRotation()
        : X(0.0)
        , Y(0.0)
        , Z(0.0)
        , W(0.0) {};
    OlyRotation(double InX, double InY, double InZ, double InW)
        : X(InX)
        , Y(InY)
        , Z(InZ)
        , W(InW) {};

    double X;
    double Y;
    double Z;
    double W;
};

} // namespace csp::systems
