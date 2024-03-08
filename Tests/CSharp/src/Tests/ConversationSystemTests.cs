#define RUN_ALL_UNIT_TESTS
using System.Threading;

using Common = Csp.Common;
using Services = Csp.Systems;
using Systems = Csp.Systems;
using Multiplayer = Csp.Multiplayer;

using CSharpTests;
using static CSharpTests.TestHelper;


namespace CSPEngine
{
    static class ConversationSystemTests
    {
        public static Multiplayer.MessageInfo AddMessageToConversation(Multiplayer.ConversationSystem convSystem, string ConversationId, string SenderDisplayName, string Message)
        {
            var result = convSystem.AddMessageToConversation(ConversationId, SenderDisplayName, Message).Result;

            Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);

            return result.GetMessageInfo();
        }


#if RUN_ALL_UNIT_TESTS || RUN_CONVERSATIONSYSTEM_TESTS || RUN_CONVERSATIONSYSTEM_TWO_CONVERSATIONS_TEST
        [Test]
        public static void TwoConversationsTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            // Log in
            var defaultTestUserId = userSystem.TestLogIn(pushCleanupFunction: false);

            // Create space
            using var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null, pushCleanupFunction: false);

            // Add the second test user to the space
            {
                using var result = spaceSystem.InviteToSpace(space.Id, UserSystemTests.AlternativeLoginEmail, true, "", "").Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
            }

            // Add self to space and fire EnterSpace event
            {
                using var result = spaceSystem.EnterSpace(space.Id).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);
            }

            var systemsManager = Systems.SystemsManager.Get();
            var connection = systemsManager.GetMultiplayerConnection();
            var entitySystem = systemsManager.GetSpaceEntitySystem();

            entitySystem.OnEntityCreated += (s, e) => { };

            var conversationSystem = connection.GetConversationSystem();

            string firstConversationId;
            string secondConversationId;

            string firstMessageIdToBeDeleted;
            string secondMessageIdToBeDeleted;

            var defaultTestUserDisplayName = UserSystemTests.GetFullProfileByUserId(userSystem, defaultTestUserId).DisplayName;
            var defaultConversationMessage = "this is a message from the C# tests world";

            // Add message to the first conversation
            {
                var newConversationId = conversationSystem.CreateConversation("TestMessage").Result;

                Assert.AreEqual(newConversationId.GetResultCode(), Services.EResultCode.Success);

                using var result = conversationSystem.GetConversationInformation(newConversationId.GetValue()).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
                Assert.AreEqual(result.GetConversationInfo().ConversationId, newConversationId.GetValue());
                Assert.AreEqual(result.GetConversationInfo().UserID, defaultTestUserId);
                Assert.AreEqual(result.GetConversationInfo().UserDisplayName, defaultTestUserDisplayName);
                Assert.AreEqual(result.GetConversationInfo().Message, "TestMessage");
                Assert.IsFalse(result.GetConversationInfo().Edited);
                Assert.IsFalse(result.GetConversationInfo().Resolved);

                var defaultTransform = new Multiplayer.SpaceTransform();

                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Position.X, defaultTransform.Position.X);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Position.Y, defaultTransform.Position.Y);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Position.Z, defaultTransform.Position.Z);

                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.W, defaultTransform.Rotation.W);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.X, defaultTransform.Rotation.X);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.Y, defaultTransform.Rotation.Y);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.Z, defaultTransform.Rotation.Z);

                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Scale.X, defaultTransform.Scale.X);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Scale.Y, defaultTransform.Scale.Y);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Scale.Z, defaultTransform.Scale.Z);

                using var createdMessageInfo = AddMessageToConversation(conversationSystem, newConversationId.GetValue(), defaultTestUserDisplayName, defaultConversationMessage);

                Assert.AreEqual(createdMessageInfo.UserID, defaultTestUserId);
                Assert.AreEqual(createdMessageInfo.Message, defaultConversationMessage);
                Assert.AreEqual(createdMessageInfo.UserDisplayName, defaultTestUserDisplayName);

                firstConversationId = createdMessageInfo.ConversationId;

                LogDebug($"Conversation created. Id: {firstConversationId}");
                LogDebug($"Message with Id: {createdMessageInfo.Id} was added by {defaultTestUserDisplayName} to conversation Id: {firstConversationId}");
            }

            // add message to the first conversation
            {
                using var createdMessageInfo = AddMessageToConversation(conversationSystem, firstConversationId, defaultTestUserDisplayName, defaultConversationMessage);

                Assert.AreEqual(createdMessageInfo.UserID, defaultTestUserId);
                Assert.AreEqual(createdMessageInfo.Message, defaultConversationMessage);
                Assert.AreEqual(createdMessageInfo.UserDisplayName, defaultTestUserDisplayName);
                Assert.AreEqual(createdMessageInfo.ConversationId, firstConversationId);

                firstMessageIdToBeDeleted = createdMessageInfo.Id;

                LogDebug($"Message with Id: {createdMessageInfo.Id} was added by {defaultTestUserDisplayName} to conversation Id: {firstConversationId}");
            }

            // add message to the first conversation
            {
                using var createdMessageInfo = AddMessageToConversation(conversationSystem, firstConversationId, defaultTestUserDisplayName, defaultConversationMessage);

                Assert.AreEqual(createdMessageInfo.UserID, defaultTestUserId);
                Assert.AreEqual(createdMessageInfo.Message, defaultConversationMessage);
                Assert.AreEqual(createdMessageInfo.UserDisplayName, defaultTestUserDisplayName);
                Assert.AreEqual(createdMessageInfo.ConversationId, firstConversationId);

                firstConversationId = createdMessageInfo.ConversationId;

                LogDebug($"Message with Id: {createdMessageInfo.Id} was added by {defaultTestUserDisplayName} to conversation Id: {firstConversationId}");
            }

            UserSystemTests.LogOut(userSystem);

            // Log in with the second account
            var altUserId = userSystem.TestLogIn(UserSystemTests.AlternativeLoginEmail, UserSystemTests.AlternativeLoginPassword, pushCleanupFunction: false);

            // We don't care when the profile copy gets destroyed, so we don't explicitly call Dispose() and opt to instead let the GC do its thing
            var alternativeTestUserDisplayName = UserSystemTests.GetFullProfileByUserId(userSystem, altUserId).DisplayName;

            // Add messages to the previous conversation
            {
                using var createdMessageInfo = AddMessageToConversation(conversationSystem, firstConversationId, alternativeTestUserDisplayName, defaultConversationMessage);

                Assert.AreEqual(createdMessageInfo.UserID, altUserId);
                Assert.AreEqual(createdMessageInfo.Message, defaultConversationMessage);
                Assert.AreEqual(createdMessageInfo.UserDisplayName, alternativeTestUserDisplayName);
                Assert.AreEqual(createdMessageInfo.ConversationId, firstConversationId);

                LogDebug($"Message with Id: {createdMessageInfo.Id} was added by {alternativeTestUserDisplayName} to conversation Id: {firstConversationId}");
            }

            {
                using var createdMessageInfo = AddMessageToConversation(conversationSystem, firstConversationId, alternativeTestUserDisplayName, defaultConversationMessage);

                Assert.AreEqual(createdMessageInfo.UserID, altUserId);
                Assert.AreEqual(createdMessageInfo.Message, defaultConversationMessage);
                Assert.AreEqual(createdMessageInfo.UserDisplayName, alternativeTestUserDisplayName);
                Assert.AreEqual(createdMessageInfo.ConversationId, firstConversationId);

                LogDebug($"Message with Id: {createdMessageInfo.Id} was added by {alternativeTestUserDisplayName} to conversation Id: {firstConversationId}");
            }

            // Create a new conversation
            {
                var newConversationId = conversationSystem.CreateConversation("TestMessage").Result;

                Assert.AreEqual(newConversationId.GetResultCode(), Services.EResultCode.Success);

                using var result = conversationSystem.GetConversationInformation(newConversationId.GetValue()).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
                Assert.AreEqual(result.GetConversationInfo().ConversationId, newConversationId.GetValue());
                Assert.AreEqual(result.GetConversationInfo().UserID, altUserId);
                Assert.AreEqual(result.GetConversationInfo().UserDisplayName, alternativeTestUserDisplayName);
                Assert.AreEqual(result.GetConversationInfo().Message, "TestMessage");
                Assert.IsFalse(result.GetConversationInfo().Edited);
                Assert.IsFalse(result.GetConversationInfo().Resolved);

                var defaultTransform = new Multiplayer.SpaceTransform();

                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Position.X, defaultTransform.Position.X);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Position.Y, defaultTransform.Position.Y);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Position.Z, defaultTransform.Position.Z);

                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.W, defaultTransform.Rotation.W);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.X, defaultTransform.Rotation.X);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.Y, defaultTransform.Rotation.Y);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.Z, defaultTransform.Rotation.Z);

                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Scale.X, defaultTransform.Scale.X);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Scale.Y, defaultTransform.Scale.Y);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Scale.Z, defaultTransform.Scale.Z);

                using var createdMessageInfo = AddMessageToConversation(conversationSystem, newConversationId.GetValue(), alternativeTestUserDisplayName, defaultConversationMessage);

                Assert.AreEqual(createdMessageInfo.UserID, altUserId);
                Assert.AreEqual(createdMessageInfo.Message, defaultConversationMessage);
                Assert.AreEqual(createdMessageInfo.UserDisplayName, alternativeTestUserDisplayName);

                secondConversationId = createdMessageInfo.ConversationId;
                secondMessageIdToBeDeleted = createdMessageInfo.Id;

                LogDebug($"Conversation created. Id: {secondConversationId}");
                LogDebug($"Message with Id: {createdMessageInfo.Id} was added by {alternativeTestUserDisplayName} to conversation Id: {secondConversationId}");
            }

            // Retrieve all messages from first conversation
            {
                using var result = conversationSystem.GetMessagesFromConversation(firstConversationId, null, null).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);

                using var messages = result.GetMessages();

                Assert.AreEqual(messages.Size(), 5UL);
                Assert.AreEqual(result.GetTotalCount(), 5UL);
            }

            // Delete one message from first conversation
            {
                using var result = conversationSystem.DeleteMessage(firstMessageIdToBeDeleted).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
            }

            // Retrieve again remaining messages from first conversation
            {
                using var result = conversationSystem.GetMessagesFromConversation(firstConversationId, null, null).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);

                using var messages = result.GetMessages();

                Assert.AreEqual(messages.Size(), 4UL);
                Assert.AreEqual(result.GetTotalCount(), 4UL);
            }

            // Delete first conversation entirely
            {
                using var result = conversationSystem.DeleteConversation(firstConversationId).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
            }

            // Retrieve all messages from second conversation
            {
                using var result = conversationSystem.GetMessagesFromConversation(secondConversationId, null, null).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);

                using var messages = result.GetMessages();

                Assert.AreEqual(messages.Size(), 1UL);
                Assert.AreEqual(result.GetTotalCount(), 1UL);
            }

            // Delete the only message from the second conversation
            {
                using var result = conversationSystem.DeleteMessage(secondMessageIdToBeDeleted).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
            }

            // Retrieve the messages from the second conversation
            {
                using var result = conversationSystem.GetMessagesFromConversation(secondConversationId, null, null).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);

                using var messages = result.GetMessages();

                Assert.AreEqual(messages.Size(), 0UL);
                Assert.AreEqual(result.GetTotalCount(), 0UL);
            }

            // Delete second conversation entirely even if it doesn't contain messages anymore
            {
                using var result = conversationSystem.DeleteConversation(secondConversationId).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
            }

            spaceSystem.ExitSpace();

            UserSystemTests.LogOut(userSystem);

            // Log in with the space creator in order to delete it
            _ = UserSystemTests.LogIn(userSystem);
            SpaceSystemTests.DeleteSpace(spaceSystem, space);
            space.Dispose();
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_CONVERSATIONSYSTEM_TESTS || RUN_CONVERSATIONSYSTEM_GET_MESSAGES_TEST
        [Test]
        public static void GetMessagesTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _,
                out _, out _, out _, out _, out _,out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            // Log in
            var defaultTestUserId = userSystem.TestLogIn(pushCleanupFunction: false);

            // Create space
            using var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null, pushCleanupFunction: false);

            // Add the second test user to the space
            {
                using var result = spaceSystem.InviteToSpace(space.Id, UserSystemTests.AlternativeLoginEmail, true, "", "").Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
            }

            // Add self to space and fire EnterSpace event
            {
                using var result = spaceSystem.EnterSpace(space.Id).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);
            }

            var systemsManager = Systems.SystemsManager.Get();
            var connection = systemsManager.GetMultiplayerConnection();
            var entitySystem = systemsManager.GetSpaceEntitySystem();

            entitySystem.OnEntityCreated += (s, e) => { };

            var conversationSystem = connection.GetConversationSystem();

            string conversationId;
            string firstMessageId;
            string secondMessageId;

            var defaultTestUserDisplayName = UserSystemTests.GetFullProfileByUserId(userSystem, defaultTestUserId).DisplayName;
            var defaultConversationMessage = "this is a message from the C# tests world";

            {
                var newConversationId = conversationSystem.CreateConversation("TestMessage").Result;

                Assert.AreEqual(newConversationId.GetResultCode(), Services.EResultCode.Success);

                using var result = conversationSystem.GetConversationInformation(newConversationId.GetValue()).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
                Assert.AreEqual(result.GetConversationInfo().ConversationId, newConversationId.GetValue());
                Assert.AreEqual(result.GetConversationInfo().UserID, defaultTestUserId);
                Assert.AreEqual(result.GetConversationInfo().UserDisplayName, defaultTestUserDisplayName);
                Assert.AreEqual(result.GetConversationInfo().Message, "TestMessage");
                Assert.IsFalse(result.GetConversationInfo().Edited);
                Assert.IsFalse(result.GetConversationInfo().Resolved);

                var defaultTransform = new Multiplayer.SpaceTransform();

                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Position.X, defaultTransform.Position.X);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Position.Y, defaultTransform.Position.Y);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Position.Z, defaultTransform.Position.Z);

                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.W, defaultTransform.Rotation.W);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.X, defaultTransform.Rotation.X);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.Y, defaultTransform.Rotation.Y);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.Z, defaultTransform.Rotation.Z);

                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Scale.X, defaultTransform.Scale.X);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Scale.Y, defaultTransform.Scale.Y);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Scale.Z, defaultTransform.Scale.Z);

                using var createdMessageInfo = AddMessageToConversation(conversationSystem, newConversationId.GetValue(), defaultTestUserDisplayName, defaultConversationMessage);

                conversationId = createdMessageInfo.ConversationId;
                firstMessageId = createdMessageInfo.Id;
            }

            UserSystemTests.LogOut(userSystem);

            // Log in with the second account
            var altUserId = userSystem.TestLogIn(UserSystemTests.AlternativeLoginEmail, UserSystemTests.AlternativeLoginPassword, pushCleanupFunction: false);

            var alternativeTestUserDisplayName = UserSystemTests.GetFullProfileByUserId(userSystem, altUserId).DisplayName;

            secondMessageId = AddMessageToConversation(conversationSystem, conversationId, alternativeTestUserDisplayName, defaultConversationMessage).Id;

            // Check that the second user can retrieve both added messages
            {
                using var result = conversationSystem.GetMessage(firstMessageId).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);

                using var retrievedMessageInfo = result.GetMessageInfo();

                Assert.AreEqual(retrievedMessageInfo.Id, firstMessageId);
                Assert.AreEqual(retrievedMessageInfo.UserID, defaultTestUserId);
                Assert.AreEqual(retrievedMessageInfo.ConversationId, conversationId);
                Assert.AreEqual(retrievedMessageInfo.Message, defaultConversationMessage);
                Assert.AreEqual(retrievedMessageInfo.UserDisplayName, defaultTestUserDisplayName);
            }

            {
                using var result = conversationSystem.GetMessage(secondMessageId).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);

                using var retrievedMessageInfo = result.GetMessageInfo();

                Assert.AreEqual(retrievedMessageInfo.Id, secondMessageId);
                Assert.AreEqual(retrievedMessageInfo.UserID, altUserId);
                Assert.AreEqual(retrievedMessageInfo.ConversationId, conversationId);
                Assert.AreEqual(retrievedMessageInfo.Message, defaultConversationMessage);
                Assert.AreEqual(retrievedMessageInfo.UserDisplayName, alternativeTestUserDisplayName);
            }

            // Check that the second user can retrieve the messages from the conversation using pagination
            {
                using var result = conversationSystem.GetMessagesFromConversation(conversationId, 0, 1).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);

                using var messages = result.GetMessages();

                Assert.AreEqual(messages.Size(), 1UL);
                Assert.AreEqual(result.GetTotalCount(), 2UL);

                using var currMessage = messages[0];

                Assert.IsFalse(string.IsNullOrEmpty(currMessage.Id));
                Assert.IsFalse(string.IsNullOrEmpty(currMessage.UserID));
                Assert.AreEqual(currMessage.ConversationId, conversationId);
                Assert.AreEqual(currMessage.Message, defaultConversationMessage);
                Assert.IsFalse(string.IsNullOrEmpty(currMessage.UserDisplayName));
            }

            UserSystemTests.LogOut(userSystem);

            // Log in again with the default user
            _ = userSystem.TestLogIn(pushCleanupFunction: false);

            // Check that the default user can retrieve both added messages
            {
                using var result = conversationSystem.GetMessage(firstMessageId).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);

                using var retrievedMessageInfo = result.GetMessageInfo();

                Assert.AreEqual(retrievedMessageInfo.Id, firstMessageId);
                Assert.AreEqual(retrievedMessageInfo.UserID, defaultTestUserId);
                Assert.AreEqual(retrievedMessageInfo.ConversationId, conversationId);
                Assert.AreEqual(retrievedMessageInfo.Message, defaultConversationMessage);
                Assert.AreEqual(retrievedMessageInfo.UserDisplayName, defaultTestUserDisplayName);
            }

            {
                using var result = conversationSystem.GetMessage(secondMessageId).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);

                using var retrievedMessageInfo = result.GetMessageInfo();

                Assert.AreEqual(retrievedMessageInfo.Id, secondMessageId);
                Assert.AreEqual(retrievedMessageInfo.UserID, altUserId);
                Assert.AreEqual(retrievedMessageInfo.ConversationId, conversationId);
                Assert.AreEqual(retrievedMessageInfo.Message, defaultConversationMessage);
                Assert.AreEqual(retrievedMessageInfo.UserDisplayName, alternativeTestUserDisplayName);
            }

            // Check that the default user can retrieve the messages from the conversation using pagination
            {
                using var result = conversationSystem.GetMessagesFromConversation(conversationId, 1, 1).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);

                using var messages = result.GetMessages();

                Assert.AreEqual(messages.Size(), 1UL);
                Assert.AreEqual(result.GetTotalCount(), 2UL);

                using var currMessage = messages[0];

                Assert.IsFalse(string.IsNullOrEmpty(currMessage.Id));
                Assert.IsFalse(string.IsNullOrEmpty(currMessage.UserID));
                Assert.AreEqual(currMessage.ConversationId, conversationId);
                Assert.AreEqual(currMessage.Message, defaultConversationMessage);
                Assert.IsFalse(string.IsNullOrEmpty(currMessage.UserDisplayName));
            }

            {
                var result = conversationSystem.DeleteConversation(conversationId).Result;
                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
            }

            spaceSystem.ExitSpace();

            SpaceSystemTests.DeleteSpace(spaceSystem, space);
            UserSystemTests.LogOut(userSystem);
        }
