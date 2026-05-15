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

#include "Multiplayer/NetworkEventSerialisation.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "Multiplayer/MCS/MCSTypes.h"
#include "TestHelpers.h"

#include <gtest/gtest.h>
#include <signalrclient/signalr_value.h>
#include <Debug/Logging.h>

using namespace csp::multiplayer;
using namespace csp::common;

namespace
{
constexpr uint64_t DataTypeString = static_cast<uint64_t>(mcs::ItemComponentDataType::STRING);
constexpr uint64_t DataTypeStringDictionary = static_cast<uint64_t>(mcs::ItemComponentDataType::STRING_DICTIONARY);
constexpr uint64_t DataTypeNullableBool = static_cast<uint64_t>(mcs::ItemComponentDataType::NULLABLE_BOOL);

signalr::value ConstructComponentElement(uint64_t typeId, const signalr::value& value)
{
    std::vector<signalr::value> signalRValue = { value };
    std::vector<signalr::value> componentElement = { signalr::value(typeId), signalr::value(signalRValue) };
    return signalr::value(componentElement);
};

std::vector<signalr::value> ConstructEventValues(const std::map<uint64_t, signalr::value>& components)
{
    std::vector<signalr::value> eventValues;
    eventValues.push_back(signalr::value(std::string("AsyncCallCompleted"))); // Event Type
    eventValues.push_back(signalr::value((uint64_t)123)); // SenderClientId
    eventValues.push_back(signalr::value(nullptr)); // RecipientClientId
    eventValues.push_back(signalr::value(components)); // Components map
    return eventValues;
}
}

CSP_INTERNAL_TEST(CSPEngine, NetworkEventSerialisationTests, DeserializeAsyncCallCompletedEventOldStructureTest)
{
    csp::common::LogSystem logSystem;

    const csp::common::String operationName("DuplicateSpace");
    const csp::common::String referenceId("new_space-abc-123");
    const csp::common::String referenceType("GroupId");

    // Components map:
    // 0: OperationName (ItemComponentDataType::STRING)
    // 1: ReferenceId (ItemComponentDataType::STRING)
    // 2: ReferenceType (ItemComponentDataType::STRING)
    std::map<uint64_t, signalr::value> components;

    components[0] = ConstructComponentElement(DataTypeString, signalr::value(operationName.c_str()));
    components[1] = ConstructComponentElement(DataTypeString, signalr::value(referenceId.c_str()));
    components[2] = ConstructComponentElement(DataTypeString, signalr::value(referenceType.c_str()));

    // Construct EventValues vector: [ EventName, SenderClientId, Recipient (null), Components map ]
    std::vector<signalr::value> eventValues = ConstructEventValues(components);

    csp::common::AsyncCallCompletedEventData parsed = DeserializeAsyncCallCompletedEvent(eventValues, logSystem);

    EXPECT_EQ(parsed.OperationName, operationName);
    EXPECT_EQ(parsed.ReferenceId, referenceId);
    EXPECT_EQ(parsed.ReferenceType, referenceType);
}

CSP_INTERNAL_TEST(CSPEngine, NetworkEventSerialisationTests, DeserializeAsyncCallCompletedEventNewStructureTest)
{
    csp::common::LogSystem logSystem;

    const csp::common::String operationName("DuplicateSpace");
    const csp::common::String originalSpaceId("orig_space-abc-123");
    const csp::common::String newSpaceId("new_space-abc-123");
    const csp::common::String statusReason("Success");

    // Components map for new structure:
    // 0: OperationName (ItemComponentDataType::STRING)
    // 1: References (ItemComponentDataType::STRING_DICTIONARY) - each entry is an ItemComponentData ([TypeId, [Field]])
    // 2: Success (ItemComponentDataType::NULLABLE_BOOL)
    // 3: StatusReason (ItemComponentDataType::STRING)
    std::map<uint64_t, signalr::value> components;

    // OperationName
    components[0] = ConstructComponentElement(DataTypeString, signalr::value(operationName.c_str()));

    // References
    std::map<std::string, signalr::value> refsMap;
    refsMap["SpaceId"] = ConstructComponentElement(DataTypeString, signalr::value(newSpaceId.c_str()));
    refsMap["OriginalSpaceId"] = ConstructComponentElement(DataTypeString, signalr::value(originalSpaceId.c_str()));
    components[1] = ConstructComponentElement(DataTypeStringDictionary, signalr::value(refsMap));

    // Success
    components[2] = ConstructComponentElement(DataTypeNullableBool, signalr::value(true));

    // StatusReason
    components[3] = ConstructComponentElement(DataTypeString, signalr::value(statusReason.c_str()));

    // Construct EventValues vector: [ EventName, SenderClientId, Recipient (null), Components(uint map) ]
    std::vector<signalr::value> eventValues = ConstructEventValues(components);

    csp::common::AsyncCallCompletedEventData parsed = DeserializeAsyncCallCompletedEvent(eventValues, logSystem);

    // Verify new event structure was parsed corrctly
    EXPECT_EQ(parsed.OperationName, operationName);
    EXPECT_TRUE(parsed.Success);
    EXPECT_EQ(parsed.StatusReason, statusReason);
    
    ASSERT_TRUE(parsed.References.HasKey("SpaceId"));
    ASSERT_TRUE(parsed.References.HasKey("OriginalSpaceId"));
    EXPECT_EQ(parsed.References["SpaceId"], newSpaceId);
    EXPECT_EQ(parsed.References["OriginalSpaceId"], originalSpaceId);

    // Check backwards compatibility
    // If the References map contains the key "SpaceId", the deserializer sets the old ReferenceId and ReferenceType properties
    EXPECT_EQ(parsed.ReferenceId, newSpaceId);
    EXPECT_EQ(parsed.ReferenceType, "GroupId"); // The key used to be "GroupId"
}
