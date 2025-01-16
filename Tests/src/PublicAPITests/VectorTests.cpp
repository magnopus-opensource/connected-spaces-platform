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
#include "TestHelpers.h"

#include "gtest/gtest.h"

using namespace csp::common;

#if RUN_ALL_UNIT_TESTS || RUN_VECTOR_TESTS
CSP_PUBLIC_TEST(CSPEngine, VectorTests, IsNearlyEqualTest)
{
    const Vector3 MyVector = { 0, 0, 0 };
    const Vector3 MyVector2 = { 0.0001f, 0, 0 };
    const Vector3 MyVector3 = { 0.002f, 0, 0 };

    EXPECT_TRUE(MyVector == MyVector2);

    EXPECT_TRUE(MyVector != MyVector3);

    const Vector4 MyVector4_A = { 0, 0, 0, 0 };
    const Vector4 MyVector4_B = { 0, 0, 0.0001f, 0 };
    const Vector4 MyVector4_C = { 0, 0, 0, 0.002f };

    EXPECT_TRUE(MyVector4_A == MyVector4_B);

    EXPECT_TRUE(MyVector4_A != MyVector4_C);
}
#endif