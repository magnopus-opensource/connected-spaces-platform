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
#include "Awaitable.h"
#include "CSP/CSPFoundation.h"
#include "CSP/Common/Optional.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Multiplayer/SpaceEntitySystem.h"
#include "CSP/Multiplayer/conversation/ConversationSystem.h"
#include "CSP/Systems/Spaces/Space.h"
#include "CSP/Systems/Spaces/UserRoles.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "Multiplayer/SpaceEntityKeys.h"
#include "SpaceSystemTestHelpers.h"
#include "TestHelpers.h"
#include "UserSystemTestHelpers.h"

#include "gtest/gtest.h"

using namespace csp::multiplayer;

namespace
{

void OnConnect();
void OnDisconnect(bool ok);
void OnDelete();

std::atomic_bool IsTestComplete;
std::atomic_bool IsDisconnected;
std::atomic_bool IsReadyForUpdate;
MultiplayerConnection* Connection;
SpaceEntitySystem* EntitySystem;
SpaceEntity* TestUser;
SpaceEntity* TestObject;

int WaitForTestTimeoutCountMs;
const int WaitForTestTimeoutLimit	= 20000;
const int NumberOfEntityUpdateTicks = 5;
int ReceivedEntityUpdatesCount;

bool EventSent	   = false;
bool EventReceived = false;

ReplicatedValue ObjectFloatProperty;
ReplicatedValue ObjectBoolProperty;
ReplicatedValue ObjectIntProperty;
ReplicatedValue ObjectStringProperty;

bool RequestPredicate(const csp::systems::ResultBase& Result)
{
	return Result.GetResultCode() != csp::systems::EResultCode::InProgress;
}

csp::multiplayer::MessageInfo AddMessageToConversation(csp::multiplayer::ConversationSystem* ConvSystem,
													   const csp::common::String& ConversationId,
													   const csp::common::String& Message)
{
	auto [Result] = AWAIT_PRE(ConvSystem, AddMessageToConversation, RequestPredicate, ConversationId, Message);
	EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

	return Result.GetMessageInfo();
}

} // namespace