#endif
#if RUN_ALL_UNIT_TESTS || RUN_CONVERSATIONSYSTEM_TESTS || RUN_CONVERSATIONSYSTEM_EMPTY_CONVERSATION_TEST
        [Test]
        public static void EmptyConversationTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _,
                out _, out _, out _, out _, out _,out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            // Log in
            var defaultTestUserId = userSystem.TestLogIn(pushCleanupFunction: false);

            // Create space
            using var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null, pushCleanupFunction: false);

            // Add self to space and fire EnterSpace event
            {
                using var result = spaceSystem.EnterSpace(space.Id).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);
            }

            var systemsManager = Systems.SystemsManager.Get();
            var connection = systemsManager.GetMultiplayerConnection();
            var entitySystem = systemsManager.GetSpaceEntitySystem();

            entitySystem.OnEntityCreated += (s, e) => { };

            var conversationSystem = connection.GetConversationSystem();

            string conversationId;
            string firstMessageId;

            var defaultTestUserDisplayName = UserSystemTests.GetFullProfileByUserId(userSystem, defaultTestUserId).DisplayName;
            var defaultConversationMessage = "this is a message from the C# tests world";

            var newConversationId = conversationSystem.CreateConversation("TestMessage").Result;

            Assert.AreEqual(newConversationId.GetResultCode(), Services.EResultCode.Success);

            {
                using var result = conversationSystem.GetConversationInformation(newConversationId.GetValue()).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
                Assert.AreEqual(result.GetConversationInfo().ConversationId, newConversationId.GetValue());
                Assert.AreEqual(result.GetConversationInfo().UserID, defaultTestUserId);
                Assert.AreEqual(result.GetConversationInfo().UserDisplayName, defaultTestUserDisplayName);
                Assert.AreEqual(result.GetConversationInfo().Message, "TestMessage");
                Assert.IsFalse(result.GetConversationInfo().Edited);
                Assert.IsFalse(result.GetConversationInfo().Resolved);

                var defaultTransform = new Multiplayer.SpaceTransform();

                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Position.X, defaultTransform.Position.X);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Position.Y, defaultTransform.Position.Y);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Position.Z, defaultTransform.Position.Z);

                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.W, defaultTransform.Rotation.W);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.X, defaultTransform.Rotation.X);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.Y, defaultTransform.Rotation.Y);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.Z, defaultTransform.Rotation.Z);

                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Scale.X, defaultTransform.Scale.X);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Scale.Y, defaultTransform.Scale.Y);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Scale.Z, defaultTransform.Scale.Z);
            }

            {
                using var createdMessageInfo = AddMessageToConversation(conversationSystem, newConversationId.GetValue(), defaultTestUserDisplayName, defaultConversationMessage);

                conversationId = createdMessageInfo.ConversationId;
                firstMessageId = createdMessageInfo.Id;

                Assert.AreEqual(newConversationId.GetValue(), conversationId);
            }

            // Cleanup
            {
                var result = conversationSystem.DeleteConversation(conversationId).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
            }

            spaceSystem.ExitSpace();

            SpaceSystemTests.DeleteSpace(spaceSystem, space);
            UserSystemTests.LogOut(userSystem);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_CONVERSATIONSYSTEM_TESTS || RUN_CONVERSATION_NEWMESSAGE_NETWORKEVENT_TEST
        [Test]
        public static void ConversationNewMessageCallbackTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out _, out _, out _, out _, out _,out _);

            // Log in
            var defaultTestUserId = UserSystemTests.LogIn(userSystem);
            var defaultTestUserDisplayName = UserSystemTests.GetFullProfileByUserId(userSystem, defaultTestUserId).DisplayName;

            // Create space
            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-REWIND";
            string testAssetCollectionName = GenerateUniqueString("OLY-UNITTEST-ASSETCOLLECTION-REWIND");
            string testAssetName = GenerateUniqueString("OLY-UNITTEST-ASSET-REWIND");

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            // Add self to space and fire EnterSpace event
            {
                using var result = spaceSystem.EnterSpace(space.Id).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);
            }

            var systemsManager = Systems.SystemsManager.Get();
            var connection = systemsManager.GetMultiplayerConnection();
            var entitySystem = systemsManager.GetSpaceEntitySystem();

            entitySystem.OnEntityCreated += (s, e) => { };

            // Setup Asset callback
            var gotMessage = false;
            var conversationId = "";

            connection.OnConversationSystem += (s, p) =>
            {
                if (gotMessage)
                    return;

                Assert.AreEqual(p.MessageType, Multiplayer.ConversationMessageType.NewMessage);
                Assert.AreEqual(p.MessageValue, conversationId);

                gotMessage = true;
            };

            var conversationSystem = connection.GetConversationSystem();

            // Create conversation
            {
                var newConversationId = conversationSystem.CreateConversation("TestMessage").Result;

                Assert.AreEqual(newConversationId.GetResultCode(), Services.EResultCode.Success);

                using var result = conversationSystem.GetConversationInformation(newConversationId.GetValue()).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
                Assert.AreEqual(result.GetConversationInfo().ConversationId, newConversationId.GetValue());
                Assert.AreEqual(result.GetConversationInfo().UserID, defaultTestUserId);
                Assert.AreEqual(result.GetConversationInfo().UserDisplayName, defaultTestUserDisplayName);
                Assert.AreEqual(result.GetConversationInfo().Message, "TestMessage");
                Assert.IsFalse(result.GetConversationInfo().Edited);
                Assert.IsFalse(result.GetConversationInfo().Resolved);

                var defaultTransform = new Multiplayer.SpaceTransform();

                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Position.X, defaultTransform.Position.X);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Position.Y, defaultTransform.Position.Y);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Position.Z, defaultTransform.Position.Z);

                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.W, defaultTransform.Rotation.W);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.X, defaultTransform.Rotation.X);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.Y, defaultTransform.Rotation.Y);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.Z, defaultTransform.Rotation.Z);

                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Scale.X, defaultTransform.Scale.X);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Scale.Y, defaultTransform.Scale.Y);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Scale.Z, defaultTransform.Scale.Z);

                conversationId = newConversationId.GetValue();
            }

            // Add message
            {
                using var result = conversationSystem.AddMessageToConversation(conversationId, "Test", "test").Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
            }
            {
                // Generate Networkevent as SendNetworkEvent doesnt fire sender callback
                var array = new Common.Array<Multiplayer.ReplicatedValue>(2);
                array[0] = new Multiplayer.ReplicatedValue((int)Multiplayer.ConversationMessageType.NewMessage);
                array[1] = new Multiplayer.ReplicatedValue(conversationId);
                connection.SendNetworkEventToClient("ConversationSystem", array, connection.GetClientId());
            }

            // Wait for message
            const int WAIT_FOR_TEST_TIMEOUT_LIMIT_MS = 10000;   // 10 seconds
            var elapsed = 0;

            while (!gotMessage && elapsed < WAIT_FOR_TEST_TIMEOUT_LIMIT_MS)
            {
                Thread.Sleep(50);
                elapsed += 50;
            }

            Assert.IsTrue(gotMessage);

            // Cleanup
            {
                using var result = conversationSystem.DeleteConversation(conversationId).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
            }

            spaceSystem.ExitSpace();
        }
