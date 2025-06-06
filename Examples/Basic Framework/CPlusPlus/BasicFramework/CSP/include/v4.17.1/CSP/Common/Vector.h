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

namespace csp::common
{

/// @brief Represents a 3 dimensional vector.
class CSP_API Vector3
{
public:
    /// @brief Returns a Vector3 with all fields set to 0.
    /// @return const Vector3&
    static const Vector3& Zero();

    /// @brief Returns a Vector3 with all fields set to 1.
    /// @return const Vector3&
    static const Vector3& One();

    /// @brief Constructs a Vector3 with all fields set to 0.
    Vector3()
        : X(0)
        , Y(0)
        , Z(0)
    {
    }

    /// @brief Constructs a Vector3 with the given x, y, z values.
    /// @param X float
    /// @param Y float
    /// @param Z float
    Vector3(float X, float Y, float Z)
        : X(X)
        , Y(Y)
        , Z(Z)
    {
    }

    CSP_START_IGNORE
    bool operator==(Vector3 Other) const;
    bool operator!=(Vector3 Other) const;
    CSP_END_IGNORE

    float X;
    float Y;
    float Z;
};

/// @brief Represents a 4 dimensional vector.
class CSP_API Vector4
{
public:
    /// @brief Returns a Vector4 with all fields set to 0.
    /// @return const Vector4&
    static const Vector4& Zero();
    /// @brief Returns a Vector4 with all fields set to 1.
    /// @return const Vector4&
    static const Vector4& One();

    /// @brief Returns a Vector4 that represents an identity quaternion (0, 0, 0, 1).
    /// @return const Vector4&
    static const Vector4& Identity();

    /// @brief Constructs a Vector4 with all fields set to 0.
    Vector4()
        : X(0)
        , Y(0)
        , Z(0)
        , W(0)
    {
    }

    /// @brief Constructs a Vector4 with the given x, y, z, w values.
    /// @param X float
    /// @param Y float
    /// @param Z float
    /// @param W float
    Vector4(float X, float Y, float Z, float W)
        : X(X)
        , Y(Y)
        , Z(Z)
        , W(W)
    {
    }

    CSP_START_IGNORE
    bool operator==(Vector4 Other) const;
    bool operator!=(Vector4 Other) const;
    CSP_END_IGNORE

    float X;
    float Y;
    float Z;
    float W;
};

} // namespace csp::common
