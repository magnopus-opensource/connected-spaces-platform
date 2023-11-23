using System.Threading;
using System.IO;

using Csp;
using Common = Csp.Common;
using Systems = Csp.Systems;
using Multiplayer = Csp.Multiplayer;

using CSharpTests;
using MultiplayerTestClient;

using static CSharpTests.TestHelper;
using System;

namespace CSPEngine
{
    static class MultiplayerTests
    {
        static void Disconnect(Multiplayer.MultiplayerConnection connection)
        {
            var res = connection.Disconnect().Result;

            Assert.IsTrue(res);

            LogDebug("Multiplayer disconnected");
        }

        static void Connect(Multiplayer.MultiplayerConnection connection, bool pushCleanupFunction = true)
        {
            var res = connection.Connect().Result;

            Assert.IsTrue(res);

            res = connection.InitialiseConnection().Result;

            Assert.IsTrue(res);

            LogDebug("Multiplayer connected");

            if (pushCleanupFunction)
                PushCleanupFunction(() => Disconnect(connection));
        }

        static void DeleteEntity(Multiplayer.SpaceEntitySystem entitySystem, Multiplayer.SpaceEntity entity, bool disposeFoundationResources = true)
        {
            var id = entity.GetId();

            var ok = entitySystem.DestroyEntity(entity).Result;

            Assert.IsTrue(ok);

            LogDebug($"Entity deleted (Id: {id})");

            if (disposeFoundationResources)
                entity.Dispose();
        }

        static void CreateAvatar(Multiplayer.SpaceEntitySystem entitySystem, string name, string avatarId, out Multiplayer.SpaceEntity entity, bool pushCleanupFunction = true, bool disposeFoundationResources = true)
        {
            using var transform = new Multiplayer.SpaceTransform();
            var res = entitySystem.CreateAvatar(name, transform, Multiplayer.AvatarState.Idle, avatarId, Multiplayer.AvatarPlayMode.Default).Result;

            Assert.IsTrue(res.PointerIsValid);

            entity = res;
            var outEntity = entity;

            LogDebug($"Avatar created (Id: {entity.GetId()})");

            if (pushCleanupFunction)
                PushCleanupFunction(() => DeleteEntity(entitySystem, outEntity, disposeFoundationResources));
        }

        static void CreateCreatorAvatar(Multiplayer.SpaceEntitySystem entitySystem, string name, string avatarId, out Multiplayer.SpaceEntity entity)
        {
            using var transform = new Multiplayer.SpaceTransform();
            var res = entitySystem.CreateAvatar(name, transform, Multiplayer.AvatarState.Idle, avatarId, Multiplayer.AvatarPlayMode.Creator).Result;

            Assert.IsTrue(res.PointerIsValid);

            entity = res;
            var outEntity = entity;

            LogDebug($"Avatar created (Id: {entity.GetId()})");

            PushCleanupFunction(() => DeleteEntity(entitySystem, outEntity));
        }

        public static void CreateObject(Multiplayer.SpaceEntitySystem entitySystem, string name, out Multiplayer.SpaceEntity entity, bool pushCleanupFunction = true)
        {
            using var transform = new Multiplayer.SpaceTransform();
            var res = entitySystem.CreateObject(name, transform).Result;

            Assert.IsTrue(res.PointerIsValid);

            entity = res;
            var outEntity = entity;

            LogDebug($"Object created (Id: {entity.GetId()})");

            if (pushCleanupFunction)
                PushCleanupFunction(() => DeleteEntity(entitySystem, outEntity));
        }

        public static Multiplayer.MultiplayerConnection CreateMultiplayerConnection(string spaceId, bool pushCleanupFunction = true)
        {
            var connection = new Multiplayer.MultiplayerConnection(spaceId);

            if (pushCleanupFunction)
                PushCleanupFunction(() => connection.Dispose());

            return connection;
        }

        static void OnEntityUpdate(object sender, (Multiplayer.SpaceEntity entity, Multiplayer.SpaceEntityUpdateFlags arg2, Common.Array<Multiplayer.ComponentUpdateInfo>) eventArgs)
        {
            var transform = eventArgs.entity.GetTransform();

            LogDebug($"Received update (Entity Id: {eventArgs.entity.GetId()}, " +
                $"Pos: [{transform.Position.X:0.##}, {transform.Position.Y:0.##}, {transform.Position.Z:0.##}], " +
                $"Rot: [{transform.Rotation.X:0.##}, {transform.Rotation.Y:0.##}, {transform.Rotation.Z:0.##}, {transform.Rotation.W:0.##}])");
        }


#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_MANUAL_CONNECTION_TEST
        [Test]
        public static void ManualConnectionTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-REWIND";

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            var connection = CreateMultiplayerConnection(space.Id);
            var entitySystem = connection.GetSpaceEntitySystem();

            Connect(connection);

            Assert.AreEqual(connection.GetConnectionState(), Multiplayer.ConnectionState.Connected);

            // Create object
            var name = "TestObject";
            CreateObject(entitySystem, name, out var entity);

            Assert.AreEqual(entity.GetEntityType(), Multiplayer.SpaceEntityType.Object);
            Assert.AreEqual(entity.GetName(), name);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_CREATEAVATAR_TEST
        [Test]
        public static void CreateAvatarTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-REWIND";

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            var connection = CreateMultiplayerConnection(space.Id);
            var entitySystem = connection.GetSpaceEntitySystem();

            Connect(connection);

            // Create entity
            var name = "TestAvatar";
            var avatarId = "NotARealAvatarId";
            var locomotionModel = Multiplayer.LocomotionModel.Grounded;
            CreateAvatar(entitySystem, name, avatarId, out var entity);

            Assert.AreEqual(entity.GetEntityType(), Multiplayer.SpaceEntityType.Avatar);
            Assert.AreEqual(entity.GetName(), name);

            var components = entity.GetComponents();

            Assert.AreEqual(components.Size(), 1UL);

            using var component = components[0];

            Assert.AreEqual(component.GetComponentType(), Multiplayer.ComponentType.AvatarData);

            using var avatarComponent = new Multiplayer.AvatarSpaceComponent(component);

            Assert.AreEqual(avatarComponent.GetAvatarId(), avatarId);
            Assert.AreEqual(avatarComponent.GetLocomotionModel(), locomotionModel);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_CREATECREATORAVATAR_TEST
        [Test]
        public static void CreateCreatorAvatarTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-REWIND";

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            var connection = CreateMultiplayerConnection(space.Id);
            var entitySystem = connection.GetSpaceEntitySystem();

            Connect(connection);

            // Create entity
            var name = "TestAvatar";
            var avatarId = "NotARealAvatarId";
            CreateCreatorAvatar(entitySystem, name, avatarId, out var entity);

            Assert.AreEqual(entity.GetEntityType(), Multiplayer.SpaceEntityType.Avatar);
            Assert.AreEqual(entity.GetName(), name);

            var components = entity.GetComponents();

            Assert.AreEqual(components.Size(), 1UL);

            using var component = components[0];

            Assert.AreEqual(component.GetComponentType(), Multiplayer.ComponentType.AvatarData);

            using var avatarComponent = new Multiplayer.AvatarSpaceComponent(component);

            Assert.AreEqual(avatarComponent.GetAvatarId(), avatarId);
            Assert.AreEqual(avatarComponent.GetAvatarPlayMode(), Multiplayer.AvatarPlayMode.Creator);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_UPDATEOBJECT_TEST
        [Test]
        public static void UpdateObjectTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _, out _, out _);

            var spaceName = GenerateUniqueString("CSP_CSHARP_TEST_SPACE");
            var spaceDescription = "Created by C# test - UpdateObjectTest";

            // Log in
            var userId = UserSystemTests.LogIn(userSystem);

            LogDebug(userId);

            // Create space
            var space = SpaceSystemTests.CreateSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            var connection = CreateMultiplayerConnection(space.Id);
            var entitySystem = connection.GetSpaceEntitySystem();

            Connect(connection);

            // Create object
            var name = "TestObject";
            CreateObject(entitySystem, name, out var entity);

            Assert.AreEqual(entity.GetEntityType(), Multiplayer.SpaceEntityType.Object);
            Assert.AreEqual(entity.GetName(), name);

            using var position = entity.GetPosition();

            Assert.AreEqual(position.X, 0);
            Assert.AreEqual(position.Y, 0);
            Assert.AreEqual(position.Z, 0);

            var modelAssetId = "NotARealAssetId";
            ushort componentKey = 0;
            var gotUpdate = false;

            entity.OnUpdate += (s, e) =>
            {
                LogDebug("Received entity update");

                var component = e.arg1.GetComponent(componentKey);
                var staticModel = component.As<Multiplayer.StaticModelSpaceComponent>();

                Assert.AreEqual(staticModel.GetExternalResourceAssetId(), modelAssetId);

                gotUpdate = true;
            };

            // Add component to object 
            {
                var component = entity.AddComponent(Multiplayer.ComponentType.StaticModel);
                componentKey = component.GetId();

                var staticModel = component.As<Multiplayer.StaticModelSpaceComponent>();
                staticModel.SetExternalResourceAssetId(modelAssetId);

                LogDebug("Sending entity update...");

                entity.QueueUpdate();
                entitySystem.ProcessPendingEntityOperations();

                LogDebug("Entity update sent!");

                while (!gotUpdate)
                    Thread.Sleep(10);
            }
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_UPDATEREMOTEOBJECT_TEST
        [Test]
        public static void UpdateRemoteObjectTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _, out _, out _);

            var spaceName = GenerateUniqueString("CSP_CSHARP_TEST_SPACE");
            var spaceDescription = "Created by C# test - UpdateObjectTest";

            // Log in
            _ = userSystem.TestLogIn(UserSystemTests.AlternativeLoginEmail, UserSystemTests.AlternativeLoginPassword, Services.EResultCode.Success, false);

            var InviteUsers = new Systems.InviteUserRoleInfoCollection();
            InviteUsers.EmailLinkUrl = "https://dev.magnoverse.space";
            InviteUsers.SignupUrl = "https://dev.magnoverse.space";
            InviteUsers.InviteUserRoleInfos = new Csp.Common.Array<Systems.InviteUserRoleInfo>(1);
            InviteUsers.InviteUserRoleInfos[0] = new Systems.InviteUserRoleInfo { UserEmail = UserSystemTests.DefaultLoginEmail, UserRole = Systems.SpaceUserRole.User };

            // Create space
            var space = SpaceSystemTests.CreateSpace(spaceSystem, spaceName, spaceDescription, Systems.SpaceAttributes.Private, null, InviteUsers, null, pushCleanupFunction: false);

            var connection = CreateMultiplayerConnection(space.Id, false);
            var entitySystem = connection.GetSpaceEntitySystem();

            entitySystem.OnEntityCreated += (s, e) =>
            {
                LogDebug("Created Entity");
            };

            Connect(connection, false);

            // Create object
            var name = "TestObject";
            CreateObject(entitySystem, name, out var entity, false);

            entity.OnUpdate += (s, e) =>
            {
                LogDebug("Received entity update");
            };

            Assert.AreEqual(entity.GetEntityType(), Multiplayer.SpaceEntityType.Object);
            Assert.AreEqual(entity.GetName(), name);

            var entityID = entity.GetId();

            Disconnect(connection);
            connection.Dispose();

            UserSystemTests.LogOut(userSystem);

            _ = userSystem.TestLogIn(pushCleanupFunction: false);

            // Connect
            connection = CreateMultiplayerConnection(space.Id, pushCleanupFunction: false);
            entitySystem = connection.GetSpaceEntitySystem();

            var resetEvent = new ManualResetEvent(false);

            entitySystem.OnEntityCreated += (s, e) =>
            {
                if (e.GetId() == entityID)
                    resetEvent.Set();
            };

            Connect(connection, pushCleanupFunction: false);

            resetEvent.WaitOne(10000);

            var otherUserEntity = entitySystem.FindSpaceEntityById(entityID);
            var originalOwnerID = otherUserEntity.GetOwnerId();

            otherUserEntity.SetPosition(new Common.Vector3(1.0f, 2.0f, 3.0f));

            // Send entity update
            otherUserEntity.QueueUpdate();

            entitySystem.ProcessPendingEntityOperations();

            Assert.AreEqual(1.0f, otherUserEntity.GetPosition().X);
            Assert.AreEqual(2.0f, otherUserEntity.GetPosition().Y);
            Assert.AreEqual(3.0f, otherUserEntity.GetPosition().Z);
            Assert.AreNotEqual(originalOwnerID, otherUserEntity.GetOwnerId());

            DeleteEntity(entitySystem, otherUserEntity);
            Disconnect(connection);
            connection.Dispose();

            UserSystemTests.LogOut(userSystem);

            // Log in
            _ = userSystem.TestLogIn(UserSystemTests.AlternativeLoginEmail, UserSystemTests.AlternativeLoginPassword, Services.EResultCode.Success);

            SpaceSystemTests.DeleteSpace(spaceSystem, space);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_RUN_SCRIPT_TEST
        [Test]
        public static void RunScriptTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _, out _, out _);

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-REWIND";

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            var connection = CreateMultiplayerConnection(space.Id);
            var entitySystem = connection.GetSpaceEntitySystem();

            var scriptSystemReady = new ManualResetEvent(false);

            entitySystem.OnEntityCreated += (s, e) => { };
            entitySystem.OnScriptSystemReady += (s, e) => {
                Console.WriteLine("ScriptSystemReady called");

                scriptSystemReady.Set();
            };

            Connect(connection);

            // we'll be using this in a few places below as part of the test, so we declare it upfront
            const string ScriptText = @"
                var entities = TheEntitySystem.getEntities();
                var entityIndex = TheEntitySystem.getIndexOfEntity(ThisEntity.id);

                globalThis.onTick = () =>
                {
                    var model = entities[entityIndex].getAnimatedModelComponents()[0];
                    model.position = [10, 10, 10];
                }

                ThisEntity.subscribeToMessage(""entityTick"", ""onTick"");
            ";

            // A local avatar needs to be created in order for LeaderElection to initialise the script system
            CreateAvatar(entitySystem, "Player 1", "MyCoolAvatar", out var avatar);

            var ok = scriptSystemReady.WaitOne(5000);

            Assert.IsTrue(ok);

            // Create an AnimatedModelComponent and have the script update it's position
            const string name = "ScriptTestObject";
            CreateObject(entitySystem, name, out var entity, false);

            var component = entity.AddComponent(Multiplayer.ComponentType.AnimatedModel);
            var animatedModelComponent = component.As<Multiplayer.AnimatedModelSpaceComponent>();

            component = entity.AddComponent(Multiplayer.ComponentType.ScriptData);
            var scriptComponent = component.As<Multiplayer.ScriptSpaceComponent>();

            entity.QueueUpdate();
            entitySystem.ProcessPendingEntityOperations();

            scriptComponent.SetScriptSource(ScriptText);

            ok = entity.GetScript().Invoke();

            Assert.IsTrue(ok);

            CSPFoundation.Tick();

            var ScriptHasErrors = entity.GetScript().HasError();

            Assert.IsFalse(ScriptHasErrors);
            Assert.AreEqual(animatedModelComponent.GetPosition().X, 10f);
            Assert.AreEqual(animatedModelComponent.GetPosition().Y, 10f);
            Assert.AreEqual(animatedModelComponent.GetPosition().Z, 10f);
        }

