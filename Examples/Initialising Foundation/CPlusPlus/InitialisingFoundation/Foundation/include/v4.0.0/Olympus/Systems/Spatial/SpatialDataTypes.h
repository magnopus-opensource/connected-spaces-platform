#pragma once
#include "Olympus/OlympusCommon.h"

namespace oly_systems
{

class OLY_API GeoLocation
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

    bool IsValid() const;

    bool operator==(const GeoLocation& Other) const;
    bool operator!=(const GeoLocation& Other) const;
};

class OLY_API OlyRotation
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

} // namespace oly_systems
