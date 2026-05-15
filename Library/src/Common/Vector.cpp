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

#include "CSP/Common/Vector.h"

#include <cmath>

namespace csp::common
{

static constexpr float FloatComparatorThreshold = 0.001f;

bool IsNearlyEqual(const float a, const float b) { return abs(a - b) < FloatComparatorThreshold; }

const Vector2& Vector2::Zero()
{
    static Vector2 zero = { 0, 0 };

    return zero;
}

const Vector2& Vector2::One()
{
    static Vector2 one = { 1, 1 };

    return one;
}

Vector2 Vector2::operator+(const Vector2& other) const { return Vector2(X + other.X, Y + other.Y); }

Vector2 Vector2::operator-(const Vector2& other) const { return Vector2(X - other.X, Y - other.Y); }

Vector2 Vector2::operator/(float divisor) const { return Vector2(X / divisor, Y / divisor); }

Vector2 Vector2::operator*(const Vector2& other) const { return Vector2(X * other.X, Y * other.Y); }

Vector2 Vector2::operator*(float scalar) const { return Vector2(X * scalar, Y * scalar); }

bool Vector2::operator==(const Vector2 other) const { return IsNearlyEqual(X, other.X) && IsNearlyEqual(Y, other.Y); }

bool Vector2::operator!=(const Vector2 other) const { return !(*this == other); }

const Vector3& Vector3::Zero()
{
    static Vector3 zero = { 0, 0, 0 };

    return zero;
}

const Vector3& Vector3::One()
{
    static Vector3 one = { 1, 1, 1 };

    return one;
}

Vector3 Vector3::operator+(const Vector3& other) const { return Vector3(X + other.X, Y + other.Y, Z + other.Z); }

Vector3 Vector3::operator-(const Vector3& other) const { return Vector3(X - other.X, Y - other.Y, Z - other.Z); }

Vector3 Vector3::operator/(float divisor) const { return Vector3(X / divisor, Y / divisor, Z / divisor); }

Vector3 Vector3::operator*(const Vector3& other) const { return Vector3(X * other.X, Y * other.Y, Z * other.Z); }

Vector3 Vector3::operator*(float scalar) const { return Vector3(X * scalar, Y * scalar, Z * scalar); }

bool Vector3::operator==(const Vector3 other) const { return IsNearlyEqual(X, other.X) && IsNearlyEqual(Y, other.Y) && IsNearlyEqual(Z, other.Z); }

bool Vector3::operator!=(const Vector3 other) const { return !(*this == other); }

const Vector4& Vector4::Zero()
{
    static Vector4 zero = { 0, 0, 0, 0 };

    return zero;
}

const Vector4& Vector4::One()
{
    static Vector4 one = { 1, 1, 1, 1 };

    return one;
}

const Vector4& Vector4::Identity()
{
    static Vector4 identity = { 0, 0, 0, 1 };

    return identity;
}

Vector4 Vector4::operator+(const Vector4& other) const { return Vector4(X + other.X, Y + other.Y, Z + other.Z, W + other.W); }

Vector4 Vector4::operator-(const Vector4& other) const { return Vector4(X - other.X, Y - other.Y, Z - other.Z, W - other.W); }

Vector4 Vector4::operator/(float divisor) const { return Vector4(X / divisor, Y / divisor, Z / divisor, W / divisor); }

Vector4 Vector4::operator*(const Vector4& other) const { return Vector4(X * other.X, Y * other.Y, Z * other.Z, W * other.W); }

Vector4 Vector4::operator*(float scalar) const { return Vector4(X * scalar, Y * scalar, Z * scalar, W * scalar); }

bool Vector4::operator==(const Vector4 other) const
{
    return IsNearlyEqual(X, other.X) && IsNearlyEqual(Y, other.Y) && IsNearlyEqual(Z, other.Z) && IsNearlyEqual(W, other.W);
}

bool Vector4::operator!=(const Vector4 other) const { return !(*this == other); }

} // namespace csp::common
