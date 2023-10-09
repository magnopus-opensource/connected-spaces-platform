#include "Multiplayer/EventSerialisation.h"

#include "Debug/Logging.h"
#include "Multiplayer/MultiplayerKeyConstants.h"

using namespace csp::multiplayer;

EventDeserialiser::EventDeserialiser() : SenderClientId(0)
{
}

void EventDeserialiser::ParseCommon(const std::vector<signalr::value>& EventValues)
{
	/*
	 * class EventMessage
	 * [0] string EventType
	 * [1] uint SenderClientId
	 * [2] uint? RecipientClientId
	 *
	 * RecipientClientId can be processed if needed, but currently not required, though note it is a nullable uint,
	   null for an all-client broadcast, and a uint for the intended receiving client's Id : RecipientClientId = EventValues[2];
	*/

	EventType	   = (csp::common::String) EventValues[0].as_string().c_str();
	SenderClientId = EventValues[1].as_uinteger();
}


void EventDeserialiser::Parse(const std::vector<signalr::value>& EventValues)
{
	ParseCommon(EventValues);

	/*
	 * [3] map<uint, vec> Components
	 */

	if (!EventValues[3].is_null())
	{
		const std::map<uint64_t, signalr::value>& Components = EventValues[3].as_uint_map();

		EventData = csp::common::Array<csp::multiplayer::ReplicatedValue>(Components.size());
		int i	  = 0;

		for (auto& Component : Components)
		{
			// Component is in form [TypeId, [Field0, Field1, ...]]
			auto Type	   = Component.second.as_array()[0].as_uinteger();
			auto& Value	   = Component.second.as_array()[1].as_array()[0]; // ItemComponentData<T> only has a single field
			EventData[i++] = ParseSignalRComponent(Type, Value);
		}
	}
}

csp::multiplayer::ReplicatedValue EventDeserialiser::ParseSignalRComponent(uint64_t TypeId, const signalr::value& Component) const
{
	csp::multiplayer::ReplicatedValue ReplicatedValue;

	if (TypeId == csp::multiplayer::msgpack_typeids::ItemComponentData::NULLABLE_BOOL)
	{
		ReplicatedValue = Component.as_bool();
	}
	else if (TypeId == csp::multiplayer::msgpack_typeids::ItemComponentData::NULLABLE_INT64)
	{
		if (Component.is_integer())
		{
			ReplicatedValue = (int64_t) Component.as_integer();
		}
		else
		{
			ReplicatedValue = (int64_t) Component.as_uinteger();
		}
	}
	else if (TypeId == csp::multiplayer::msgpack_typeids::ItemComponentData::NULLABLE_DOUBLE)
	{
		ReplicatedValue = (float) Component.as_double();
	}
	else if (TypeId == csp::multiplayer::msgpack_typeids::ItemComponentData::STRING)
	{
		ReplicatedValue = Component.as_string().c_str();
	}
	else if (TypeId == csp::multiplayer::msgpack_typeids::ItemComponentData::FLOAT_ARRAY)
	{
		auto& Array = Component.as_array();

		if (Array.size() == 3)
		{
			ReplicatedValue = csp::common::Vector3 {(float) Array[0].as_double(), (float) Array[1].as_double(), (float) Array[2].as_double()};
		}
		else if (Array.size() == 4)
		{
			ReplicatedValue = csp::common::Vector4 {(float) Array[0].as_double(),
													(float) Array[1].as_double(),
													(float) Array[2].as_double(),
													(float) Array[3].as_double()};
		}
		else
		{
			CSP_LOG_ERROR_MSG("Unsupported event argument type: Only Vector3 and Vector4 float array arguments are accepted.");
		}
	}
	else if (TypeId == csp::multiplayer::msgpack_typeids::ItemComponentData::NULLABLE_UINT16)
	{
		ReplicatedValue = (int64_t) Component.as_uinteger();
	}
	else
	{
		CSP_LOG_ERROR_MSG("Unsupported event argument type.");
	}

	return ReplicatedValue;
}

AssetChangedEventDeserialiser::AssetChangedEventDeserialiser() : ChangeType(EAssetChangeType::Invalid), AssetType(csp::systems::EAssetType::IMAGE)
{
}

void AssetChangedEventDeserialiser::Parse(const std::vector<signalr::value>& EventValues)
{
	EventDeserialiser::Parse(EventValues);

	EAssetChangeType AssetChangeType = EAssetChangeType::Invalid;

	if (EventData[0].GetInt() < static_cast<int64_t>(EAssetChangeType::Num))
	{
		AssetChangeType = static_cast<EAssetChangeType>(EventData[0].GetInt());
	}
	else
	{
		CSP_LOG_ERROR_MSG("AssetDetailChangedEvent - AssetChangeType out of range of acceptable enum values.");
	}

	AssetId			  = EventData[1].GetString();
	Version			  = EventData[2].GetString();
	AssetType		  = csp::systems::ConvertDTOAssetDetailType(EventData[3].GetString());
	AssetCollectionId = EventData[4].GetString();
}

ChatEventDeserialiser::ChatEventDeserialiser() : MessageType(ConversationMessageType::MessageInformation)
{
}

void ChatEventDeserialiser::Parse(const std::vector<signalr::value>& EventValues)
{
	EventDeserialiser::Parse(EventValues);

	MessageType	 = static_cast<ConversationMessageType>(EventData[0].GetInt());
	MessageValue = EventData[1].GetString();
}

void UserPermissionsChangedEventDeserialiser::Parse(const std::vector<signalr::value>& EventValues)
{
	ParseCommon(EventValues);

	/*
	 * [3] map<uint, vec> Components, where Components is structured as follows:
	 * | Name              | Component ID | Type         | Notes (units, ranges)                                                     |
	 * |-------------------|--------------|--------------|---------------------------------------------------------------------------|
	 * | **SpaceId**       | 1            | String       | Id of the space that has updated permissions                              |
	 * | **GroupRoles**    | 100          | String Array | Array of user permissions (viewer,creator,owner) that belongs to the user |
	 * | **ChangeType**    | 101          | String       | Created, Updated, Removed                                                 |
	 * | **UserId**        | 102          | String       | The userId that was changed                                               |
	 * |-------------------|--------------|--------------|---------------------------------------------------------------------------|
	 */
}