#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_USE_PORTAL_TEST
        [Test]
        public static void UsePortalTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _, out _, out _);

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create spaces
            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-REWIND";

            string testSpaceName2 = GenerateUniqueString("OLY-UNITTEST-MULTI-REWIND-2");
            string testSpaceDescription2 = "OLY-UNITTEST-MULTIDESC-REWIND";

            var space1 = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);
            var space2 = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName2, testSpaceDescription2, Systems.SpaceAttributes.Private, null, null, null);

            string portalSpaceId;

            {
                using var result = spaceSystem.EnterSpace(space1.Id).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);
            }

            {
                using var connection = CreateMultiplayerConnection(space1.Id, false);
                var entitySystem = connection.GetSpaceEntitySystem();

                entitySystem.OnEntityCreated += (s, e) => { };

                Connect(connection, false);

                // Ensure we're in the first space
                Assert.AreEqual(spaceSystem.GetCurrentSpace().Id, space1.Id);

                // Create Avatar
                var name = "TestAvatar";
                var avatarId = "NotARealAvatarId";
                CreateAvatar(entitySystem, name, avatarId, out var entity, false);

                // Create object to represent the portal
                var objectName = "TestObject";
                CreateObject(entitySystem, objectName, out var createdObject, false);

                // Create portal component
                var component = createdObject.AddComponent(Multiplayer.ComponentType.Portal);
                var portalComponent = component.As<Multiplayer.PortalSpaceComponent>();
                portalComponent.SetSpaceId(space2.Id);

                portalSpaceId = portalComponent.GetSpaceId();

                // Disconnect from the SignalR server
                Disconnect(connection);
            }

            spaceSystem.ExitSpace();

            /*
		        User would now interact with the portal
	        */

            {
                using var result = spaceSystem.EnterSpace(space2.Id).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);
            }

            {
                using var connection = CreateMultiplayerConnection(space2.Id, false);
                var entitySystem = connection.GetSpaceEntitySystem();

                entitySystem.OnEntityCreated += (s, e) => { };

                Connect(connection, false);

                // Ensure we're in the second space
                using var spaceIds = new Common.Array<string>(1);
                spaceIds[0] = portalSpaceId;

                using var result = spaceSystem.GetSpacesByIds(spaceIds).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);
                Assert.IsGreaterThan(result.GetSpaces().Size(), 0);

                using var spaces = result.GetSpaces();
                using var portalSpace = spaces[0];

                Assert.AreEqual(spaceSystem.GetCurrentSpace().Id, portalSpaceId);

                // Create Avatar
                var name = "TestAvatar";
                var avatarId = "NotARealAvatarId";
                CreateAvatar(entitySystem, name, avatarId, out var entity, false);

                // Clean up
                Disconnect(connection);
            }

            spaceSystem.ExitSpace();
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_PORTAL_SCRIPT_INTERFACE_TEST
        [Test]
        public static void PortalScriptInterfaceTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _, out _, out _);

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create spaces
            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-REWIND";

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            var connection = CreateMultiplayerConnection(space.Id);
            var entitySystem = connection.GetSpaceEntitySystem();

            entitySystem.OnEntityCreated += (s, e) => { };

            Connect(connection);

            // Create object to represent the portal
            var objectName = "TestObject";
            CreateObject(entitySystem, objectName, out var createdObject, false);

            // Create portal component
            var component = createdObject.AddComponent(Multiplayer.ComponentType.Portal);
            var portalComponent = component.As<Multiplayer.PortalSpaceComponent>();

            var initialPosition = new Common.Vector3(1.1f, 2.2f, 3.3f);
            portalComponent.SetSpaceId("initialTestSpaceId");
            portalComponent.SetIsEnabled(false);
            portalComponent.SetPosition(initialPosition);
            portalComponent.SetRadius(123.123f);

            createdObject.QueueUpdate();
            entitySystem.ProcessPendingEntityOperations();

            Assert.AreEqual(portalComponent.GetSpaceId(), "initialTestSpaceId");
            Assert.AreEqual(portalComponent.GetIsEnabled(), false);
            Assert.AreEqual(portalComponent.GetPosition().X, initialPosition.X);
            Assert.AreEqual(portalComponent.GetPosition().Y, initialPosition.Y);
            Assert.AreEqual(portalComponent.GetPosition().Z, initialPosition.Z);
            Assert.AreEqual(portalComponent.GetRadius(), 123.123f);

            // Setup script
            const string portalScriptText = @"
                var portal = ThisEntity.getPortalComponents()[0];
		        portal.spaceId = ""secondTestSpaceId"";
		        portal.isEnabled = true;
		        portal.position = [4.4, 5.5, 6.6];
		        portal.radius = 456.456;
            ";

            createdObject.GetScript().SetScriptSource(portalScriptText);
            createdObject.GetScript().Invoke();

            entitySystem.ProcessPendingEntityOperations();

            Assert.AreEqual(portalComponent.GetSpaceId(), "secondTestSpaceId");
            Assert.AreEqual(portalComponent.GetIsEnabled(), true);
            Assert.AreEqual(portalComponent.GetPosition().X, 4.4f);
            Assert.AreEqual(portalComponent.GetPosition().Y, 5.5f);
            Assert.AreEqual(portalComponent.GetPosition().Z, 6.6f);
            Assert.AreEqual(portalComponent.GetRadius(), 456.456f);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_PORTAL_THUMBNAIL_TEST
        [Test]
        public static void PortalThumbnailTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _, out _, out _);

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create spaces
            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-REWIND";

            var source = new Systems.FileAssetDataSource
            {
                FilePath = Path.GetFullPath("assets/OKO.png")
            };

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, source);

            var connection = CreateMultiplayerConnection(space.Id);
            var entitySystem = connection.GetSpaceEntitySystem();

            entitySystem.OnEntityCreated += (s, e) => { };

            Connect(connection);

            // Create object to represent the portal
            var objectName = "TestObject";
            CreateObject(entitySystem, objectName, out var createdObject, false);

            // Create portal component
            var component = createdObject.AddComponent(Multiplayer.ComponentType.Portal);
            var portalComponent = component.As<Multiplayer.PortalSpaceComponent>();

            portalComponent.SetSpaceId(space.Id);
            var result = portalComponent.GetSpaceThumbnail().Result;

            Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
            Assert.IsFalse(string.IsNullOrEmpty(result.GetUri()));
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_DELETE_MULTIPLE_ENTITIES_TEST
        [Test]
        public static void DeleteMultipleEntitiesTest()
        {
            // Test for OB-1046
            // If the rate limiter hasn't processed all PendingOutgoingUpdates after SpaceEntity deletion it will crash when trying to process them

            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _, out _, out _);

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-REWIND";

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            var connection = CreateMultiplayerConnection(space.Id);
            var entitySystem = connection.GetSpaceEntitySystem();

            entitySystem.OnEntityCreated += (s, e) => { };

            Connect(connection);

            // Create 3 seperate objects to ensure there is too many updates for the rate limiter to process in one tick

            // Create object
            var objectName = "TestObject";

            CreateObject(entitySystem, objectName, out var createdObject, false);
            var component = createdObject.AddComponent(Multiplayer.ComponentType.Image);
            createdObject.QueueUpdate();

            // Create object 2
            CreateObject(entitySystem, objectName, out var createdObject2, false);
            var component2 = createdObject2.AddComponent(Multiplayer.ComponentType.Image);
            createdObject2.QueueUpdate();

            // Create object 3
            CreateObject(entitySystem, objectName, out var createdObject3, false);
            var component3 = createdObject3.AddComponent(Multiplayer.ComponentType.Image);
            createdObject3.QueueUpdate();

            // Destroy Entites
            entitySystem.DestroyEntity(createdObject);
            entitySystem.DestroyEntity(createdObject2);
            entitySystem.DestroyEntity(createdObject3);

            CSPFoundation.Tick();

            createdObject3.Dispose();
            createdObject2.Dispose();
            createdObject.Dispose();
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_ASSET_PROCESSED_CALLBACK_TEST
        [Test]
        public static void AssetProcessedCallbackTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out _, out _, out _, out _, out _, out _);

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-REWIND";
            string testAssetCollectionName = GenerateUniqueString("OLY-UNITTEST-ASSETCOLLECTION-REWIND");
            string testAssetName = GenerateUniqueString("OLY-UNITTEST-ASSET-REWIND");

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            var connection = CreateMultiplayerConnection(space.Id);
            var entitySystem = connection.GetSpaceEntitySystem();

            entitySystem.OnEntityCreated += (s, e) => { };

            Connect(connection);

            // Setup Asset callback
            var assetDetailBlobChangedCallbackCalled = false;
            var callbackAssetId = "";

            connection.OnAssetDetailBlobChanged += (s, p) =>
            {
                if (assetDetailBlobChangedCallbackCalled)
                {
                    return;
                }

                Assert.AreEqual(p.ChangeType, Multiplayer.EAssetChangeType.Created);
                Assert.AreEqual(p.AssetType, Systems.EAssetType.MODEL);

                callbackAssetId = p.AssetId;
                assetDetailBlobChangedCallbackCalled = true;
            };

            // Create asset collection
            AssetSystemTests.CreateAssetCollection(assetSystem, space, null, testAssetCollectionName, null, null, out var collection);

            // Create asset
            AssetSystemTests.CreateAsset(assetSystem, collection, testAssetName, null, null, out var asset);

            // Upload data
            var source = new Systems.FileAssetDataSource
            {
                FilePath = Path.GetFullPath("assets/test.json")
            };

            AssetSystemTests.UploadAssetData(assetSystem, collection, asset, source, out var uri);

            // Wait for message
            var waitForTestTimeoutCountMs = 0;
            const int waitForTestTimeoutLimitMs = 2000;

            while (assetDetailBlobChangedCallbackCalled == false && waitForTestTimeoutCountMs < waitForTestTimeoutLimitMs)
            {
                Thread.Sleep(50);
                waitForTestTimeoutCountMs += 50;
            }

            Assert.IsTrue(assetDetailBlobChangedCallbackCalled);
            Assert.AreEqual(callbackAssetId, asset.Id);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_DELETE_SCRIPT_TEST
        [Test]
        public static void DeleteScriptTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out _, out _, out _, out _, out _, out _);

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-REWIND";
            string testAssetCollectionName = GenerateUniqueString("OLY-UNITTEST-ASSETCOLLECTION-REWIND");
            string testAssetName = GenerateUniqueString("OLY-UNITTEST-ASSET-REWIND");

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            var connection = CreateMultiplayerConnection(space.Id);
            var entitySystem = connection.GetSpaceEntitySystem();

            entitySystem.OnEntityCreated += (s, e) => { };

            Connect(connection);

            // Setup script
            const string scriptText = @"

                var entities = TheEntitySystem.getEntities();
		        var entityIndex = TheEntitySystem.getIndexOfEntity(ThisEntity.id);

		        globalThis.onTick = () => {
			        var entity = entities[entityIndex];
			        entity.position = [10, 10, 10];
		        }
 
		        ThisEntity.subscribeToMessage(""entityTick"", ""onTick"");

            ";

            // Create object
            var objectName = "TestObject";

            CreateObject(entitySystem, objectName, out var createdObject, false);

            // Create script
            var component = createdObject.AddComponent(Multiplayer.ComponentType.ScriptData);
            var scriptComponent = component.As<Multiplayer.ScriptSpaceComponent>();

            scriptComponent.SetScriptSource(scriptText);
            createdObject.GetScript().Invoke();

            createdObject.QueueUpdate();
            entitySystem.ProcessPendingEntityOperations();

            // Ensure position is set to 0
            var position = createdObject.GetPosition();

            Assert.AreEqual(position.X, 0);
            Assert.AreEqual(position.Y, 0);
            Assert.AreEqual(position.Z, 0);

            // Delete script component
            createdObject.RemoveComponent(scriptComponent.GetId());

            createdObject.QueueUpdate();
            entitySystem.ProcessPendingEntityOperations();

            // Tick to attempt to call scripts tick event
            CSPFoundation.Tick();

            createdObject.QueueUpdate();
            entitySystem.ProcessPendingEntityOperations();

            // Ensure position is still set to 0
            position = createdObject.GetPosition();

            Assert.AreEqual(position.X, 0);
            Assert.AreEqual(position.Y, 0);
            Assert.AreEqual(position.Z, 0);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_DELETE_AND_CHANGE_COMPONENT_TEST
        [Test]
        public static void DeleteAndChangeComponentTest()
        {
            // Test for: OB-864
            // Second script deletion test adds a second component to the object with the script

            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out _, out _, out _, out _, out _, out _);

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-REWIND";
            string testAssetCollectionName = GenerateUniqueString("OLY-UNITTEST-ASSETCOLLECTION-REWIND");
            string testAssetName = GenerateUniqueString("OLY-UNITTEST-ASSET-REWIND");

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            var connection = CreateMultiplayerConnection(space.Id);
            var entitySystem = connection.GetSpaceEntitySystem();

            entitySystem.OnEntityCreated += (s, e) => { };

            Connect(connection);

            // Setup script
            const string scriptText = @"

                var entities = TheEntitySystem.getEntities();
		        var entityIndex = TheEntitySystem.getIndexOfEntity(ThisEntity.id);

		        globalThis.onTick = () => {
			        var entity = entities[entityIndex];
			        entity.position = [10, 10, 10];
		        }
 
		        ThisEntity.subscribeToMessage(""entityTick"", ""onTick"");

            ";

            // Create object
            var objectName = "TestObject";

            CreateObject(entitySystem, objectName, out var createdObject, false);

            // Create animated model component
            var component = createdObject.AddComponent(Multiplayer.ComponentType.AnimatedModel);
            var animatedComponent = component.As<Multiplayer.AnimatedModelSpaceComponent>();

            // Create script
            component = createdObject.AddComponent(Multiplayer.ComponentType.ScriptData);
            var scriptComponent = component.As<Multiplayer.ScriptSpaceComponent>();

            scriptComponent.SetScriptSource(scriptText);
            createdObject.GetScript().Invoke();

            createdObject.QueueUpdate();
            entitySystem.ProcessPendingEntityOperations();

            // Make a component update
            animatedComponent.SetPosition(Common.Vector3.One());

            // Delete script component
            createdObject.RemoveComponent(scriptComponent.GetId());

            createdObject.QueueUpdate();
            entitySystem.ProcessPendingEntityOperations();

            // Ensure entity update doesn't crash
            CSPFoundation.Tick();

            createdObject.QueueUpdate();
            entitySystem.ProcessPendingEntityOperations();
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_CONVERSATION_COMPONENT_TEST
        [Test]
        public static void ConversationComponentTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _, out _, out _);

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create spaces
            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-REWIND";

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            {
                var result = spaceSystem.EnterSpace(space.Id).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);
            }

            var connection = CreateMultiplayerConnection(space.Id);
            var entitySystem = connection.GetSpaceEntitySystem();

            entitySystem.OnEntityCreated += (s, e) => { };

            Connect(connection);

            // Create object to represent the conversation
            var objectName = "TestObject";
            CreateObject(entitySystem, objectName, out var createdObject, false);

            // Create Conversation component
            var component = createdObject.AddComponent(Multiplayer.ComponentType.Conversation);
            var conversationComponent = component.As<Multiplayer.ConversationSpaceComponent>();

            Assert.AreEqual(conversationComponent.GetIsActive(), true);
            Assert.AreEqual(conversationComponent.GetIsVisible(), true);

            conversationComponent.SetIsActive(false);
            conversationComponent.SetIsVisible(false);

            Assert.AreEqual(conversationComponent.GetIsActive(), false);
            Assert.AreEqual(conversationComponent.GetIsVisible(), false);

            var defaultTransform = new Multiplayer.SpaceTransform();
            Assert.AreEqual(conversationComponent.GetPosition().X, defaultTransform.Position.X);
            Assert.AreEqual(conversationComponent.GetPosition().Y, defaultTransform.Position.Y);
            Assert.AreEqual(conversationComponent.GetPosition().Z, defaultTransform.Position.Z);

            var newPosition = new Common.Vector3(1, 2, 3);
            conversationComponent.SetPosition(newPosition);

            Assert.AreEqual(conversationComponent.GetPosition().X, newPosition.X);
            Assert.AreEqual(conversationComponent.GetPosition().Y, newPosition.Y);
            Assert.AreEqual(conversationComponent.GetPosition().Z, newPosition.Z);

            Assert.AreEqual(conversationComponent.GetRotation().W, defaultTransform.Rotation.W);
            Assert.AreEqual(conversationComponent.GetRotation().X, defaultTransform.Rotation.X);
            Assert.AreEqual(conversationComponent.GetRotation().Y, defaultTransform.Rotation.Y);
            Assert.AreEqual(conversationComponent.GetRotation().Z, defaultTransform.Rotation.Z);

            var newRotation = new Common.Vector4(1, 2, 3, 4);
            conversationComponent.SetRotation(newRotation);

            Assert.AreEqual(conversationComponent.GetRotation().W, newRotation.W);
            Assert.AreEqual(conversationComponent.GetRotation().X, newRotation.X);
            Assert.AreEqual(conversationComponent.GetRotation().Y, newRotation.Y);
            Assert.AreEqual(conversationComponent.GetRotation().Z, newRotation.Z);

            Assert.AreEqual(conversationComponent.GetTitle(), "");
            Assert.AreEqual(conversationComponent.GetDate(), "");
            Assert.AreEqual(conversationComponent.GetNumberOfReplies(), 0);

            conversationComponent.SetTitle("TestTitle");
            conversationComponent.SetDate("01-02-1993");
            conversationComponent.SetNumberOfReplies(2);

            Assert.AreEqual(conversationComponent.GetTitle(), "TestTitle");
            Assert.AreEqual(conversationComponent.GetDate(), "01-02-1993");
            Assert.AreEqual(conversationComponent.GetNumberOfReplies(), 2);

            string conversationId;
            string messageId;

            {
                var result = conversationComponent.CreateConversation("TestMessage").Result;

                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);

                {
                    using var resultInfo = conversationComponent.GetConversationInfo().Result;

                    Assert.AreEqual(resultInfo.GetResultCode(), Services.EResultCode.Success);
                    Assert.IsFalse(resultInfo.GetConversationInfo().Resolved);

                    using var TestdefaultTransform = new Multiplayer.SpaceTransform();

                    Assert.AreEqual(resultInfo.GetConversationInfo().CameraPosition.Position.X, TestdefaultTransform.Position.X);
                    Assert.AreEqual(resultInfo.GetConversationInfo().CameraPosition.Position.Y, TestdefaultTransform.Position.Y);
                    Assert.AreEqual(resultInfo.GetConversationInfo().CameraPosition.Position.Z, TestdefaultTransform.Position.Z);

                    Assert.AreEqual(resultInfo.GetConversationInfo().CameraPosition.Rotation.W, TestdefaultTransform.Rotation.W);
                    Assert.AreEqual(resultInfo.GetConversationInfo().CameraPosition.Rotation.X, TestdefaultTransform.Rotation.X);
                    Assert.AreEqual(resultInfo.GetConversationInfo().CameraPosition.Rotation.Y, TestdefaultTransform.Rotation.Y);
                    Assert.AreEqual(resultInfo.GetConversationInfo().CameraPosition.Rotation.Z, TestdefaultTransform.Rotation.Z);

                    Assert.AreEqual(resultInfo.GetConversationInfo().CameraPosition.Scale.X, defaultTransform.Scale.X);
                    Assert.AreEqual(resultInfo.GetConversationInfo().CameraPosition.Scale.Y, defaultTransform.Scale.Y);
                    Assert.AreEqual(resultInfo.GetConversationInfo().CameraPosition.Scale.Z, defaultTransform.Scale.Z);
                    Assert.AreEqual(resultInfo.GetConversationInfo().Message, "TestMessage");
                }

                conversationId = result.GetValue();
            }

            {
                using var result = conversationComponent.GetConversationInfo().Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
                Assert.IsFalse(result.GetConversationInfo().Resolved);

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
            }

            {
                using var newData = new Multiplayer.ConversationInfo();
                using var cameraTransformValue = new Multiplayer.SpaceTransform(Common.Vector3.One(), Common.Vector4.One(), Common.Vector3.One());
                newData.Resolved = true;
                newData.CameraPosition = cameraTransformValue;
                newData.Message = "TestMessage1";

                using var result = conversationComponent.SetConversationInfo(newData).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
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
                Assert.AreEqual(result.GetConversationInfo().Message, "TestMessage1");
            }

            {
                using var result = conversationComponent.AddMessage("Test").Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);

                messageId = result.GetMessageInfo().Id;

                Assert.IsFalse(result.GetMessageInfo().Edited);
            }

            {
                using var result = conversationComponent.GetMessage(messageId).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);
                Assert.AreEqual(messageId, result.GetMessageInfo().Id);

            }

            {
                using var result = conversationComponent.GetMessageInfo(messageId).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
                Assert.IsFalse(result.GetMessageInfo().Edited);
            }

            {
                using var newData = new Multiplayer.MessageInfo();
                newData.Message = "newTest";

                using var result = conversationComponent.SetMessageInfo(messageId, newData).Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Success);
                Assert.IsTrue(result.GetMessageInfo().Edited);
            }

            {
                var result = conversationComponent.GetAllMessages().Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);
                Assert.AreEqual(result.GetTotalCount().ToString(), "1");
                Assert.AreEqual(messageId, result.GetMessages()[0].Id);
            }

            {
                var result = conversationComponent.DeleteMessage(messageId).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);
            }

            {
                var result = conversationComponent.DeleteConversation().Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);
            }

            spaceSystem.ExitSpace();
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_CONVERSATION_COMPONENT_MOVE_TEST
        [Test]
        public static void ConversationComponentMoveTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _, out _, out _);

            // Log in
            var defaultTestUserId = UserSystemTests.LogIn(userSystem);
            var defaultTestUserDisplayName = UserSystemTests.GetFullProfileByUserId(userSystem, defaultTestUserId).DisplayName;

            // Create spaces
            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-REWIND";

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            {
                var result = spaceSystem.EnterSpace(space.Id).Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);
            }

            var connection = CreateMultiplayerConnection(space.Id);
            var entitySystem = connection.GetSpaceEntitySystem();

            entitySystem.OnEntityCreated += (s, e) => { };

            Connect(connection);

            // Create object to represent the conversation
            var objectName1 = "TestObject1";
            CreateObject(entitySystem, objectName1, out var createdObject1, false);

            var objectName2 = "TestObject2";
            CreateObject(entitySystem, objectName2, out var createdObject2, false);

            // Create Conversation component
            var component1 = createdObject1.AddComponent(Multiplayer.ComponentType.Conversation);
            var conversationComponent1 = component1.As<Multiplayer.ConversationSpaceComponent>();
            var component2 = createdObject2.AddComponent(Multiplayer.ComponentType.Conversation);
            var conversationComponent2 = component2.As<Multiplayer.ConversationSpaceComponent>();

            string conversationId;

            {
                var result = conversationComponent1.CreateConversation("TestMessage").Result;
                var resCode = result.GetResultCode();
                Assert.AreEqual(resCode, Services.EResultCode.Success);
                conversationId = result.GetValue();
            }

            using var defaultTransform = new Multiplayer.SpaceTransform();

            {
                using var resultInfo = conversationComponent1.GetConversationInfo().Result;

                Assert.AreEqual(resultInfo.GetResultCode(), Services.EResultCode.Success);
                Assert.AreEqual(resultInfo.GetConversationInfo().ConversationId, conversationId);
                Assert.AreEqual(resultInfo.GetConversationInfo().UserID, defaultTestUserId);
                Assert.AreEqual(resultInfo.GetConversationInfo().UserDisplayName, defaultTestUserDisplayName);
                Assert.AreEqual(resultInfo.GetConversationInfo().Message, "TestMessage");
                Assert.IsFalse(resultInfo.GetConversationInfo().Edited);
                Assert.IsFalse(resultInfo.GetConversationInfo().Resolved);

                Assert.AreEqual(resultInfo.GetConversationInfo().CameraPosition.Position.X, defaultTransform.Position.X);
                Assert.AreEqual(resultInfo.GetConversationInfo().CameraPosition.Position.Y, defaultTransform.Position.Y);
                Assert.AreEqual(resultInfo.GetConversationInfo().CameraPosition.Position.Z, defaultTransform.Position.Z);

                Assert.AreEqual(resultInfo.GetConversationInfo().CameraPosition.Rotation.W, defaultTransform.Rotation.W);
                Assert.AreEqual(resultInfo.GetConversationInfo().CameraPosition.Rotation.X, defaultTransform.Rotation.X);
                Assert.AreEqual(resultInfo.GetConversationInfo().CameraPosition.Rotation.Y, defaultTransform.Rotation.Y);
                Assert.AreEqual(resultInfo.GetConversationInfo().CameraPosition.Rotation.Z, defaultTransform.Rotation.Z);

                Assert.AreEqual(resultInfo.GetConversationInfo().CameraPosition.Scale.X, defaultTransform.Scale.X);
                Assert.AreEqual(resultInfo.GetConversationInfo().CameraPosition.Scale.Y, defaultTransform.Scale.Y);
                Assert.AreEqual(resultInfo.GetConversationInfo().CameraPosition.Scale.Z, defaultTransform.Scale.Z);
            }

            {
                using var result = conversationComponent2.GetConversationInfo().Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Failed);
            }

            {
                var ok = conversationComponent2.MoveConversationFromComponent(conversationComponent1);

                Assert.IsTrue(ok);
            }

            {
                using var result = conversationComponent1.GetConversationInfo().Result;

                Assert.AreEqual(result.GetResultCode(), Services.EResultCode.Failed);
            }

            {
                using var resultInfo = conversationComponent2.GetConversationInfo().Result;

                Assert.AreEqual(resultInfo.GetResultCode(), Services.EResultCode.Success);
                Assert.AreEqual(resultInfo.GetConversationInfo().ConversationId, conversationId);
                Assert.AreEqual(resultInfo.GetConversationInfo().UserID, defaultTestUserId);
                Assert.AreEqual(resultInfo.GetConversationInfo().UserDisplayName, defaultTestUserDisplayName);
                Assert.AreEqual(resultInfo.GetConversationInfo().Message, "TestMessage");
                Assert.IsFalse(resultInfo.GetConversationInfo().Edited);
                Assert.IsFalse(resultInfo.GetConversationInfo().Resolved);

                Assert.AreEqual(resultInfo.GetConversationInfo().CameraPosition.Position.X, defaultTransform.Position.X);
                Assert.AreEqual(resultInfo.GetConversationInfo().CameraPosition.Position.Y, defaultTransform.Position.Y);
                Assert.AreEqual(resultInfo.GetConversationInfo().CameraPosition.Position.Z, defaultTransform.Position.Z);

                Assert.AreEqual(resultInfo.GetConversationInfo().CameraPosition.Rotation.W, defaultTransform.Rotation.W);
                Assert.AreEqual(resultInfo.GetConversationInfo().CameraPosition.Rotation.X, defaultTransform.Rotation.X);
                Assert.AreEqual(resultInfo.GetConversationInfo().CameraPosition.Rotation.Y, defaultTransform.Rotation.Y);
                Assert.AreEqual(resultInfo.GetConversationInfo().CameraPosition.Rotation.Z, defaultTransform.Rotation.Z);

                Assert.AreEqual(resultInfo.GetConversationInfo().CameraPosition.Scale.X, defaultTransform.Scale.X);
                Assert.AreEqual(resultInfo.GetConversationInfo().CameraPosition.Scale.Y, defaultTransform.Scale.Y);
                Assert.AreEqual(resultInfo.GetConversationInfo().CameraPosition.Scale.Z, defaultTransform.Scale.Z);
            }

            {
                // Test moving when the destination component already has a conversation associated
                var ok = conversationComponent2.MoveConversationFromComponent(conversationComponent1);

                Assert.IsFalse(ok);
            }

            {
                using var result = conversationComponent2.DeleteConversation().Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);
            }

            spaceSystem.ExitSpace();
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_CONVERSATION_COMPONENT_SCRIPT_TEST
        [Test]
        public static void ConversationComponentScriptTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _, out _, out _);

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create spaces
            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-REWIND";

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            var connection = CreateMultiplayerConnection(space.Id);
            var entitySystem = connection.GetSpaceEntitySystem();

            entitySystem.OnEntityCreated += (s, e) => { };

            Connect(connection);

            // Create object to represent the audio
            var objectName = "TestObject";
            CreateObject(entitySystem, objectName, out var createdObject, false);

            // Create conversation component
            var component = createdObject.AddComponent(Multiplayer.ComponentType.Conversation);
            var conversationComponent = component.As<Multiplayer.ConversationSpaceComponent>();

            createdObject.QueueUpdate();
            entitySystem.ProcessPendingEntityOperations();

            // Setup script
            const string conversationScriptText = @"
			    var conversation = ThisEntity.getConversationComponents()[0];
			    conversation.isVisible = false;
			    conversation.isActive = false;
			    conversation.position = [1,2,3];
			    conversation.rotation = [4,5,6,7];
            ";

            createdObject.GetScript().SetScriptSource(conversationScriptText);
            createdObject.GetScript().Invoke();

            entitySystem.ProcessPendingEntityOperations();

            // Ensure values are set correctly

            Assert.IsFalse(conversationComponent.GetIsVisible());
            Assert.IsFalse(conversationComponent.GetIsActive());

            using var newPosition = new Common.Vector3(1, 2, 3);

            Assert.AreEqual(conversationComponent.GetPosition().X, newPosition.X);
            Assert.AreEqual(conversationComponent.GetPosition().Y, newPosition.Y);
            Assert.AreEqual(conversationComponent.GetPosition().Z, newPosition.Z);

            using var newRotation = new Common.Vector4(4, 5, 6, 7);

            Assert.AreEqual(conversationComponent.GetRotation().W, newRotation.W);
            Assert.AreEqual(conversationComponent.GetRotation().X, newRotation.X);
            Assert.AreEqual(conversationComponent.GetRotation().Y, newRotation.Y);
            Assert.AreEqual(conversationComponent.GetRotation().Z, newRotation.Z);
        }
