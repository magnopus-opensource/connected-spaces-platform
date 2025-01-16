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
#include "CSP/Common/Vector.h"

namespace csp::multiplayer
{

/// @brief Simple class for holding the position, rotation and scale of a transform.
class CSP_API SpaceTransform
{
public:
    /// @brief Default constuctor for the SpaceTransform
    SpaceTransform();

    /// @brief Custom constructor for the SpaceTransform.
    /// @param Position csp::common::Vector3 : The position value for the transform.
    /// @param Rotation csp::common::Vector4 : The rotation value for the transform.
    /// @param Scale csp::common::Vector3 : The scale value for the transform.
    SpaceTransform(const csp::common::Vector3& Position, const csp::common::Vector4& Rotation, const csp::common::Vector3& Scale);

    /// @brief Equality operator
    /// @param SpaceTransform Transform
    bool operator==(const SpaceTransform& Transform) const;

    /// @brief Multiplication operator
    /// @param SpaceTransform Transform
    /// @note This mimics the operations of a matrix transform multiplication, but converts the back into a recognisable form.
    SpaceTransform operator*(const SpaceTransform& Transform) const;
    /// @brief The position value for the transform.
    csp::common::Vector3 Position;

    /// @brief The rotation value for the transform.
    csp::common::Vector4 Rotation;

    /// @brief The scale value for the transform.
    csp::common::Vector3 Scale;
};

} // namespace csp::multiplayer
