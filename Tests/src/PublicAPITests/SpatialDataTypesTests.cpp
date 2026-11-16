/*
 * Copyright 2026 Magnopus LLC

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

#include "CSP/Systems/Spatial/SpatialDataTypes.h"
#include "TestHelpers.h"

#include "gtest/gtest.h"

using namespace csp::systems;

CSP_PUBLIC_TEST(CSPEngine, SpatialDataTypesTests, GeoLocationValidTest)
{
    // Valid locations

    GeoLocation ValidLocation1(0.0, 0.0);
    EXPECT_TRUE(ValidLocation1.IsValid());

    GeoLocation ValidLocation2(160.0, 45.0);
    EXPECT_TRUE(ValidLocation2.IsValid());

    GeoLocation ValidLocation3(170.0, -70.0);
    EXPECT_TRUE(ValidLocation3.IsValid());

    GeoLocation ValidLocation4(-150.0, -80.0);
    EXPECT_TRUE(ValidLocation4.IsValid());

    GeoLocation ValidLocation5(-150.0, 70.0);
    EXPECT_TRUE(ValidLocation5.IsValid());

    // Valid boundary locations

    GeoLocation ValidLocation6(180.0, 90.0);
    EXPECT_TRUE(ValidLocation6.IsValid());

    GeoLocation ValidLocation7(-180.0, -90.0);
    EXPECT_TRUE(ValidLocation7.IsValid());

    // Invalid locations

    GeoLocation InvalidLocation1(120.0, -100.0);
    EXPECT_FALSE(InvalidLocation1.IsValid());

    GeoLocation InvalidLocation2(-170.0, 95.0);
    EXPECT_FALSE(InvalidLocation2.IsValid());

    GeoLocation InvalidLocation3(190.0, -60.0);
    EXPECT_FALSE(InvalidLocation3.IsValid());

    GeoLocation InvalidLocation4(-200.0, 85.0);
    EXPECT_FALSE(InvalidLocation4.IsValid());
}
