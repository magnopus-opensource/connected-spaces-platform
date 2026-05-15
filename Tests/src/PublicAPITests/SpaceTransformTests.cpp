/*
 * Copyright 2025 Magnopus LLC

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
#include "TestHelpers.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "gtest/gtest.h"

CSP_PUBLIC_TEST(CSPEngine, SpaceTransformTests, DefaultConstructTest)
{
    const csp::multiplayer::SpaceTransform spaceTransform;
    EXPECT_EQ(spaceTransform.Position, csp::common::Vector3::Zero());
    EXPECT_EQ(spaceTransform.Rotation, csp::common::Vector4::Identity());
    EXPECT_EQ(spaceTransform.Scale, csp::common::Vector3::One());
}

CSP_PUBLIC_TEST(CSPEngine, SpaceTransformTests, ConstructTest)
{
    const csp::common::Vector3 pos { 1.0f, 2.0f, 3.0f };
    const csp::common::Vector4 rot { 15.0f, 35.0f, -10.0f, 1.0f };
    const csp::common::Vector3 scale { 3.0f, 2.0f, 1.0f };

    const csp::multiplayer::SpaceTransform spaceTransform(pos, rot, scale);
    EXPECT_EQ(spaceTransform.Position, pos);
    EXPECT_EQ(spaceTransform.Rotation, rot);
    EXPECT_EQ(spaceTransform.Scale, scale);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceTransformTests, EqualityTest)
{
    const csp::common::Vector3 pos { 1.0f, 2.0f, 3.0f };
    const csp::common::Vector4 rot { 15.0f, 35.0f, -10.0f, 1.0f };
    const csp::common::Vector3 scale { 3.0f, 2.0f, 1.0f };

    const csp::multiplayer::SpaceTransform spaceTransform1(pos, rot, scale);
    const csp::multiplayer::SpaceTransform spaceTransform2(pos, rot, scale);
    const csp::multiplayer::SpaceTransform spaceTransformIdentity;

    EXPECT_EQ(spaceTransform1, spaceTransform2);
    EXPECT_NE(spaceTransform1, spaceTransformIdentity);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceTransformTests, MulitplicationIdentityAxiomTest)
{
    // Two identity matrices should be identity
    const csp::multiplayer::SpaceTransform identity;
    EXPECT_EQ((identity * identity), identity);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceTransformTests, MulitplicationIdentityTransformTest)
{
    const csp::multiplayer::SpaceTransform identity;
    // For mathy trig reasons, this is quaternion languge for "ninety degrees" (not really ... sort of)
    const float sqrtTwoOverTwo = std::sqrt(2.0f) / 2.0f;
    const csp::common::Vector4 ninetyDegAroundXNormalizedQuat { sqrtTwoOverTwo, 0.0f, 0.0f, sqrtTwoOverTwo };

    // An identity matrix multiplied into another matrix (in either order) should just be the other matrix
    const csp::multiplayer::SpaceTransform translated { { 1.0f, 2.0f, 3.0f }, { 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f } };
    const csp::multiplayer::SpaceTransform rotated { { 0.0f, 0.0f, 0.0f }, ninetyDegAroundXNormalizedQuat, { 1.0f, 1.0f, 1.0f } };
    const csp::multiplayer::SpaceTransform scaled { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 1.0f }, { 2.0f, 3.0f, 4.0f } };

    EXPECT_EQ((identity * translated), translated);
    EXPECT_EQ((translated * identity), translated);
    EXPECT_EQ((identity * rotated), rotated);
    EXPECT_EQ((rotated * identity), rotated);
    EXPECT_EQ((identity * scaled), scaled);
    EXPECT_EQ((scaled * identity), scaled);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceTransformTests, MulitplicationTRSTest)
{
    const csp::multiplayer::SpaceTransform identity;
    // For mathy trig reasons, this is quaternion languge for "ninety degrees" (not really ... sort of)
    const float sqrtTwoOverTwo = std::sqrt(2.0f) / 2.0f;

    // An angle-axis rotation equivalent to a 90 degree rotation along the (1,1,0) axis.
    // This should produce a final (euler) rotation of approx 90, 45, 45
    const csp::common::Vector4 ninetyDegAroundXyAxisNormalizedQuat { 0.5f, 0.5f, 0.0f, sqrtTwoOverTwo };

    // Apply a complicated transform to the identity.
    const csp::multiplayer::SpaceTransform transformation { { 1.0f, 0.0f, 1.0f }, ninetyDegAroundXyAxisNormalizedQuat, { 2.0f, 1.0f, 4.0f } };

    const auto output = identity * transformation;
    EXPECT_EQ(output.Position, (csp::common::Vector3 { 1.0f, 0.0f, 1.0f }));

    glm::vec3 euler = glm::eulerAngles(glm::normalize(glm::quat { output.Rotation.W, output.Rotation.X, output.Rotation.Y, output.Rotation.Z }));

    const float epsilon = 1e-5f; // Matrix operations involving euler conversions arn't terribly stable at the floating point level cross platform.
    EXPECT_NEAR(euler.x, glm::radians(90.0f), epsilon); // 90 degrees
    EXPECT_NEAR(euler.y, glm::radians(45.0f), epsilon); // 45 degrees
    EXPECT_NEAR(euler.z, glm::radians(45.0f), epsilon); // 45 degrees

    EXPECT_EQ(output.Scale, (csp::common::Vector3 { 2.0f, 1.0f, 4.0f }));
}

CSP_PUBLIC_TEST(CSPEngine, SpaceTransformTests, MulitplicationTRSTestNonNormalQuat)
{
    const csp::multiplayer::SpaceTransform identity;
    // For mathy trig reasons, this is quaternion languge for "ninety degrees" (not really ... sort of)
    const float sqrtTwoOverTwo = std::sqrt(2.0f) / 2.0f;

    // An angle-axis rotation equivilent to a 90 degree rotation along the (1,1,0) axis.
    // This should produce a final (euler) rotation of approx 90, 45, 45
    // Scale by an arbitrary factor to make this non-normal, to test that the transformation code can handle that.
    const csp::common::Vector4 ninetyDegAroundXyAxisNonNormalizedQuat { 0.5f * 2.5f, 0.5f * 2.5f, 0.0f * 2.5f, sqrtTwoOverTwo * 2.5f };

    // Apply a complicated transform to the identity.
    const csp::multiplayer::SpaceTransform transformation { { 1.0f, 0.0f, 1.0f }, ninetyDegAroundXyAxisNonNormalizedQuat, { 2.0f, 1.0f, 4.0f } };

    const auto output = identity * transformation;
    EXPECT_EQ(output.Position, (csp::common::Vector3 { 1.0f, 0.0f, 1.0f }));

    glm::vec3 euler = glm::eulerAngles(glm::normalize(glm::quat { output.Rotation.W, output.Rotation.X, output.Rotation.Y, output.Rotation.Z }));

    const float epsilon = 1e-5f; // Matrix operations involving euler conversions arn't terribly stable at the floating point level cross platform.
    EXPECT_NEAR(euler.x, glm::radians(90.0f), epsilon); // 90 degrees
    EXPECT_NEAR(euler.y, glm::radians(45.0f), epsilon); // 45 degrees
    EXPECT_NEAR(euler.z, glm::radians(45.0f), epsilon); // 45 degrees

    EXPECT_EQ(output.Scale, (csp::common::Vector3 { 2.0f, 1.0f, 4.0f }));
}