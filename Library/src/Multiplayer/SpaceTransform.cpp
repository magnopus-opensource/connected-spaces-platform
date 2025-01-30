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

#define GLM_ENABLE_EXPERIMENTAL // glm::decompose is still technically an experimental feature, but better than a hand-rolled solution in my opinion.
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <tuple>

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

bool csp::multiplayer::SpaceTransform::operator!=(const SpaceTransform& Transform) const { return !(*this == Transform); }

namespace
{
/*
 * Return the TRS components derived from the position, quaternion and scale values set in this transform.
 * First is T(Translation), Second is R(Rotation), Third is S(Scale)
 * All Mat4s for uniform application, despite slight space redundancy in T and S matrices.
 */
std::tuple<glm::mat4, glm::mat4, glm::mat4> GetTRSComponents(const csp::multiplayer::SpaceTransform& Transform)
{
    glm::mat4 TMatrix = glm::mat4(1.0f); // mat4(1.0f) gives an identity matrix
    TMatrix = glm::translate(TMatrix, glm::vec3 { Transform.Position.X, Transform.Position.Y, Transform.Position.Z });

    // Quaternions must be normalized in order to not introduce crazy chaos.
    glm::quat NormalQ = glm::normalize(glm::quat(Transform.Rotation.W, Transform.Rotation.X, Transform.Rotation.Y, Transform.Rotation.Z));
    glm::mat4 RMatrix = glm::toMat4(std::move(NormalQ));

    glm::mat4 SMatrix = glm::mat4(1.0f);
    SMatrix = glm::scale(SMatrix, glm::vec3(Transform.Scale.X, Transform.Scale.Y, Transform.Scale.Z));

    return { std::move(TMatrix), std::move(RMatrix), std::move(SMatrix) };
}
}

csp::multiplayer::SpaceTransform csp::multiplayer::SpaceTransform::operator*(const SpaceTransform& Transform) const
{
    // Transformation order is important when applying TRS matrices. Scale first, then Rotation, then Translation.
    // Otherwise, non-uniform scaling is effected by previous rotation, introducing skew, which is not normally desired.
    const auto [T1, R1, S1] = GetTRSComponents(*this);
    const auto [T2, R2, S2] = GetTRSComponents(Transform);

    const auto TRS1 = T1 * R1 * S1; // Remember, "First" in matrix composition means the last element listed in the multiplication
    const auto TRS2 = T2 * R2 * S2;
    const auto ComposedTRS = TRS1 * TRS2;

    glm::vec3 Scale;
    glm::quat Rotation;
    glm::vec3 Translation;
    glm::vec3 Skew;
    glm::vec4 Perspective;
    glm::decompose(ComposedTRS, Scale, Rotation, Translation, Skew, Perspective);
    // We're ignoring skew and perspective, as TRS composition should not have introduced any,
    // provided inputs are normalized.
    // Note, if you're getting confusing outputs for rotation when you have negative scales, you're
    // probably flipping the rotation, which is _fine_, but hard to understand without visualization.

    return SpaceTransform(
        { Translation.x, Translation.y, Translation.z }, { Rotation.x, Rotation.y, Rotation.z, Rotation.w }, { Scale.x, Scale.y, Scale.z });
}