#endif


#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_AUDIO_COMPONENT_TEST
        [Test]
        public static void AudioComponentTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out _, out _, out _, out _, out _, out _);

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-REWIND";

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            // Connect to the SignalR server
            var connection = CreateMultiplayerConnection(space.Id);
            var entitySystem = connection.GetSpaceEntitySystem();

            entitySystem.OnEntityCreated += (s, e) => { };

            Connect(connection);

            // Create object to represent the audio component
            var objectName = "TestObject";
            CreateObject(entitySystem, objectName, out var createdObject, false);

            // Create audio component
            var component = createdObject.AddComponent(Multiplayer.ComponentType.Audio);
            var audioComponent = component.As<Multiplayer.AudioSpaceComponent>();

            // Ensure defaults are set
            var pos = audioComponent.GetPosition();

            Assert.AreEqual(pos.X, 0);
            Assert.AreEqual(pos.Y, 0);
            Assert.AreEqual(pos.Z, 0);
            Assert.AreEqual(audioComponent.GetPlaybackState(), Multiplayer.AudioPlaybackState.Reset);
            Assert.AreEqual(audioComponent.GetAudioType(), Multiplayer.AudioType.Global);
            Assert.AreEqual(audioComponent.GetAudioAssetId(), "");
            Assert.AreEqual(audioComponent.GetAssetCollectionId(), "");
            Assert.AreEqual(audioComponent.GetAttenuationRadius(), 10);
            Assert.AreEqual(audioComponent.GetIsLoopPlayback(), false);
            Assert.AreEqual(audioComponent.GetTimeSincePlay(), 0);
            Assert.AreEqual(audioComponent.GetVolume(), 1.0f);
            Assert.AreEqual(audioComponent.GetIsEnabled(), true);

            // Set new values
            const string assetId = "TEST_ASSET_ID";
            const string assetCollectionId = "TEST_COLLECTION_ID";

            audioComponent.SetPosition(Common.Vector3.One());
            audioComponent.SetPlaybackState(Multiplayer.AudioPlaybackState.Play);
            audioComponent.SetAudioType(Multiplayer.AudioType.Spatial);
            audioComponent.SetAudioAssetId(assetId);
            audioComponent.SetAssetCollectionId(assetCollectionId);
            audioComponent.SetAttenuationRadius(100);
            audioComponent.SetIsLoopPlayback(true);
            audioComponent.SetTimeSincePlay(1);
            audioComponent.SetVolume(0.33f);
            audioComponent.SetIsEnabled(false);

            // Ensure values are set correctly
            pos.Dispose();
            pos = audioComponent.GetPosition();

            Assert.AreEqual(pos.X, 1);
            Assert.AreEqual(pos.Y, 1);
            Assert.AreEqual(pos.Z, 1);
            Assert.AreEqual(audioComponent.GetPlaybackState(), Multiplayer.AudioPlaybackState.Play);
            Assert.AreEqual(audioComponent.GetAudioType(), Multiplayer.AudioType.Spatial);
            Assert.AreEqual(audioComponent.GetAudioAssetId(), assetId);
            Assert.AreEqual(audioComponent.GetAssetCollectionId(), assetCollectionId);
            Assert.AreEqual(audioComponent.GetAttenuationRadius(), 100);
            Assert.AreEqual(audioComponent.GetIsLoopPlayback(), true);
            Assert.AreEqual(audioComponent.GetTimeSincePlay(), 1);
            Assert.AreEqual(audioComponent.GetVolume(), 0.33f);
            Assert.AreEqual(audioComponent.GetIsEnabled(), false);

            // Test invalid volume values
            audioComponent.SetVolume(1.5f);

            Assert.AreEqual(audioComponent.GetVolume(), 0.33f);

            audioComponent.SetVolume(-2.5f);

            Assert.AreEqual(audioComponent.GetVolume(), 0.33f);

            // Test boundary volume values
            audioComponent.SetVolume(1.0f);

            Assert.AreEqual(audioComponent.GetVolume(), 1.0f);

            audioComponent.SetVolume(0.0f);

            Assert.AreEqual(audioComponent.GetVolume(), 0.0f);

            pos.Dispose();
        }
