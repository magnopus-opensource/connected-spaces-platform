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

/// @brief Represents a 2 dimensional vector.
class CSP_API Vector2
{
public:
    /// @brief Returns a Vector2 with all fields set to 0.
    /// @return const Vector2&
    static const Vector2& Zero();

    /// @brief Returns a Vector2 with all fields set to 1.
    /// @return const Vector2&
    static const Vector2& One();

    /// @brief Constructs a Vector2 with all fields set to 0.
    Vector2()
        : X(0)
        , Y(0)
    {
    }

    /// @brief Constructs a Vector2 with the given x, y values.
    /// @param X float
    /// @param Y float
    Vector2(float x, float y)
        : X(x)
        , Y(y)
    {
    }

    /// @brief Member by member addition with another Vector2
    /// @param Vector2 Other
    Vector2 operator+(const Vector2& other) const;

    /// @brief Subtracts another Vector2 from this one
    /// @param Vector2 Other
    Vector2 operator-(const Vector2& other) const;

    /// @brief Divides the Vector2 by divisor
    /// @param float Divisor
    Vector2 operator/(float divisor) const;

    /// @brief Member by member multiplication with another Vector2
    /// @param Vector2 Other
    Vector2 operator*(const Vector2& other) const;

    /// @brief Multiplies the Vector2 by a scalar
    /// @param float Scalar
    Vector2 operator*(float scalar) const;

    /// @brief Whether two vectors are equal to one another. Has a .001f accuracy.
    /// @param Vector3 Vector to compare
    bool operator==(Vector2 other) const;

    /// @brief Whether two vectors are inequal to one another. Has a .001f accuracy.
    /// @param Vector3 Vector to compare
    bool operator!=(Vector2 other) const;

    float X;
    float Y;
};

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
    Vector3(float x, float y, float z)
        : X(x)
        , Y(y)
        , Z(z)
    {
    }

    /// @brief Member by member addition with another Vector3
    /// @param Vector3 Other
    Vector3 operator+(const Vector3& other) const;

    /// @brief Subtracts another Vector3 from this one
    /// @param Vector3 Other
    Vector3 operator-(const Vector3& other) const;

    /// @brief Divides the Vector3 by divisor
    /// @param float Divisor
    Vector3 operator/(float divisor) const;

    /// @brief Member by member multiplication with another Vector3
    /// @param Vector3 Other
    Vector3 operator*(const Vector3& other) const;

    /// @brief Multiplies the Vector3 by a scalar
    /// @param float Scalar
    Vector3 operator*(float scalar) const;

    /// @brief Whether two vectors are equal to one another. Has a .001f accuracy.
    /// @param Vector3 Vector to compare
    bool operator==(Vector3 other) const;

    /// @brief Whether two vectors are inequal to one another. Has a .001f accuracy.
    /// @param Vector3 Vector to compare
    bool operator!=(Vector3 other) const;

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
    Vector4(float x, float y, float z, float w)
        : X(x)
        , Y(y)
        , Z(z)
        , W(w)
    {
    }

    /// @brief Member by member addition with another Vector4
    /// @param Vector4 Other
    Vector4 operator+(const Vector4& other) const;

    /// @brief Subtracts another Vector4 from this one
    /// @param Vector4 Other
    Vector4 operator-(const Vector4& other) const;

    /// @brief Divides the Vector4 by divisor
    /// @param float Divisor
    Vector4 operator/(float divisor) const;

    /// @brief Member by member multiplication with another Vector4
    /// @param Vector4 Other
    Vector4 operator*(const Vector4& other) const;

    /// @brief Multiplies the Vector4 by a scalar
    /// @param float Scalar
    Vector4 operator*(float scalar) const;

    /// @brief Whether two vectors are equal to one another. Has a .001f accuracy.
    /// @param Vector3 Vector to compare
    bool operator==(Vector4 other) const;

    /// @brief Whether two vectors are inequal to one another. Has a .001f accuracy.
    /// @param Vector3 Vector to compare
    bool operator!=(Vector4 other) const;

    float X;
    float Y;
    float Z;
    float W;
};

} // namespace csp::common
