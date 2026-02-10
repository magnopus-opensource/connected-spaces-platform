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

#include "../SpaceSystemTestHelpers.h"
#include "../UserSystemTestHelpers.h"
#include "Awaitable.h"
#include "CSP/CSPFoundation.h"
#include "CSP/Multiplayer/ComponentExtensions.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/SystemsManager.h"
#include "TestHelpers.h"

#include "gtest/gtest.h"
#include <chrono>
#include <filesystem>
#include <thread>

namespace
{

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

CSP_PUBLIC_TEST(CSPEngine, ComponentExtensionsTests, WithoutExtendedComponent)
{
    // An extension with no component to extend should not cause crashes when attempting to set or get properties, but should log errors and return
    // invalid values.
	csp::multiplayer::ComponentExtensions Extensions;
    
    // Test that setting a property on an extension with a null ExtendedComponent does not throw, but logs an error.
    testing::internal::CaptureStderr();
    EXPECT_NO_THROW(Extensions.SetProperty("MyExtensionProperty", true));
    
    {
        const std::string ExpectedError(
            "Attempted to set a property from a component extension that has a null ExtendedComponent. This indicates a logical "
            "error during component initialization.");

        const std::string OutStdErr = testing::internal::GetCapturedStderr();
        EXPECT_NE(OutStdErr.find(ExpectedError), std::string::npos);
    }

    // Test that getting a property on an extension with a null ExtendedComponent logs an error and returns an invalid
    // ReplicatedValue.
    testing::internal::CaptureStderr();
    const csp::common::ReplicatedValue ExtensionProperty = Extensions.GetProperty("MyExtensionProperty");
    EXPECT_EQ(ExtensionProperty.GetReplicatedValueType(), csp::common::ReplicatedValueType::InvalidType);
    
    {
        const std::string ExpectedError(
            "Attempted to get a property from a component extension that has a null ExtendedComponent. This indicates a logical "
            "error during component initialization.");

        const std::string OutStdErr = testing::internal::GetCapturedStderr();
        EXPECT_NE(OutStdErr.find(ExpectedError), std::string::npos);
    }

    EXPECT_EQ(Extensions.HasProperty("MyExtensionProperty"), false);
}

CSP_PUBLIC_TEST(CSPEngine, ComponentExtensionsTests, WithExtendedComponent)
{ 
    csp::common::LogSystem* LogSystem = csp::systems::SystemsManager::Get().GetLogSystem();
    csp::multiplayer::SpaceEntity Entity;
    csp::multiplayer::ComponentBase Component(csp::multiplayer::ComponentType::Core, LogSystem, &Entity);

    // Creating an extension with a valid component; should allow properties to be set and retrieved successfully.
    csp::multiplayer::ComponentExtensions Extensions(&Component);
    Extensions.SetProperty("MyExtensionProperty", true);

    // Test that we can retrieve the property we just set, and that it has the expected value.
    const csp::common::ReplicatedValue ExtensionProperty = Extensions.GetProperty("MyExtensionProperty");
    EXPECT_EQ(ExtensionProperty.GetReplicatedValueType(), csp::common::ReplicatedValueType::Boolean);
    EXPECT_EQ(ExtensionProperty, true);
}