#endif


#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_IMAGE_COMPONENT_TEST
        [Test]
        public static void ImageComponentTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out _, out _, out _, out _, out _, out _);

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-REWIND";

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            // Connect to the SignalR server
            var connection = CreateMultiplayerConnection(space.Id);
            var entitySystem = connection.GetSpaceEntitySystem();

            entitySystem.OnEntityCreated += (s, e) => { };

            Connect(connection);

            // Create object to represent the audio component
            var objectName = "TestObject";
            CreateObject(entitySystem, objectName, out var createdObject, false);

            // Create audio component
            var component = createdObject.AddComponent(Multiplayer.ComponentType.Image);
            var imageComponent = component.As<Multiplayer.ImageSpaceComponent>();

            // Ensure defaults are set
            var pos = imageComponent.GetPosition();

            Assert.AreEqual(pos.X, 0);
            Assert.AreEqual(pos.Y, 0);
            Assert.AreEqual(pos.Z, 0);

            // Ensure defaults are set
            Assert.AreEqual(imageComponent.GetBillboardMode(), Multiplayer.BillboardMode.Off);
            Assert.AreEqual(imageComponent.GetDisplayMode(), Multiplayer.DisplayMode.DoubleSided);
            Assert.AreEqual(imageComponent.GetIsARVisible(), true);
            Assert.AreEqual(imageComponent.GetIsEmissive(), false);

            // Set new values
            const string assetId = "TEST_ASSET_ID";
            const string assetCollectionId = "TEST_COLLECTION_ID";

            imageComponent.SetPosition(Common.Vector3.One());
            imageComponent.SetAssetCollectionId(assetCollectionId);
            imageComponent.SetImageAssetId(assetId);
            imageComponent.SetBillboardMode(Multiplayer.BillboardMode.Billboard);
            imageComponent.SetDisplayMode(Multiplayer.DisplayMode.SingleSided);
            imageComponent.SetIsARVisible(false);
            imageComponent.SetIsEmissive(true);

            // Ensure values are set correctly
            pos.Dispose();
            pos = imageComponent.GetPosition();

            Assert.AreEqual(pos.X, 1);
            Assert.AreEqual(pos.Y, 1);
            Assert.AreEqual(pos.Z, 1);
            Assert.AreEqual(imageComponent.GetAssetCollectionId(), assetCollectionId);
            Assert.AreEqual(imageComponent.GetImageAssetId(), assetId);
            Assert.AreEqual(imageComponent.GetBillboardMode(), Multiplayer.BillboardMode.Billboard);
            Assert.AreEqual(imageComponent.GetDisplayMode(), Multiplayer.DisplayMode.SingleSided);
            Assert.AreEqual(imageComponent.GetIsARVisible(), false);
            Assert.AreEqual(imageComponent.GetIsEmissive(), true);

            pos.Dispose();
        }