#endif

        // Disabling because it's written in a way that will never work. We will always get notifications about new messages before the other ones...
#if false //RUN_ALL_UNIT_TESTS || RUN_CONVERSATIONSYSTEM_TESTS || RUN_CONVERSATION_DELETEMESSAGE_NETWORKEVENT_TEST
        [Test]
        public static void ConversationDeleteMessageCallbackTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out _, out _, out _, out _,out _);

            // Log in
            var defaultTestUserId = UserSystemTests.LogIn(userSystem);
            var defaultTestUserDisplayName = UserSystemTests.GetFullProfileByUserId(userSystem, defaultTestUserId).DisplayName;

            // Create space
            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-REWIND";
            string testAssetCollectionName = GenerateUniqueString("OLY-UNITTEST-ASSETCOLLECTION-REWIND");
            string testAssetName = GenerateUniqueString("OLY-UNITTEST-ASSET-REWIND");

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            // Add self to space and fire EnterSpace event
            {
                using var result = spaceSystem.EnterSpace(space.Id).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);
            }

            entitySystem.OnEntityCreated += (s, e) => { };

            // Fetch all entities, etc.
            {
                var ok = connection.InitialiseConnection().Result;

                Assert.IsTrue(ok);
            }

            // Setup Asset callback
            var gotMessage = false;
            var conversationId = "";
            var messageId = "";

            var conversationSystem = connection.GetConversationSystem();

            // Create conversation
            {
                var newConversationId = conversationSystem.CreateConversation("TestMessage").Result;

                Assert.AreEqual(newConversationId.GetResultCode(), Services.EResultCode.Success);

                using var result = conversationSystem.GetConversationInformation(newConversationId.GetValue()).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
                Assert.AreEqual(result.GetConversationInfo().ConversationId, newConversationId.GetValue());
                Assert.AreEqual(result.GetConversationInfo().UserID, defaultTestUserId);
                Assert.AreEqual(result.GetConversationInfo().UserDisplayName, defaultTestUserDisplayName);
                Assert.AreEqual(result.GetConversationInfo().Message, "TestMessage");
                Assert.IsFalse(result.GetConversationInfo().Edited);
                Assert.IsFalse(result.GetConversationInfo().Resolved);

                var defaultTransform = new Multiplayer.SpaceTransform();

                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Position.X, defaultTransform.Position.X);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Position.Y, defaultTransform.Position.Y);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Position.Z, defaultTransform.Position.Z);

                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.W, defaultTransform.Rotation.W);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.X, defaultTransform.Rotation.X);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.Y, defaultTransform.Rotation.Y);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.Z, defaultTransform.Rotation.Z);

                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Scale.X, defaultTransform.Scale.X);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Scale.Y, defaultTransform.Scale.Y);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Scale.Z, defaultTransform.Scale.Z);

                conversationId = newConversationId.GetValue();
            }

            // Add message
            {
                using var result = conversationSystem.AddMessageToConversation(conversationId, "Test", "test").Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);

                messageId = result.GetMessageInfo().Id;
            }

            connection.OnConversationSystem += (s, p) =>
            {
                if (gotMessage)
                {
                    return;
                }

                Assert.AreEqual(p.MessageType, Multiplayer.ConversationMessageType.DeleteMessage);
                Assert.AreEqual(p.MessageValue, messageId);

                gotMessage = true;
            };

            // Delete message
            {
                using var result = conversationSystem.DeleteMessage(messageId).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
            }

            {
                // Generate Networkevent as SendNetworkEvent doesnt fire sender callback
                var array = new Common.Array<Multiplayer.ReplicatedValue>(2);
                array[0] = new Multiplayer.ReplicatedValue((int)Multiplayer.ConversationMessageType.DeleteMessage);
                array[1] = new Multiplayer.ReplicatedValue(messageId);
                connection.SendNetworkEventToClient("ConversationSystem", array, connection.GetClientId());
            }

            // Wait for message
            const int WAIT_FOR_TEST_TIMEOUT_LIMIT_MS = 10000;   // 10 seconds
            var elapsed = 0;

            while (!gotMessage && elapsed < WAIT_FOR_TEST_TIMEOUT_LIMIT_MS)
            {
                Thread.Sleep(50);
                elapsed += 50;
            }

            Assert.IsTrue(gotMessage);

            // Clean up
            {
                using var result = conversationSystem.DeleteConversation(conversationId).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
            }

            connection.Dispose();

            spaceSystem.ExitSpace();

        }
