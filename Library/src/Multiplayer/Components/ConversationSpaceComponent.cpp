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

#include "CSP/Multiplayer/Components/ConversationSpaceComponent.h"

#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/Assets/AssetSystem.h"
#include "CSP/Systems/Spaces/SpaceSystem.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "CSP/Web/HTTPResponseCodes.h"
#include "CallHelpers.h"
#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Multiplayer/Conversation/ConversationSystemHelpers.h"
#include "Multiplayer/Script/ComponentBinding/ConversationSpaceComponentScriptInterface.h"
#include "Systems/ResultHelpers.h"

#include <msgpack/v1/object_fwd_decl.hpp>


using namespace csp::systems;


namespace csp::multiplayer
{

csp::multiplayer::ConversationSpaceComponent::ConversationSpaceComponent(SpaceEntity* Parent) : ComponentBase(ComponentType::Conversation, Parent)
{
	Properties[static_cast<uint32_t>(ConversationPropertyKeys::ConversationId)]				= "";
	Properties[static_cast<uint32_t>(ConversationPropertyKeys::IsActive)]					= true;
	Properties[static_cast<uint32_t>(ConversationPropertyKeys::IsVisible)]					= true;
	Properties[static_cast<uint32_t>(ConversationPropertyKeys::Position)]					= csp::common::Vector3 {0, 0, 0};
	Properties[static_cast<uint32_t>(ConversationPropertyKeys::Rotation)]					= csp::common::Vector4 {0, 0, 0, 1};
	Properties[static_cast<uint32_t>(ConversationPropertyKeys::Title)]						= "";
	Properties[static_cast<uint32_t>(ConversationPropertyKeys::Resolved)]					= false;
	Properties[static_cast<uint32_t>(ConversationPropertyKeys::ConversationCameraPosition)] = csp::common::Vector3 {0, 0, 0};

	SetScriptInterface(CSP_NEW ConversationSpaceComponentScriptInterface(this));
}

void ConversationSpaceComponent::CreateConversation(const csp::common::String& Message, StringResultCallback Callback)
{
	if (!GetConversationId().IsEmpty())
	{
		CSP_LOG_WARN_MSG("This component already has an associated conversation! No new conversation was created as a result.");

		INVOKE_IF_NOT_NULL(Callback, MakeInvalid<StringResult>());

		return;
	}

	const auto UserSystem = csp::systems::SystemsManager::Get().GetUserSystem();

	if (UserSystem->GetLoginState().State != csp::systems::ELoginState::LoggedIn)
	{
		CSP_LOG_ERROR_MSG("ConversationSpaceComponent::CreateConversation() failed: Log in before adding a message");

		INVOKE_IF_NOT_NULL(Callback, MakeInvalid<csp::systems::StringResult>());

		return;
	};

	auto* SpaceSystem		 = csp::systems::SystemsManager::Get().GetSpaceSystem();
	const auto& CurrentSpace = SpaceSystem->GetCurrentSpace();

	if (CurrentSpace.Id.IsEmpty())
	{
		CSP_LOG_ERROR_MSG("ConversationSpaceComponent::CreateConversation() failed: Enter a space before adding a message");

		INVOKE_IF_NOT_NULL(Callback, MakeInvalid<csp::systems::StringResult>());

		return;
	}

	const auto& UserId	= UserSystem->GetLoginState().UserId;
	const auto& SpaceId = CurrentSpace.Id;

	// We need to firstly create the comment container asset collection
	const csp::systems::AssetCollectionResultCallback AddCommentContainerCallback
		= [this, Callback](const csp::systems::AssetCollectionResult& AddCommentContainerResult)
	{
		if (AddCommentContainerResult.GetResultCode() == csp::systems::EResultCode::InProgress)
		{
			return;
		}

		if (AddCommentContainerResult.GetResultCode() == csp::systems::EResultCode::Success)
		{
			csp::common::String ConversationId = AddCommentContainerResult.GetAssetCollection().Id;

			csp::systems::StringResult InternalResult(AddCommentContainerResult.GetResultCode(), AddCommentContainerResult.GetHttpResultCode());
			InternalResult.SetValue(ConversationId);
			SetConversationId(ConversationId);
			INVOKE_IF_NOT_NULL(Callback, InternalResult);
		}
		else
		{
			CSP_LOG_FORMAT(csp::systems::LogLevel::Log,
						   "The Comment Container asset collection creation was not successful. ResCode: %d, HttpResCode: %d",
						   (int) AddCommentContainerResult.GetResultCode(),
						   AddCommentContainerResult.GetHttpResultCode());

			const csp::systems::StringResult InternalResult(AddCommentContainerResult.GetResultCode(), AddCommentContainerResult.GetHttpResultCode());
			INVOKE_IF_NOT_NULL(Callback, InternalResult);
		}
	};

	const auto UniqueAssetCollectionName	= ConversationSystemHelpers::GetUniqueConversationContainerAssetCollectionName(SpaceId, UserId);
	MessageInfo DefaultConversationInfo		= MessageInfo();
	DefaultConversationInfo.Message			= Message;
	DefaultConversationInfo.EditedTimestamp = "";
	DefaultConversationInfo.UserId			= UserId;
	DefaultConversationInfo.IsConversation	= true;

	auto* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
	AssetSystem->CreateAssetCollection(SpaceId,
									   nullptr,
									   UniqueAssetCollectionName,
									   ConversationSystemHelpers::GenerateConversationAssetCollectionMetadata(DefaultConversationInfo),
									   csp::systems::EAssetCollectionType::COMMENT_CONTAINER,
									   nullptr,
									   AddCommentContainerCallback);
}

void ConversationSpaceComponent::DeleteConversation(csp::systems::NullResultCallback Callback)
{
	auto ConversationId = GetConversationId();

	if (ConversationId.IsEmpty())
	{
		CSP_LOG_ERROR_MSG("The conversation ID of the component was empty when DeleteConversation was called! No update to the conversation was "
						  "issued as a result.");

		INVOKE_IF_NOT_NULL(Callback, MakeInvalid<NullResult>());

		return;
	}

	csp::systems::AssetCollectionsResultCallback GetMessagesCallback
		= [Callback, ConversationId, this](const csp::systems::AssetCollectionsResult& GetMessagesResult)
	{
		if (GetMessagesResult.GetResultCode() == csp::systems::EResultCode::InProgress)
		{
			return;
		}

		if (GetMessagesResult.GetResultCode() == csp::systems::EResultCode::Failed)
		{
			CSP_LOG_FORMAT(csp::systems::LogLevel::Log,
						   "The retrieval of Message asset collections was not successful. ResCode: %d, HttpResCode: %d",
						   (int) GetMessagesResult.GetResultCode(),
						   GetMessagesResult.GetHttpResultCode());

			csp::systems::NullResult InternalResult(GetMessagesResult.GetResultCode(), GetMessagesResult.GetHttpResultCode());
			INVOKE_IF_NOT_NULL(Callback, InternalResult);

			return;
		}

		const csp::systems::NullResultCallback DeleteMessagesCallback
			= [Callback, ConversationId, this](const csp::systems::NullResult& DeleteMessagesResult)
		{
			if (DeleteMessagesResult.GetResultCode() == csp::systems::EResultCode::Failed)
			{
				CSP_LOG_FORMAT(csp::systems::LogLevel::Log,
							   "Not all Message asset collections were deleted. ResCode: %d, HttpResCode: %d",
							   (int) DeleteMessagesResult.GetResultCode(),
							   DeleteMessagesResult.GetHttpResultCode());

				INVOKE_IF_NOT_NULL(Callback, MakeInvalid<csp::systems::NullResult>());

				return;
			}

			const csp::multiplayer::MessageResultCallback DeleteConversationAssetCollectionCallback
				= [Callback](const csp::multiplayer::MessageResult& DeleteConversationAssetCollectionResult)
			{
				if (DeleteConversationAssetCollectionResult.GetResultCode() == csp::systems::EResultCode::InProgress)
				{
					return;
				}

				if (DeleteConversationAssetCollectionResult.GetResultCode() == csp::systems::EResultCode::Failed)
				{
					CSP_LOG_FORMAT(csp::systems::LogLevel::Log,
								   "The deletion of the Conversation asset collection was not successful. ResCode: %d, HttpResCode: %d",
								   (int) DeleteConversationAssetCollectionResult.GetResultCode(),
								   DeleteConversationAssetCollectionResult.GetHttpResultCode());

					INVOKE_IF_NOT_NULL(Callback, MakeInvalid<csp::systems::NullResult>());

					return;
				}

				INVOKE_IF_NOT_NULL(Callback, DeleteConversationAssetCollectionResult);
			};

			// Delete also the conversation asset collection
			csp::systems::AssetCollection ConversationAssetCollection;
			ConversationAssetCollection.Id = ConversationId;

			DeleteMessage(ConversationId, DeleteConversationAssetCollectionCallback);
		};

		const csp::systems::NullResultCallback NullResultCallback
			= [Callback, ConversationId, this](const csp::systems::NullResult& NullResultCallbackResult)
		{
			const MultiplayerConnection::ErrorCodeCallbackHandler SignalRCallback = [Callback, NullResultCallbackResult](ErrorCode Error)
			{
				if (Error != ErrorCode::None)
				{
					CSP_LOG_ERROR_MSG("DeleteConversation: SignalR connection: Error");

					INVOKE_IF_NOT_NULL(Callback, MakeInvalid<csp::systems::NullResult>());

					return;
				}

				INVOKE_IF_NOT_NULL(Callback, NullResultCallbackResult);
			};

			auto EventBus = csp::systems::SystemsManager::Get().GetEventBus();
			EventBus->SendNetworkEvent("ConversationSystem",
									   {ReplicatedValue((int64_t) ConversationMessageType::DeleteConversation), ConversationId},
									   SignalRCallback);
		};

		auto Messages = GetMessagesResult.GetAssetCollections();
		DeleteMessages(Messages, NullResultCallback);
	};

	auto* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();

	Array<csp::systems::EAssetCollectionType> PrototypeTypes = {csp::systems::EAssetCollectionType::COMMENT};

	// Retrieve first the messages from this conversation
	AssetSystem->FindAssetCollections(nullptr, ConversationId, nullptr, PrototypeTypes, nullptr, nullptr, nullptr, nullptr, GetMessagesCallback);
}

void ConversationSpaceComponent::AddMessage(const csp::common::String& Message, MessageResultCallback Callback)
{
	auto ConversationId = GetConversationId();

	if (ConversationId.IsEmpty())
	{
		CSP_LOG_ERROR_MSG("The conversation ID of this component was empty! No update to the conversation was issued as a result.");

		INVOKE_IF_NOT_NULL(Callback, MakeInvalid<MessageResult>());

		return;
	}

	auto* UserSystem = csp::systems::SystemsManager::Get().GetUserSystem();

	if (UserSystem->GetLoginState().State != csp::systems::ELoginState::LoggedIn)
	{
		CSP_LOG_ERROR_MSG("ConversationSpaceComponent::AddMessage() failed: Log in before adding a message");

		INVOKE_IF_NOT_NULL(Callback, MakeInvalid<MessageResult>());

		return;
	}

	auto* SpaceSystem		 = csp::systems::SystemsManager::Get().GetSpaceSystem();
	const auto& CurrentSpace = SpaceSystem->GetCurrentSpace();

	if (CurrentSpace.Id.IsEmpty())
	{
		CSP_LOG_ERROR_MSG("ConversationSpaceComponent::AddMessage() failed: Enter a space before adding a message");

		INVOKE_IF_NOT_NULL(Callback, MakeInvalid<MessageResult>());

		return;
	}

	const auto& UserId = UserSystem->GetLoginState().UserId;

	const MessageResultCallback MessageResultCallback = [Callback, ConversationId, this](const MessageResult& MessageResultCallbackResult)
	{
		const MultiplayerConnection::ErrorCodeCallbackHandler SignalRCallback = [Callback, MessageResultCallbackResult, this](ErrorCode Error)
		{
			if (Error != ErrorCode::None)
			{
				CSP_LOG_ERROR_MSG("AddMessage: SignalR connection: Error");

				const MessageResult InternalResult(csp::systems::EResultCode::Failed,
												   static_cast<uint16_t>(csp::web::EResponseCodes::ResponseInternalServerError));
				INVOKE_IF_NOT_NULL(Callback, InternalResult);

				return;
			}

			INVOKE_IF_NOT_NULL(Callback, MessageResultCallbackResult);
		};

		auto EventBus = csp::systems::SystemsManager::Get().GetEventBus();
		EventBus->SendNetworkEvent("ConversationSystem",
								   {ReplicatedValue((int64_t) ConversationMessageType::NewMessage), ConversationId},
								   SignalRCallback);
	};

	StoreConversationMessage(CurrentSpace, UserId, Message, MessageResultCallback);
}

void ConversationSpaceComponent::DeleteMessage(const csp::common::String& MessageId, MessageResultCallback Callback)
{
	csp::systems::AssetCollection MessageAssetCollection;
	MessageAssetCollection.Id = MessageId;

	const csp::systems::NullResultCallback NullCallback = [Callback, MessageId, this](const csp::systems::NullResult& NullCallbackResult)
	{
		const MultiplayerConnection::ErrorCodeCallbackHandler SignalRCallback = [Callback, NullCallbackResult](ErrorCode Error)
		{
			if (Error != ErrorCode::None)
			{
				CSP_LOG_ERROR_MSG("DeleteMessage: SignalR connection: Error");

				INVOKE_IF_NOT_NULL(Callback, MakeInvalid<csp::multiplayer::MessageResult>());

				return;
			}

			csp::multiplayer::MessageResult Result(NullCallbackResult.GetResultCode(), NullCallbackResult.GetHttpResultCode());

			INVOKE_IF_NOT_NULL(Callback, Result);
		};

		auto EventBus = csp::systems::SystemsManager::Get().GetEventBus();
		EventBus->SendNetworkEvent("ConversationSystem",
								   {ReplicatedValue((int64_t) ConversationMessageType::DeleteMessage), MessageId},
								   SignalRCallback);
	};

	auto* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
	AssetSystem->DeleteAssetCollection(MessageAssetCollection, NullCallback);
}

bool ConversationSpaceComponent::GetIsVisible() const
{
	return GetBooleanProperty(static_cast<uint32_t>(ConversationPropertyKeys::IsVisible));
}

void ConversationSpaceComponent::SetIsVisible(const bool Value)
{
	SetProperty(static_cast<uint32_t>(ConversationPropertyKeys::IsVisible), Value);
}

bool ConversationSpaceComponent::GetIsActive() const
{
	return GetBooleanProperty(static_cast<uint32_t>(ConversationPropertyKeys::IsActive));
}

/* IPositionComponent */

const csp::common::Vector3& ConversationSpaceComponent::GetPosition() const
{
	return GetVector3Property(static_cast<uint32_t>(ConversationPropertyKeys::Position));
}

void ConversationSpaceComponent::SetPosition(const csp::common::Vector3& Value)
{
	SetProperty(static_cast<uint32_t>(ConversationPropertyKeys::Position), Value);
}

/* IRotationComponent */

const csp::common::Vector4& ConversationSpaceComponent::GetRotation() const
{
	return GetVector4Property(static_cast<uint32_t>(ConversationPropertyKeys::Rotation));
}

void ConversationSpaceComponent::SetRotation(const csp::common::Vector4& Value)
{
	SetProperty(static_cast<uint32_t>(ConversationPropertyKeys::Rotation), Value);
}

void ConversationSpaceComponent::GetMessagesFromConversation(const csp::common::Optional<int>& ResultsSkipNumber,
															 const csp::common::Optional<int>& ResultsMaxNumber,
															 MessageCollectionResultCallback Callback)
{
	auto ConversationId = GetConversationId();

	if (ConversationId.IsEmpty())
	{
		CSP_LOG_ERROR_MSG("The conversation ID of the component was empty when DeleteConversation was called! No update to the conversation was "
						  "issued as a result.");

		INVOKE_IF_NOT_NULL(Callback, MakeInvalid<MessageCollectionResult>());

		return;
	}

	csp::systems::AssetCollectionsResultCallback GetMessagesCallback = [Callback](const csp::systems::AssetCollectionsResult& GetMessagesResult)
	{
		if (GetMessagesResult.GetResultCode() == csp::systems::EResultCode::InProgress)
		{
			return;
		}

		MessageCollectionResult InternalResult(GetMessagesResult.GetResultCode(), GetMessagesResult.GetHttpResultCode());

		if (GetMessagesResult.GetResultCode() == csp::systems::EResultCode::Failed)
		{
			CSP_LOG_FORMAT(csp::systems::LogLevel::Log,
						   "The retrieval of Message asset collections was not successful. ResCode: %d, HttpResCode: %d",
						   static_cast<int>(GetMessagesResult.GetResultCode()),
						   GetMessagesResult.GetHttpResultCode());

			INVOKE_IF_NOT_NULL(Callback, InternalResult);

			return;
		}

		InternalResult.SetTotalCount(GetMessagesResult.GetTotalCount());
		InternalResult.FillMessageInfoCollection(GetMessagesResult.GetAssetCollections());
		INVOKE_IF_NOT_NULL(Callback, InternalResult);
	};

	Array<csp::systems::EAssetCollectionType> PrototypeTypes = {csp::systems::EAssetCollectionType::COMMENT};

	auto* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
	AssetSystem->FindAssetCollections(nullptr,
									  ConversationId,
									  nullptr,
									  PrototypeTypes,
									  nullptr,
									  nullptr,
									  ResultsSkipNumber,
									  ResultsMaxNumber,
									  GetMessagesCallback);
}

void ConversationSpaceComponent::GetConversationInfo(ConversationResultCallback Callback)
{
	auto ConversationId = GetConversationId();

	if (ConversationId.IsEmpty())
	{
		CSP_LOG_ERROR_MSG("This component does not have an associated conversation.");

		INVOKE_IF_NOT_NULL(Callback, MakeInvalid<ConversationResult>());

		return;
	}

	auto* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();

	csp::systems::AssetCollectionResultCallback GetConversationCallback = [=](const csp::systems::AssetCollectionResult& GetConversationResult)
	{
		if (GetConversationResult.GetResultCode() == csp::systems::EResultCode::InProgress)
		{
			return;
		}

		ConversationResult InternalResult(GetConversationResult.GetResultCode(), GetConversationResult.GetHttpResultCode());

		if (GetConversationResult.GetResultCode() == csp::systems::EResultCode::Failed)
		{
			INVOKE_IF_NOT_NULL(Callback, InternalResult);

			return;
		}

		InternalResult.FillConversationInfo(GetConversationResult.GetAssetCollection());
		INVOKE_IF_NOT_NULL(Callback, InternalResult);
	};

	AssetSystem->GetAssetCollectionById(ConversationId, GetConversationCallback);
}

void ConversationSpaceComponent::SetConversationInfo(const MessageInfo& ConversationData, ConversationResultCallback Callback)
{
	auto ConversationId = GetConversationId();

	if (ConversationId.IsEmpty())
	{
		CSP_LOG_ERROR_MSG("This component does not have an associated conversation.");

		INVOKE_IF_NOT_NULL(Callback, MakeInvalid<ConversationResult>());

		return;
	}

	auto* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();

	csp::systems::AssetCollectionResultCallback GetConversationCallback
		= [Callback, ConversationId, ConversationData, AssetSystem, this](const csp::systems::AssetCollectionResult& GetConversationResult)
	{
		if (GetConversationResult.GetResultCode() == csp::systems::EResultCode::InProgress)
		{
			return;
		}

		if (GetConversationResult.GetResultCode() == csp::systems::EResultCode::Failed)
		{
			CSP_LOG_FORMAT(csp::systems::LogLevel::Log,
						   "The retrieval of Conversation asset collections was not successful. ResCode: %d, HttpResCode: %d",
						   (int) GetConversationResult.GetResultCode(),
						   GetConversationResult.GetHttpResultCode());

			const ConversationResult InternalResult(GetConversationResult.GetResultCode(), GetConversationResult.GetHttpResultCode());
			INVOKE_IF_NOT_NULL(Callback, InternalResult);

			return;
		}

		csp::systems::AssetCollectionResultCallback GetUpdatedConversationCallback
			= [Callback, GetConversationResult, ConversationId, this](const csp::systems::AssetCollectionResult& GetUpdatedConversationResult)

		{
			if (GetUpdatedConversationResult.GetResultCode() == csp::systems::EResultCode::InProgress)
			{
				return;
			}

			if (GetUpdatedConversationResult.GetResultCode() == csp::systems::EResultCode::Failed)
			{
				CSP_LOG_FORMAT(csp::systems::LogLevel::Log,
							   "The Update of Conversation asset collections was not successful. ResCode: %d, HttpResCode: %d",
							   (int) GetConversationResult.GetResultCode(),
							   GetConversationResult.GetHttpResultCode());

				const ConversationResult InternalResult(GetConversationResult.GetResultCode(), GetConversationResult.GetHttpResultCode());
				INVOKE_IF_NOT_NULL(Callback, InternalResult);

				return;
			}

			const MultiplayerConnection::ErrorCodeCallbackHandler SignalRCallback = [Callback, GetUpdatedConversationResult](ErrorCode Error)
			{
				if (Error != ErrorCode::None)
				{
					CSP_LOG_ERROR_MSG("SetConversationInfo: SignalR connection: Error");

					INVOKE_IF_NOT_NULL(Callback, MakeInvalid<ConversationResult>());

					return;
				}

				ConversationResult Result(GetUpdatedConversationResult.GetResultCode(), GetUpdatedConversationResult.GetHttpResultCode());
				Result.FillConversationInfo(GetUpdatedConversationResult.GetAssetCollection());
				INVOKE_IF_NOT_NULL(Callback, Result);
			};

			auto EventBus = csp::systems::SystemsManager::Get().GetEventBus();
			EventBus->SendNetworkEvent("ConversationSystem",
									   {ReplicatedValue((int64_t) ConversationMessageType::ConversationInformation), ConversationId},
									   SignalRCallback);
		};

		MessageInfo NewConversationData(ConversationData);

		if (NewConversationData.EditedTimestamp.IsEmpty())
		{
			const auto CurrentConversationData
				= ConversationSystemHelpers::GetMessageInfoFromMessageAssetCollection(GetConversationResult.GetAssetCollection());
			NewConversationData.EditedTimestamp = CurrentConversationData.EditedTimestamp;
		}

		AssetSystem->UpdateAssetCollectionMetadata(GetConversationResult.GetAssetCollection(),
												   ConversationSystemHelpers::GenerateConversationAssetCollectionMetadata(NewConversationData),
												   nullptr,
												   GetUpdatedConversationCallback);
	};

	AssetSystem->GetAssetCollectionById(ConversationId, GetConversationCallback);
}

void ConversationSpaceComponent::GetMessageInfo(const csp::common::String& MessageId, MessageResultCallback Callback)
{
	const csp::systems::AssetCollectionResultCallback GetMessageCallback = [Callback](const csp::systems::AssetCollectionResult& GetMessageResult)
	{
		if (GetMessageResult.GetResultCode() == csp::systems::EResultCode::InProgress)
		{
			return;
		}

		MessageResult InternalResult(GetMessageResult.GetResultCode(), GetMessageResult.GetHttpResultCode());

		if (GetMessageResult.GetResultCode() == csp::systems::EResultCode::Failed)
		{
			CSP_LOG_FORMAT(csp::systems::LogLevel::Log,
						   "The retrieval of the Message asset collection was not successful. ResCode: %d, HttpResCode: %d",
						   static_cast<int>(GetMessageResult.GetResultCode()),
						   GetMessageResult.GetHttpResultCode());

			INVOKE_IF_NOT_NULL(Callback, InternalResult);

			return;
		}

		InternalResult.FillMessageInfo(GetMessageResult.GetAssetCollection());
		INVOKE_IF_NOT_NULL(Callback, InternalResult);
	};

	auto* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
	AssetSystem->GetAssetCollectionById(MessageId, GetMessageCallback);
}

void ConversationSpaceComponent::SetMessageInfo(const csp::common::String& MessageId, const MessageInfo& MessageData, MessageResultCallback Callback)
{
	auto* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();

	csp::systems::AssetCollectionResultCallback GetMessageCallback
		= [Callback, MessageId, MessageData, AssetSystem, this](const csp::systems::AssetCollectionResult& GetMessageResult)
	{
		csp::systems::AssetCollectionResultCallback GetUpdatedMessageCallback
			= [Callback, MessageId, GetMessageResult, this](const csp::systems::AssetCollectionResult& GetUpdatedMessageResult)

		{
			if (GetUpdatedMessageResult.GetResultCode() == csp::systems::EResultCode::InProgress)
			{
				return;
			}

			if (GetUpdatedMessageResult.GetResultCode() == csp::systems::EResultCode::Failed)
			{
				CSP_LOG_FORMAT(csp::systems::LogLevel::Log,
							   "The Update of Message asset collections was not successful. ResCode: %d, HttpResCode: %d",
							   static_cast<int>(GetMessageResult.GetResultCode()),
							   GetMessageResult.GetHttpResultCode());

				const MessageResult InternalResult(GetMessageResult.GetResultCode(), GetMessageResult.GetHttpResultCode());
				INVOKE_IF_NOT_NULL(Callback, InternalResult);

				return;
			}

			const MultiplayerConnection::ErrorCodeCallbackHandler SignalRCallback = [Callback, GetUpdatedMessageResult](ErrorCode Error)
			{
				if (Error != ErrorCode::None)
				{
					CSP_LOG_ERROR_MSG("SetMessageInfo: SignalR connection: Error");

					INVOKE_IF_NOT_NULL(Callback, MakeInvalid<MessageResult>());

					return;
				}

				MessageResult Result(GetUpdatedMessageResult.GetResultCode(), GetUpdatedMessageResult.GetHttpResultCode());
				Result.FillMessageInfo(GetUpdatedMessageResult.GetAssetCollection());
				INVOKE_IF_NOT_NULL(Callback, Result);
			};

			auto EventBus = csp::systems::SystemsManager::Get().GetEventBus();
			EventBus->SendNetworkEvent("ConversationSystem",
									   {ReplicatedValue((int64_t) ConversationMessageType::MessageInformation), MessageId},
									   SignalRCallback);
		};

		if (GetMessageResult.GetResultCode() == csp::systems::EResultCode::InProgress)
		{
			return;
		}

		if (GetMessageResult.GetResultCode() == csp::systems::EResultCode::Failed)
		{
			CSP_LOG_FORMAT(csp::systems::LogLevel::Log,
						   "The retrieval of Message asset collections was not successful. ResCode: %d, HttpResCode: %d",
						   static_cast<int>(GetMessageResult.GetResultCode()),
						   GetMessageResult.GetHttpResultCode());

			const MessageResult InternalResult(GetMessageResult.GetResultCode(), GetMessageResult.GetHttpResultCode());
			INVOKE_IF_NOT_NULL(Callback, InternalResult);

			return;
		}

		MessageInfo NewMessageData(MessageData);

		if (MessageData.EditedTimestamp.IsEmpty())
		{
			MessageInfo MsgInfo = ConversationSystemHelpers::GetMessageInfoFromMessageAssetCollection(GetMessageResult.GetAssetCollection());
			NewMessageData.EditedTimestamp = MsgInfo.EditedTimestamp;
		}

		AssetSystem->UpdateAssetCollectionMetadata(GetMessageResult.GetAssetCollection(),
												   ConversationSystemHelpers::GenerateMessageAssetCollectionMetadata(NewMessageData),
												   nullptr,
												   GetUpdatedMessageCallback);
	};

	AssetSystem->GetAssetCollectionById(MessageId, GetMessageCallback);
}

void ConversationSpaceComponent::SetIsActive(const bool Value)
{
	SetProperty(static_cast<uint32_t>(ConversationPropertyKeys::IsActive), Value);
}

void ConversationSpaceComponent::SetTitle(const csp::common::String& Value)
{
	SetProperty(static_cast<uint32_t>(ConversationPropertyKeys::Title), Value);
}

const csp::common::String& ConversationSpaceComponent::GetTitle() const
{
	return GetStringProperty(static_cast<uint32_t>(ConversationPropertyKeys::Title));
}

void ConversationSpaceComponent::SetResolved(bool Value)
{
	SetProperty(static_cast<uint32_t>(ConversationPropertyKeys::Resolved), Value);
}

bool ConversationSpaceComponent::GetResolved() const
{
	return GetBooleanProperty(static_cast<uint32_t>(ConversationPropertyKeys::Resolved));
}

void ConversationSpaceComponent::SetConversationCameraPosition(const csp::common::Vector3& InValue)
{
	SetProperty(static_cast<uint32_t>(ConversationPropertyKeys::ConversationCameraPosition), InValue);
}

const csp::common::Vector3& ConversationSpaceComponent::GetConversationCameraPosition() const
{
	return GetVector3Property(static_cast<uint32_t>(ConversationPropertyKeys::ConversationCameraPosition));
}

void ConversationSpaceComponent::GetNumberOfReplies(NumberOfRepliesResultCallback Callback)
{
	auto* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();

	auto GetMessageCountCallback = [Callback](const csp::systems::AssetCollectionCountResult& GetMessageResult)
	{
		NumberOfRepliesResult Result(GetMessageResult);
		Result.SetCount(GetMessageResult.GetCount());
		Callback(Result);
	};

	auto ConversationId													  = GetConversationId();
	static const Array<csp::systems::EAssetCollectionType> PrototypeTypes = {csp::systems::EAssetCollectionType::COMMENT};

	AssetSystem->GetAssetCollectionCount(nullptr, ConversationId, nullptr, PrototypeTypes, nullptr, nullptr, GetMessageCountCallback);
}

void ConversationSpaceComponent::SetConversationId(const csp::common::String& Value)
{
	SetProperty(static_cast<uint32_t>(ConversationPropertyKeys::ConversationId), Value);
}

void ConversationSpaceComponent::RemoveConversationId()
{
	RemoveProperty(static_cast<uint32_t>(ConversationPropertyKeys::ConversationId));
}

const csp::common::String& ConversationSpaceComponent::GetConversationId() const
{
	return GetStringProperty(static_cast<uint32_t>(ConversationPropertyKeys::ConversationId));
}

void ConversationSpaceComponent::StoreConversationMessage(const csp::systems::Space& Space,
														  const csp::common::String& UserId,
														  const csp::common::String& Message,
														  MessageResultCallback Callback) const
{
	auto ConversationId = GetConversationId();

	if (ConversationId.IsEmpty())
	{
		CSP_LOG_WARN_MSG("This component does not have a conversation set! Call CreateConversation before doing message operations.");

		INVOKE_IF_NOT_NULL(Callback, MakeInvalid<MessageResult>());

		return;
	}

	const csp::systems::AssetCollectionResultCallback AddCommentCallback = [=](const csp::systems::AssetCollectionResult& AddCommentResult)
	{
		if (AddCommentResult.GetResultCode() == csp::systems::EResultCode::InProgress)
		{
			return;
		}

		if (AddCommentResult.GetResultCode() == csp::systems::EResultCode::Failed)
		{
			CSP_LOG_FORMAT(csp::systems::LogLevel::Log,
						   "The Comment asset collection creation was not successful. ResCode: %d, HttpResCode: %d",
						   static_cast<int>(AddCommentResult.GetResultCode()),
						   AddCommentResult.GetHttpResultCode());

			const MessageResult InternalResult(AddCommentResult.GetResultCode(), AddCommentResult.GetHttpResultCode());
			INVOKE_IF_NOT_NULL(Callback, InternalResult);

			return;
		}

		MessageResult Result;
		Result.FillMessageInfo(AddCommentResult.GetAssetCollection());
		INVOKE_IF_NOT_NULL(Callback, Result);
	};

	const auto AssetSystem				 = csp::systems::SystemsManager::Get().GetAssetSystem();
	const auto UniqueAssetCollectionName = ConversationSystemHelpers::GetUniqueMessageAssetCollectionName(Space.Id, UserId);
	MessageInfo DefaultMessageInfo		 = MessageInfo();
	DefaultMessageInfo.EditedTimestamp	 = "";
	DefaultMessageInfo.Message			 = Message;
	DefaultMessageInfo.UserId			 = UserId;
	DefaultMessageInfo.IsConversation	 = false;
	const auto MessageMetadata			 = ConversationSystemHelpers::GenerateMessageAssetCollectionMetadata(DefaultMessageInfo);

	AssetSystem->CreateAssetCollection(Space.Id,
									   ConversationId,
									   UniqueAssetCollectionName,
									   MessageMetadata,
									   csp::systems::EAssetCollectionType::COMMENT,
									   nullptr,
									   AddCommentCallback);
}

void ConversationSpaceComponent::DeleteMessages(csp::common::Array<csp::systems::AssetCollection>& Messages,
												csp::systems::NullResultCallback Callback)
{
	const auto MessagesCount = Messages.Size();

	if (MessagesCount == 0)
	{
		csp::systems::NullResult InternalResult(csp::systems::EResultCode::Success, (uint16_t) csp::web::EResponseCodes::ResponseNoContent);
		INVOKE_IF_NOT_NULL(Callback, InternalResult);

		return;
	}

	const auto AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();

	AssetSystem->DeleteMultipleAssetCollections(Messages, Callback);
}

uint64_t NumberOfRepliesResult::GetCount() const
{
	return Count;
}

void NumberOfRepliesResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
	ResultBase::OnResponse(ApiResponse);
}

void NumberOfRepliesResult::SetCount(uint64_t Value)
{
	Count = Value;
}

} // namespace csp::multiplayer