#endif


#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_REFLECTION_COMPONENT_TEST
        [Test]
        public static void ReflectionComponentTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out _, out _, out _, out _, out _, out _);

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-REWIND";

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            // Connect to the SignalR server
            var connection = CreateMultiplayerConnection(space.Id);
            var entitySystem = connection.GetSpaceEntitySystem();

            entitySystem.OnEntityCreated += (s, e) => { };

            Connect(connection);

            // Create object to represent the audio component
            var objectName = "TestObject";
            CreateObject(entitySystem, objectName, out var createdObject, false);

            // Create audio component
            var component = createdObject.AddComponent(Multiplayer.ComponentType.Reflection);
            var reflectionComponent = component.As<Multiplayer.ReflectionSpaceComponent>();

            // Ensure defaults are set
            var pos = reflectionComponent.GetPosition();

            Assert.AreEqual(pos.X, 0);
            Assert.AreEqual(pos.Y, 0);
            Assert.AreEqual(pos.Z, 0);

            // Ensure defaults are set
            Assert.AreEqual(reflectionComponent.GetReflectionShape(), Multiplayer.ReflectionShape.UnitBox);

            // Set new values
            const string assetId = "TEST_ASSET_ID";
            const string assetCollectionId = "TEST_COLLECTION_ID";

            reflectionComponent.SetPosition(Common.Vector3.One());
            reflectionComponent.SetAssetCollectionId(assetCollectionId);
            reflectionComponent.SetReflectionAssetId(assetId);
            reflectionComponent.SetReflectionShape(Multiplayer.ReflectionShape.UnitSphere);

            // Ensure values are set correctly
            pos.Dispose();
            pos = reflectionComponent.GetPosition();

            Assert.AreEqual(pos.X, 1);
            Assert.AreEqual(pos.Y, 1);
            Assert.AreEqual(pos.Z, 1);
            Assert.AreEqual(reflectionComponent.GetAssetCollectionId(), assetCollectionId);
            Assert.AreEqual(reflectionComponent.GetReflectionAssetId(), assetId);
            Assert.AreEqual(reflectionComponent.GetReflectionShape(), Multiplayer.ReflectionShape.UnitSphere);

            pos.Dispose();
        }
#endif


#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_VIDEO_PLAYER_COMPONENT_TEST
        [Test]
        public static void VideoPlayerComponentTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out _, out _, out _, out _, out _, out _);

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-REWIND";

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            var connection = CreateMultiplayerConnection(space.Id);
            var entitySystem = connection.GetSpaceEntitySystem();

            entitySystem.OnEntityCreated += (s, e) => { };

            Connect(connection);

            // Create object to represent the audio component
            var objectName = "TestObject";
            CreateObject(entitySystem, objectName, out var createdObject, false);

            // Create audio component
            var component = createdObject.AddComponent(Multiplayer.ComponentType.VideoPlayer);
            var videoComponent = component.As<Multiplayer.VideoPlayerSpaceComponent>();

            // Ensure defaults are set
            var pos = videoComponent.GetPosition();

            Assert.AreEqual(pos.X, 0);
            Assert.AreEqual(pos.Y, 0);
            Assert.AreEqual(pos.Z, 0);

            // Ensure defaults are set
            Assert.AreEqual(videoComponent.GetPlaybackState(), Multiplayer.VideoPlayerPlaybackState.Reset);
            Assert.AreEqual(videoComponent.GetVideoAssetURL(), "");
            Assert.AreEqual(videoComponent.GetVideoAssetId(), "");
            Assert.AreEqual(videoComponent.GetAssetCollectionId(), "");
            Assert.AreEqual(videoComponent.GetAttenuationRadius(), 10);
            Assert.AreEqual(videoComponent.GetIsLoopPlayback(), false);
            Assert.AreEqual(videoComponent.GetTimeSincePlay(), 0);
            Assert.AreEqual(videoComponent.GetIsStateShared(), false);
            Assert.AreEqual(videoComponent.GetIsAutoPlay(), false);
            Assert.AreEqual(videoComponent.GetIsAutoResize(), false);
            Assert.AreEqual(videoComponent.GetCurrentPlayheadPosition(), 0);
            Assert.AreEqual(videoComponent.GetVideoPlayerSourceType(), Multiplayer.VideoPlayerSourceType.AssetSource);
            Assert.AreEqual(videoComponent.GetIsVisible(), true);
            Assert.AreEqual(videoComponent.GetIsEnabled(), true);

            // Set new values
            const string assetId = "TEST_ASSET_ID";
            const string assetCollectionId = "TEST_COLLECTION_ID";

            videoComponent.SetPosition(Common.Vector3.One());
            videoComponent.SetPlaybackState(Multiplayer.VideoPlayerPlaybackState.Play);
            videoComponent.SetVideoAssetURL("http://youtube.com/avideo");
            videoComponent.SetVideoAssetId(assetId);
            videoComponent.SetAssetCollectionId(assetCollectionId);
            videoComponent.SetAttenuationRadius(100);
            videoComponent.SetIsLoopPlayback(true);
            videoComponent.SetTimeSincePlay(1);
            videoComponent.SetIsStateShared(true);
            videoComponent.SetIsAutoPlay(true);
            videoComponent.SetIsAutoResize(true);
            videoComponent.SetCurrentPlayheadPosition(1.0f);
            videoComponent.SetVideoPlayerSourceType(Multiplayer.VideoPlayerSourceType.URLSource);
            videoComponent.SetIsVisible(false);
            videoComponent.SetIsEnabled(false);

            pos.Dispose();
            pos = videoComponent.GetPosition();

            // Ensure values are set correctly
            Assert.AreEqual(pos.X, 1);
            Assert.AreEqual(pos.Y, 1);
            Assert.AreEqual(pos.Z, 1);
            Assert.AreEqual(videoComponent.GetPlaybackState(), Multiplayer.VideoPlayerPlaybackState.Play);
            Assert.AreEqual(videoComponent.GetVideoAssetURL(), "http://youtube.com/avideo");
            Assert.AreEqual(videoComponent.GetAssetCollectionId(), assetCollectionId);
            Assert.AreEqual(videoComponent.GetVideoAssetId(), assetId);
            Assert.AreEqual(videoComponent.GetAttenuationRadius(), 100);
            Assert.AreEqual(videoComponent.GetIsLoopPlayback(), true);
            Assert.AreEqual(videoComponent.GetTimeSincePlay(), 1);
            Assert.AreEqual(videoComponent.GetIsStateShared(), true);
            Assert.AreEqual(videoComponent.GetIsAutoPlay(), true);
            Assert.AreEqual(videoComponent.GetIsAutoResize(), true);
            Assert.AreEqual(videoComponent.GetCurrentPlayheadPosition(), 1.0f);
            Assert.AreEqual(videoComponent.GetVideoPlayerSourceType(), Multiplayer.VideoPlayerSourceType.URLSource);
            Assert.AreEqual(videoComponent.GetIsVisible(), false);
            Assert.AreEqual(videoComponent.GetIsEnabled(), false);

            pos.Dispose();
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_COLLISION_COMPONENT_TEST
        [Test]
        public static void CollisionComponentTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out _, out _, out _, out _, out _, out _);

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-REWIND";

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            var connection = CreateMultiplayerConnection(space.Id);
            var entitySystem = connection.GetSpaceEntitySystem();

            entitySystem.OnEntityCreated += (s, e) => { };

            Connect(connection);

            // Create object to represent the Collision component
            var objectName = "TestObject";
            CreateObject(entitySystem, objectName, out var createdObject, false);

            // Create Collision component
            var component = createdObject.AddComponent(Multiplayer.ComponentType.Collision);
            var collisionComponent = component.As<Multiplayer.CollisionSpaceComponent>();

            // Ensure defaults are set
            var pos = collisionComponent.GetPosition();

            Assert.AreEqual(pos.X, 0);
            Assert.AreEqual(pos.Y, 0);
            Assert.AreEqual(pos.Z, 0);

            var scale = collisionComponent.GetScale();

            Assert.AreEqual(scale.X, 1);
            Assert.AreEqual(scale.Y, 1);
            Assert.AreEqual(scale.Z, 1);

            Common.Vector3 unscaledBoundingMin = collisionComponent.GetUnscaledBoundingBoxMin();
            Common.Vector3 unscaledBoundingMax = collisionComponent.GetUnscaledBoundingBoxMax();
            Common.Vector3 scaledBoundingMin = collisionComponent.GetScaledBoundingBoxMin();
            Common.Vector3 scaledBoundingMax = collisionComponent.GetScaledBoundingBoxMax();

            // Ensure defaults are set
            Assert.AreEqual(unscaledBoundingMin.X, -0.5f);
            Assert.AreEqual(unscaledBoundingMin.Y, -0.5f);
            Assert.AreEqual(unscaledBoundingMin.Z, -0.5f);
            Assert.AreEqual(unscaledBoundingMax.X, 0.5f);
            Assert.AreEqual(unscaledBoundingMax.Y, 0.5f);
            Assert.AreEqual(unscaledBoundingMax.Z, 0.5f);
            Assert.AreEqual(scaledBoundingMin.X, -0.5f);
            Assert.AreEqual(scaledBoundingMin.Y, -0.5f);
            Assert.AreEqual(scaledBoundingMin.Z, -0.5f);
            Assert.AreEqual(scaledBoundingMax.X, 0.5f);
            Assert.AreEqual(scaledBoundingMax.Y, 0.5f);
            Assert.AreEqual(scaledBoundingMax.Z, 0.5f);

            Assert.AreEqual(collisionComponent.GetCollisionMode(), Multiplayer.CollisionMode.Collision);
            Assert.AreEqual(collisionComponent.GetCollisionShape(), Multiplayer.CollisionShape.Box);
            Assert.AreEqual(collisionComponent.GetCollisionAssetId(), "");
            Assert.AreEqual(collisionComponent.GetAssetCollectionId(), "");

            // Set new values
            collisionComponent.SetPosition(Common.Vector3.One());
            collisionComponent.SetScale(new Common.Vector3(2f, 2f, 2f));
            collisionComponent.SetCollisionMode(Multiplayer.CollisionMode.Trigger);
            collisionComponent.SetCollisionShape(Multiplayer.CollisionShape.Mesh);
            collisionComponent.SetCollisionAssetId("TestAssetID");
            collisionComponent.SetAssetCollectionId("TestAssetCollectionID");

            // Ensure values are set correctly
            pos.Dispose();
            pos = collisionComponent.GetPosition();

            Assert.AreEqual(pos.X, 1);
            Assert.AreEqual(pos.Y, 1);
            Assert.AreEqual(pos.Z, 1);

            scale.Dispose();
            scale = collisionComponent.GetScale();

            Assert.AreEqual(scale.X, 2);
            Assert.AreEqual(scale.Y, 2);
            Assert.AreEqual(scale.Z, 2);

            unscaledBoundingMin = collisionComponent.GetUnscaledBoundingBoxMin();
            unscaledBoundingMax = collisionComponent.GetUnscaledBoundingBoxMax();
            scaledBoundingMin = collisionComponent.GetScaledBoundingBoxMin();
            scaledBoundingMax = collisionComponent.GetScaledBoundingBoxMax();

            Assert.AreEqual(unscaledBoundingMin.X, -0.5f);
            Assert.AreEqual(unscaledBoundingMin.Y, -0.5f);
            Assert.AreEqual(unscaledBoundingMin.Z, -0.5f);
            Assert.AreEqual(unscaledBoundingMax.X, 0.5f);
            Assert.AreEqual(unscaledBoundingMax.Y, 0.5f);
            Assert.AreEqual(unscaledBoundingMax.Z, 0.5f);
            Assert.AreEqual(scaledBoundingMin.X, -1f);
            Assert.AreEqual(scaledBoundingMin.Y, -1f);
            Assert.AreEqual(scaledBoundingMin.Z, -1f);
            Assert.AreEqual(scaledBoundingMax.X, 1f);
            Assert.AreEqual(scaledBoundingMax.Y, 1f);
            Assert.AreEqual(scaledBoundingMax.Z, 1f);
            Assert.AreEqual(collisionComponent.GetCollisionMode(), Multiplayer.CollisionMode.Trigger);
            Assert.AreEqual(collisionComponent.GetCollisionShape(), Multiplayer.CollisionShape.Mesh);
            Assert.AreEqual(collisionComponent.GetCollisionAssetId(), "TestAssetID");
            Assert.AreEqual(collisionComponent.GetAssetCollectionId(), "TestAssetCollectionID");

            var defaultSphereRadius = Multiplayer.CollisionSpaceComponent.GetDefaultSphereRadius();
            var defaultCapsuleHalfWidth = Multiplayer.CollisionSpaceComponent.GetDefaultCapsuleHalfWidth();
            var defaultCapsuleHalfHeight = Multiplayer.CollisionSpaceComponent.GetDefaultCapsuleHalfHeight();

            Assert.AreEqual(defaultSphereRadius, 0.5f);
            Assert.AreEqual(defaultCapsuleHalfWidth, 0.5f);
            Assert.AreEqual(defaultCapsuleHalfHeight, 1.0f);

            pos.Dispose();
            scale.Dispose();
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_AUDIO_SCRIPT_INTERFACE_TEST
        [Test]
        public static void AudioScriptInterfaceTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _, out _, out _);

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create spaces
            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-REWIND";

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            var connection = CreateMultiplayerConnection(space.Id);
            var entitySystem = connection.GetSpaceEntitySystem();

            entitySystem.OnEntityCreated += (s, e) => { };

            Connect(connection);

            // Create object to represent the audio
            var objectName = "TestObject";
            CreateObject(entitySystem, objectName, out var createdObject, false);

            // Create audio component
            var component = createdObject.AddComponent(Multiplayer.ComponentType.Audio);
            var audioComponent = component.As<Multiplayer.AudioSpaceComponent>();

            createdObject.QueueUpdate();
            entitySystem.ProcessPendingEntityOperations();

            // Setup script
            const string audioScriptText = @"

                const assetId			= ""TEST_ASSET_ID"";
                const assetCollectionId = ""TEST_COLLECTION_ID"";

                var audio = ThisEntity.getAudioComponents()[0];
                audio.position = [1, 1, 1];
                audio.playbackState = 2;
                audio.audioType = 1;
                audio.audioAssetId = assetId;
                audio.assetCollectionId = assetCollectionId;
                audio.attenuationRadius = 100;
                audio.isLoopPlayback = true;
                audio.timeSincePlay = 1;
                audio.volume = 0.25;

            ";

            createdObject.GetScript().SetScriptSource(audioScriptText);
            createdObject.GetScript().Invoke();

            entitySystem.ProcessPendingEntityOperations();

            // Ensure values are set correctly
            const string assetId = "TEST_ASSET_ID";
            const string assetCollectionId = "TEST_COLLECTION_ID";

            using var pos = audioComponent.GetPosition();

            Assert.AreEqual(pos.X, 1);
            Assert.AreEqual(pos.Y, 1);
            Assert.AreEqual(pos.Z, 1);
            Assert.AreEqual(audioComponent.GetPlaybackState(), Multiplayer.AudioPlaybackState.Play);
            Assert.AreEqual(audioComponent.GetAudioType(), Multiplayer.AudioType.Spatial);
            Assert.AreEqual(audioComponent.GetAudioAssetId(), assetId);
            Assert.AreEqual(audioComponent.GetAssetCollectionId(), assetCollectionId);
            Assert.AreEqual(audioComponent.GetAttenuationRadius(), 100);
            Assert.AreEqual(audioComponent.GetIsLoopPlayback(), true);
            Assert.AreEqual(audioComponent.GetTimeSincePlay(), 1);
            Assert.AreEqual(audioComponent.GetVolume(), 0.25f);

            // Test invalid volume values
            string volumeTestScriptText = "var audio = ThisEntity.getAudioComponents()[0];audio.volume = 1.25;";
            createdObject.GetScript().SetScriptSource(volumeTestScriptText);
            createdObject.GetScript().Invoke();
            entitySystem.ProcessPendingEntityOperations();
            Assert.AreEqual(audioComponent.GetVolume(), 0.25f);

            volumeTestScriptText = "var audio = ThisEntity.getAudioComponents()[0];audio.volume = -2.25;";
            createdObject.GetScript().SetScriptSource(volumeTestScriptText);
            createdObject.GetScript().Invoke();
            entitySystem.ProcessPendingEntityOperations();
            Assert.AreEqual(audioComponent.GetVolume(), 0.25f);

            // Test boundary volume values
            volumeTestScriptText = "var audio = ThisEntity.getAudioComponents()[0];audio.volume = 1.0;";
            createdObject.GetScript().SetScriptSource(volumeTestScriptText);
            createdObject.GetScript().Invoke();
            entitySystem.ProcessPendingEntityOperations();
            Assert.AreEqual(audioComponent.GetVolume(), 1.0f);

            volumeTestScriptText = "var audio = ThisEntity.getAudioComponents()[0];audio.volume = 0.0;";
            createdObject.GetScript().SetScriptSource(volumeTestScriptText);
            createdObject.GetScript().Invoke();
            entitySystem.ProcessPendingEntityOperations();
            Assert.AreEqual(audioComponent.GetVolume(), 0.0f);
        }
