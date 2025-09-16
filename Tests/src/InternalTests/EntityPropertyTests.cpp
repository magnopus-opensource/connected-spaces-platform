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
    uint16_t TestKey = 10;
    SpaceEntityUpdateFlags TestUpdateFlag = SpaceEntityUpdateFlags::UPDATE_FLAGS_COMPONENTS;
    std::function<csp::common::ReplicatedValue()> TestToReplicatedValue;
    std::function<void(const csp::common::ReplicatedValue&)> TestFromReplicatedValue;

    EntityProperty Property { TestKey, TestUpdateFlag, TestToReplicatedValue, TestFromReplicatedValue };

    EXPECT_EQ(TestKey, Property.GetKey());
    EXPECT_EQ(TestUpdateFlag, Property.GetUpdateFlag());
}

// Ensures callbacks are called from Get/Set functions.
CSP_INTERNAL_TEST(CSPEngine, EntityPropertyTests, PropertyCallbackTest)
{
    uint16_t TestKey = 10;
    SpaceEntityUpdateFlags TestUpdateFlag = SpaceEntityUpdateFlags::UPDATE_FLAGS_COMPONENTS;

    ::testing::MockFunction<csp::common::ReplicatedValue()> MockToReplicatedValue;
    std::function<csp::common::ReplicatedValue()> TestToReplicatedValue = MockToReplicatedValue.AsStdFunction();

    ::testing::MockFunction<void(const csp::common::ReplicatedValue&)> MockFromReplicatedValue;
    std::function<void(const csp::common::ReplicatedValue&)> TestFromReplicatedValue = MockFromReplicatedValue.AsStdFunction();

    EntityProperty Property { TestKey, TestUpdateFlag, TestToReplicatedValue, TestFromReplicatedValue };

    EXPECT_CALL(MockToReplicatedValue, Call()).Times(1);
    EXPECT_CALL(MockFromReplicatedValue, Call(csp::common::ReplicatedValue { 0ll })).Times(1);

    Property.Get();
    Property.Set(csp::common::ReplicatedValue { 0ll });
}

// Ensures the Set function correctly sets the value via the callback and Get returns the updated value.
CSP_INTERNAL_TEST(CSPEngine, EntityPropertyTests, PropertySetGetTest)
{
    uint16_t TestKey = 10;
    SpaceEntityUpdateFlags TestUpdateFlag = SpaceEntityUpdateFlags::UPDATE_FLAGS_COMPONENTS;

    int64_t TestValue = 0;

    auto TestToReplicatedValue = [&TestValue]() { return csp::common::ReplicatedValue { TestValue }; };
    auto TestFromReplicatedValue = [&TestValue](const csp::common::ReplicatedValue& Value) { TestValue = Value.GetInt(); };

    EntityProperty Property { TestKey, TestUpdateFlag, TestToReplicatedValue, TestFromReplicatedValue };

    if (Property.Get().GetReplicatedValueType() != csp::common::ReplicatedValueType::Integer)
    {
        FAIL();
    }

    EXPECT_EQ(TestValue, 0);
    EXPECT_EQ(Property.Get().GetInt(), TestValue);

    Property.Set(csp::common::ReplicatedValue { 100ll });

    EXPECT_EQ(TestValue, 100);
    EXPECT_EQ(Property.Get().GetInt(), TestValue);
}