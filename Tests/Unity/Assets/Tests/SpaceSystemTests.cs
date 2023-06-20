using System.Collections;

using NUnit.Framework;
using UnityEngine;
using UnityEngine.TestTools;

using Systems = Olympus.Foundation.Systems;
using Services = Olympus.Foundation.Services;


namespace Tests
{
    public class SpaceSystemTests : TestsBase
    {
        public static IEnumerator DeleteSpace(Systems.SpaceSystem spaceSystem, Systems.Space space)
        {
            var task = spaceSystem.DeleteSpace(space);

            yield return task.RunAsCoroutine();

            var result = task.Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            Debug.Log($"Space deleted (Id: { space.Id })");
        }

        public static IEnumerator CreateSpace(Result<Systems.Space> outSpace, Systems.SpaceSystem spaceSystem, string name, string description, Systems.SpaceType spaceType, string spaceMetadata = null, Olympus.Foundation.Common.Array<string> extraUserIds = null)
        {
            var task = spaceSystem.CreateSpace(name, description, Systems.SpaceType.Private, null, null, "", null);

            yield return task.RunAsCoroutine();

            var result = task.Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            outSpace.Value = result.GetSpace();
            Debug.Log($"Space created (Id: { outSpace.Value.Id }, Name: { outSpace.Value.Name })");
            var _outSpace = outSpace.Value;
            PushCleanupFunction(() => DeleteSpace(spaceSystem, _outSpace));
        }

        static IEnumerator UpdateSpace(Result<Systems.BasicSpace> outBasicSpace, Systems.SpaceSystem spaceSystem, Systems.Space space, string newName, string newDescription)
        {
            var task = spaceSystem.UpdateSpace(space, newName, newDescription, null);

            yield return task.RunAsCoroutine();

            var result = task.Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            outBasicSpace.Value = result.GetSpace();
        }

        static IEnumerator GetSpace(Result<Systems.Space> outSpace, Systems.SpaceSystem spaceSystem, string spaceId)
        {
            var task = spaceSystem.GetSpace(spaceId);

            yield return task.RunAsCoroutine();

            var result = task.Result;
            var resCode = result.GetResultCode();

            Assert.AreEqual(resCode, Services.EResultCode.Success);

            outSpace.Value = result.GetSpace();
        }


        [UnityTest, Order(1)]
        public IEnumerator CreateSpaceTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            // Log in
            var userId = new Result<string>();
            yield return UserSystemTests.LogIn(userId, userSystem);

            // Create space
            var space = new Result<Systems.Space>();
            yield return CreateSpace(space, spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceType.Private);

            Assert.AreEqual(space.Value.Name, testSpaceName);
        }

        [UnityTest, Order(2)]
        public IEnumerator UpdateSpaceDetailsTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            // Log in
            var userId = new Result<string>();
            yield return UserSystemTests.LogIn(userId, userSystem);

            // Create space
            var space = new Result<Systems.Space>();
            yield return CreateSpace(space, spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceType.Private);

            string newTestSpaceName = testSpaceName + "-Updated";
            string newTestSpaceDescription = testSpaceDescription + "-Updated";

            var updatedSpace = new Result<Systems.BasicSpace>();
            yield return UpdateSpace(updatedSpace, spaceSystem, space, newTestSpaceName, newTestSpaceDescription);

            Assert.AreEqual(updatedSpace.Value.Name, newTestSpaceName);
            Assert.AreEqual(updatedSpace.Value.Description, newTestSpaceDescription);

            yield return GetSpace(space, spaceSystem, space.Value.Id);
            Assert.AreEqual(space.Value.Name, newTestSpaceName);
            Assert.AreEqual(space.Value.Description, newTestSpaceDescription);
        }

        [UnityTest, Order(2)]
        public IEnumerator GetSpacesTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            // Log in
            var userId = new Result<string>();
            yield return UserSystemTests.LogIn(userId, userSystem);

            // Create space
            var space = new Result<Systems.Space>();
            yield return CreateSpace(space, spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceType.Private);

            // Get spaces
            {
                var task = spaceSystem.GetSpaces();

                yield return task.RunAsCoroutine();

                var result = task.Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);

                var spaces = result.GetSpaces();

                Assert.Greater(spaces.Size(), 0);

                var found = false;

                for (uint i = 0; i < spaces.Size(); i++)
                {
                    if (spaces[i].Name == testSpaceName)
                    {
                        found = true;

                        break;
                    }
                }

                Assert.True(found);
            }
        }

        [UnityTest, Order(3)]
        public IEnumerator GetSpaceTest()
        {
            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-SPACE-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-SPACEDESC-REWIND";

            // Log in
            var userId = new Result<string>();
            yield return UserSystemTests.LogIn(userId, userSystem);

            // Create space
            var space = new Result<Systems.Space>();
            yield return CreateSpace(space, spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceType.Private);

            // Get space
            {
                var task = spaceSystem.GetSpace(space.Value.Id);

                yield return task.RunAsCoroutine();

                var result = task.Result;
                var resCode = result.GetResultCode();

                Assert.AreEqual(resCode, Services.EResultCode.Success);

                var resultSpace = result.GetSpace();

                Assert.AreEqual(resultSpace.Name, space.Value.Name);
            }
        }
    }
}