#endif



#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_IMAGE_SCRIPT_INTERFACE_TEST
        [Test]
        public static void ImageScriptInterfaceTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _, out _, out _);

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create spaces
            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-REWIND";

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            var connection = CreateMultiplayerConnection(space.Id);
            var entitySystem = connection.GetSpaceEntitySystem();

            entitySystem.OnEntityCreated += (s, e) => { };

            Connect(connection);

            // Create object to represent the image
            var objectName = "TestObject";
            CreateObject(entitySystem, objectName, out var createdObject, false);

            // Create image component
            var component = createdObject.AddComponent(Multiplayer.ComponentType.Image);
            var imageComponent = component.As<Multiplayer.ImageSpaceComponent>();

            Assert.AreEqual(imageComponent.GetBillboardMode(), Multiplayer.BillboardMode.Off);
            Assert.AreEqual(imageComponent.GetDisplayMode(), Multiplayer.DisplayMode.DoubleSided);
            Assert.AreEqual(imageComponent.GetIsVisible(), true);
            Assert.AreEqual(imageComponent.GetIsEmissive(), false);

            createdObject.QueueUpdate();
            entitySystem.ProcessPendingEntityOperations();

            // Setup script
            const string imageScriptText = @"

                const assetId			= ""TEST_ASSET_ID"";
                const assetCollectionId = ""TEST_COLLECTION_ID"";

                var image = ThisEntity.getImageComponents()[0];
		        image.position = [2, 1, 3];
                image.isVisible = false;
		        image.isEmissive = true;
		        image.displayMode = 2;
		        image.billboardMode = 1;

            ";

            createdObject.GetScript().SetScriptSource(imageScriptText);
            createdObject.GetScript().Invoke();

            entitySystem.ProcessPendingEntityOperations();

            using var pos = imageComponent.GetPosition();

            Assert.AreEqual(pos.X, 2);
            Assert.AreEqual(pos.Y, 1);
            Assert.AreEqual(pos.Z, 3);
            Assert.AreEqual(imageComponent.GetBillboardMode(), Multiplayer.BillboardMode.Billboard);
            Assert.AreEqual(imageComponent.GetDisplayMode(), Multiplayer.DisplayMode.DoubleSidedReversed);
            Assert.AreEqual(imageComponent.GetIsVisible(), false);
            Assert.AreEqual(imageComponent.GetIsEmissive(), true);
        }
#endif


