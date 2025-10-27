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

#include "TestHelpers.h"
#include "gtest/gtest.h"

#include "RAIIMockLogger.h"

CSP_INTERNAL_TEST(CSPEngine, FeatureFlagTests, DefaultFeatureFlagTest)
{
    csp::CSPFoundation::__ResetFeatureFlagsForTesting();

    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    const csp::EFeatureFlag TEST_FLAG_A = static_cast<csp::EFeatureFlag>(9001);
    const csp::EFeatureFlag TEST_FLAG_B = static_cast<csp::EFeatureFlag>(9002);
    const csp::EFeatureFlag TEST_FLAG_C = static_cast<csp::EFeatureFlag>(9003);

    csp::CSPFoundation::__AddFeatureFlagForTesting(TEST_FLAG_A, false, "Description for Test Flag A - initialized: false");
    csp::CSPFoundation::__AddFeatureFlagForTesting(TEST_FLAG_B, true, "Description for Test Flag B - initialized: true");
    csp::CSPFoundation::__AddFeatureFlagForTesting(TEST_FLAG_C, false, "Description for Test Flag C - initialized: false");

    ASSERT_FALSE(csp::CSPFoundation::IsCSPFeatureEnabled(TEST_FLAG_A));
    ASSERT_TRUE(csp::CSPFoundation::IsCSPFeatureEnabled(TEST_FLAG_B));
    ASSERT_FALSE(csp::CSPFoundation::IsCSPFeatureEnabled(TEST_FLAG_C));

    csp::CSPFoundation::Shutdown();
}

CSP_INTERNAL_TEST(CSPEngine, FeatureFlagTests, CreateFeatureFlagTest)
{
    csp::CSPFoundation::__ResetFeatureFlagsForTesting();

    const csp::EFeatureFlag TEST_FLAG_A = static_cast<csp::EFeatureFlag>(9001);
    const csp::EFeatureFlag TEST_FLAG_B = static_cast<csp::EFeatureFlag>(9002);

    const csp::common::String FlagDescriptionA = "Description for Test Flag A - initialized: false";
    const csp::common::String FlagDescriptionB = "Description for Test Flag B - initialized: false";

    csp::CSPFoundation::__AddFeatureFlagForTesting(TEST_FLAG_A, false, FlagDescriptionA);
    csp::CSPFoundation::__AddFeatureFlagForTesting(TEST_FLAG_B, false, FlagDescriptionB);

    csp::common::Array<csp::FeatureFlag> FeatureFlags = { { TEST_FLAG_A, true }, { TEST_FLAG_B, false } };

    InitialiseFoundationWithUserAgentInfoAndFeatureFlags(EndpointBaseURI(), FeatureFlags);

    ASSERT_TRUE(csp::CSPFoundation::IsCSPFeatureEnabled(TEST_FLAG_A));
    ASSERT_FALSE(csp::CSPFoundation::IsCSPFeatureEnabled(TEST_FLAG_B));

    ASSERT_EQ(csp::CSPFoundation::GetCSPFeatureFlagDescription(TEST_FLAG_A), FlagDescriptionA);
    ASSERT_EQ(csp::CSPFoundation::GetCSPFeatureFlagDescription(TEST_FLAG_B), FlagDescriptionB);

    csp::CSPFoundation::Shutdown();
}

CSP_INTERNAL_TEST(CSPEngine, FeatureFlagTests, NoFeatureFlagsSpecifiedTest)
{
    csp::CSPFoundation::__ResetFeatureFlagsForTesting();

    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    ASSERT_FALSE(csp::CSPFoundation::IsCSPFeatureEnabled(csp::EFeatureFlag::Invalid));

    csp::CSPFoundation::Shutdown();
}

CSP_INTERNAL_TEST(CSPEngine, FeatureFlagTests, UnkownFeatureFlagIsEnabledTest)
{
    csp::CSPFoundation::__ResetFeatureFlagsForTesting();

    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    {
        RAIIMockLogger MockLogger {};

        // Ensure the required 'unknown feature flag' error message is logged when we try to check the enabled state of an invlaid flag
        const csp::common::String IsEnabledErrorMsg = "Unknown feature flag queried with integer value: 9999";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Warning, IsEnabledErrorMsg)).Times(1);

        const csp::EFeatureFlag UNKNOWN_TEST_FLAG = static_cast<csp::EFeatureFlag>(9999);

        ASSERT_FALSE(csp::CSPFoundation::IsCSPFeatureEnabled(UNKNOWN_TEST_FLAG));
    }

    csp::CSPFoundation::Shutdown();
}

