#pragma once

#include "Olympus/OlympusCommon.h"

namespace oly_common
{

class OLY_API Vector3
{
public:
    /**
     * @brief Returns a Vector3 with all fields set to 0
     */
    static const Vector3& Zero();

    /**
     * @brief Returns a Vector3 with all fields set to 1
     */
    static const Vector3& One();

    Vector3()
        : X(0)
        , Y(0)
        , Z(0)
    {
    }

    Vector3(float X, float Y, float Z)
        : X(X)
        , Y(Y)
        , Z(Z)
    {
    }

    OLY_START_IGNORE
    bool operator==(Vector3 Other) const;
    bool operator!=(Vector3 Other) const;
    OLY_END_IGNORE

    float X;
    float Y;
    float Z;
};

class OLY_API Vector4
{
public:
    /**
     * @brief Returns a Vector4 with all fields set to 0
     */
    static const Vector4& Zero();

    /**
     * @brief Returns a Vector4 with all fields set to 1
     */
    static const Vector4& One();

    /**
     * @brief Returns a Vector4 that represents an identity quaternion (0, 0, 0, 1)
     */
    static const Vector4& Identity();

    Vector4()
        : X(0)
        , Y(0)
        , Z(0)
        , W(0)
    {
    }

    Vector4(float X, float Y, float Z, float W)
        : X(X)
        , Y(Y)
        , Z(Z)
        , W(W)
    {
    }

    OLY_START_IGNORE
    bool operator==(Vector4 Other) const;
    bool operator!=(Vector4 Other) const;
    OLY_END_IGNORE

    float X;
    float Y;
    float Z;
    float W;
};

} // namespace oly_common
