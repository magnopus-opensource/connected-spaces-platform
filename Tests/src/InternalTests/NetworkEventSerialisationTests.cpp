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

signalr::value ConstructComponentElement(uint64_t TypeId, const signalr::value& Value)
{
    std::vector<signalr::value> SignalRValue = { Value };
    std::vector<signalr::value> ComponentElement = { signalr::value(TypeId), signalr::value(SignalRValue) };
    return signalr::value(ComponentElement);
};

std::vector<signalr::value> ConstructEventValues(const std::map<uint64_t, signalr::value>& Components)
{
    std::vector<signalr::value> EventValues;
    EventValues.push_back(signalr::value(std::string("AsyncCallCompleted"))); // Event Type
    EventValues.push_back(signalr::value((uint64_t)123)); // SenderClientId
    EventValues.push_back(signalr::value(nullptr)); // RecipientClientId
    EventValues.push_back(signalr::value(Components)); // Components map
    return EventValues;
}
}

CSP_INTERNAL_TEST(CSPEngine, NetworkEventSerialisationTests, DeserializeAsyncCallCompletedEventOldStructureTest)
{
    csp::common::LogSystem LogSystem;

    const csp::common::String OperationName("DuplicateSpace");
    const csp::common::String ReferenceId("new_space-abc-123");
    const csp::common::String ReferenceType("GroupId");

    // Components map:
    // 0: OperationName (ItemComponentDataType::STRING)
    // 1: ReferenceId (ItemComponentDataType::STRING)
    // 2: ReferenceType (ItemComponentDataType::STRING)
    std::map<uint64_t, signalr::value> Components;

    Components[0] = ConstructComponentElement(DataTypeString, signalr::value(OperationName.c_str()));
    Components[1] = ConstructComponentElement(DataTypeString, signalr::value(ReferenceId.c_str()));
    Components[2] = ConstructComponentElement(DataTypeString, signalr::value(ReferenceType.c_str()));

    // Construct EventValues vector: [ EventName, SenderClientId, Recipient (null), Components map ]
    std::vector<signalr::value> EventValues = ConstructEventValues(Components);

    csp::common::AsyncCallCompletedEventData Parsed = DeserializeAsyncCallCompletedEvent(EventValues, LogSystem);

    EXPECT_EQ(Parsed.OperationName, OperationName);
    EXPECT_EQ(Parsed.ReferenceId, ReferenceId);
    EXPECT_EQ(Parsed.ReferenceType, ReferenceType);
}

CSP_INTERNAL_TEST(CSPEngine, NetworkEventSerialisationTests, DeserializeAsyncCallCompletedEventNewStructureTest)
{
    csp::common::LogSystem LogSystem;

    const csp::common::String OperationName("DuplicateSpace");
    const csp::common::String OriginalSpaceId("orig_space-abc-123");
    const csp::common::String NewSpaceId("new_space-abc-123");
    const csp::common::String StatusReason("Success");

    // Components map for new structure:
    // 0: OperationName (ItemComponentDataType::STRING)
    // 1: References (ItemComponentDataType::STRING_DICTIONARY) - each entry is an ItemComponentData ([TypeId, [Field]])
    // 2: Success (ItemComponentDataType::NULLABLE_BOOL)
    // 3: StatusReason (ItemComponentDataType::STRING)
    std::map<uint64_t, signalr::value> Components;

    // OperationName
    Components[0] = ConstructComponentElement(DataTypeString, signalr::value(OperationName.c_str()));

    // References
    std::map<std::string, signalr::value> RefsMap;
    RefsMap["SpaceId"] = ConstructComponentElement(DataTypeString, signalr::value(NewSpaceId.c_str()));
    RefsMap["OriginalSpaceId"] = ConstructComponentElement(DataTypeString, signalr::value(OriginalSpaceId.c_str()));
    Components[1] = ConstructComponentElement(DataTypeStringDictionary, signalr::value(RefsMap));

    // Success
    Components[2] = ConstructComponentElement(DataTypeNullableBool, signalr::value(true));

    // StatusReason
    Components[3] = ConstructComponentElement(DataTypeString, signalr::value(StatusReason.c_str()));

    // Construct EventValues vector: [ EventName, SenderClientId, Recipient (null), Components(uint map) ]
    std::vector<signalr::value> EventValues = ConstructEventValues(Components);

    csp::common::AsyncCallCompletedEventData Parsed = DeserializeAsyncCallCompletedEvent(EventValues, LogSystem);

    // Verify new event structure was parsed corrctly
    EXPECT_EQ(Parsed.OperationName, OperationName);
    EXPECT_TRUE(Parsed.Success);
    EXPECT_EQ(Parsed.StatusReason, StatusReason);
    
    ASSERT_TRUE(Parsed.References.HasKey("SpaceId"));
    ASSERT_TRUE(Parsed.References.HasKey("OriginalSpaceId"));
    EXPECT_EQ(Parsed.References["SpaceId"], NewSpaceId);
    EXPECT_EQ(Parsed.References["OriginalSpaceId"], OriginalSpaceId);

    // Check backwards compatibility
    // If the References map contains the key "SpaceId", the deserializer sets the old ReferenceId and ReferenceType properties
    EXPECT_EQ(Parsed.ReferenceId, NewSpaceId);
    EXPECT_EQ(Parsed.ReferenceType, "GroupId"); // The key used to be "GroupId"
}
