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

bool IsNearlyEqual(const float A, const float B) { return abs(A - B) < FloatComparatorThreshold; }

const Vector2& Vector2::Zero()
{
    static Vector2 _Zero = { 0, 0 };

    return _Zero;
}

const Vector2& Vector2::One()
{
    static Vector2 _One = { 1, 1 };

    return _One;
}

Vector2 Vector2::operator+(const Vector2& Other) const { return Vector2(X + Other.X, Y + Other.Y); }

Vector2 Vector2::operator-(const Vector2& Other) const { return Vector2(X - Other.X, Y - Other.Y); }

Vector2 Vector2::operator/(float Divisor) const { return Vector2(X / Divisor, Y / Divisor); }

Vector2 Vector2::operator*(const Vector2& Other) const { return Vector2(X * Other.X, Y * Other.Y); }

Vector2 Vector2::operator*(float Scalar) const { return Vector2(X * Scalar, Y * Scalar); }

bool Vector2::operator==(const Vector2 Other) const { return IsNearlyEqual(X, Other.X) && IsNearlyEqual(Y, Other.Y); }

bool Vector2::operator!=(const Vector2 Other) const { return !(*this == Other); }

const Vector3& Vector3::Zero()
{
    static Vector3 _Zero = { 0, 0, 0 };

    return _Zero;
}

const Vector3& Vector3::One()
{
    static Vector3 _One = { 1, 1, 1 };

    return _One;
}

Vector3 Vector3::operator+(const Vector3& Other) const { return Vector3(X + Other.X, Y + Other.Y, Z + Other.Z); }

Vector3 Vector3::operator-(const Vector3& Other) const { return Vector3(X - Other.X, Y - Other.Y, Z - Other.Z); }

Vector3 Vector3::operator/(float Divisor) const { return Vector3(X / Divisor, Y / Divisor, Z / Divisor); }

Vector3 Vector3::operator*(const Vector3& Other) const { return Vector3(X * Other.X, Y * Other.Y, Z * Other.Z); }

Vector3 Vector3::operator*(float Scalar) const { return Vector3(X * Scalar, Y * Scalar, Z * Scalar); }

bool Vector3::operator==(const Vector3 Other) const { return IsNearlyEqual(X, Other.X) && IsNearlyEqual(Y, Other.Y) && IsNearlyEqual(Z, Other.Z); }

bool Vector3::operator!=(const Vector3 Other) const { return !(*this == Other); }

const Vector4& Vector4::Zero()
{
    static Vector4 _Zero = { 0, 0, 0, 0 };

    return _Zero;
}

const Vector4& Vector4::One()
{
    static Vector4 _One = { 1, 1, 1, 1 };

    return _One;
}

const Vector4& Vector4::Identity()
{
    static Vector4 _Identity = { 0, 0, 0, 1 };

    return _Identity;
}

Vector4 Vector4::operator+(const Vector4& Other) const { return Vector4(X + Other.X, Y + Other.Y, Z + Other.Z, W + Other.W); }

Vector4 Vector4::operator-(const Vector4& Other) const { return Vector4(X - Other.X, Y - Other.Y, Z - Other.Z, W - Other.W); }

Vector4 Vector4::operator/(float Divisor) const { return Vector4(X / Divisor, Y / Divisor, Z / Divisor, W / Divisor); }

Vector4 Vector4::operator*(const Vector4& Other) const { return Vector4(X * Other.X, Y * Other.Y, Z * Other.Z, W * Other.W); }

Vector4 Vector4::operator*(float Scalar) const { return Vector4(X * Scalar, Y * Scalar, Z * Scalar, W * Scalar); }

bool Vector4::operator==(const Vector4 Other) const
{
    return IsNearlyEqual(X, Other.X) && IsNearlyEqual(Y, Other.Y) && IsNearlyEqual(Z, Other.Z) && IsNearlyEqual(W, Other.W);
}

bool Vector4::operator!=(const Vector4 Other) const { return !(*this == Other); }

} // namespace csp::common
