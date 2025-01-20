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

#include "CSP/Multiplayer/SpaceTransform.h"

#include <glm/gtc/quaternion.hpp>

csp::multiplayer::SpaceTransform::SpaceTransform()
    : Position(0.0f, 0.0f, 0.0f)
    , Rotation(0.0f, 0.0f, 0.0f, 1.0f)
    , Scale(1.0f, 1.0f, 1.0f)
{
}

csp::multiplayer::SpaceTransform::SpaceTransform(
    const csp::common::Vector3& Position, const csp::common::Vector4& Rotation, const csp::common::Vector3& Scale)
    : Position(Position)
    , Rotation(Rotation)
    , Scale(Scale)
{
}

bool csp::multiplayer::SpaceTransform::operator==(const SpaceTransform& Transform) const
{
    return Position == Transform.Position && Rotation == Transform.Rotation && Scale == Transform.Scale;
}

csp::multiplayer::SpaceTransform csp::multiplayer::SpaceTransform::operator*(const SpaceTransform& Transform) const
{
    glm::quat Orientation { Rotation.X, Rotation.Y, Rotation.Z, Rotation.W };
    glm::quat OtherOrientation { Transform.Rotation.X, Transform.Rotation.Y, Transform.Rotation.Z, Transform.Rotation.W };
    glm::quat FinalOrientation = OtherOrientation * Orientation;
    // Temporarily supress the unused variable warning
    [[maybe_unused]] auto temporary = glm::normalize(FinalOrientation);
    return SpaceTransform(
        Position + Transform.Position, { FinalOrientation.x, FinalOrientation.y, FinalOrientation.z, FinalOrientation.w }, Scale * Transform.Scale);
}
