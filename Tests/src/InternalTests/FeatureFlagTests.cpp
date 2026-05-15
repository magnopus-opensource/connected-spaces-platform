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

    const csp::EFeatureFlag testFlagA = static_cast<csp::EFeatureFlag>(9001);
    const csp::EFeatureFlag testFlagB = static_cast<csp::EFeatureFlag>(9002);
    const csp::EFeatureFlag testFlagC = static_cast<csp::EFeatureFlag>(9003);

    csp::CSPFoundation::__AddFeatureFlagForTesting(testFlagA, false, "Description for Test Flag A - initialized: false");
    csp::CSPFoundation::__AddFeatureFlagForTesting(testFlagB, true, "Description for Test Flag B - initialized: true");
    csp::CSPFoundation::__AddFeatureFlagForTesting(testFlagC, false, "Description for Test Flag C - initialized: false");

    ASSERT_FALSE(csp::CSPFoundation::IsFeatureEnabled(testFlagA));
    ASSERT_TRUE(csp::CSPFoundation::IsFeatureEnabled(testFlagB));
    ASSERT_FALSE(csp::CSPFoundation::IsFeatureEnabled(testFlagC));

    csp::CSPFoundation::Shutdown();
}

CSP_INTERNAL_TEST(CSPEngine, FeatureFlagTests, CreateFeatureFlagTest)
{
    csp::CSPFoundation::__ResetFeatureFlagsForTesting();

    const csp::EFeatureFlag testFlagA = static_cast<csp::EFeatureFlag>(9001);
    const csp::EFeatureFlag testFlagB = static_cast<csp::EFeatureFlag>(9002);

    const csp::common::String flagDescriptionA = "Description for Test Flag A - initialized: false";
    const csp::common::String flagDescriptionB = "Description for Test Flag B - initialized: false";

    csp::CSPFoundation::__AddFeatureFlagForTesting(testFlagA, false, flagDescriptionA);
    csp::CSPFoundation::__AddFeatureFlagForTesting(testFlagB, false, flagDescriptionB);

    csp::common::Array<csp::FeatureFlag> featureFlags = { { testFlagA, true }, { testFlagB, false } };

    InitialiseFoundationWithUserAgentInfoAndFeatureFlags(EndpointBaseURI(), featureFlags);

    ASSERT_TRUE(csp::CSPFoundation::IsFeatureEnabled(testFlagA));
    ASSERT_FALSE(csp::CSPFoundation::IsFeatureEnabled(testFlagB));

    ASSERT_EQ(csp::CSPFoundation::GetFeatureFlagDescription(testFlagA), flagDescriptionA);
    ASSERT_EQ(csp::CSPFoundation::GetFeatureFlagDescription(testFlagB), flagDescriptionB);

    csp::CSPFoundation::Shutdown();
}

CSP_INTERNAL_TEST(CSPEngine, FeatureFlagTests, NoFeatureFlagsSpecifiedTest)
{
    csp::CSPFoundation::__ResetFeatureFlagsForTesting();

    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    ASSERT_FALSE(csp::CSPFoundation::IsFeatureEnabled(csp::EFeatureFlag::Invalid));

    csp::CSPFoundation::Shutdown();
}

CSP_INTERNAL_TEST(CSPEngine, FeatureFlagTests, UnkownFeatureFlagIsEnabledTest)
{
    csp::CSPFoundation::__ResetFeatureFlagsForTesting();

    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    {
        RAIIMockLogger mockLogger {};

        // Ensure the required 'unknown feature flag' error message is logged when we try to check the enabled state of an invlaid flag
        const csp::common::String isEnabledErrorMsg = "Unknown feature flag queried with integer value: 9999";
        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Warning, isEnabledErrorMsg)).Times(1);

        const csp::EFeatureFlag unknownTestFlag = static_cast<csp::EFeatureFlag>(9999);

        ASSERT_FALSE(csp::CSPFoundation::IsFeatureEnabled(unknownTestFlag));
    }

    csp::CSPFoundation::Shutdown();
}

