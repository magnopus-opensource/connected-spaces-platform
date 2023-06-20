using System;
using System.Collections;
using System.Threading;

using NUnit.Framework;
using UnityEngine;
using UnityEngine.TestTools;

using Common = Olympus.Foundation.Common;
using Multiplayer = Olympus.Foundation.Multiplayer;
using Systems = Olympus.Foundation.Systems;


namespace Tests
{
    public class MultiplayerTests : TestsBase
    {
        static IEnumerator Disconnect(Multiplayer.MultiplayerConnection connection)
        {
            var task = connection.Disconnect();

            yield return task.RunAsCoroutine();

            var res = task.Result;

            Assert.True(res);

            Debug.Log("Multiplayer disconnected");
        }

        static IEnumerator Connect(Multiplayer.MultiplayerConnection connection)
        {
            {
                var task = connection.Connect();

                yield return task.RunAsCoroutine();

                var res = task.Result;

                Assert.True(res);
            }

            {
                var task = connection.InitialiseConnection();

                yield return task.RunAsCoroutine();

                var res = task.Result;

                Assert.True(res);

            }

            Debug.Log("Multiplayer connected");
            PushCleanupFunction(() => Disconnect(connection));
        }

        static IEnumerator DeleteEntity(Multiplayer.SpaceEntitySystem entitySystem, Multiplayer.SpaceEntity entity)
        {
            var id = entity.GetId();
            var task = entitySystem.DestroyEntity(entity);

            yield return task.RunAsCoroutine();

            var res = task.Result;

            Assert.True(res);

            Debug.Log($"Object deleted (Id: { id })");
        }

        static IEnumerator CreateAvatar(Result<Multiplayer.SpaceEntity> outAvatar, Multiplayer.SpaceEntitySystem entitySystem, string name, string avatarId)
        {
            var transform = new Multiplayer.SpaceTransform();
            var task = entitySystem.CreateAvatar(name, transform, Multiplayer.AvatarState.Idle, avatarId, Multiplayer.AvatarPlayMode.Default);

            yield return task.RunAsCoroutine();

            var res = task.Result;

            Assert.True(res.PointerIsValid);

            outAvatar.Value = res;
            Debug.Log($"Object created (Id: { outAvatar.Value.GetId() })");
            outAvatar.Value.OnUpdate += OnEntityUpdate;
            var _outAvatar = outAvatar.Value;
            PushCleanupFunction(() => DeleteEntity(entitySystem, _outAvatar));
        }

        static void OnEntityUpdate(object sender, (Multiplayer.SpaceEntity entity, Multiplayer.SpaceEntityUpdateFlags arg2, Common.Array<Multiplayer.ComponentUpdateInfo>) eventArgs)
        {
            var transform = eventArgs.entity.GetTransform();
            Debug.Log($"Received update (Entity Id: { eventArgs.entity.GetId() }, " +
                $"Pos: [{ transform.Position.X:0.##}, { transform.Position.Y:0.##}, { transform.Position.Z:0.##}], " +
                $"Rot: [{ transform.Rotation.X:0.##}, { transform.Rotation.Y:0.##}, { transform.Rotation.Z:0.##}, { transform.Rotation.W:0.##}])");
        }


        [UnityTest, Order(1)]
        public static IEnumerator CreateAvatarTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-REWIND";

            // Log in
            var userId = new Result<string>();
            yield return UserSystemTests.LogIn(userId, userSystem);

            // Create space
            var space = new Result<Systems.Space>();
            yield return SpaceSystemTests.CreateSpace(space, spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceType.Private, null, null);

            var connection = new Multiplayer.MultiplayerConnection(space.Value.Id);

            // Connect
            yield return Connect(connection);

            var entitySystem = connection.GetSpaceEntitySystem();

            // Create avatar
            var name = "TestAvatar";
            var avatarId = "NotARealAvatarId";
            var avatar = new Result<Multiplayer.SpaceEntity>();
            yield return CreateAvatar(avatar, entitySystem, name, avatarId);

            Assert.AreEqual(avatar.Value.GetEntityType(), Multiplayer.SpaceEntityType.Avatar);
            Assert.AreEqual(avatar.Value.GetName(), name);

            var components = avatar.Value.GetComponents();

            Assert.AreEqual(components.Size(), 1UL);

            var component = components[0];

            Assert.AreEqual(component.GetComponentType(), Multiplayer.ComponentType.AvatarData);

            var avatarComponent = component.As<Multiplayer.AvatarSpaceComponent>();

            Assert.AreEqual(avatarComponent.GetAvatarId(), avatarId);

            var script = avatar.Value.GetScript();

            Assert.AreEqual(script.GetOwnerId(), 0);
        }

        [UnityTest, Order(1)]
        public static IEnumerator MovementTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-REWIND";

            // Log in
            var userId = new Result<string>();
            yield return UserSystemTests.LogIn(userId, userSystem);

            // Create space
            var space = new Result<Systems.Space>();
            yield return SpaceSystemTests.CreateSpace(space, spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceType.Private, null, null);

            var connection = new Multiplayer.MultiplayerConnection(space.Value.Id);

            // Connect
            yield return Connect(connection);

            var entitySystem = connection.GetSpaceEntitySystem();

            // Create avatar
            var name = "TestAvatar";
            var avatarId = "NotARealAvatarId";
            var avatar = new Result<Multiplayer.SpaceEntity>();
            yield return CreateAvatar(avatar, entitySystem, name, avatarId);

            // Move avatar
            {
                avatar.Value.SetPosition(new Common.Vector3(0, 10, 0));
                avatar.Value.QueueUpdate();
                entitySystem.ProcessPendingEntityOperations();
            }
        }
    }
}
