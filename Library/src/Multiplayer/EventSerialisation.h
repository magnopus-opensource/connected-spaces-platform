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

#pragma once

#include "CSP/Common/Array.h"
#include "CSP/Common/ReplicatedValue.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/Conversation/Conversation.h"
#include "CSP/Multiplayer/EventData.h"

#include <signalrclient/signalr_value.h>
#include <type_traits>

namespace csp::common
{
class LogSystem;
}

namespace csp::multiplayer
{

// Utility method to extract the sequence key index, used in a few places for understanding sequence events.
csp::common::String GetSequenceKeyIndex(const csp::common::String& SequenceKey, unsigned int Index);

// Deserialize the data general purpose event that requires no special custom deserialization.
EventData DeserializeGeneralPurposeEvent(const std::vector<signalr::value>& EventValues, csp::common::LogSystem& LogSystem);

// Specialized deserializataion for events triggered when an asset referenced by the space changes.
AssetDetailBlobChangedEventData DeserializeAssetDetailBlobChangedEvent(
    const std::vector<signalr::value>& EventValues, csp::common::LogSystem& LogSystem);

// Specialized deserializataion for events triggered when a conversation event happens.
ConversationEventData DeserializeConversationEvent(const std::vector<signalr::value>& EventValues, csp::common::LogSystem& LogSystem);

// Specialized deserializataion for events triggered when a user in the space's access permissions change.
AccessControlChangedEventData DeserializeAccessControlChangedEvent(const std::vector<signalr::value>& EventValues, csp::common::LogSystem& LogSystem);

// Specialized deserializataion for events triggered when a sequence in the space changes.
SequenceChangedEventData DeserializeSequenceChangedEvent(const std::vector<signalr::value>& EventValues, csp::common::LogSystem& LogSystem);

// Specialized deserializataion for events triggered when a hotspot sequence in the space changes.
SequenceHotspotChangedEventData DeserializeSequenceHotspotChangedEvent(
    const std::vector<signalr::value>& EventValues, csp::common::LogSystem& LogSystem);

} // namespace csp::multiplayer
