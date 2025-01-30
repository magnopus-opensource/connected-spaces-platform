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

#if RUN_ALL_UNIT_TESTS || RUN_SPACETRANSFORM_TESTS || RUN_SPACETRANSFORM_DEFAULT_CONSTRUCT_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceTransformTests, DefaultConstructTest)
{
    const csp::multiplayer::SpaceTransform SpaceTransform;
    EXPECT_EQ(SpaceTransform.Position, csp::common::Vector3::Zero());
    EXPECT_EQ(SpaceTransform.Rotation, csp::common::Vector4::Identity());
    EXPECT_EQ(SpaceTransform.Scale, csp::common::Vector3::One());
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACETRANSFORM_TESTS || RUN_SPACETRANSFORM_CONSTRUCT_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceTransformTests, ConstructTest)
{
    const csp::common::Vector3 Pos { 1.0f, 2.0f, 3.0f };
    const csp::common::Vector4 Rot { 15.0f, 35.0f, -10.0f, 1.0f };
    const csp::common::Vector3 Scale { 3.0f, 2.0f, 1.0f };

    const csp::multiplayer::SpaceTransform SpaceTransform(Pos, Rot, Scale);
    EXPECT_EQ(SpaceTransform.Position, Pos);
    EXPECT_EQ(SpaceTransform.Rotation, Rot);
    EXPECT_EQ(SpaceTransform.Scale, Scale);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACETRANSFORM_TESTS || RUN_SPACETRANSFORM_EQUALITY_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceTransformTests, EqualityTest)
{
    const csp::common::Vector3 Pos { 1.0f, 2.0f, 3.0f };
    const csp::common::Vector4 Rot { 15.0f, 35.0f, -10.0f, 1.0f };
    const csp::common::Vector3 Scale { 3.0f, 2.0f, 1.0f };

    const csp::multiplayer::SpaceTransform SpaceTransform1(Pos, Rot, Scale);
    const csp::multiplayer::SpaceTransform SpaceTransform2(Pos, Rot, Scale);
    const csp::multiplayer::SpaceTransform SpaceTransformIdentity;

    EXPECT_EQ(SpaceTransform1, SpaceTransform2);
    EXPECT_NE(SpaceTransform1, SpaceTransformIdentity);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACETRANSFORM_TESTS || RUN_SPACETRANSFORM_MULTIPLICATION_IDENTITY_AXIOM_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceTransformTests, MulitplicationIdentityAxiomTest)
{
    // Two identity matrices should be identity
    const csp::multiplayer::SpaceTransform Identity;
    EXPECT_EQ((Identity * Identity), Identity);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACETRANSFORM_TESTS || RUN_SPACETRANSFORM_MULTIPLICATION_IDENTITY_TRANSFORM_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceTransformTests, MulitplicationIdentityTransformTest)
{
    const csp::multiplayer::SpaceTransform Identity;
    // For mathy trig reasons, this is quaternion languge for "ninety degrees" (not really ... sort of)
    const float SqrtTwoOverTwo = std::sqrt(2.0f) / 2.0f;
    const csp::common::Vector4 NinetyDegAroundXNormalizedQuat { SqrtTwoOverTwo, 0.0f, 0.0f, SqrtTwoOverTwo };

    // An identity matrix multiplied into another matrix (in either order) should just be the other matrix
    const csp::multiplayer::SpaceTransform Translated { { 1.0f, 2.0f, 3.0f }, { 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f } };
    const csp::multiplayer::SpaceTransform Rotated { { 0.0f, 0.0f, 0.0f }, NinetyDegAroundXNormalizedQuat, { 1.0f, 1.0f, 1.0f } };
    const csp::multiplayer::SpaceTransform Scaled { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 1.0f }, { 2.0f, 3.0f, 4.0f } };

    EXPECT_EQ((Identity * Translated), Translated);
    EXPECT_EQ((Translated * Identity), Translated);
    EXPECT_EQ((Identity * Rotated), Rotated);
    EXPECT_EQ((Rotated * Identity), Rotated);
    EXPECT_EQ((Identity * Scaled), Scaled);
    EXPECT_EQ((Scaled * Identity), Scaled);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACETRANSFORM_TESTS || RUN_SPACETRANSFORM_MULTIPLICATION_TRS_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceTransformTests, MulitplicationTRSTest)
{
    const csp::multiplayer::SpaceTransform Identity;
    // For mathy trig reasons, this is quaternion languge for "ninety degrees" (not really ... sort of)
    const float SqrtTwoOverTwo = std::sqrt(2.0f) / 2.0f;

    // An angle-axis rotation equivalent to a 90 degree rotation along the (1,1,0) axis.
    // This should produce a final (euler) rotation of approx 90, 45, 45
    const csp::common::Vector4 NinetyDegAroundXYAxisNormalizedQuat { 0.5f, 0.5f, 0.0f, SqrtTwoOverTwo };

    // Apply a complicated transform to the identity.
    const csp::multiplayer::SpaceTransform Transformation { { 1.0f, 0.0f, 1.0f }, NinetyDegAroundXYAxisNormalizedQuat, { 2.0f, 1.0f, 4.0f } };

    const auto Output = Identity * Transformation;
    EXPECT_EQ(Output.Position, (csp::common::Vector3 { 1.0f, 0.0f, 1.0f }));

    glm::vec3 Euler = glm::eulerAngles(glm::normalize(glm::quat { Output.Rotation.W, Output.Rotation.X, Output.Rotation.Y, Output.Rotation.Z }));

    const float epsilon = 1e-5f; // Matrix operations involving euler conversions arn't terribly stable at the floating point level cross platform.
    EXPECT_NEAR(Euler.x, glm::radians(90.0f), epsilon); // 90 degrees
    EXPECT_NEAR(Euler.y, glm::radians(45.0f), epsilon); // 45 degrees
    EXPECT_NEAR(Euler.z, glm::radians(45.0f), epsilon); // 45 degrees

    EXPECT_EQ(Output.Scale, (csp::common::Vector3 { 2.0f, 1.0f, 4.0f }));
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_SPACETRANSFORM_TESTS || RUN_SPACETRANSFORM_MULTIPLICATION_NON_NORMAL_QUAT_TRS_TEST
CSP_PUBLIC_TEST(CSPEngine, SpaceTransformTests, MulitplicationTRSTestNonNormalQuat)
{
    const csp::multiplayer::SpaceTransform Identity;
    // For mathy trig reasons, this is quaternion languge for "ninety degrees" (not really ... sort of)
    const float SqrtTwoOverTwo = std::sqrt(2.0f) / 2.0f;

    // An angle-axis rotation equivilent to a 90 degree rotation along the (1,1,0) axis.
    // This should produce a final (euler) rotation of approx 90, 45, 45
    // Scale by an arbitrary factor to make this non-normal, to test that the transformation code can handle that.
    const csp::common::Vector4 NinetyDegAroundXYAxisNonNormalizedQuat { 0.5f * 2.5f, 0.5f * 2.5f, 0.0f * 2.5f, SqrtTwoOverTwo * 2.5f };

    // Apply a complicated transform to the identity.
    const csp::multiplayer::SpaceTransform Transformation { { 1.0f, 0.0f, 1.0f }, NinetyDegAroundXYAxisNonNormalizedQuat, { 2.0f, 1.0f, 4.0f } };

    const auto Output = Identity * Transformation;
    EXPECT_EQ(Output.Position, (csp::common::Vector3 { 1.0f, 0.0f, 1.0f }));

    glm::vec3 Euler = glm::eulerAngles(glm::normalize(glm::quat { Output.Rotation.W, Output.Rotation.X, Output.Rotation.Y, Output.Rotation.Z }));

    const float epsilon = 1e-5f; // Matrix operations involving euler conversions arn't terribly stable at the floating point level cross platform.
    EXPECT_NEAR(Euler.x, glm::radians(90.0f), epsilon); // 90 degrees
    EXPECT_NEAR(Euler.y, glm::radians(45.0f), epsilon); // 45 degrees
    EXPECT_NEAR(Euler.z, glm::radians(45.0f), epsilon); // 45 degrees

    EXPECT_EQ(Output.Scale, (csp::common::Vector3 { 2.0f, 1.0f, 4.0f }));
}
#endif