CSP_INTERNAL_TEST(CSPEngine, FeatureFlagTests, GetFeatureFlagDescriptionTest)
{
    csp::CSPFoundation::__ResetFeatureFlagsForTesting();

    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    const csp::EFeatureFlag TEST_FLAG_A = static_cast<csp::EFeatureFlag>(9001);

    const csp::common::String FlagDescription = "Description for Test Flag A - initialized: false";

    csp::CSPFoundation::__AddFeatureFlagForTesting(TEST_FLAG_A, false, FlagDescription);

    ASSERT_EQ(csp::CSPFoundation::GetCSPFeatureFlagDescription(TEST_FLAG_A), FlagDescription);

    csp::CSPFoundation::Shutdown();
}

CSP_INTERNAL_TEST(CSPEngine, FeatureFlagTests, UnkownFeatureFlagDescriptionTest)
{
    csp::CSPFoundation::__ResetFeatureFlagsForTesting();

    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    {
        RAIIMockLogger MockLogger {};

        // Ensure the required 'unknown feature flag' error message is logged when we try to get a description for an invlaid flag
        const csp::common::String IsEnabledErrorMsg = "Unknown feature flag description requested with integer value: 9999";
        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Warning, IsEnabledErrorMsg)).Times(1);

        const csp::EFeatureFlag UNKNOWN_TEST_FLAG = static_cast<csp::EFeatureFlag>(9999);

        ASSERT_EQ(csp::CSPFoundation::GetCSPFeatureFlagDescription(UNKNOWN_TEST_FLAG), "");
    }

    csp::CSPFoundation::Shutdown();
}

CSP_INTERNAL_TEST(CSPEngine, FeatureFlagTests, GetFeatureFlagsTest)
{
    csp::CSPFoundation::__ResetFeatureFlagsForTesting();

    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto CurrentFeatureFlags = csp::CSPFoundation::GetFeatureFlags();

    ASSERT_EQ(CurrentFeatureFlags.Size(), 0);

    const csp::EFeatureFlag TEST_FLAG_A = static_cast<csp::EFeatureFlag>(9001);
    const csp::EFeatureFlag TEST_FLAG_B = static_cast<csp::EFeatureFlag>(9002);
    const csp::EFeatureFlag TEST_FLAG_C = static_cast<csp::EFeatureFlag>(9003);

    const csp::common::String FlagDescriptionA = "Description for Test Flag A - initialized: false";
    const csp::common::String FlagDescriptionB = "Description for Test Flag B - initialized: true";
    const csp::common::String FlagDescriptionC = "Description for Test Flag C - initialized: false";

    csp::CSPFoundation::__AddFeatureFlagForTesting(TEST_FLAG_A, false, FlagDescriptionA);
    csp::CSPFoundation::__AddFeatureFlagForTesting(TEST_FLAG_B, true, FlagDescriptionB);
    csp::CSPFoundation::__AddFeatureFlagForTesting(TEST_FLAG_C, false, FlagDescriptionC);

    auto UpdatedFeatureFlags = csp::CSPFoundation::GetFeatureFlags();

    ASSERT_EQ(UpdatedFeatureFlags.Size(), 3);

    ASSERT_EQ(UpdatedFeatureFlags[0].Type, TEST_FLAG_A);
    ASSERT_EQ(UpdatedFeatureFlags[0].Enabled, false);
    ASSERT_EQ(UpdatedFeatureFlags[0].Description, FlagDescriptionA);

    ASSERT_EQ(UpdatedFeatureFlags[1].Type, TEST_FLAG_B);
    ASSERT_EQ(UpdatedFeatureFlags[1].Enabled, true);
    ASSERT_EQ(UpdatedFeatureFlags[1].Description, FlagDescriptionB);

    ASSERT_EQ(UpdatedFeatureFlags[2].Type, TEST_FLAG_C);
    ASSERT_EQ(UpdatedFeatureFlags[2].Enabled, false);
    ASSERT_EQ(UpdatedFeatureFlags[2].Description, FlagDescriptionC);

    csp::CSPFoundation::Shutdown();
}
