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

#include "Multiplayer/SpaceEntityStatePatcher.h"
#include "TestHelpers.h"

#include "gtest/gtest.h"

using namespace csp::multiplayer;

// Ensures constructor arguments are set correctly.
CSP_INTERNAL_TEST(CSPEngine, EntityPropertyTests, PropertyConstructorTest)
{
    SpaceEntityComponentKey testKey = SpaceEntityComponentKey::Name;
    SpaceEntityUpdateFlags testUpdateFlag = SpaceEntityUpdateFlags::UPDATE_FLAGS_COMPONENTS;
    std::function<csp::common::ReplicatedValue()> testToReplicatedValue;
    std::function<void(const csp::common::ReplicatedValue&)> testFromReplicatedValue;

    EntityProperty property { testKey, testUpdateFlag, testToReplicatedValue, testFromReplicatedValue };

    EXPECT_EQ(testKey, property.GetKey());
    EXPECT_EQ(testUpdateFlag, property.GetUpdateFlag());
}

// Ensures callbacks are called from Get/Set functions.
CSP_INTERNAL_TEST(CSPEngine, EntityPropertyTests, PropertyCallbackTest)
{
    SpaceEntityComponentKey testKey = SpaceEntityComponentKey::Name;
    SpaceEntityUpdateFlags testUpdateFlag = SpaceEntityUpdateFlags::UPDATE_FLAGS_COMPONENTS;

    ::testing::MockFunction<csp::common::ReplicatedValue()> mockToReplicatedValue;
    std::function<csp::common::ReplicatedValue()> testToReplicatedValue = mockToReplicatedValue.AsStdFunction();

    ::testing::MockFunction<void(const csp::common::ReplicatedValue&)> mockFromReplicatedValue;
    std::function<void(const csp::common::ReplicatedValue&)> testFromReplicatedValue = mockFromReplicatedValue.AsStdFunction();

    EntityProperty property { testKey, testUpdateFlag, testToReplicatedValue, testFromReplicatedValue };

    EXPECT_CALL(mockToReplicatedValue, Call()).Times(1);
    EXPECT_CALL(mockFromReplicatedValue, Call(csp::common::ReplicatedValue { static_cast<int64_t>(0ll) })).Times(1);

    property.Get();
    property.Set(csp::common::ReplicatedValue { static_cast<int64_t>(0ll) });
}

// Ensures the Set function correctly sets the value via the callback and Get returns the updated value.
CSP_INTERNAL_TEST(CSPEngine, EntityPropertyTests, PropertySetGetTest)
{
    SpaceEntityComponentKey testKey = SpaceEntityComponentKey::Name;
    SpaceEntityUpdateFlags testUpdateFlag = SpaceEntityUpdateFlags::UPDATE_FLAGS_COMPONENTS;

    int64_t testValue = 0;

    auto testToReplicatedValue = [&testValue]() { return csp::common::ReplicatedValue { testValue }; };
    auto testFromReplicatedValue = [&testValue](const csp::common::ReplicatedValue& value) { testValue = value.GetInt(); };

    EntityProperty property { testKey, testUpdateFlag, testToReplicatedValue, testFromReplicatedValue };

    if (property.Get().GetReplicatedValueType() != csp::common::ReplicatedValueType::Integer)
    {
        FAIL();
    }

    EXPECT_EQ(testValue, 0);
    EXPECT_EQ(property.Get().GetInt(), testValue);

    property.Set(csp::common::ReplicatedValue { static_cast<int64_t>(100ll) });

    EXPECT_EQ(testValue, 100);
    EXPECT_EQ(property.Get().GetInt(), testValue);
}