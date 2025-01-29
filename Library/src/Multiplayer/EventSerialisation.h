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
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/EventParameters.h"
#include "CSP/Multiplayer/ReplicatedValue.h"

#include <signalrclient/signalr_value.h>

namespace csp::multiplayer
{

csp::common::String GetSequenceKeyIndex(const csp::common::String& SequenceKey, unsigned int Index);

// Generic deserialiser for multiplayer events. It can be derived from and
// its behavior can be overridden if specialised handling is needed for
// certain events.
class EventDeserialiser
{
public:
    // Creates an empty event deserialiser.
    EventDeserialiser();

    // The generic means to populate this deserialiser's members given a set of event values.
    virtual void Parse(const std::vector<signalr::value>& EventValues);

    // Returns a string describing the type of event.
    csp::common::String GetEventType() const { return EventType; }

    // Returns the unique integer identifer for the client from which the event was invoked.
    uint64_t GetSenderClientId() const { return SenderClientId; }

    // Returns the event data that has been synthesised from the parsed event
    // values that were parsed.
    const csp::common::Array<csp::multiplayer::ReplicatedValue>& GetEventData() const { return EventData; }

protected:
    void ParseCommon(const std::vector<signalr::value>& EventValues);

    csp::multiplayer::ReplicatedValue ParseSignalRComponent(uint64_t TypeId, const signalr::value& Component) const;

    csp::common::String EventType;
    uint64_t SenderClientId;
    csp::common::Array<csp::multiplayer::ReplicatedValue> EventData;
};

// A specialised deserialiser for handling events triggered when an asset referenced by the space changes.
class AssetChangedEventDeserialiser : public EventDeserialiser
{
public:
    virtual void Parse(const std::vector<signalr::value>& EventValues) override;

    const AssetDetailBlobParams& GetEventParams() const { return EventParams; }

private:
    AssetDetailBlobParams EventParams;
};

// A specialised deserialiser for handling events triggered when a conversation event happens.
class ConversationEventDeserialiser : public EventDeserialiser
{
public:
    virtual void Parse(const std::vector<signalr::value>& EventValues) override;

    const ConversationSystemParams& GetEventParams() const { return EventParams; }

private:
    ConversationSystemParams EventParams;
};

// A specialised deserialiser for handling events triggered when a user in the space's access permissions change.
class UserPermissionsChangedEventDeserialiser : public EventDeserialiser
{
public:
    virtual void Parse(const std::vector<signalr::value>& EventValues) override;

    const UserPermissionsParams& GetEventParams() const { return EventParams; }

private:
    UserPermissionsParams EventParams;
};

class SequenceChangedEventDeserialiser : public EventDeserialiser
{
public:
    virtual void Parse(const std::vector<signalr::value>& EventValues) override;

    const SequenceChangedParams& GetEventParams() const { return EventParams; }

private:
    SequenceChangedParams EventParams;
};

/// A deserialiser for getting SequenceHotspot data from an event:
/// UpdateType - The update type for the Sequence Hierarchy: Create, Update, Rename, Delete
/// SpaceId - The unique identifer of the space this hotspot sequence relates to.
/// Name - The name of the hotspot which has been changed.
/// NewName - In the case of renames, describes the new name of the sequence.
class SequenceHotspotChangedEventDeserialiser : public EventDeserialiser
{
public:
    virtual void Parse(const std::vector<signalr::value>& EventValues) override;

    const SequenceHotspotChangedParams& GetEventParams() const { return EventParams; }

private:
    SequenceHotspotChangedParams EventParams;
};

} // namespace csp::multiplayer