#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_SPLINE_COMPONENT_TEST
        [Test]
        public static void SplineComponentTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out _, out _, out _, out _, out _, out _);

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-REWIND";

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            var connection = CreateMultiplayerConnection(space.Id);
            var entitySystem = connection.GetSpaceEntitySystem();

            entitySystem.OnEntityCreated += (s, e) => { };

            Connect(connection);

            // Create object to represent the spline component
            var objectName = "TestObject";
            CreateObject(entitySystem, objectName, out var createdObject, false);

            // Create spline component
            var component = createdObject.AddComponent(Multiplayer.ComponentType.Spline);
            var splineComponent = component.As<Multiplayer.SplineSpaceComponent>();

            // Build waypoints list
            using var wayPoints = new Common.List<Common.Vector3> { };
            using var pos0 = new Common.Vector3(0, 0, 0);
            using var pos1 = new Common.Vector3(0, 1000, 0);
            using var pos2 = new Common.Vector3(0, 2000, 0);
            using var pos3 = new Common.Vector3(0, 3000, 0);
            using var pos4 = new Common.Vector3(0, 4000, 0);
            using var pos5 = new Common.Vector3(0, 5000, 0);

            wayPoints.Append(pos0);
            wayPoints.Append(pos1);
            wayPoints.Append(pos2);
            wayPoints.Append(pos3);
            wayPoints.Append(pos4);
            wayPoints.Append(pos5);

            splineComponent.SetWaypoints(wayPoints);

            {
                // Evaluate cubic interpolate spline
                var result = splineComponent.GetLocationAlongSpline(1.0f);

                // Expect final waypoint to be the same
                Assert.AreEqual(result.X, wayPoints[wayPoints.Size() - 1].X);
                Assert.AreEqual(result.Y, wayPoints[wayPoints.Size() - 1].Y);
                Assert.AreEqual(result.Z, wayPoints[wayPoints.Size() - 1].Z);
            }
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_SPLINE_SCRIPT_INTERFACE_TEST
        [Test]
        public static void SplineScriptInterfaceTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _, out _, out _);

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create spaces
            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-REWIND";

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            var connection = CreateMultiplayerConnection(space.Id);
            var entitySystem = connection.GetSpaceEntitySystem();

            entitySystem.OnEntityCreated += (s, e) => { };

            Connect(connection);

            // Create object to represent the spline
            var objectName = "TestObject";
            CreateObject(entitySystem, objectName, out var createdObject, false);

            // Create spline component
            var component = createdObject.AddComponent(Multiplayer.ComponentType.Spline);
            var splineComponent = component.As<Multiplayer.SplineSpaceComponent>();

            createdObject.QueueUpdate();
            entitySystem.ProcessPendingEntityOperations();

            using var wayPoints = new Common.List<Common.Vector3> { };
            using var pos0 = new Common.Vector3(0, 0, 0);
            using var pos1 = new Common.Vector3(0, 1000, 0);
            using var pos2 = new Common.Vector3(0, 2000, 0);
            using var pos3 = new Common.Vector3(0, 3000, 0);
            using var pos4 = new Common.Vector3(0, 4000, 0);
            using var pos5 = new Common.Vector3(0, 5000, 0);

            wayPoints.Append(pos0);
            wayPoints.Append(pos1);
            wayPoints.Append(pos2);
            wayPoints.Append(pos3);
            wayPoints.Append(pos4);
            wayPoints.Append(pos5);

            // Setup script
            const string splineScriptText = @"
		        var spline = ThisEntity.getSplineComponents()[0];
		        
		        var waypoints = [[0, 0, 0], [0, 1000, 0], [0, 2000, 0], [0, 3000, 0], [0, 4000, 0], [0, 5000, 0]];
		        spline.setWaypoints(waypoints);
		        var positionResult = spline.getLocationAlongSpline(1);
            ";

            createdObject.GetScript().SetScriptSource(splineScriptText);
            createdObject.GetScript().Invoke();

            entitySystem.ProcessPendingEntityOperations();

            Assert.AreEqual(splineComponent.GetWaypoints()[wayPoints.Size() - 1].X, wayPoints[wayPoints.Size() - 1].X);
            Assert.AreEqual(splineComponent.GetWaypoints()[wayPoints.Size() - 1].Y, wayPoints[wayPoints.Size() - 1].Y);
            Assert.AreEqual(splineComponent.GetWaypoints()[wayPoints.Size() - 1].Z, wayPoints[wayPoints.Size() - 1].Z);
            Assert.AreEqual(splineComponent.GetWaypoints().Size(), wayPoints.Size());
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_USE_CUSTOM_TEST
        [Test]
        public static void UseCustomTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _, out _, out _);

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-REWIND");
            const string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-REWIND";
            const string objectName = "TestObject";
            const string applicationOrigin = "Application Origin";
            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            {
                using var connection = CreateMultiplayerConnection(space.Id, false);
                var entitySystem = connection.GetSpaceEntitySystem();

                entitySystem.OnEntityCreated += (s, e) => { };

                Connect(connection, false);

                // Create object to represent the Custom Component
                CreateObject(entitySystem, objectName, out var createdObject, false);

                // Create Custom component
                var component = createdObject.AddComponent(Multiplayer.ComponentType.Custom);
                var customComponent = component.As<Multiplayer.CustomSpaceComponent>();

                // Application Origin Check
                customComponent.SetApplicationOrigin(applicationOrigin);

                Assert.AreEqual(customComponent.GetApplicationOrigin(), applicationOrigin);

                // Vector Check
                customComponent.SetCustomProperty("Vector3", new Multiplayer.ReplicatedValue(new Common.Vector3(1, 1, 1)));

                Assert.AreEqual(customComponent.GetCustomProperty("Vector3").GetVector3().X, new Common.Vector3(1, 1, 1).X);
                Assert.AreEqual(customComponent.GetCustomProperty("Vector3").GetVector3().Y, new Common.Vector3(1, 1, 1).Y);
                Assert.AreEqual(customComponent.GetCustomProperty("Vector3").GetVector3().Z, new Common.Vector3(1, 1, 1).Z);

                customComponent.SetCustomProperty("Vector4", new Multiplayer.ReplicatedValue(new Common.Vector4(1, 1, 1, 1)));

                Assert.AreEqual(customComponent.GetCustomProperty("Vector4").GetVector4().W, new Common.Vector4(1, 1, 1, 1).W);
                Assert.AreEqual(customComponent.GetCustomProperty("Vector4").GetVector4().X, new Common.Vector4(1, 1, 1, 1).X);
                Assert.AreEqual(customComponent.GetCustomProperty("Vector4").GetVector4().Y, new Common.Vector4(1, 1, 1, 1).Y);
                Assert.AreEqual(customComponent.GetCustomProperty("Vector4").GetVector4().Z, new Common.Vector4(1, 1, 1, 1).Z);

                // String Check
                customComponent.SetCustomProperty("String", new Multiplayer.ReplicatedValue("OKO"));

                Assert.AreEqual(customComponent.GetCustomProperty("String").GetString(), "OKO");

                // Boolean Check
                customComponent.SetCustomProperty("Boolean", new Multiplayer.ReplicatedValue(true));

                Assert.AreEqual(customComponent.GetCustomProperty("Boolean").GetBool(), true);

                // Integer Check
                customComponent.SetCustomProperty("Integer", new Multiplayer.ReplicatedValue(1));

                Assert.AreEqual(customComponent.GetCustomProperty("Integer").GetInt(), 1);

                // Float Check
                customComponent.SetCustomProperty("Float", new Multiplayer.ReplicatedValue(1.0f));

                Assert.AreEqual(customComponent.GetCustomProperty("Float").GetFloat(), 1.0f);

                // Has Key Check
                Assert.AreEqual(customComponent.HasCustomProperty("Boolean"), true);
                Assert.AreEqual(customComponent.HasCustomProperty("BooleanFalse"), false);

                // Size Check, Including Application Origin
                Assert.AreEqual(customComponent.GetNumProperties(), 7);

                // Remove Key Check
                customComponent.RemoveCustomProperty("Boolean");

                Assert.AreEqual(customComponent.GetNumProperties(), 6);

                entitySystem.QueueEntityUpdate(createdObject);
                entitySystem.ProcessPendingEntityOperations();
                Thread.Sleep(3000);

                Disconnect(connection);
            }

            {
                using var connection = CreateMultiplayerConnection(space.Id, false);
                var entitySystem = connection.GetSpaceEntitySystem();

                var testComplete = false;

                entitySystem.OnEntityCreated += (s, e) =>
                {
                    Assert.AreEqual(e.GetName(), objectName);

                    Assert.AreEqual(e.GetComponents().Size(), (ulong)1);

                    var component = e.GetComponent(0);
                    Assert.AreEqual(component.GetComponentType(), Multiplayer.ComponentType.Custom);

                    var customComponent = component.As<Multiplayer.CustomSpaceComponent>();

                    // Application Origin Check
                    Assert.AreEqual(customComponent.GetApplicationOrigin(), applicationOrigin);

                    // Vector Check
                    Assert.AreEqual(customComponent.GetCustomProperty("Vector3").GetVector3().X, new Common.Vector3(1, 1, 1).X);
                    Assert.AreEqual(customComponent.GetCustomProperty("Vector3").GetVector3().Y, new Common.Vector3(1, 1, 1).Y);
                    Assert.AreEqual(customComponent.GetCustomProperty("Vector3").GetVector3().Z, new Common.Vector3(1, 1, 1).Z);

                    Assert.AreEqual(customComponent.GetCustomProperty("Vector4").GetVector4().W, new Common.Vector4(1, 1, 1, 1).W);
                    Assert.AreEqual(customComponent.GetCustomProperty("Vector4").GetVector4().X, new Common.Vector4(1, 1, 1, 1).X);
                    Assert.AreEqual(customComponent.GetCustomProperty("Vector4").GetVector4().Y, new Common.Vector4(1, 1, 1, 1).Y);
                    Assert.AreEqual(customComponent.GetCustomProperty("Vector4").GetVector4().Z, new Common.Vector4(1, 1, 1, 1).Z);

                    // String Check
                    Assert.AreEqual(customComponent.GetCustomProperty("String").GetString(), "OKO");

                    // Integer Check
                    Assert.AreEqual(customComponent.GetCustomProperty("Integer").GetInt(), 1);

                    // Float Check
                    Assert.AreEqual(customComponent.GetCustomProperty("Float").GetFloat(), 1.0f);

                    // Does Not Have Key Check
                    Assert.AreEqual(customComponent.HasCustomProperty("Boolean"), false);

                    // Flag Tests Completed
                    testComplete = true;
                };

                Connect(connection, false);

                // Wait until test complete
                while (!testComplete)
                {
                    Thread.Sleep(100);
                }

                Disconnect(connection);
            }
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_CUSTOM_COMPONENT_SCRIPT_INTERFACE_SUBSCRIPTION_TEST
        [Test]
        public static void CustomComponentScriptInterfaceSubscriptionTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _, out _, out _);

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create spaces
            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-REWIND";

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            var connection = CreateMultiplayerConnection(space.Id);
            var entitySystem = connection.GetSpaceEntitySystem();

            entitySystem.OnEntityCreated += (s, e) => { };
            
            Connect(connection);

            // A local avatar needs to be created in order for LeaderElection to initialise the script system
            CreateAvatar(entitySystem, "Player 1", "MyCoolAvatar", out var avatar);

            // Create object to represent the audio
            var objectName = "TestObject";
            CreateObject(entitySystem, objectName, out var createdObject, false);

            // Create audio component
            var component = createdObject.AddComponent(Multiplayer.ComponentType.Custom);
            var customComponent = component.As<Multiplayer.CustomSpaceComponent>();

            customComponent.SetCustomProperty("Number", new Multiplayer.ReplicatedValue(0));
            customComponent.SetCustomProperty("NumberChanged", new Multiplayer.ReplicatedValue(false));

            createdObject.QueueUpdate();
            entitySystem.ProcessPendingEntityOperations();

            // Setup script
            const string scriptText = @"
                var custom = ThisEntity.getCustomComponents()[0];
                custom.setCustomProperty(""testFloat"", 1.234);
                custom.setCustomProperty(""testInt"", 1234);

                globalThis.onValueChanged = () => {
                  custom.setCustomProperty(""NumberChanged"", true);
                }  

                // Subscribe to entity events 
                ThisEntity.subscribeToPropertyChange(custom.id, custom.getCustomPropertySubscriptionKey(""Number""), ""valueChanged"");
                ThisEntity.subscribeToMessage(""valueChanged"", ""onValueChanged"");    
            ";

            createdObject.GetScript().SetScriptSource(scriptText);
            createdObject.GetScript().Invoke();

            entitySystem.ProcessPendingEntityOperations();

            Assert.AreEqual(customComponent.GetCustomProperty("testFloat").GetFloat(), 1.234f);
            Assert.AreEqual(customComponent.GetCustomProperty("testInt").GetInt(), 1234);
            Assert.AreEqual(customComponent.GetCustomProperty("Number").GetInt(), 0);
            Assert.IsFalse(customComponent.GetCustomProperty("NumberChanged").GetBool());

            customComponent.SetCustomProperty("Number", new Multiplayer.ReplicatedValue(100));

            Assert.AreEqual(customComponent.GetCustomProperty("Number").GetInt(), 100);
            Assert.IsTrue(customComponent.GetCustomProperty("NumberChanged").GetBool());
        }
#endif


