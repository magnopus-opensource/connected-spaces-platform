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
#include "CSP/Multiplayer/Conversation/ConversationSystem.h"

#include "CSP/Systems/Assets/AssetSystem.h"
#include "CSP/Systems/Spaces/SpaceSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "ConversationSystemHelpers.h"
#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Web/HttpResponse.h"


namespace csp::multiplayer
{

ConversationSystem::ConversationSystem(MultiplayerConnection* Connection) : MultiPlayerConnection(Connection), Connection(nullptr)
{
}

void ConversationSystem::SetConnection(csp::multiplayer::SignalRConnection* InConnection)
{
	Connection = InConnection;
}

void ConversationSystem::StoreConversationMessage(const csp::common::String& ConversationId,
												  const csp::systems::Space& Space,
												  const csp::common::String& UserId,
												  const csp::common::String& SenderDisplayName,
												  const csp::common::String& Message,
												  MessageResultCallback Callback) const
{
	const csp::systems::AssetCollectionResultCallback AddCommentCallback = [=](const csp::systems::AssetCollectionResult& AddCommentResult)
	{
		if (AddCommentResult.GetResultCode() == csp::services::EResultCode::InProgress)
		{
			return;
		}

		if (AddCommentResult.GetResultCode() == csp::services::EResultCode::Success)
		{
			MessageResult Result;
			Result.FillMessageInfo(AddCommentResult.GetAssetCollection());
			Callback(Result);
		}
		else
		{
			CSP_LOG_FORMAT(csp::systems::LogLevel::Log,
						   "The Comment asset collection creation was not successful. ResCode: %d, HttpResCode: %d",
						   (int) AddCommentResult.GetResultCode(),
						   AddCommentResult.GetHttpResultCode());

			const MessageResult InternalResult(AddCommentResult.GetResultCode(), AddCommentResult.GetHttpResultCode());
			Callback(InternalResult);
		}
	};

	const auto AssetSystem				 = csp::systems::SystemsManager::Get().GetAssetSystem();
	const auto UniqueAssetCollectionName = ConversationSystemHelpers::GetUniqueMessageAssetCollectionName(Space.Id, UserId);
	MessageInfo DefaultMessageInfo		 = MessageInfo();
	DefaultMessageInfo.Edited			 = false;
	DefaultMessageInfo.Message			 = Message;
	DefaultMessageInfo.UserDisplayName	 = SenderDisplayName;
	const auto MessageMetadata			 = ConversationSystemHelpers::GenerateMessageAssetCollectionMetadata(DefaultMessageInfo);

	AssetSystem->CreateAssetCollection(Space.Id,
									   ConversationId,
									   UniqueAssetCollectionName,
									   MessageMetadata,
									   csp::systems::EAssetCollectionType::COMMENT,
									   nullptr,
									   AddCommentCallback);
}

void ConversationSystem::DeleteMessages(const csp::common::Array<csp::systems::AssetCollection>& Messages, csp::systems::NullResultCallback Callback)
{
	const auto MessagesCount = Messages.Size();
	if (MessagesCount == 0)
	{
		csp::systems::NullResult InternalResult(csp::services::EResultCode::Success, (uint16_t) csp::web::EResponseCodes::ResponseNoContent);
		Callback(InternalResult);

		return;
	}

	const auto AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();

	auto DeletionCounterDeleter = [](size_t* ptr)
	{
		CSP_DELETE(ptr);
	};

	std::shared_ptr<size_t> DeletionCounter(CSP_NEW size_t, DeletionCounterDeleter);
	*DeletionCounter = 0;

	for (auto idx = 0; idx < MessagesCount; ++idx)
	{
		const csp::systems::NullResultCallback DeleteCommentCallback = [=](const csp::systems::NullResult& DeleteCommentResult)
		{
			if (DeleteCommentResult.GetResultCode() == csp::services::EResultCode::Success)
			{
				++*DeletionCounter;

				if (*DeletionCounter == MessagesCount)
				{
					Callback(DeleteCommentResult);
				}
			}
			else if (DeleteCommentResult.GetResultCode() == csp::services::EResultCode::Failed)
			{

				CSP_LOG_FORMAT(csp::systems::LogLevel::Log,
							   "Delete asset collection for message ID %s has failed. ResCode: %d, HttpResCode: %d",
							   Messages[idx].Id.c_str(),
							   (int) DeleteCommentResult.GetResultCode(),
							   DeleteCommentResult.GetHttpResultCode());

				++*DeletionCounter;

				if (*DeletionCounter == MessagesCount)
				{
					Callback(DeleteCommentResult);
				}
			}
		};

		AssetSystem->DeleteAssetCollection(Messages[idx], DeleteCommentCallback);
	}
}

void ConversationSystem::CreateConversation(const csp::common::String& Message, csp::systems::StringResultCallback Callback)
{
	const auto SpaceSystem	= csp::systems::SystemsManager::Get().GetSpaceSystem();
	const auto CurrentSpace = SpaceSystem->GetCurrentSpace();
	csp::common::String return_value;

	assert(CurrentSpace.Id.IsEmpty() != true && "Enter a space before adding a message");

	const auto UserSystem = csp::systems::SystemsManager::Get().GetUserSystem();

	assert(UserSystem->GetLoginState().State == csp::systems::ELoginState::LoggedIn && "Log in before adding a message");

	const auto LoggedInUserId = UserSystem->GetLoginState().UserId;

	csp::systems::ProfileResultCallback GetProfileCallback = [=](const csp::systems::ProfileResult& GetProfileResult)
	{
		if (GetProfileResult.GetResultCode() == csp::services::EResultCode::InProgress)
		{
			return;
		}
		if (GetProfileResult.GetResultCode() == csp::services::EResultCode::Failed)
		{
			const csp::systems::StringResult InternalResult(GetProfileResult.GetResultCode(), GetProfileResult.GetHttpResultCode());
			Callback(InternalResult);
		}
		if (GetProfileResult.GetResultCode() == csp::services::EResultCode::Success)
		{
			// We need to firstly create the comment container asset collection
			const csp::systems::AssetCollectionResultCallback AddCommentContainerCallback
				= [=](const csp::systems::AssetCollectionResult& AddCommentContainerResult)
			{
				if (AddCommentContainerResult.GetResultCode() == csp::services::EResultCode::InProgress)
				{
					return;
				}

				if (AddCommentContainerResult.GetResultCode() == csp::services::EResultCode::Success)
				{
					const auto ConversationId = AddCommentContainerResult.GetAssetCollection().Id;
					csp::systems::StringResult InternalResult(AddCommentContainerResult.GetResultCode(),
															  AddCommentContainerResult.GetHttpResultCode());
					InternalResult.SetValue(ConversationId);
					Callback(InternalResult);
				}
				else
				{
					CSP_LOG_FORMAT(csp::systems::LogLevel::Log,
								   "The Comment Container asset collection creation was not successful. ResCode: %d, HttpResCode: %d",
								   (int) AddCommentContainerResult.GetResultCode(),
								   AddCommentContainerResult.GetHttpResultCode());

					const csp::systems::StringResult InternalResult(AddCommentContainerResult.GetResultCode(),
																	AddCommentContainerResult.GetHttpResultCode());
					Callback(InternalResult);
				}
			};

			const auto UniqueAssetCollectionName
				= ConversationSystemHelpers::GetUniqueConversationContainerAssetCollectionName(CurrentSpace.Id, LoggedInUserId);
			const auto AssetSystem					 = csp::systems::SystemsManager::Get().GetAssetSystem();
			ConversationInfo DefaultConversationInfo = ConversationInfo();
			DefaultConversationInfo.UserDisplayName	 = GetProfileResult.GetProfile().DisplayName;
			DefaultConversationInfo.Message			 = Message;
			DefaultConversationInfo.Edited			 = false;
			DefaultConversationInfo.Resolved		 = false;
			DefaultConversationInfo.CameraPosition	 = SpaceTransform();
			AssetSystem->CreateAssetCollection(CurrentSpace.Id,
											   nullptr,
											   UniqueAssetCollectionName,
											   ConversationSystemHelpers::GenerateConversationAssetCollectionMetadata(DefaultConversationInfo),
											   csp::systems::EAssetCollectionType::COMMENT_CONTAINER,
											   nullptr,
											   AddCommentContainerCallback);
		}
	};
	UserSystem->GetProfileByUserId(LoggedInUserId, GetProfileCallback);
}

void ConversationSystem::AddMessageToConversation(const csp::common::String& ConversationId,
												  const csp::common::String& SenderDisplayName,
												  const csp::common::String& Message,
												  MessageResultCallback Callback)
{
	const auto SpaceSystem	= csp::systems::SystemsManager::Get().GetSpaceSystem();
	const auto CurrentSpace = SpaceSystem->GetCurrentSpace();
	assert(CurrentSpace.Id.IsEmpty() != true && "Enter a space before adding a message");

	const auto UserSystem = csp::systems::SystemsManager::Get().GetUserSystem();
	assert(UserSystem->GetLoginState().State == csp::systems::ELoginState::LoggedIn && "Log in before adding a message");
	const auto LoggedInUserId											= UserSystem->GetLoginState().UserId;
	const csp::multiplayer::MessageResultCallback MessageResultCallback = [=](const MessageResult& MessageResultCallbackResult)
	{
		const csp::multiplayer::MultiplayerConnection::CallbackHandler signalRCallback = [=](const bool& signalRCallbackResult)
		{
			if (!signalRCallbackResult)
			{
				CSP_LOG_ERROR_MSG("AddMessageToConversation: SignalR connection: Error");
				const csp::multiplayer::MessageResult InternalResult(csp::services::EResultCode::Failed,
																	 (uint16_t) csp::services::EResultCode::Failed);
				Callback(InternalResult);
			}
			Callback(MessageResultCallbackResult);
		};
		MultiPlayerConnection->SendNetworkEvent("ConversationSystem",
												{ReplicatedValue((int64_t) ConversationMessageType::NewMessage), ConversationId},
												signalRCallback);
	};

	StoreConversationMessage(ConversationId, CurrentSpace, LoggedInUserId, SenderDisplayName, Message, MessageResultCallback);
}

void ConversationSystem::GetMessagesFromConversation(const csp::common::String& ConversationId,
													 const csp::common::Optional<int>& ResultsSkipNumber,
													 const csp::common::Optional<int>& ResultsMaxNumber,
													 MessageCollectionResultCallback Callback)
{
	csp::systems::AssetCollectionsResultCallback GetMessagesCallback = [=](const csp::systems::AssetCollectionsResult& GetMessagesResult)
	{
		if (GetMessagesResult.GetResultCode() == csp::services::EResultCode::InProgress)
		{
			return;
		}

		if (GetMessagesResult.GetResultCode() == csp::services::EResultCode::Success)
		{
			MessageCollectionResult Result(GetMessagesResult.GetTotalCount());
			Result.FillMessageInfoCollection(GetMessagesResult.GetAssetCollections());
			Callback(Result);
		}
		else
		{
			CSP_LOG_FORMAT(csp::systems::LogLevel::Log,
						   "The retrieval of Message asset collections was not successful. ResCode: %d, HttpResCode: %d",
						   (int) GetMessagesResult.GetResultCode(),
						   GetMessagesResult.GetHttpResultCode());

			const MessageCollectionResult InternalResult(GetMessagesResult.GetResultCode(), GetMessagesResult.GetHttpResultCode());
			Callback(InternalResult);
		}
	};

	const auto AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();

	AssetSystem->GetAssetCollectionsByCriteria(nullptr,
											   ConversationId,
											   csp::systems::EAssetCollectionType::COMMENT,
											   nullptr,
											   nullptr,
											   ResultsSkipNumber,
											   ResultsMaxNumber,
											   GetMessagesCallback);
}

void ConversationSystem::GetMessage(const csp::common::String& MessageId, MessageResultCallback Callback)
{
	const csp::systems::AssetCollectionResultCallback GetMessageCallback = [=](const csp::systems::AssetCollectionResult& GetMessageResult)
	{
		if (GetMessageResult.GetResultCode() == csp::services::EResultCode::InProgress)
		{
			return;
		}

		if (GetMessageResult.GetResultCode() == csp::services::EResultCode::Success)
		{
			MessageResult Result;
			Result.FillMessageInfo(GetMessageResult.GetAssetCollection());
			Callback(Result);
		}
		else
		{
			CSP_LOG_FORMAT(csp::systems::LogLevel::Log,
						   "The retrieval of the Message asset collection was not successful. ResCode: %d, HttpResCode: %d",
						   (int) GetMessageResult.GetResultCode(),
						   GetMessageResult.GetHttpResultCode());

			const MessageResult InternalResult(GetMessageResult.GetResultCode(), GetMessageResult.GetHttpResultCode());
			Callback(InternalResult);
		}
	};

	const auto AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
	AssetSystem->GetAssetCollectionById(MessageId, GetMessageCallback);
}

void ConversationSystem::SetMessageInformation(const csp::common::String& MessageId, const MessageInfo& MessageData, MessageResultCallback Callback)
{
	const auto AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();

	csp::systems::AssetCollectionResultCallback GetMessageCallback = [=](const csp::systems::AssetCollectionResult& GetMessageResult)
	{
		csp::systems::AssetCollectionResultCallback GetUpdatedMessageCallback
			= [=](const csp::systems::AssetCollectionResult& GetUpdatedMessageResult)

		{
			if (GetUpdatedMessageResult.GetResultCode() == csp::services::EResultCode::InProgress)
			{
				return;
			}
			if (GetUpdatedMessageResult.GetResultCode() == csp::services::EResultCode::Success)
			{
				const csp::multiplayer::MultiplayerConnection::CallbackHandler signalRCallback = [=](const bool& signalRCallbackResult)
				{
					if (!signalRCallbackResult)
					{
						CSP_LOG_ERROR_MSG("UpdatedMessageInfo: SignalR connection: Error");
					}
					MessageResult Result;
					Result.FillMessageInfo(GetUpdatedMessageResult.GetAssetCollection());
					Callback(Result);
				};

				MultiPlayerConnection->SendNetworkEvent("ConversationSystem",
														{ReplicatedValue((int64_t) ConversationMessageType::MessageInformation), MessageId},
														signalRCallback);
			}
			else
			{
				CSP_LOG_FORMAT(csp::systems::LogLevel::Log,
							   "The Update of Message asset collections was not successful. ResCode: %d, HttpResCode: %d",
							   (int) GetMessageResult.GetResultCode(),
							   GetMessageResult.GetHttpResultCode());
				const csp::multiplayer::MessageResult InternalResult(GetMessageResult.GetResultCode(), GetMessageResult.GetHttpResultCode());
				Callback(InternalResult);
			}
		};
		if (GetMessageResult.GetResultCode() == csp::services::EResultCode::InProgress)
		{
			return;
		}
		if (GetMessageResult.GetResultCode() == csp::services::EResultCode::Success)
		{
			MessageInfo NewMessageData(MessageData);

			if (MessageData.Edited != true)
			{
				if (ConversationSystemHelpers::GetMessageInfoFromMessageAssetCollection(GetMessageResult.GetAssetCollection()).Message
					!= NewMessageData.Message)
				{
					NewMessageData.Edited = true;
				}
			}
			AssetSystem->UpdateAssetCollectionMetadata(GetMessageResult.GetAssetCollection(),
													   ConversationSystemHelpers::GenerateMessageAssetCollectionMetadata(NewMessageData),
													   GetUpdatedMessageCallback);
		}
		else
		{
			CSP_LOG_FORMAT(csp::systems::LogLevel::Log,
						   "The retrieval of Message asset collections was not successful. ResCode: %d, HttpResCode: %d",
						   (int) GetMessageResult.GetResultCode(),
						   GetMessageResult.GetHttpResultCode());
			const csp::multiplayer::MessageResult InternalResult(GetMessageResult.GetResultCode(), GetMessageResult.GetHttpResultCode());
			Callback(InternalResult);
		}
	};
	AssetSystem->GetAssetCollectionById(MessageId, GetMessageCallback);
}

void ConversationSystem::GetMessageInformation(const csp::common::String& MessageId, MessageResultCallback Callback)
{
	const auto AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();

	csp::systems::AssetCollectionResultCallback GetMessageCallback = [=](const csp::systems::AssetCollectionResult& GetMessageResult)
	{
		if (GetMessageResult.GetResultCode() == csp::services::EResultCode::InProgress)
		{
			return;
		}
		if (GetMessageResult.GetResultCode() == csp::services::EResultCode::Success)
		{
			MessageResult Result;
			Result.FillMessageInfo(GetMessageResult.GetAssetCollection());
			Callback(Result);
		}
	};

	AssetSystem->GetAssetCollectionById(MessageId, GetMessageCallback);
}

void ConversationSystem::DeleteConversation(const csp::common::String& ConversationId, csp::systems::NullResultCallback Callback)
{
	csp::systems::AssetCollectionsResultCallback GetMessagesCallback = [=](const csp::systems::AssetCollectionsResult& GetMessagesResult)
	{
		if (GetMessagesResult.GetResultCode() == csp::services::EResultCode::InProgress)
		{
			return;
		}

		if (GetMessagesResult.GetResultCode() == csp::services::EResultCode::Success)
		{
			const csp::systems::NullResultCallback DeleteMessagesCallback = [=](const csp::systems::NullResult& DeleteMessagesResult)
			{
				if (DeleteMessagesResult.GetResultCode() == csp::services::EResultCode::Failed)
				{
					CSP_LOG_FORMAT(csp::systems::LogLevel::Log,
								   "Not all Message asset collections were deleted. ResCode: %d, HttpResCode: %d",
								   (int) DeleteMessagesResult.GetResultCode(),
								   DeleteMessagesResult.GetHttpResultCode());
				}

				const csp::systems::NullResultCallback DeleteConversationAssetCollectionCallback
					= [=](const csp::systems::NullResult& DeleteConversationAssetCollectionResult)
				{
					if (DeleteConversationAssetCollectionResult.GetResultCode() == csp::services::EResultCode::InProgress)
					{
						return;
					}

					if (DeleteConversationAssetCollectionResult.GetResultCode() == csp::services::EResultCode::Failed)
					{
						CSP_LOG_FORMAT(csp::systems::LogLevel::Log,
									   "The deletion of the Conversation asset collection was not successful. ResCode: %d, HttpResCode: %d",
									   (int) DeleteConversationAssetCollectionResult.GetResultCode(),
									   DeleteConversationAssetCollectionResult.GetHttpResultCode());
					}

					Callback(DeleteConversationAssetCollectionResult);
				};

				// Delete also the conversation asset collection
				csp::systems::AssetCollection ConversationAssetCollection;
				ConversationAssetCollection.Id = ConversationId;

				DeleteMessages({ConversationAssetCollection}, DeleteConversationAssetCollectionCallback);
			};
			const csp::systems::NullResultCallback NullResultCallback = [=](const csp::systems::NullResult& NullResultCallbackResult)
			{
				const csp::multiplayer::MultiplayerConnection::CallbackHandler signalRCallback = [=](const bool& signalRCallbackResult)
				{
					if (!signalRCallbackResult)
					{
						CSP_LOG_ERROR_MSG("DeleteConversation: SignalR connection: Error");
					}
					Callback(NullResultCallbackResult);
				};
				MultiPlayerConnection->SendNetworkEvent("ConversationSystem",
														{ReplicatedValue((int64_t) ConversationMessageType::DeleteConversation), ConversationId},
														signalRCallback);
			};

			const auto Messages = GetMessagesResult.GetAssetCollections();
			DeleteMessages(Messages, NullResultCallback);
		}
		else
		{
			CSP_LOG_FORMAT(csp::systems::LogLevel::Log,
						   "The retrieval of Message asset collections was not successful. ResCode: %d, HttpResCode: %d",
						   (int) GetMessagesResult.GetResultCode(),
						   GetMessagesResult.GetHttpResultCode());

			const csp::systems::NullResult InternalResult(GetMessagesResult.GetResultCode(), GetMessagesResult.GetHttpResultCode());

			Callback(InternalResult);
		}
	};

	const auto AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();

	// Retrieve first the messages from this conversation
	AssetSystem->GetAssetCollectionsByCriteria(nullptr,
											   ConversationId,
											   csp::systems::EAssetCollectionType::COMMENT,
											   nullptr,
											   nullptr,
											   nullptr,
											   nullptr,
											   GetMessagesCallback);
}

void ConversationSystem::DeleteMessage(const csp::common::String& MessageId, csp::systems::NullResultCallback Callback)
{
	csp::systems::AssetCollection MessageAssetCollection;
	MessageAssetCollection.Id							= MessageId;
	const csp::systems::NullResultCallback NullCallback = [=](const csp::systems::NullResult& NullCallbackResult)
	{
		const csp::multiplayer::MultiplayerConnection::CallbackHandler signalRCallback = [=](const bool& signalRCallbackResult)
		{
			if (!signalRCallbackResult)
			{
				CSP_LOG_ERROR_MSG("DeleteMessage: SignalR connection: Error");
			}
			Callback(NullCallbackResult);
		};
		MultiPlayerConnection->SendNetworkEvent("ConversationSystem",
												{ReplicatedValue((int64_t) ConversationMessageType::DeleteMessage), MessageId},
												signalRCallback);
	};
	const auto AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
	AssetSystem->DeleteAssetCollection(MessageAssetCollection, NullCallback);
}

void ConversationSystem::SetConversationInformation(const csp::common::String& ConversationId,
													const ConversationInfo& ConversationData,
													ConversationResultCallback Callback)
{
	const auto AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();

	csp::systems::AssetCollectionResultCallback GetConversationCallback = [=](const csp::systems::AssetCollectionResult& GetConversationResult)
	{
		if (GetConversationResult.GetResultCode() == csp::services::EResultCode::InProgress)
		{
			return;
		}
		if (GetConversationResult.GetResultCode() == csp::services::EResultCode::Success)
		{
			csp::systems::ProfileResultCallback GetProfileCallback = [=](const csp::systems::ProfileResult& GetProfileResult)
			{
				if (GetProfileResult.GetResultCode() == csp::services::EResultCode::InProgress)
				{
					return;
				}
				if (GetProfileResult.GetResultCode() == csp::services::EResultCode::Failed)
				{
					const ConversationResult InternalResult(GetProfileResult.GetResultCode(), GetProfileResult.GetHttpResultCode());
					Callback(InternalResult);
				}
				if (GetProfileResult.GetResultCode() == csp::services::EResultCode::Success)
				{
					csp::systems::AssetCollectionResultCallback GetUpdatedConversationCallback
						= [=](const csp::systems::AssetCollectionResult& GetUpdatedConversationResult)

					{
						if (GetUpdatedConversationResult.GetResultCode() == csp::services::EResultCode::InProgress)
						{
							return;
						}
						if (GetUpdatedConversationResult.GetResultCode() == csp::services::EResultCode::Success)
						{
							const csp::multiplayer::MultiplayerConnection::CallbackHandler signalRCallback = [=](const bool& signalRCallbackResult)
							{
								if (signalRCallbackResult)
								{
									ConversationResult Result;
									Result.FillConversationInfo(GetUpdatedConversationResult.GetAssetCollection());
									Callback(Result);
								}
								else
								{
									CSP_LOG_ERROR_MSG("AddMessageToConversation: SignalR connection: Error");
									const csp::multiplayer::ConversationResult InternalResult(csp::services::EResultCode::Failed,
																							  (uint16_t) csp::services::EResultCode::Failed);
									Callback(InternalResult);
								}
							};

							MultiPlayerConnection->SendNetworkEvent(
								"ConversationSystem",
								{ReplicatedValue((int64_t) ConversationMessageType::ConversationInformation), ConversationId},
								signalRCallback);
						}
						else
						{
							CSP_LOG_FORMAT(csp::systems::LogLevel::Log,
										   "The Update of Conversation asset collections was not successful. ResCode: %d, HttpResCode: %d",
										   (int) GetConversationResult.GetResultCode(),
										   GetConversationResult.GetHttpResultCode());
							const csp::multiplayer::ConversationResult InternalResult(GetConversationResult.GetResultCode(),
																					  GetConversationResult.GetHttpResultCode());
							Callback(InternalResult);
						}
					};

					ConversationInfo NewConversationData(ConversationData);
					NewConversationData.UserDisplayName = GetProfileResult.GetProfile().DisplayName;
					if (!NewConversationData.Edited)
					{
						const auto CurrentConversationData
							= ConversationSystemHelpers::GetMessageInfoFromMessageAssetCollection(GetConversationResult.GetAssetCollection());
						NewConversationData.Edited = CurrentConversationData.Message != NewConversationData.Message;
					}

					AssetSystem->UpdateAssetCollectionMetadata(
						GetConversationResult.GetAssetCollection(),
						ConversationSystemHelpers::GenerateConversationAssetCollectionMetadata(NewConversationData),
						GetUpdatedConversationCallback);
				}
			};
			const auto UserSystem = csp::systems::SystemsManager::Get().GetUserSystem();
			UserSystem->GetProfileByUserId(UserSystem->GetLoginState().UserId, GetProfileCallback);
		}
		else
		{
			CSP_LOG_FORMAT(csp::systems::LogLevel::Log,
						   "The retrieval of Conversation asset collections was not successful. ResCode: %d, HttpResCode: %d",
						   (int) GetConversationResult.GetResultCode(),
						   GetConversationResult.GetHttpResultCode());
			const csp::multiplayer::ConversationResult InternalResult(GetConversationResult.GetResultCode(),
																	  GetConversationResult.GetHttpResultCode());
			Callback(InternalResult);
		}
	};

	AssetSystem->GetAssetCollectionById(ConversationId, GetConversationCallback);
}

void ConversationSystem::GetConversationInformation(const csp::common::String& ConversationId, ConversationResultCallback Callback)
{
	const auto AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();

	csp::systems::AssetCollectionResultCallback GetConversationCallback = [=](const csp::systems::AssetCollectionResult& GetConversationResult)
	{
		if (GetConversationResult.GetResultCode() == csp::services::EResultCode::InProgress)
		{
			return;
		}
		if (GetConversationResult.GetResultCode() == csp::services::EResultCode::Success)
		{
			ConversationResult Result;
			Result.FillConversationInfo(GetConversationResult.GetAssetCollection());
			Callback(Result);
		}
	};

	AssetSystem->GetAssetCollectionById(ConversationId, GetConversationCallback);
}

} // namespace csp::multiplayer