#endif

        // Disabling because it's written in a way that will never work. We will always get notifications about new messages before the other ones...
#if false // RUN_ALL_UNIT_TESTS || RUN_CONVERSATIONSYSTEM_TESTS || RUN_CONVERSATION_DELETECONVERSATION_NETWORKEVENT_TEST
        [Test]
        public static void ConversationNewConversationCallbackTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out _, out _, out _, out _,out _);

            // Log in
            var defaultTestUserId = UserSystemTests.LogIn(userSystem);
            var defaultTestUserDisplayName = UserSystemTests.GetFullProfileByUserId(userSystem, defaultTestUserId).DisplayName;

            // Create space
            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-REWIND";
            string testAssetCollectionName = GenerateUniqueString("OLY-UNITTEST-ASSETCOLLECTION-REWIND");
            string testAssetName = GenerateUniqueString("OLY-UNITTEST-ASSET-REWIND");

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            // Add self to space and fire EnterSpace event
            {
                using var result = spaceSystem.EnterSpace(space.Id).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);
            }

            var connection = new Multiplayer.MultiplayerConnection(space.Id);
            var entitySystem = connection.GetSpaceEntitySystem();

            entitySystem.OnEntityCreated += (s, e) => { };

            // Connect to multiplayer service
            {
                var ok = connection.Connect().Result;

                Assert.IsTrue(ok);
            }

            // Fetch all entities, etc.
            {
                var ok = connection.InitialiseConnection().Result;

                Assert.IsTrue(ok);
            }

            var gotMessage = false;
            var conversationId = "";

            var conversationSystem = connection.GetConversationSystem();

            // Create conversation
            {
                var newConversationId = conversationSystem.CreateConversation("TestMessage").Result;

                Assert.AreEqual(newConversationId.GetResultCode(), Services.EResultCode.Success);

                using var result = conversationSystem.GetConversationInformation(newConversationId.GetValue()).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
                Assert.AreEqual(result.GetConversationInfo().ConversationId, newConversationId.GetValue());
                Assert.AreEqual(result.GetConversationInfo().UserID, defaultTestUserId);
                Assert.AreEqual(result.GetConversationInfo().UserDisplayName, defaultTestUserDisplayName);
                Assert.AreEqual(result.GetConversationInfo().Message, "TestMessage");
                Assert.IsFalse(result.GetConversationInfo().Edited);
                Assert.IsFalse(result.GetConversationInfo().Resolved);

                var defaultTransform = new Multiplayer.SpaceTransform();

                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Position.X, defaultTransform.Position.X);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Position.Y, defaultTransform.Position.Y);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Position.Z, defaultTransform.Position.Z);

                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.W, defaultTransform.Rotation.W);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.X, defaultTransform.Rotation.X);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.Y, defaultTransform.Rotation.Y);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.Z, defaultTransform.Rotation.Z);

                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Scale.X, defaultTransform.Scale.X);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Scale.Y, defaultTransform.Scale.Y);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Scale.Z, defaultTransform.Scale.Z);

                conversationId = newConversationId.GetValue();
            }

            // Add message
            {
                using var result = conversationSystem.AddMessageToConversation(conversationId, "Test", "test").Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
            }

            connection.OnConversationSystem += (s, p) =>
            {
                if (gotMessage)
                {
                    return;
                }

                Assert.AreEqual(p.MessageType, Multiplayer.ConversationMessageType.DeleteConversation);
                Assert.AreEqual(p.MessageValue, conversationId);

                gotMessage = true;
            };

            {
                using var result = conversationSystem.DeleteConversation(conversationId).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
            }

            {
                // Generate Networkevent as SendNetworkEvent doesnt fire sender callback
                var array = new Common.Array<Multiplayer.ReplicatedValue>(2);
                array[0] = new Multiplayer.ReplicatedValue(new Multiplayer.ReplicatedValue((int)Multiplayer.ConversationMessageType.DeleteConversation));
                array[1] = new Multiplayer.ReplicatedValue(conversationId);
                connection.SendNetworkEventToClient("ConversationSystem", array, connection.GetClientId());
            }

            // Wait for message
            const int WAIT_FOR_TEST_TIMEOUT_LIMIT_MS = 10000;   // 10 seconds
            var elapsed = 0;

            while (!gotMessage && elapsed < WAIT_FOR_TEST_TIMEOUT_LIMIT_MS)
            {
                Thread.Sleep(50);
                elapsed += 50;
            }

            Assert.IsTrue(gotMessage);

            // Disconnect
            {
                var ok = connection.Disconnect().Result;

                Assert.IsTrue(ok);
            }

            connection.Dispose();

            spaceSystem.ExitSpace();
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_CONVERSATIONSYSTEM_TESTS || RUN_CONVERSATION_UPDATE_INFO_TEST
        [Test]
        public static void ConversationUpdateInfoTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out _, out _, out _, out _, out _,out _);

            // Log in
            var defaultTestUserId = UserSystemTests.LogIn(userSystem);
            var defaultTestUserDisplayName = UserSystemTests.GetFullProfileByUserId(userSystem, defaultTestUserId).DisplayName;

            // Create space
            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-OKO");
            string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-OKO";
            string testAssetCollectionName = GenerateUniqueString("OLY-UNITTEST-ASSETCOLLECTION-OKO");
            string testAssetName = GenerateUniqueString("OLY-UNITTEST-ASSET-OKO");

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            // Add self to space and fire EnterSpace event
            {
                using var result = spaceSystem.EnterSpace(space.Id).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);
            }

            var systemsManager = Systems.SystemsManager.Get();
            var connection = systemsManager.GetMultiplayerConnection();
            var entitySystem = systemsManager.GetSpaceEntitySystem();

            entitySystem.OnEntityCreated += (s, e) => { };

            var gotMessage = false;
            var conversationId = "";

            connection.OnConversationSystem += (s, p) =>
            {
                if (gotMessage)
                {
                    return;
                }

                Assert.AreEqual(p.MessageType, Multiplayer.ConversationMessageType.ConversationInformation);
                Assert.AreEqual(p.MessageValue, conversationId);

                gotMessage = true;
            };

            var conversationSystem = connection.GetConversationSystem();

            // Create conversation
            {
                var newConversationId = conversationSystem.CreateConversation("TestMessage").Result;

                Assert.AreEqual(newConversationId.GetResultCode(), Services.EResultCode.Success);

                using var result = conversationSystem.GetConversationInformation(newConversationId.GetValue()).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
                Assert.AreEqual(result.GetConversationInfo().ConversationId, newConversationId.GetValue());
                Assert.AreEqual(result.GetConversationInfo().UserID, defaultTestUserId);
                Assert.AreEqual(result.GetConversationInfo().UserDisplayName, defaultTestUserDisplayName);
                Assert.AreEqual(result.GetConversationInfo().Message, "TestMessage");
                Assert.IsFalse(result.GetConversationInfo().Edited);
                Assert.IsFalse(result.GetConversationInfo().Resolved);

                var defaultTransform = new Multiplayer.SpaceTransform();

                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Position.X, defaultTransform.Position.X);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Position.Y, defaultTransform.Position.Y);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Position.Z, defaultTransform.Position.Z);

                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.W, defaultTransform.Rotation.W);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.X, defaultTransform.Rotation.X);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.Y, defaultTransform.Rotation.Y);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.Z, defaultTransform.Rotation.Z);

                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Scale.X, defaultTransform.Scale.X);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Scale.Y, defaultTransform.Scale.Y);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Scale.Z, defaultTransform.Scale.Z);


                conversationId = newConversationId.GetValue();
            }

            {
                using var result = conversationSystem.GetConversationInformation(conversationId).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
                Assert.AreEqual(result.GetConversationInfo().ConversationId, conversationId);
                Assert.AreEqual(result.GetConversationInfo().UserID, defaultTestUserId);
                Assert.AreEqual(result.GetConversationInfo().UserDisplayName, defaultTestUserDisplayName);
                Assert.AreEqual(result.GetConversationInfo().Message, "TestMessage");
                Assert.IsFalse(result.GetConversationInfo().Edited);
                Assert.IsFalse(result.GetConversationInfo().Resolved);

                var defaultTransform = new Multiplayer.SpaceTransform();

                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Position.X, defaultTransform.Position.X);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Position.Y, defaultTransform.Position.Y);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Position.Z, defaultTransform.Position.Z);

                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.W, defaultTransform.Rotation.W);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.X, defaultTransform.Rotation.X);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.Y, defaultTransform.Rotation.Y);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.Z, defaultTransform.Rotation.Z);

                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Scale.X, defaultTransform.Scale.X);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Scale.Y, defaultTransform.Scale.Y);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Scale.Z, defaultTransform.Scale.Z);
            }

            {
                using var newData = new Multiplayer.ConversationInfo();
                using var cameraTransformValue = new Multiplayer.SpaceTransform(Common.Vector3.One(), Common.Vector4.One(), Common.Vector3.One());

                newData.Message = "TestMessage1";
                newData.Resolved = true;
                newData.CameraPosition = cameraTransformValue;

                using var result = conversationSystem.SetConversationInformation(conversationId, newData).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
                Assert.AreEqual(result.GetConversationInfo().ConversationId, conversationId);
                Assert.AreEqual(result.GetConversationInfo().UserID, defaultTestUserId);
                Assert.AreEqual(result.GetConversationInfo().UserDisplayName, defaultTestUserDisplayName);
                Assert.AreEqual(result.GetConversationInfo().Message, "TestMessage1");
                Assert.IsTrue(result.GetConversationInfo().Edited);
                Assert.IsTrue(result.GetConversationInfo().Resolved);

                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Position.X, cameraTransformValue.Position.X);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Position.Y, cameraTransformValue.Position.Y);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Position.Z, cameraTransformValue.Position.Z);

                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.W, cameraTransformValue.Rotation.W);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.X, cameraTransformValue.Rotation.X);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.Y, cameraTransformValue.Rotation.Y);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.Z, cameraTransformValue.Rotation.Z);

                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Scale.X, cameraTransformValue.Scale.X);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Scale.Y, cameraTransformValue.Scale.Y);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Scale.Z, cameraTransformValue.Scale.Z);
            }

            // Generate Networkevent as SendNetworkEvent doesnt fire sender callback
            var array = new Common.Array<Multiplayer.ReplicatedValue>(2);
            array[0] = new Multiplayer.ReplicatedValue(new Multiplayer.ReplicatedValue((int)Multiplayer.ConversationMessageType.ConversationInformation));
            array[1] = new Multiplayer.ReplicatedValue(conversationId);
            connection.SendNetworkEventToClient("ConversationSystem", array, connection.GetClientId());

            {
                using var result = conversationSystem.DeleteConversation(conversationId).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
            }

            // Wait for message
            const int WAIT_FOR_TEST_TIMEOUT_LIMIT_MS = 10000;   // 10 seconds
            var elapsed = 0;

            while (!gotMessage && elapsed < WAIT_FOR_TEST_TIMEOUT_LIMIT_MS)
            {
                Thread.Sleep(50);
                elapsed += 50;
            }

            Assert.IsTrue(gotMessage);

            spaceSystem.ExitSpace();
        }