CSP_INTERNAL_TEST(CSPEngine, FeatureFlagTests, GetFeatureFlagDescriptionTest)
{
    csp::CSPFoundation::__ResetFeatureFlagsForTesting();

    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    const csp::EFeatureFlag testFlagA = static_cast<csp::EFeatureFlag>(9001);

    const csp::common::String flagDescription = "Description for Test Flag A - initialized: false";

    csp::CSPFoundation::__AddFeatureFlagForTesting(testFlagA, false, flagDescription);

    ASSERT_EQ(csp::CSPFoundation::GetFeatureFlagDescription(testFlagA), flagDescription);

    csp::CSPFoundation::Shutdown();
}

CSP_INTERNAL_TEST(CSPEngine, FeatureFlagTests, UnkownFeatureFlagDescriptionTest)
{
    csp::CSPFoundation::__ResetFeatureFlagsForTesting();

    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    {
        RAIIMockLogger mockLogger {};

        // Ensure the required 'unknown feature flag' error message is logged when we try to get a description for an invlaid flag
        const csp::common::String isEnabledErrorMsg = "Unknown feature flag description requested with integer value: 9999";
        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Warning, isEnabledErrorMsg)).Times(1);

        const csp::EFeatureFlag unknownTestFlag = static_cast<csp::EFeatureFlag>(9999);

        ASSERT_EQ(csp::CSPFoundation::GetFeatureFlagDescription(unknownTestFlag), "");
    }

    csp::CSPFoundation::Shutdown();
}

CSP_INTERNAL_TEST(CSPEngine, FeatureFlagTests, GetFeatureFlagsTest)
{
    csp::CSPFoundation::__ResetFeatureFlagsForTesting();

    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto currentFeatureFlags = csp::CSPFoundation::GetFeatureFlags();

    ASSERT_EQ(currentFeatureFlags.Size(), 0);

    const csp::EFeatureFlag testFlagA = static_cast<csp::EFeatureFlag>(9001);
    const csp::EFeatureFlag testFlagB = static_cast<csp::EFeatureFlag>(9002);
    const csp::EFeatureFlag testFlagC = static_cast<csp::EFeatureFlag>(9003);

    const csp::common::String flagDescriptionA = "Description for Test Flag A - initialized: false";
    const csp::common::String flagDescriptionB = "Description for Test Flag B - initialized: true";
    const csp::common::String flagDescriptionC = "Description for Test Flag C - initialized: false";

    csp::CSPFoundation::__AddFeatureFlagForTesting(testFlagA, false, flagDescriptionA);
    csp::CSPFoundation::__AddFeatureFlagForTesting(testFlagB, true, flagDescriptionB);
    csp::CSPFoundation::__AddFeatureFlagForTesting(testFlagC, false, flagDescriptionC);

    auto updatedFeatureFlags = csp::CSPFoundation::GetFeatureFlags();

    ASSERT_EQ(updatedFeatureFlags.Size(), 3);

    ASSERT_EQ(updatedFeatureFlags[0].Type, testFlagA);
    ASSERT_EQ(updatedFeatureFlags[0].Enabled, false);
    ASSERT_EQ(updatedFeatureFlags[0].GetDescription(), flagDescriptionA);

    ASSERT_EQ(updatedFeatureFlags[1].Type, testFlagB);
    ASSERT_EQ(updatedFeatureFlags[1].Enabled, true);
    ASSERT_EQ(updatedFeatureFlags[1].GetDescription(), flagDescriptionB);

    ASSERT_EQ(updatedFeatureFlags[2].Type, testFlagC);
    ASSERT_EQ(updatedFeatureFlags[2].Enabled, false);
    ASSERT_EQ(updatedFeatureFlags[2].GetDescription(), flagDescriptionC);

    csp::CSPFoundation::Shutdown();
}
