#pragma once
#include "CSP/Common/Array.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/Conversation/Conversation.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/ReplicatedValue.h"
#include "CSP/Systems/Assets/Asset.h"

#include <signalrclient/signalr_value.h>

namespace csp::multiplayer
{

// Generic deserialiser for multiplayer events. It can be derived from and
// its constructor behaviour can be overridden if specialised handling is needed for
// certain events.
class EventDeserialiser
{
public:
	// Creates an empty event deserialiser.
	EventDeserialiser();

	// The generic means to populate this deserialiser's members given a set of event values.
	virtual void Parse(const std::vector<signalr::value>& EventValues);

	// Returns a string describing the type of event.
	csp::common::String GetEventType() const
	{
		return EventType;
	}

	// Returns the unique integer identifer for the client from which the event was invoked.
	uint64_t GetSenderClientId() const
	{
		return SenderClientId;
	}

	// Returns the event data that has been synthesised from the parsed event
	// values that were passed in when constructing this object.
	const csp::common::Array<csp::multiplayer::ReplicatedValue>& GetEventData() const
	{
		return EventData;
	}

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
	AssetChangedEventDeserialiser();

	virtual void Parse(const std::vector<signalr::value>& EventValues) override;

	EAssetChangeType GetChangeType() const
	{
		return ChangeType;
	}
	csp::common::String GetAssetId() const
	{
		return AssetId;
	}
	csp::common::String GetVersion() const
	{
		return Version;
	}
	csp::systems::EAssetType GetAssetType() const
	{
		return AssetType;
	}
	csp::common::String GetAssetCollectionId() const
	{
		return AssetCollectionId;
	}

private:
	EAssetChangeType ChangeType;
	csp::common::String AssetId;
	csp::common::String Version;
	csp::systems::EAssetType AssetType;
	csp::common::String AssetCollectionId;
};

// A specialised deserialiser for handling events triggered when a chat message event happens.
class ChatEventDeserialiser : public EventDeserialiser
{
public:
	ChatEventDeserialiser();

	virtual void Parse(const std::vector<signalr::value>& EventValues) override;

	ConversationMessageType GetMessageType() const
	{
		return MessageType;
	}
	csp::common::String GetMessageValue() const
	{
		return MessageValue;
	}

private:
	ConversationMessageType MessageType;
	csp::common::String MessageValue;
};

// A specialised deserialiser for handling events triggered when a user in the space's access permissions change.
class UserPermissionsChangedEventDeserialiser : public EventDeserialiser
{
public:
	virtual void Parse(const std::vector<signalr::value>& EventValues) override;

	csp::common::String GetSpaceId() const
	{
		return SpaceId;
	}
	const csp::common::Array<csp::common::String>& GetUserRoles() const
	{
		return UserRoles;
	}
	EPermissionChangeType GetChangeType() const
	{
		return ChangeType;
	}
	csp::common::String GetUserId() const
	{
		return UserId;
	}

private:
	csp::common::String SpaceId;
	csp::common::Array<csp::common::String> UserRoles;
	EPermissionChangeType ChangeType;
	csp::common::String UserId;
};

} // namespace csp::multiplayer