#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_NETWORKEVENT_TEST
        [Test]
        public static void NetworkEventTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-REWIND";

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            var connection = CreateMultiplayerConnection(space.Id);
            var entitySystem = connection.GetSpaceEntitySystem();

            Connect(connection);

            bool gotEvent = false;

            //TODO: Create callback delegate for recieve event
            void ListenDelegate(bool s, Common.Array<Multiplayer.ReplicatedValue> e)
            {
                for (ulong i = 0; i < e.Size(); i++)
                {
                    switch (i)
                    {
                        case 0:
                            Assert.AreEqual(e[i].GetString(), "TestString");
                            break;
                        case 1:
                            Assert.AreEqual(e[i].GetFloat(), 42.127f);
                            break;
                    }
                }

                gotEvent = true;
            }

            // Listen for network event
            connection.ListenNetworkEvent("TestEvent", ListenDelegate);

            // Send Network event to self
            using var TestString = new Multiplayer.ReplicatedValue("TestString");
            using var TestFloat = new Multiplayer.ReplicatedValue(42.127f);
            using var Params = new Common.Array<Multiplayer.ReplicatedValue>(2);

            Params[0] = TestString;
            Params[1] = TestFloat;

            connection.SendNetworkEventToClient("TestEvent", Params, connection.GetClientId());

            while (!gotEvent)
                Thread.Sleep(10);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_REGISTER_ACTION_HANDLER_TEST
        [Test]
        public static void RegisterActionHandlerTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out _, out _, out _, out _, out _, out _);

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-REWIND";

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            // Connect to the SignalR server
            var connection = CreateMultiplayerConnection(space.Id);
            var entitySystem = connection.GetSpaceEntitySystem();

            Connect(connection);
            
            // Create object to represent the spline component
            var objectName = "TestObject";
            CreateObject(entitySystem, objectName, out var createdObject, false);

            // Create spline component
            var component = createdObject.AddComponent(Multiplayer.ComponentType.Spline);
            var splineComponent = component.As<Multiplayer.SplineSpaceComponent>();

            bool ActionCalled = false;

            splineComponent.RegisterActionHandler("TestAction", (arg1, arg2, arg3) =>
            {
                ActionCalled = true;
            });

            splineComponent.InvokeAction("TestAction", "TestParam");

            Assert.IsTrue(ActionCalled);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_FOG_COMPONENT_TEST
        [Test]
        public static void FogComponentTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _, out _, out _);

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create spaces
            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-REWIND";

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            var connection = CreateMultiplayerConnection(space.Id);
            var entitySystem = connection.GetSpaceEntitySystem();

            entitySystem.OnEntityCreated += (s, e) => { };
            
            Connect(connection);

            // Create object to represent the fog
            var objectName = "TestObject";
            CreateObject(entitySystem, objectName, out var createdObject, false);

            // Create fog component
            var component = createdObject.AddComponent(Multiplayer.ComponentType.Fog);
            var fogComponent = component.As<Multiplayer.FogSpaceComponent>();

            // Ensure defaults are set
            Assert.AreEqual(fogComponent.GetFogMode(), Multiplayer.FogMode.Linear);
            Assert.AreEqual(fogComponent.GetPosition().X, 0);
            Assert.AreEqual(fogComponent.GetPosition().Y, 0);
            Assert.AreEqual(fogComponent.GetPosition().Z, 0);
            Assert.AreEqual(fogComponent.GetRotation().X, 0);
            Assert.AreEqual(fogComponent.GetRotation().Y, 0);
            Assert.AreEqual(fogComponent.GetRotation().Z, 0);
            Assert.AreEqual(fogComponent.GetRotation().W, 1);
            Assert.AreEqual(fogComponent.GetScale().X, 1);
            Assert.AreEqual(fogComponent.GetScale().Y, 1);
            Assert.AreEqual(fogComponent.GetScale().Z, 1);
            Assert.AreEqual(fogComponent.GetStartDistance(), 0.0f);
            Assert.AreEqual(fogComponent.GetEndDistance(), 0.0f);
            Assert.AreEqual(fogComponent.GetColor().X, 0.8f);
            Assert.AreEqual(fogComponent.GetColor().Y, 0.9f);
            Assert.AreEqual(fogComponent.GetColor().Z, 1.0f);
            Assert.AreEqual(fogComponent.GetDensity(), 0.2f);
            Assert.AreEqual(fogComponent.GetHeightFalloff(), 0.2f);
            Assert.AreEqual(fogComponent.GetMaxOpacity(), 1.0f);
            Assert.IsFalse(fogComponent.GetIsVolumetric());

            fogComponent.SetFogMode(Multiplayer.FogMode.Exponential);
            fogComponent.SetPosition(new Common.Vector3(1, 1, 1));
            fogComponent.SetRotation(new Common.Vector4(1, 1, 1, 2));
            fogComponent.SetScale(new Common.Vector3(2, 2, 2));
            fogComponent.SetStartDistance(1.1f);
            fogComponent.SetEndDistance(2.2f);
            fogComponent.SetColor(new Common.Vector3(1, 1, 1));
            fogComponent.SetDensity(3.0f);
            fogComponent.SetHeightFalloff(4.0f);
            fogComponent.SetMaxOpacity(5.0f);
            fogComponent.SetIsVolumetric(true);

            // Ensure values are set correctly
            Assert.AreEqual(fogComponent.GetFogMode(), Multiplayer.FogMode.Exponential);
            Assert.AreEqual(fogComponent.GetPosition().X, 1);
            Assert.AreEqual(fogComponent.GetPosition().Y, 1);
            Assert.AreEqual(fogComponent.GetPosition().Z, 1);
            Assert.AreEqual(fogComponent.GetRotation().X, 1);
            Assert.AreEqual(fogComponent.GetRotation().Y, 1);
            Assert.AreEqual(fogComponent.GetRotation().Z, 1);
            Assert.AreEqual(fogComponent.GetRotation().W, 2);
            Assert.AreEqual(fogComponent.GetScale().X, 2);
            Assert.AreEqual(fogComponent.GetScale().Y, 2);
            Assert.AreEqual(fogComponent.GetScale().Z, 2);
            Assert.AreEqual(fogComponent.GetStartDistance(), 1.1f);
            Assert.AreEqual(fogComponent.GetEndDistance(), 2.2f);
            Assert.AreEqual(fogComponent.GetColor().X, 1);
            Assert.AreEqual(fogComponent.GetColor().Y, 1);
            Assert.AreEqual(fogComponent.GetColor().Z, 1);
            Assert.AreEqual(fogComponent.GetDensity(), 3.0f);
            Assert.AreEqual(fogComponent.GetHeightFalloff(), 4.0f);
            Assert.AreEqual(fogComponent.GetMaxOpacity(), 5.0f);
            Assert.IsTrue(fogComponent.GetIsVolumetric());
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_FOG_SCRIPT_INTERFACE_TEST
        [Test]
        public static void FogScriptInterfaceTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _, out _, out _, out _, out _, out _);

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create spaces
            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-REWIND";

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            var connection = CreateMultiplayerConnection(space.Id);
            var entitySystem = connection.GetSpaceEntitySystem();

            entitySystem.OnEntityCreated += (s, e) => { };

            Connect(connection);

            // Create object to represent the fog
            var objectName = "TestObject";
            CreateObject(entitySystem, objectName, out var createdObject, false);

            // Create fog component
            var component = createdObject.AddComponent(Multiplayer.ComponentType.Fog);
            var fogComponent = component.As<Multiplayer.FogSpaceComponent>();

            createdObject.QueueUpdate();
            entitySystem.ProcessPendingEntityOperations();

            // Setup script
            const string fogScriptText = @"
		        var fog = ThisEntity.getFogComponents()[0];
		        fog.fogMode = 1;
		        fog.position = [1, 1, 1];
		        fog.rotation = [1, 1, 1, 2];
		        fog.scale = [2, 2, 2];
		        fog.startDistance = 1.1;
		        fog.endDistance = 2.2;
		        fog.color = [1, 1, 1];
		        fog.density = 3.3;
		        fog.heightFalloff = 4.4;
		        fog.maxOpacity = 5.5;
		        fog.isVolumetric = true;
            ";

            createdObject.GetScript().SetScriptSource(fogScriptText);
            createdObject.GetScript().Invoke();

            entitySystem.ProcessPendingEntityOperations();

            // Ensure values are set correctly
            Assert.AreEqual(fogComponent.GetFogMode(), Multiplayer.FogMode.Exponential);
            Assert.AreEqual(fogComponent.GetPosition().X, 1);
            Assert.AreEqual(fogComponent.GetPosition().Y, 1);
            Assert.AreEqual(fogComponent.GetPosition().Z, 1);
            Assert.AreEqual(fogComponent.GetRotation().X, 1);
            Assert.AreEqual(fogComponent.GetRotation().Y, 1);
            Assert.AreEqual(fogComponent.GetRotation().Z, 1);
            Assert.AreEqual(fogComponent.GetRotation().W, 2);
            Assert.AreEqual(fogComponent.GetScale().X, 2);
            Assert.AreEqual(fogComponent.GetScale().Y, 2);
            Assert.AreEqual(fogComponent.GetScale().Z, 2);
            Assert.AreEqual(fogComponent.GetStartDistance(), 1.1f);
            Assert.AreEqual(fogComponent.GetEndDistance(), 2.2f);
            Assert.AreEqual(fogComponent.GetColor().X, 1);
            Assert.AreEqual(fogComponent.GetColor().Y, 1);
            Assert.AreEqual(fogComponent.GetColor().Z, 1);
            Assert.AreEqual(fogComponent.GetDensity(), 3.3f);
            Assert.AreEqual(fogComponent.GetHeightFalloff(), 4.4f);
            Assert.AreEqual(fogComponent.GetMaxOpacity(), 5.5f);
            Assert.IsTrue(fogComponent.GetIsVolumetric());
        }
#endif

        /** 
         * Note that this test is currently intended to be run locally (not on the build server) to
         * facilitate multiplayer testing as it uses hardwired creds. We'll need to add support for
         * more than two users in the test_account_creds file first
         */

#if RUN_MULTIPLAYER_MULTIPROCESSTEST
        [Test]
        public static void MultiProcessTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _,out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-REWIND";

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            var connection = CreateMultiplayerConnection(space.Id);

            // Connect
            Connect(connection);

            var entitySystem = connection.GetSpaceEntitySystem();

            const int numClientInstances = 4;

            IMultiplayerTestClient[] clients = new IMultiplayerTestClient[numClientInstances];

            for (int i=0; i < numClientInstances; ++i)
            {
                string clientId = String.Format("TestClient{0}", i+1);

                // @todo - we'll need to extend the test_account_creds parsing to handle
                // more than two accounts before this can be run on the build server

                // For now you should replace these creds with your own for obvious reasons
                string userId = String.Format("will.cowling+test{0}@magnopus.com", i+1);
                string pwd = "12345678";

                IMultiplayerTestClient client = CreateMultiplayerTestClient(clientId);
                client.Login(userId, pwd);
                client.ConnectToSpace(space.Id);
                clients[i] = client;
            }

            // Create entity
            var name = "TestAvatar";
            var avatarId = "NotARealAvatarId";
            CreateAvatar(entitySystem, name, avatarId, out var entity);

            const int totalFameCount = 40;
            int frameCount = 0;
            while (frameCount < totalFameCount)
            {
                Log("[ LocalClient ] ", ConsoleColor.Blue, $"Frame {frameCount} : Ticking...");
                CSPFoundation.Tick();

                for (int i = 0; i < numClientInstances; ++i)
                {
                    IMultiplayerTestClient client = clients[i];
                    client.Tick();
                }

                Thread.Sleep(200);
                ++frameCount;
            }

            for (int i = 0; i < numClientInstances; ++i)
            {
                IMultiplayerTestClient client = clients[i];

                client.Disconnect();
                client.Logout();
                client.Close();
            }
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MULTIPLAYER_TESTS || RUN_MULTIPLAYER_INVALID_COMPONENT_TEST
        [Test]
        public static void InvalidComponentTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out var assetSystem, out _, out _, out _, out _, out _, out _, out _);

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-REWIND";

            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            // Connect to the SignalR server
            var connection = CreateMultiplayerConnection(space.Id);
            var entitySystem = connection.GetSpaceEntitySystem();

            entitySystem.OnEntityCreated += (s, e) => { };

            Connect(connection);

            // Create object to represent the audio component
            var objectName = "TestObject";
            CreateObject(entitySystem, objectName, out var createdObject, false);

            // Create audio component
            var component = createdObject.AddComponent(Multiplayer.ComponentType.Invalid);

            createdObject.QueueUpdate();
            entitySystem.ProcessPendingEntityOperations();
        }
#endif

#if RUN_MULTIPLAYER_CLIENTELECTIONTEST
        private static void CreateMultiProcessTestScript(Multiplayer.SpaceEntitySystem entitySystem, out Multiplayer.SpaceEntity scriptEntity)
        {
            // we'll be using this in a few places below as part of the test, so we declare it upfront
            const string ScriptText = @"
                var entities = TheEntitySystem.getEntities();
                var entityIndex = TheEntitySystem.getIndexOfEntity(ThisEntity.id);

                globalThis.onTick = () =>
                {
                    OKO.Log(""-- TestScript onTick called --"");
                    var model = entities[entityIndex].getAnimatedModelComponents()[0];
                    model.position = [10, 10, 10];
                }

                ThisEntity.subscribeToMessage(""entityTick"", ""onTick"");
            ";

            // Create an AnimatedModelComponent and have the script update it's position
            const string name = "ScriptTestObject";
            CreateObject(entitySystem, name, out var entity, false);

            var component = entity.AddComponent(Multiplayer.ComponentType.AnimatedModel);
            var animatedModelComponent = component.As<Multiplayer.AnimatedModelSpaceComponent>();

            component = entity.AddComponent(Multiplayer.ComponentType.ScriptData);
            var scriptComponent = component.As<Multiplayer.ScriptSpaceComponent>();

            scriptComponent.SetScriptSource(ScriptText);
            entity.GetScript().Invoke();

            entity.QueueUpdate();
            entitySystem.ProcessPendingEntityOperations();

            scriptEntity = entity;
        }

        [Test]
        public static void ClientElectionTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _,out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-REWIND";

            const bool EnableLeaderElection = true;
            
            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            const int numClientInstances = 4;

            IMultiplayerTestClient[] clients = new IMultiplayerTestClient[numClientInstances];

            for (int i=0; i < numClientInstances; ++i)
            {
                string clientId = String.Format("TestClient{0}", i+1);
                string userId = String.Format("will.cowling+test{0}@magnopus.com", i+1);
                string pwd = "12345678";

                IMultiplayerTestClient client = CreateMultiplayerTestClient(clientId);
                client.Login(userId, pwd);
                client.ConnectToSpace(space.Id, EnableLeaderElection);
                clients[i] = client;
                
                // Create entity
                string clientAvatarName = String.Format("TestAvatar{0}", i+1);
                string clientAvatarId = String.Format("NotARealAvatarId{0}", i+1);
                client.CreateAvatar(clientAvatarName, clientAvatarId);
            }

            
            var connection = CreateMultiplayerConnection(space.Id);

            // Enable Leader Election Feature flag
            var entitySystem = connection.GetSpaceEntitySystem();
            if (EnableLeaderElection)
            {
                entitySystem.EnableLeaderElection();
            }

            // Connect
            Connect(connection);

            entitySystem.OnEntityCreated += (s, e) => { Console.WriteLine("[ LocalClient ] OnEntityCreated"); };

            // Create entity
            var name = "TestAvatar";
            var avatarId = "NotARealAvatarId";
            CreateAvatar(entitySystem, name, avatarId, out var entity);
            CreateMultiProcessTestScript(entitySystem, out var scriptEntity);

            CSPFoundation.Tick();

            int removedCount = 0;
            
            const int totalFameCount = 40;
            int frameCount = 0;
            while (frameCount < totalFameCount)
            {
                Log("[ LocalClient ] ", ConsoleColor.Blue, $"Frame {frameCount} : Ticking...");
                CSPFoundation.Tick();

                for (int i = removedCount; i < numClientInstances; ++i)
                {
                    IMultiplayerTestClient client = clients[i];
                    client.Tick();
                }

                // Drop the leader half way through the test
                if (frameCount == (totalFameCount/2))
                {
                    Log("[ LocalClient ] ", ConsoleColor.Blue, "*** Destroying Leader ***");

                    // Destroy the first client (which should be the leader)
                    // to force renegotiation
                    IMultiplayerTestClient client = clients[0];
                    
                    client.DestroyAvatar();
                    client.Disconnect();
                    client.Logout();
                    client.Close();

                    ++removedCount;
                }
                
                Thread.Sleep(200);
                ++frameCount;
            }
            
            for (int i = removedCount; i < numClientInstances; ++i)
            {
                IMultiplayerTestClient client = clients[i];

                client.DestroyAvatar();
                client.Disconnect();
                client.Logout();
                client.Close();
            }
        }
#endif
    }
}


