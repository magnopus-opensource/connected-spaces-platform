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
    const csp::common::Vector3& position, const csp::common::Vector4& rotation, const csp::common::Vector3& scale)
    : Position(position)
    , Rotation(rotation)
    , Scale(scale)
{
}

bool csp::multiplayer::SpaceTransform::operator==(const SpaceTransform& transform) const
{
    return Position == transform.Position && Rotation == transform.Rotation && Scale == transform.Scale;
}

bool csp::multiplayer::SpaceTransform::operator!=(const SpaceTransform& transform) const { return !(*this == transform); }

namespace
{
/*
 * Return the TRS components derived from the position, quaternion and scale values set in this transform.
 * First is T(Translation), Second is R(Rotation), Third is S(Scale)
 * All Mat4s for uniform application, despite slight space redundancy in T and S matrices.
 */
std::tuple<glm::mat4, glm::mat4, glm::mat4> GetTRSComponents(const csp::multiplayer::SpaceTransform& transform)
{
    glm::mat4 tMatrix = glm::mat4(1.0f); // mat4(1.0f) gives an identity matrix
    tMatrix = glm::translate(tMatrix, glm::vec3 { transform.Position.X, transform.Position.Y, transform.Position.Z });

    // Quaternions must be normalized in order to not introduce crazy chaos.
    glm::quat normalQ = glm::normalize(glm::quat(transform.Rotation.W, transform.Rotation.X, transform.Rotation.Y, transform.Rotation.Z));
    glm::mat4 rMatrix = glm::toMat4(std::move(normalQ));

    glm::mat4 sMatrix = glm::mat4(1.0f);
    sMatrix = glm::scale(sMatrix, glm::vec3(transform.Scale.X, transform.Scale.Y, transform.Scale.Z));

    return { std::move(tMatrix), std::move(rMatrix), std::move(sMatrix) };
}
}

csp::multiplayer::SpaceTransform csp::multiplayer::SpaceTransform::operator*(const SpaceTransform& transform) const
{
    // Transformation order is important when applying TRS matrices. Scale first, then Rotation, then Translation.
    // Otherwise, non-uniform scaling is effected by previous rotation, introducing skew, which is not normally desired.
    const auto [T1, R1, S1] = GetTRSComponents(*this);
    const auto [T2, R2, S2] = GetTRSComponents(transform);

    const auto trS1 = T1 * R1 * S1; // Remember, "First" in matrix composition means the last element listed in the multiplication
    const auto trS2 = T2 * R2 * S2;
    const auto composedTrs = trS1 * trS2;

    glm::vec3 transformScale;
    glm::quat transformRotation;
    glm::vec3 translation;
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(composedTrs, transformScale, transformRotation, translation, skew, perspective);
    // We're ignoring skew and perspective, as TRS composition should not have introduced any,
    // provided inputs are normalized.
    // Note, if you're getting confusing outputs for rotation when you have negative scales, you're
    // probably flipping the rotation, which is _fine_, but hard to understand without visualization.

    return SpaceTransform({ translation.x, translation.y, translation.z },
        { transformRotation.x, transformRotation.y, transformRotation.z, transformRotation.w },
        { transformScale.x, transformScale.y, transformScale.z });
}