#if RUN_ALL_UNIT_TESTS || RUN_CONVERSATIONSYSTEM_TESTS || RUN_CONVERSATIONSYSTEM_CREATE_CONVERSATION_ID
CSP_PUBLIC_TEST(CSPEngine, ConversationSystemTests, CreateConversationId)
{
	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* Connection	 = SystemsManager.GetMultiplayerConnection();
	auto* EntitySystem	 = SystemsManager.GetSpaceEntitySystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	csp::common::String DefaultTestUserId;

	// Log in
	LogIn(UserSystem, DefaultTestUserId);

	const auto DefaultTestUserDisplayName = GetFullProfileByUserId(UserSystem, DefaultTestUserId).DisplayName;

	// Create space
	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

	EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

	EntitySystem->SetEntityCreatedCallback(
		[](csp::multiplayer::SpaceEntity* Entity)
		{
		});

	auto* ConvSystem = Connection->GetConversationSystem();
	csp::common::String ConversationId;

	auto [Result]  = AWAIT_PRE(ConvSystem, CreateConversation, RequestPredicate, "TestMessage");
	ConversationId = Result.GetValue();

	EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

	{
		auto [Result] = AWAIT_PRE(ConvSystem, GetConversationInformation, RequestPredicate, ConversationId);

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

		EXPECT_EQ(Result.GetConversationInfo().ConversationId, ConversationId);
		EXPECT_EQ(Result.GetConversationInfo().UserId, DefaultTestUserId);
		EXPECT_EQ(Result.GetConversationInfo().Message, "TestMessage");
		EXPECT_EQ(Result.GetConversationInfo().EditedTimestamp, "");
	}

	const auto DefaultConversationMessage = "this is a message from the tests world";
	csp::multiplayer::MessageInfo CreatedMessageInfo;
	csp::multiplayer::MessageInfo RetrievedMessageInfo;
	csp::common::String FirstMessageId;

	// Add message to Conversation
	{
		CreatedMessageInfo = AddMessageToConversation(ConvSystem, Result.GetValue(), DefaultConversationMessage);
		ConversationId	   = CreatedMessageInfo.ConversationId;
		FirstMessageId	   = CreatedMessageInfo.MessageId;
		EXPECT_EQ(ConversationId, Result.GetValue());
	}

	// Get message From Conversation
	{
		auto [Result] = AWAIT_PRE(ConvSystem, GetMessagesFromConversation, RequestPredicate, ConversationId, 0, 1);
		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

		auto Messages = Result.GetMessages();
		EXPECT_EQ(Messages.Size(), 1);
		EXPECT_EQ(Messages[0].Message, DefaultConversationMessage);
	}

	{
		auto [Result] = AWAIT(ConvSystem, DeleteConversation, ConversationId);
		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
	}

	SpaceSystem->ExitSpace([](const csp::systems::NullResult& Result){});

	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_CONVERSATIONSYSTEM_TESTS || RUN_CONVERSATIONSYSTEM_GET_MESSAGES_TEST
CSP_PUBLIC_TEST(CSPEngine, ConversationSystemTests, GetMessagesTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* Connection	 = SystemsManager.GetMultiplayerConnection();
	auto* EntitySystem	 = SystemsManager.GetSpaceEntitySystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	csp::common::String DefaultTestUserId;

	// Log in
	LogIn(UserSystem, DefaultTestUserId);

	// Create space
	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	// add the second test user to the space
	auto [Result] = AWAIT_PRE(SpaceSystem, InviteToSpace, RequestPredicate, Space.Id, AlternativeLoginEmail, true, "", "");
	EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

	auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

	EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

	EntitySystem->SetEntityCreatedCallback(
		[](csp::multiplayer::SpaceEntity* Entity)
		{
		});

	auto* ConvSystem = Connection->GetConversationSystem();

	csp::multiplayer::MessageInfo CreatedMessageInfo;
	csp::multiplayer::MessageInfo RetrievedMessageInfo;

	csp::common::String ConversationId;
	csp::common::String FirstMessageId;
	csp::common::String SecondMessageId;

	const auto DefaultConversationMessage = "this is a message from the tests world";
	auto [ResultConvo]					  = AWAIT_PRE(ConvSystem, CreateConversation, RequestPredicate, "TestMessage");
	ConversationId						  = ResultConvo.GetValue();

	EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

	{
		auto [Result] = AWAIT_PRE(ConvSystem, GetConversationInformation, RequestPredicate, ConversationId);

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
		EXPECT_EQ(Result.GetConversationInfo().UserId, DefaultTestUserId);
		EXPECT_EQ(Result.GetConversationInfo().Message, "TestMessage");
		EXPECT_EQ(Result.GetConversationInfo().EditedTimestamp, "");
	}

	{
		CreatedMessageInfo = AddMessageToConversation(ConvSystem, ResultConvo.GetValue(), DefaultConversationMessage);
		ConversationId	   = CreatedMessageInfo.ConversationId;
		FirstMessageId	   = CreatedMessageInfo.MessageId;
	}

	AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

	LogOut(UserSystem);

	// Log in with the second account
	csp::common::String SecondTestUserId;
	LogIn(UserSystem, SecondTestUserId, AlternativeLoginEmail, AlternativeLoginPassword);

	auto [EnterResult2] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);
	EXPECT_EQ(EnterResult2.GetResultCode(), csp::systems::EResultCode::Success);

	SecondMessageId = AddMessageToConversation(ConvSystem, ConversationId, DefaultConversationMessage).MessageId;

	// check that the second user can retrieve both added messages
	{
		auto [Result] = AWAIT_PRE(ConvSystem, GetMessage, RequestPredicate, FirstMessageId);
		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

		RetrievedMessageInfo = Result.GetMessageInfo();
		EXPECT_EQ(RetrievedMessageInfo.MessageId, FirstMessageId);
		EXPECT_EQ(RetrievedMessageInfo.UserId, DefaultTestUserId);
		EXPECT_EQ(RetrievedMessageInfo.ConversationId, ConversationId);
		EXPECT_EQ(RetrievedMessageInfo.Message, DefaultConversationMessage);

		auto [Result2] = AWAIT_PRE(ConvSystem, GetMessage, RequestPredicate, SecondMessageId);
		EXPECT_EQ(Result2.GetResultCode(), csp::systems::EResultCode::Success);

		RetrievedMessageInfo = Result2.GetMessageInfo();
		EXPECT_EQ(RetrievedMessageInfo.MessageId, SecondMessageId);
		EXPECT_EQ(RetrievedMessageInfo.UserId, SecondTestUserId);
		EXPECT_EQ(RetrievedMessageInfo.ConversationId, ConversationId);
		EXPECT_EQ(RetrievedMessageInfo.Message, DefaultConversationMessage);
	}

	// check that the second user can retrieve the messages from the conversation using pagination
	{
		auto [Result] = AWAIT_PRE(ConvSystem, GetMessagesFromConversation, RequestPredicate, ConversationId, 0, 1);
		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

		auto Messages = Result.GetMessages();
		EXPECT_EQ(Messages.Size(), 1);
		EXPECT_EQ(Result.GetTotalCount(), 2);

		const auto Message = Messages[0];
		EXPECT_FALSE(Message.MessageId.IsEmpty());
		EXPECT_FALSE(Message.UserId.IsEmpty());
		EXPECT_EQ(Message.ConversationId, ConversationId);
		EXPECT_EQ(Message.Message, DefaultConversationMessage);
	}

	AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

	LogOut(UserSystem);

	// Log in again with the default user
	LogIn(UserSystem, DefaultTestUserId);

	auto [EnterResult3] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

	EXPECT_EQ(EnterResult3.GetResultCode(), csp::systems::EResultCode::Success);

	// check that the default user can retrieve both added messages
	{
		auto [Result] = AWAIT_PRE(ConvSystem, GetMessage, RequestPredicate, FirstMessageId);
		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

		RetrievedMessageInfo = Result.GetMessageInfo();
		EXPECT_EQ(RetrievedMessageInfo.MessageId, FirstMessageId);
		EXPECT_EQ(RetrievedMessageInfo.UserId, DefaultTestUserId);
		EXPECT_EQ(RetrievedMessageInfo.ConversationId, ConversationId);
		EXPECT_EQ(RetrievedMessageInfo.Message, DefaultConversationMessage);

		auto [Result2] = AWAIT_PRE(ConvSystem, GetMessage, RequestPredicate, SecondMessageId);
		EXPECT_EQ(Result2.GetResultCode(), csp::systems::EResultCode::Success);

		RetrievedMessageInfo = Result2.GetMessageInfo();
		EXPECT_EQ(RetrievedMessageInfo.MessageId, SecondMessageId);
		EXPECT_EQ(RetrievedMessageInfo.UserId, SecondTestUserId);
		EXPECT_EQ(RetrievedMessageInfo.ConversationId, ConversationId);
		EXPECT_EQ(RetrievedMessageInfo.Message, DefaultConversationMessage);
	}

	// check that the default user can retrieve the messages from the conversation using pagination
	{
		auto [Result] = AWAIT_PRE(ConvSystem, GetMessagesFromConversation, RequestPredicate, ConversationId, 1, 1);
		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

		auto Messages = Result.GetMessages();
		EXPECT_EQ(Messages.Size(), 1);
		EXPECT_EQ(Result.GetTotalCount(), 2);

		const auto Message = Messages[0];
		EXPECT_FALSE(Message.MessageId.IsEmpty());
		EXPECT_FALSE(Message.UserId.IsEmpty());
		EXPECT_EQ(Message.ConversationId, ConversationId);
		EXPECT_EQ(Message.Message, DefaultConversationMessage);
	}

	{
		auto [Result] = AWAIT_PRE(ConvSystem, DeleteConversation, RequestPredicate, ConversationId);
		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
	}

	AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_CONVERSATIONSYSTEM_TESTS || RUN_CONVERSATIONSYSTEM_TWO_CONVERSATIONS_TEST
CSP_PUBLIC_TEST(CSPEngine, ConversationSystemTests, TwoConversationsTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* Connection	 = SystemsManager.GetMultiplayerConnection();
	auto* EntitySystem	 = SystemsManager.GetSpaceEntitySystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	csp::common::String UserId;

	// Log in
	LogIn(UserSystem, UserId);

	// Create space
	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	// add the second test user to the space
	auto [Result] = AWAIT_PRE(SpaceSystem, InviteToSpace, RequestPredicate, Space.Id, AlternativeLoginEmail, true, "", "");
	EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

	auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

	EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

	EntitySystem->SetEntityCreatedCallback(
		[](csp::multiplayer::SpaceEntity* Entity)
		{
		});

	auto* ConvSystem = Connection->GetConversationSystem();
	csp::multiplayer::MessageInfo CreatedMessageInfo;

	csp::common::String FirstConversationId;
	csp::common::String SecondConversationId;

	csp::common::String FirstMessageIdToBeDeleted;
	csp::common::String SecondMessageIdToBeDeleted;

	const auto DefaultConversationMessage = "this is a message from the tests world";

	auto [ResultConvo]	= AWAIT_PRE(ConvSystem, CreateConversation, RequestPredicate, "TestMessage");
	FirstConversationId = ResultConvo.GetValue();

	EXPECT_EQ(ResultConvo.GetResultCode(), csp::systems::EResultCode::Success);

	{
		auto [Result] = AWAIT_PRE(ConvSystem, GetConversationInformation, RequestPredicate, FirstConversationId);

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
		EXPECT_EQ(Result.GetConversationInfo().UserId, UserId);
		EXPECT_EQ(Result.GetConversationInfo().Message, "TestMessage");
		EXPECT_EQ(Result.GetConversationInfo().EditedTimestamp, "");
	}

	{
		CreatedMessageInfo = AddMessageToConversation(ConvSystem, ResultConvo.GetValue(), DefaultConversationMessage);

		EXPECT_EQ(CreatedMessageInfo.UserId, UserId);
		EXPECT_EQ(CreatedMessageInfo.Message, DefaultConversationMessage);

		FirstConversationId = CreatedMessageInfo.ConversationId;

		std::cerr << "Conversation created. Id: " << FirstConversationId.c_str() << std::endl;
		std::cerr << "Message with Id: " << CreatedMessageInfo.MessageId << " was added by User " << CreatedMessageInfo.UserId
				  << " to conversation: Id: " << FirstConversationId.c_str() << std::endl;
	}

	{
		CreatedMessageInfo = AddMessageToConversation(ConvSystem, FirstConversationId, DefaultConversationMessage);

		EXPECT_EQ(CreatedMessageInfo.UserId, UserId);
		EXPECT_EQ(CreatedMessageInfo.Message, DefaultConversationMessage);
		EXPECT_EQ(CreatedMessageInfo.ConversationId, FirstConversationId);

		std::cerr << "Message with Id: " << CreatedMessageInfo.MessageId << " was added by User " << CreatedMessageInfo.UserId
				  << " to conversation: Id: " << FirstConversationId.c_str() << std::endl;

		FirstMessageIdToBeDeleted = CreatedMessageInfo.MessageId;
	}

	{
		CreatedMessageInfo = AddMessageToConversation(ConvSystem, FirstConversationId, DefaultConversationMessage);

		EXPECT_EQ(CreatedMessageInfo.UserId, UserId);
		EXPECT_EQ(CreatedMessageInfo.Message, DefaultConversationMessage);
		EXPECT_EQ(CreatedMessageInfo.ConversationId, FirstConversationId);

		std::cerr << "Message with Id: " << CreatedMessageInfo.MessageId << " was added by User " << CreatedMessageInfo.UserId
				  << " to conversation: Id: " << FirstConversationId.c_str() << std::endl;
	}

	SpaceSystem->ExitSpace([](const csp::systems::NullResult& Result){});

	LogOut(UserSystem);

	// Log in with the second account
	csp::common::String SecondTestUserId;
	LogIn(UserSystem, SecondTestUserId, AlternativeLoginEmail, AlternativeLoginPassword);

	auto [EnterResult2] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

	EXPECT_EQ(EnterResult2.GetResultCode(), csp::systems::EResultCode::Success);

	EntitySystem->SetEntityCreatedCallback(
		[](csp::multiplayer::SpaceEntity* Entity)
		{
		});

	ConvSystem = Connection->GetConversationSystem();

	// Add messages to the previous conversation
	{
		CreatedMessageInfo = AddMessageToConversation(ConvSystem, FirstConversationId, DefaultConversationMessage);

		EXPECT_EQ(CreatedMessageInfo.UserId, SecondTestUserId);
		EXPECT_EQ(CreatedMessageInfo.Message, DefaultConversationMessage);
		EXPECT_EQ(CreatedMessageInfo.ConversationId, FirstConversationId);

		std::cerr << "Message with Id: " << CreatedMessageInfo.MessageId << " was added by User " << CreatedMessageInfo.UserId
				  << " to conversation: Id: " << FirstConversationId.c_str() << std::endl;
	}

	{
		CreatedMessageInfo = AddMessageToConversation(ConvSystem, FirstConversationId, DefaultConversationMessage);

		EXPECT_EQ(CreatedMessageInfo.UserId, SecondTestUserId);
		EXPECT_EQ(CreatedMessageInfo.Message, DefaultConversationMessage);
		EXPECT_EQ(CreatedMessageInfo.ConversationId, FirstConversationId);

		std::cerr << "Message with Id: " << CreatedMessageInfo.MessageId << " was added by User " << CreatedMessageInfo.UserId
				  << " to conversation: Id: " << FirstConversationId.c_str() << std::endl;
	}

	auto [ResultConvo2]	 = AWAIT_PRE(ConvSystem, CreateConversation, RequestPredicate, "TestMessage");
	SecondConversationId = ResultConvo2.GetValue();

	EXPECT_EQ(ResultConvo2.GetResultCode(), csp::systems::EResultCode::Success);

	{
		auto [Result] = AWAIT_PRE(ConvSystem, GetConversationInformation, RequestPredicate, SecondConversationId);

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
		EXPECT_EQ(Result.GetConversationInfo().UserId, SecondTestUserId);
		EXPECT_EQ(Result.GetConversationInfo().Message, "TestMessage");
		EXPECT_EQ(Result.GetConversationInfo().EditedTimestamp, "");
	}

	// Create a new conversation
	{
		CreatedMessageInfo	 = AddMessageToConversation(ConvSystem, ResultConvo2.GetValue(), DefaultConversationMessage);
		SecondConversationId = CreatedMessageInfo.ConversationId;

		EXPECT_EQ(CreatedMessageInfo.UserId, SecondTestUserId);
		EXPECT_EQ(CreatedMessageInfo.Message, DefaultConversationMessage);

		std::cerr << "Conversation created. Id: " << SecondConversationId.c_str() << std::endl;
		std::cerr << "Message with Id: " << CreatedMessageInfo.MessageId << " was added by User " << CreatedMessageInfo.UserId
				  << " to conversation: Id: " << SecondConversationId.c_str() << std::endl;

		SecondMessageIdToBeDeleted = CreatedMessageInfo.MessageId;
	}

	// Retrieve all messages from first conversation
	{
		auto [Result] = AWAIT_PRE(ConvSystem, GetMessagesFromConversation, RequestPredicate, FirstConversationId, nullptr, nullptr);

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

		const auto& Messages = Result.GetMessages();

		EXPECT_EQ(Messages.Size(), 5);
		EXPECT_EQ(Result.GetTotalCount(), 5);
	}

	// Delete one message from first conversation
	{
		auto [Result] = AWAIT_PRE(ConvSystem, DeleteMessage, RequestPredicate, FirstMessageIdToBeDeleted);

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
	}

	// Retrieve again remaining messages from first conversation
	{
		auto [Result] = AWAIT_PRE(ConvSystem, GetMessagesFromConversation, RequestPredicate, FirstConversationId, nullptr, nullptr);

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

		const auto& Messages = Result.GetMessages();

		EXPECT_EQ(Messages.Size(), 4);
		EXPECT_EQ(Result.GetTotalCount(), 4);
	}

	// Delete first conversation entirely
	{
		auto [Result] = AWAIT_PRE(ConvSystem, DeleteConversation, RequestPredicate, FirstConversationId);

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
	}

	// Retrieve all messages from second conversation
	{
		auto [Result] = AWAIT_PRE(ConvSystem, GetMessagesFromConversation, RequestPredicate, SecondConversationId, nullptr, nullptr);

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

		const auto& Messages = Result.GetMessages();

		EXPECT_EQ(Messages.Size(), 1);
		EXPECT_EQ(Result.GetTotalCount(), 1);
	}

	// Delete the only message from the second conversation
	{
		auto [Result] = AWAIT_PRE(ConvSystem, DeleteMessage, RequestPredicate, SecondMessageIdToBeDeleted);

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
	}

	// Retrieve the messages from the second conversation
	{
		auto [Result] = AWAIT_PRE(ConvSystem, GetMessagesFromConversation, RequestPredicate, FirstConversationId, nullptr, nullptr);

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

		const auto& Messages = Result.GetMessages();

		EXPECT_EQ(Messages.Size(), 0);
		EXPECT_EQ(Result.GetTotalCount(), 0);
	}

	// Delete second conversation entirely even if it doesn't contain messages anymore
	{
		auto [Result] = AWAIT_PRE(ConvSystem, DeleteConversation, RequestPredicate, SecondConversationId);

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
	}

	SpaceSystem->ExitSpace([](const csp::systems::NullResult& Result){});

	LogOut(UserSystem);

	// Log in with the space creator in order to delete it
	LogIn(UserSystem, UserId);

	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_CONVERSATIONSYSTEM_TESTS || RUN_CONVERSATION_NEWMESSAGE_NETWORKEVENT_TEST
CSP_PUBLIC_TEST(CSPEngine, ConversationSystemTests, ConversationNewMessageNetworkEventTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* AssetSystem	 = SystemsManager.GetAssetSystem();
	auto* Connection	 = SystemsManager.GetMultiplayerConnection();
	auto* EntitySystem	 = SystemsManager.GetSpaceEntitySystem();

	const char* TestSpaceName			= "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription	= "OLY-UNITTEST-SPACEDESC-REWIND";
	const char* TestAssetCollectionName = "OLY-UNITTEST-ASSETCOLLECTION-REWIND";
	const char* TestAssetName			= "OLY-UNITTEST-ASSET-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	char UniqueAssetCollectionName[256];
	SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

	char UniqueAssetName[256];
	SPRINTF(UniqueAssetName, "%s-%s", TestAssetName, GetUniqueString().c_str());

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Create space
	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

	EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

	EntitySystem->SetEntityCreatedCallback(
		[](csp::multiplayer::SpaceEntity* Entity)
		{
		});

	// Setup Asset callback
	bool ConversationNewMessagecallbackCalled = false;
	csp::common::String ConversationId;

	auto ConversationNewMessageCallback = [&ConversationNewMessagecallbackCalled, &ConversationId](const ConversationSystemParams& Info)
	{
		if (ConversationNewMessagecallbackCalled)
		{
			return;
		}
		EXPECT_EQ(Info.MessageType, ConversationMessageType::NewMessage);
		EXPECT_EQ(Info.MessageValue, ConversationId);
		ConversationNewMessagecallbackCalled = true;
	};

	Connection->SetConversationSystemCallback(ConversationNewMessageCallback);

	ConversationSystem* ConversationSystem = Connection->GetConversationSystem();

	auto [Result]  = AWAIT_PRE(ConversationSystem, CreateConversation, RequestPredicate, "TestMessage");
	ConversationId = Result.GetValue();

	EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

	{
		auto [Result] = AWAIT_PRE(ConversationSystem, GetConversationInformation, RequestPredicate, ConversationId);

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
		EXPECT_EQ(Result.GetConversationInfo().UserId, UserId);
		EXPECT_EQ(Result.GetConversationInfo().Message, "TestMessage");
		EXPECT_EQ(Result.GetConversationInfo().EditedTimestamp, "");
	}

	{
		auto [Result] = AWAIT(ConversationSystem, AddMessageToConversation, ConversationId, "Test");

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
	}

	// Generate Networkevent as SendNetworkEvent doesnt fire sender callback
	Connection->SendNetworkEventToClient("ConversationSystem",
										 {ReplicatedValue((int64_t) ConversationMessageType::NewMessage), ConversationId},
										 Connection->GetClientId(),
										 [](ErrorCode Error)
										 {
											 ASSERT_EQ(Error, ErrorCode::None);
										 });

	// Wait for message
	auto Start		 = std::chrono::steady_clock::now();
	auto Current	 = std::chrono::steady_clock::now();
	int64_t TestTime = 0;

	while (!ConversationNewMessagecallbackCalled && TestTime < 20)
	{
		std::this_thread::sleep_for(50ms);

		Current	 = std::chrono::steady_clock::now();
		TestTime = std::chrono::duration_cast<std::chrono::seconds>(Current - Start).count();
	}

	EXPECT_TRUE(ConversationNewMessagecallbackCalled);

	{
		auto [Result] = AWAIT(ConversationSystem, DeleteConversation, ConversationId);

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
	}

	SpaceSystem->ExitSpace([](const csp::systems::NullResult& Result){});

	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_CONVERSATIONSYSTEM_TESTS || RUN_CONVERSATION_DELETEMESSAGE_NETWORKEVENT_TEST
CSP_PUBLIC_TEST(CSPEngine, ConversationSystemTests, ConversationDeleteMessageNetworkEventTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* AssetSystem	 = SystemsManager.GetAssetSystem();
	auto* Connection	 = SystemsManager.GetMultiplayerConnection();
	auto* EntitySystem	 = SystemsManager.GetSpaceEntitySystem();

	const char* TestSpaceName			= "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription	= "OLY-UNITTEST-SPACEDESC-REWIND";
	const char* TestAssetCollectionName = "OLY-UNITTEST-ASSETCOLLECTION-REWIND";
	const char* TestAssetName			= "OLY-UNITTEST-ASSET-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	char UniqueAssetCollectionName[256];
	SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

	char UniqueAssetName[256];
	SPRINTF(UniqueAssetName, "%s-%s", TestAssetName, GetUniqueString().c_str());

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Create space
	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

	EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

	EntitySystem->SetEntityCreatedCallback(
		[](csp::multiplayer::SpaceEntity* Entity)
		{
		});

	// Setup Asset callback
	bool ConversationDeleteMessagecallbackCalled = false;
	csp::common::String ConversationId;
	csp::common::String MessageId;

	auto ConversationDeleteMessageCallback = [&ConversationDeleteMessagecallbackCalled, &MessageId](const ConversationSystemParams& Info)
	{
		if (Info.MessageType == ConversationMessageType::DeleteMessage)
		{
			if (ConversationDeleteMessagecallbackCalled)
			{
				return;
			}

			EXPECT_EQ(Info.MessageValue, MessageId);
			ConversationDeleteMessagecallbackCalled = true;
		}
	};

	ConversationSystem* ConversationSystem = Connection->GetConversationSystem();
	auto [Result]						   = AWAIT_PRE(ConversationSystem, CreateConversation, RequestPredicate, "TestMessage");
	ConversationId						   = Result.GetValue();

	EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

	{
		auto [Result] = AWAIT_PRE(ConversationSystem, GetConversationInformation, RequestPredicate, ConversationId);

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
		EXPECT_EQ(Result.GetConversationInfo().UserId, UserId);
		EXPECT_EQ(Result.GetConversationInfo().Message, "TestMessage");
		EXPECT_EQ(Result.GetConversationInfo().EditedTimestamp, "");
	}

	{
		auto [Result] = AWAIT(ConversationSystem, AddMessageToConversation, ConversationId, "Test");

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

		MessageId = Result.GetMessageInfo().MessageId;
	}

	{
		Connection->SetConversationSystemCallback(ConversationDeleteMessageCallback);
		auto [Result] = AWAIT(ConversationSystem, DeleteMessage, MessageId);

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
	}

	// Generate Networkevent as SendNetworkEvent doesnt fire sender callback
	Connection->SendNetworkEventToClient("ConversationSystem",
										 {ReplicatedValue((int64_t) ConversationMessageType::DeleteMessage), MessageId},
										 Connection->GetClientId(),
										 [](ErrorCode Error)
										 {
											 ASSERT_EQ(Error, ErrorCode::None);
										 });

	// Wait for message
	auto Start		 = std::chrono::steady_clock::now();
	auto Current	 = std::chrono::steady_clock::now();
	int64_t TestTime = 0;

	while (!ConversationDeleteMessagecallbackCalled && TestTime < 20)
	{
		std::this_thread::sleep_for(50ms);

		Current	 = std::chrono::steady_clock::now();
		TestTime = std::chrono::duration_cast<std::chrono::seconds>(Current - Start).count();
	}

	EXPECT_TRUE(ConversationDeleteMessagecallbackCalled);

	{
		auto [Result] = AWAIT(ConversationSystem, DeleteConversation, ConversationId);

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
	}

	SpaceSystem->ExitSpace([](const csp::systems::NullResult& Result){});

	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_CONVERSATIONSYSTEM_TESTS || RUN_CONVERSATION_DELETECONVERSATION_NETWORKEVENT_TEST
CSP_PUBLIC_TEST(CSPEngine, ConversationSystemTests, ConversationDeleteConversationNetworkEventTest)
{
	SetRandSeed();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* AssetSystem	 = SystemsManager.GetAssetSystem();
	auto* Connection	 = SystemsManager.GetMultiplayerConnection();
	auto* EntitySystem	 = SystemsManager.GetSpaceEntitySystem();

	const char* TestSpaceName			= "OLY-UNITTEST-SPACE-REWIND";
	const char* TestSpaceDescription	= "OLY-UNITTEST-SPACEDESC-REWIND";
	const char* TestAssetCollectionName = "OLY-UNITTEST-ASSETCOLLECTION-REWIND";
	const char* TestAssetName			= "OLY-UNITTEST-ASSET-REWIND";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	char UniqueAssetCollectionName[256];
	SPRINTF(UniqueAssetCollectionName, "%s-%s", TestAssetCollectionName, GetUniqueString().c_str());

	char UniqueAssetName[256];
	SPRINTF(UniqueAssetName, "%s-%s", TestAssetName, GetUniqueString().c_str());

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Create space
	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

	EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

	EntitySystem->SetEntityCreatedCallback(
		[](csp::multiplayer::SpaceEntity* Entity)
		{
		});

	// Setup Asset callback
	bool ConversationDeleteConversationcallbackCalled = false;
	csp::common::String ConversationId;
	csp::common::String MessageId;

	auto ConversationDeleteConversationCallback
		= [&ConversationDeleteConversationcallbackCalled, &ConversationId](const ConversationSystemParams& Info)
	{
		if (Info.MessageType == ConversationMessageType::DeleteConversation)
		{
			if (ConversationDeleteConversationcallbackCalled)
			{
				return;
			}

			EXPECT_EQ(Info.MessageValue, ConversationId);

			ConversationDeleteConversationcallbackCalled = true;
		}
	};

	ConversationSystem* ConversationSystem = Connection->GetConversationSystem();

	auto [Result]  = AWAIT_PRE(ConversationSystem, CreateConversation, RequestPredicate, "TestMessage");
	ConversationId = Result.GetValue();

	EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

	{
		auto [Result] = AWAIT_PRE(ConversationSystem, GetConversationInformation, RequestPredicate, ConversationId);

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
		EXPECT_EQ(Result.GetConversationInfo().UserId, UserId);
		EXPECT_EQ(Result.GetConversationInfo().Message, "TestMessage");
		EXPECT_EQ(Result.GetConversationInfo().EditedTimestamp, "");
	}

	{
		auto [Result] = AWAIT(ConversationSystem, AddMessageToConversation, ConversationId, "Test");

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

		MessageId = Result.GetMessageInfo().MessageId;
	}

	{
		Connection->SetConversationSystemCallback(ConversationDeleteConversationCallback);
		auto [Result] = AWAIT(ConversationSystem, DeleteConversation, ConversationId);

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
	}

	// Generate Networkevent as SendNetworkEvent doesnt fire sender callback
	Connection->SendNetworkEventToClient("ConversationSystem",
										 {ReplicatedValue((int64_t) ConversationMessageType::DeleteConversation), ConversationId},
										 Connection->GetClientId(),
										 [](ErrorCode Error)
										 {
											 ASSERT_EQ(Error, ErrorCode::None);
										 });

	// Wait for message
	auto Start		 = std::chrono::steady_clock::now();
	auto Current	 = std::chrono::steady_clock::now();
	int64_t TestTime = 0;

	while (!ConversationDeleteConversationcallbackCalled && TestTime < 20)
	{
		std::this_thread::sleep_for(50ms);

		Current	 = std::chrono::steady_clock::now();
		TestTime = std::chrono::duration_cast<std::chrono::seconds>(Current - Start).count();
	}

	EXPECT_TRUE(ConversationDeleteConversationcallbackCalled);

	SpaceSystem->ExitSpace([](const csp::systems::NullResult& Result){});

	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);

	// Log out
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_CONVERSATIONSYSTEM_TESTS || RUN_CONVERSATIONSYSTEM_UPDATE_CONVERSATION_INFO
CSP_PUBLIC_TEST(CSPEngine, ConversationSystemTests, UpdateConversationInfo)
{
	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* Connection	 = SystemsManager.GetMultiplayerConnection();
	auto* EntitySystem	 = SystemsManager.GetSpaceEntitySystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-OKO";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-OKO";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Create space
	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);


	// Setup Asset callback
	bool ConversationConversationInfocallbackCalled = false;
	csp::common::String ConversationId;
	csp::common::String MessageId;

	auto ConversationConversationInformationCallback
		= [&ConversationConversationInfocallbackCalled, &ConversationId](const ConversationSystemParams& Info)
	{
		if (ConversationConversationInfocallbackCalled)
		{
			return;
		}
		EXPECT_EQ(Info.MessageType, ConversationMessageType::ConversationInformation);
		EXPECT_EQ(Info.MessageValue, ConversationId);
		ConversationConversationInfocallbackCalled = true;
	};

	auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

	EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

	EntitySystem->SetEntityCreatedCallback(
		[](csp::multiplayer::SpaceEntity* Entity)
		{
		});

	auto* ConvSystem = Connection->GetConversationSystem();

	auto [Result]  = AWAIT_PRE(ConvSystem, CreateConversation, RequestPredicate, "TestMessage");
	ConversationId = Result.GetValue();

	EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

	{
		auto [Result] = AWAIT_PRE(ConvSystem, GetConversationInformation, RequestPredicate, ConversationId);

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
		EXPECT_EQ(Result.GetConversationInfo().UserId, UserId);
		EXPECT_EQ(Result.GetConversationInfo().Message, "TestMessage");
		EXPECT_EQ(Result.GetConversationInfo().EditedTimestamp, "");
	}

	Connection->SetConversationSystemCallback(ConversationConversationInformationCallback);

	{
		auto [Result] = AWAIT_PRE(ConvSystem, GetConversationInformation, RequestPredicate, ConversationId);

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
		EXPECT_EQ(Result.GetConversationInfo().UserId, UserId);
		EXPECT_EQ(Result.GetConversationInfo().Message, "TestMessage");
		EXPECT_EQ(Result.GetConversationInfo().EditedTimestamp, "");
	}

	{
		MessageInfo NewData = MessageInfo();
		NewData.Message		   = "TestMessage1";
		NewData.IsConversation = true;

		auto [Result] = AWAIT_PRE(ConvSystem, SetConversationInformation, RequestPredicate, ConversationId, NewData);

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
		EXPECT_EQ(Result.GetConversationInfo().UserId, UserId);
		EXPECT_EQ(Result.GetConversationInfo().Message, "TestMessage1");
		EXPECT_EQ(Result.GetConversationInfo().EditedTimestamp, "");
	}

	// Generate Networkevent as SendNetworkEvent doesnt fire sender callback
	Connection->SendNetworkEventToClient("ConversationSystem",
										 {ReplicatedValue((int64_t) ConversationMessageType::ConversationInformation), ConversationId},
										 Connection->GetClientId(),
										 [](ErrorCode Error)
										 {
											 ASSERT_EQ(Error, ErrorCode::None);
										 });

	{
		auto [Result] = AWAIT(ConvSystem, DeleteConversation, ConversationId);

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
	}

	// Wait for message
	auto Start		 = std::chrono::steady_clock::now();
	auto Current	 = std::chrono::steady_clock::now();
	int64_t TestTime = 0;

	while (!ConversationConversationInfocallbackCalled && TestTime < 20)
	{
		std::this_thread::sleep_for(50ms);

		Current	 = std::chrono::steady_clock::now();
		TestTime = std::chrono::duration_cast<std::chrono::seconds>(Current - Start).count();
	}

	EXPECT_TRUE(ConversationConversationInfocallbackCalled);

	SpaceSystem->ExitSpace([](const csp::systems::NullResult& Result){});

	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);
	LogOut(UserSystem);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_CONVERSATIONSYSTEM_TESTS || RUN_CONVERSATIONSYSTEM_UPDATE_MESSAGE_INFO
CSP_PUBLIC_TEST(CSPEngine, ConversationSystemTests, UpdateMessageInfo)
{
	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();
	auto* SpaceSystem	 = SystemsManager.GetSpaceSystem();
	auto* Connection	 = SystemsManager.GetMultiplayerConnection();
	auto* EntitySystem	 = SystemsManager.GetSpaceEntitySystem();

	const char* TestSpaceName		 = "OLY-UNITTEST-SPACE-OKO";
	const char* TestSpaceDescription = "OLY-UNITTEST-SPACEDESC-OKO";

	char UniqueSpaceName[256];
	SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Create space
	csp::systems::Space Space;
	CreateSpace(SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, Space);

	// Setup Asset callback
	bool ConversationMessageInfoCallbackCalled = false;
	csp::common::String ConversationId;
	csp::common::String MessageId;

	auto ConversationMessageInformationCallback = [&ConversationMessageInfoCallbackCalled, &MessageId](const ConversationSystemParams& Info)
	{
		if (ConversationMessageInfoCallbackCalled || Info.MessageType == ConversationMessageType::NewMessage)
		{
			return;
		}

		EXPECT_EQ(Info.MessageType, ConversationMessageType::MessageInformation);
		EXPECT_EQ(Info.MessageValue, MessageId);

		ConversationMessageInfoCallbackCalled = true;
	};

	auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id);

	EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

	EntitySystem->SetEntityCreatedCallback(
		[](csp::multiplayer::SpaceEntity* Entity)
		{
		});

	auto* ConvSystem = Connection->GetConversationSystem();

	auto [Result]  = AWAIT_PRE(ConvSystem, CreateConversation, RequestPredicate, "TestMessage");
	ConversationId = Result.GetValue();

	EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

	{
		auto [Result] = AWAIT_PRE(ConvSystem, GetConversationInformation, RequestPredicate, ConversationId);

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
		EXPECT_EQ(Result.GetConversationInfo().UserId, UserId);
		EXPECT_EQ(Result.GetConversationInfo().Message, "TestMessage");
		EXPECT_EQ(Result.GetConversationInfo().EditedTimestamp, "");
	}

	Connection->SetConversationSystemCallback(ConversationMessageInformationCallback);

	{
		auto [Result] = AWAIT_PRE(ConvSystem, AddMessageToConversation, RequestPredicate, ConversationId, "test");

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);

		MessageId = Result.GetMessageInfo().MessageId;

		EXPECT_EQ(Result.GetMessageInfo().EditedTimestamp, "");
	}

	{
		auto [Result] = AWAIT_PRE(ConvSystem, GetMessageInformation, RequestPredicate, MessageId);

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
		EXPECT_EQ(Result.GetMessageInfo().EditedTimestamp, "");
	}

	{
		MessageInfo NewData = MessageInfo();
		NewData.Message		= "NewTest";

		auto [Result] = AWAIT_PRE(ConvSystem, SetMessageInformation, RequestPredicate, MessageId, NewData);

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
		EXPECT_EQ(Result.GetMessageInfo().EditedTimestamp, "");
		EXPECT_EQ(Result.GetMessageInfo().Message, NewData.Message);
	}

	// Generate Networkevent as SendNetworkEvent doesnt fire sender callback
	Connection->SendNetworkEventToClient("ConversationSystem",
										 {ReplicatedValue((int64_t) ConversationMessageType::MessageInformation), MessageId},
										 Connection->GetClientId(),
										 [](ErrorCode Error)
										 {
											 ASSERT_EQ(Error, ErrorCode::None);
										 });

	{
		auto [Result] = AWAIT(ConvSystem, DeleteConversation, ConversationId);

		EXPECT_EQ(Result.GetResultCode(), csp::systems::EResultCode::Success);
	}

	// Wait for message
	auto Start		 = std::chrono::steady_clock::now();
	auto Current	 = std::chrono::steady_clock::now();
	int64_t TestTime = 0;

	while (!ConversationMessageInfoCallbackCalled && TestTime < 20)
	{
		std::this_thread::sleep_for(50ms);

		Current	 = std::chrono::steady_clock::now();
		TestTime = std::chrono::duration_cast<std::chrono::seconds>(Current - Start).count();
	}

	EXPECT_TRUE(ConversationMessageInfoCallbackCalled);

	SpaceSystem->ExitSpace([](const csp::systems::NullResult& Result){});

	// Delete space
	DeleteSpace(SpaceSystem, Space.Id);
	LogOut(UserSystem);
}
#endif