#endif

        // Disabling because it's written in a way that will never work. We will always get notifications about new messages before the other ones...
#if false // RUN_ALL_UNIT_TESTS || RUN_CONVERSATIONSYSTEM_TESTS || RUN_MESSAGE_UPDATE_INFO_TEST
        [Test]
        public static void MessageUpdateInfoTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out _, out _, out _, out _);

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-OKO");
            string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-OKO";
            string testAssetCollectionName = GenerateUniqueString("OLY-UNITTEST-ASSETCOLLECTION-OKO");
            string testAssetName = GenerateUniqueString("OLY-UNITTEST-ASSET-OKO");

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            // Add self to space and fire EnterSpace event
            {
                using var result = spaceSystem.EnterSpace(space.Id).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);
            }

            entitySystem.OnEntityCreated += (s, e) => { };

            // Fetch all entities, etc.
            {
                var ok = connection.InitialiseConnection().Result;

                Assert.IsTrue(ok);
            }

            var gotMessage = false;
            var conversationId = "";
            var messageId = "";

            connection.OnConversationSystem += (s, p) =>
            {
                if (gotMessage)
                {
                    return;
                }

                Assert.AreEqual(p.MessageType, Multiplayer.ConversationMessageType.MessageInformation);
                Assert.AreEqual(p.MessageValue, messageId);

                gotMessage = true;
            };

            var conversationSystem = connection.GetConversationSystem();

            // Create conversation
            {
                var newConversationId = conversationSystem.CreateConversation("TestMessage").Result;

                Assert.AreEqual(newConversationId.GetResultCode(), Services.EResultCode.Success);

                using var result = conversationSystem.GetConversationInformation(newConversationId.GetValue()).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
                Assert.IsFalse(result.GetConversationInfo().Resolved);

                using var defaultTransform = new Multiplayer.SpaceTransform();

                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Position.X, defaultTransform.Position.X);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Position.Y, defaultTransform.Position.Y);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Position.Z, defaultTransform.Position.Z);

                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.W, defaultTransform.Rotation.W);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.X, defaultTransform.Rotation.X);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.Y, defaultTransform.Rotation.Y);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Rotation.Z, defaultTransform.Rotation.Z);

                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Scale.X, defaultTransform.Scale.X);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Scale.Y, defaultTransform.Scale.Y);
                Assert.AreEqual(result.GetConversationInfo().CameraPosition.Scale.Z, defaultTransform.Scale.Z);
                Assert.AreEqual(result.GetConversationInfo().Message, "TestMessage");

                conversationId = newConversationId.GetValue();
            }

            {
                using var result = conversationSystem.AddMessageToConversation(conversationId, "test", "test").Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);

                messageId = result.GetMessageInfo().Id;
            }

            {
                using var result = conversationSystem.GetMessageInformation(messageId).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
                Assert.IsFalse(result.GetMessageInfo().Edited);
            }

            {
                using var newData = new Multiplayer.MessageInfo();
                newData.Message = "newTest";

                using var result = conversationSystem.SetMessageInformation(messageId, newData).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
                Assert.IsTrue(result.GetMessageInfo().Edited);
                Assert.AreEqual(newData.Message, result.GetMessageInfo().Message);
            }


            // Generate Networkevent as SendNetworkEvent doesnt fire sender callback
            var array = new Common.Array<Multiplayer.ReplicatedValue>(2);
            array[0] = new Multiplayer.ReplicatedValue(new Multiplayer.ReplicatedValue((int)Multiplayer.ConversationMessageType.MessageInformation));
            array[1] = new Multiplayer.ReplicatedValue(messageId);
            connection.SendNetworkEventToClient("ConversationSystem", array, connection.GetClientId());

            {
                using var result = conversationSystem.DeleteConversation(conversationId).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
            }

            // Wait for message
            const int WAIT_FOR_TEST_TIMEOUT_LIMIT_MS = 10000;   // 10 seconds
            var elapsed = 0;

            while (!gotMessage && elapsed < WAIT_FOR_TEST_TIMEOUT_LIMIT_MS)
            {
                Thread.Sleep(50);
                elapsed += 50;
            }

            Assert.IsTrue(gotMessage);

            connection.Dispose();

            spaceSystem.ExitSpace();
        }
#endif
    }
}
