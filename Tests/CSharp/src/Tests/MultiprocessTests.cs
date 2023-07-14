using System;
using System.Collections.Generic;
using System.IO;

using Common = Csp.Common;
using Systems = Csp.Systems;
using Multiplayer = Csp.Multiplayer;

using CSharpTests;
using MultiplayerTestClient;
using static CSharpTests.TestHelper;
using System.Diagnostics;
using System.Threading;

namespace CSPEngine
{
    static class MultiprocessTests
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
            entitySystem.DestroyEntity(entity);

            LogDebug($"Entity deleted (Id: { id })");

            if (disposeFoundationResources)
                entity.Dispose();
        }

        static void CreateAvatar(Multiplayer.SpaceEntitySystem entitySystem, string name, string avatarId, out Multiplayer.SpaceEntity entity, bool pushCleanupFunction = true, bool disposeFoundationResources = true)
        {
            var transform = new Multiplayer.SpaceTransform();
            var res = entitySystem.CreateAvatar(name, transform, Multiplayer.AvatarState.Idle, avatarId, Multiplayer.AvatarPlayMode.Default).Result;

            Assert.IsTrue(res.PointerIsValid);

            entity = res;
            LogDebug($"Avatar created (Id: { entity.GetId() })");

            var outEntity = entity;

            if (pushCleanupFunction)
                PushCleanupFunction(() => DeleteEntity(entitySystem, outEntity, disposeFoundationResources));
        }


        static void CreateCreatorAvatar(Multiplayer.SpaceEntitySystem entitySystem, string name, string avatarId, out Multiplayer.SpaceEntity entity)
        {
            var transform = new Multiplayer.SpaceTransform();
            var res = entitySystem.CreateAvatar(name, transform, Multiplayer.AvatarState.Idle, avatarId, Multiplayer.AvatarPlayMode.Creator).Result;

            Assert.IsTrue(res.PointerIsValid);

            entity = res;
            LogDebug($"Avatar created (Id: { entity.GetId() })");
            var outEntity = entity;
            PushCleanupFunction(() => DeleteEntity(entitySystem, outEntity));
        }

        public static void CreateObject(Multiplayer.SpaceEntitySystem entitySystem, string name, out Multiplayer.SpaceEntity entity, bool pushCleanupFunction = true)
        {
            var transform = new Multiplayer.SpaceTransform();
            var res = entitySystem.CreateObject(name, transform).Result;

            Assert.IsTrue(res.PointerIsValid);

            entity = res;
            LogDebug($"Object created (Id: { entity.GetId() })");

            var outEntity = entity;

            if (pushCleanupFunction)
                PushCleanupFunction(() => DeleteEntity(entitySystem, outEntity));
        }

        static Multiplayer.MultiplayerConnection CreateMultiplayerConnection(string spaceId, bool pushCleanupFunction = true)
        {
            var connection = new Multiplayer.MultiplayerConnection(spaceId);

            if (pushCleanupFunction)
                PushCleanupFunction(() => connection.Dispose());

            return connection;
        }

        static void OnEntityUpdate(object sender, (Multiplayer.SpaceEntity entity, Multiplayer.SpaceEntityUpdateFlags arg2, Common.Array<Multiplayer.ComponentUpdateInfo>) eventArgs)
        {
            var transform = eventArgs.entity.GetTransform();
            LogDebug($"Received update (Entity Id: { eventArgs.entity.GetId() }, " +
                $"Pos: [{ transform.Position.X:0.##}, { transform.Position.Y:0.##}, { transform.Position.Z:0.##}], " +
                $"Rot: [{ transform.Rotation.X:0.##}, { transform.Rotation.Y:0.##}, { transform.Rotation.Z:0.##}, { transform.Rotation.W:0.##}])");
        }

        private static void MergeLogFiles(string sessionDirectory, string sessionName)
        {
            Log("[ ClientElectionTest ] ", ConsoleColor.Blue, $"Merging log files in {sessionDirectory}");

            if (Directory.Exists(sessionDirectory))
            {
                // Make a list of all events sorted by the event time
                SortedList<long, string> eventMap = new SortedList<long, string>();

                var files = Directory.EnumerateFiles(sessionDirectory);

                foreach (var logFile in files)
                {
                    foreach (var line in File.ReadLines(logFile))
                    {
                        if (line != null)
                        {
                            string[] eventElements;
                            eventElements = line.Split(new char[] { '[', '|' });
                            long timeStamp = 0;

                            // Check line is as expected
                            if (eventElements.Length>2  && long.TryParse(eventElements[1], out timeStamp))
                            {
                                eventMap[timeStamp] = line;
                            }
                        }
                    }
                }

                foreach (var item in eventMap)
                {
                    string fileName = String.Format("{0}\\{1}_AllClients.txt", sessionDirectory, sessionName);

                    using (StreamWriter outputFile = new StreamWriter(fileName, append: true))
                    {
                        outputFile.WriteLine(item.Value);
                    }
                }
            }
        }

        private static int FindLeader(IMultiplayerTestClient[] clients, bool[] clientValid, int numClientInstances)
        {
            for (int i = 0; i < numClientInstances; ++i)
            {
                if (clientValid[i])
                {
                    IMultiplayerTestClient client = clients[i];

                    if (client.IsTheLeader())
                    {
                        return i;
                    }
                }
            }
            return -1;
        }


        /** 
         * Note that this test is currently intended to be run locally (not on the build server) to
         * facilitate multiplayer testing and uses hardwired creds.
         */

#if true//RUN_MULTIPROCESS_CLIENTELECTIONTEST
        [Test]
        public static void ClientElectionTest()
        {
            /** 
             * Note that this test is currently intended to be run locally (not on the build server) to
             * facilitate multiplayer testing as it uses hardwired creds.
             */

            GetFoundationSystems(out var userSystem, out var spaceSystem, out _, out _, out _,out _, out _, out _);

            string testSpaceName = GenerateUniqueString("OLY-UNITTEST-MULTI-REWIND");
            string testSpaceDescription = "OLY-UNITTEST-MULTIDESC-REWIND";

            // Log in
            _ = UserSystemTests.LogIn(userSystem);

            // Create space
            var space = SpaceSystemTests.CreateSpace(spaceSystem, testSpaceName, testSpaceDescription, Systems.SpaceAttributes.Private, null, null, null);

            const int NumClientInstances = 8;
            const bool DebugClientInstances = true;

            // We start at with email account +test2 because +test1 is used to log in above and create the space
            const int EmailIndexOffset = 2;

            long sessionStart = Stopwatch.GetTimestamp();

            string sessionName = String.Format("ClientElectionTest_{0}", DateTime.Now.ToString("yyyy_MMdd_HHmm_ss_fff"));
            string sessionDirectory = String.Format("D:\\Temp\\MultiPlayerLogging\\{0}", sessionName);

            Log("[ ClientElectionTest ] ", ConsoleColor.Blue, $"Session: {sessionName}");
            Log("[ ClientElectionTest ] ", ConsoleColor.Blue, $"Using working directory: {sessionDirectory}");

            IMultiplayerTestClient[] clients = new IMultiplayerTestClient[NumClientInstances];

            // Keep track of which clients are valid and which ones have been dropped or destroyed
            bool[] clientValid = new bool[NumClientInstances];

            for (int i = 0; i < NumClientInstances; ++i)
            {
                string clientId = String.Format("TestClient{0}", i+1);
                string userId = String.Format("will.cowling+test{0}@magnopus.com", i + EmailIndexOffset);
                string pwd = "12345678";

                Log("[ ClientElectionTest ] ", ConsoleColor.Blue, $"Create Client {i+1} of {NumClientInstances} - {clientId}");

                IMultiplayerTestClient client = CreateMultiplayerTestClient(clientId, sessionDirectory, sessionName, sessionStart);
                client.Login(userId, pwd);
                client.ConnectToSpace(space.Id);
                clients[i] = client;
                clientValid[i] = true;

                // Create entity
                string clientAvatarName = String.Format("TestAvatar{0}", i+1);
                string clientAvatarId = String.Format("NotARealAvatarId{0}", i+1);
                client.CreateAvatar(clientAvatarName, clientAvatarId);
            }

            Log("[ ClientElectionTest ] ", ConsoleColor.Blue, "Create Test Script");
            clients[0].CreateMultiProcessTestScript();

            int removedCount = 0;

            // Total number of frames to run test for
            const int TotalFameCount = 150;

            // Delay to re-election(s)
            const int FirstReElectionCount = 50;
            const int SecondReElectionCount = 100;
            // Delay to leader loss simulation
            //const int LeaderLossCount = 150;

            // Number of milliseconds for each frame
            const int MillisecondsPerFrame = 200;

            if (DebugClientInstances)
            {
                // If debugging, then break here and attach processes to the debugger
                Log("[ ClientElectionTest ] ", ConsoleColor.Blue, "Attach Debugger to processes here");
            }

            int frameCount = 0;
            while (frameCount < TotalFameCount)
            {
                Log("[ ClientElectionTest ] ", ConsoleColor.Blue, $"Frame {frameCount} : Ticking...");

                for (int i = 0; i < NumClientInstances; ++i)
                {
                    if (clientValid[i])
                    {
                        IMultiplayerTestClient client = clients[i];
                        client.Tick(frameCount);
                    }
                }

                // Drop the leader half way through the test
                if (frameCount == FirstReElectionCount || frameCount == SecondReElectionCount)
                {
                    int leaderIndex = FindLeader(clients, clientValid, NumClientInstances);

                    if (leaderIndex >= 0)
                    {
                        IMultiplayerTestClient leader = clients[leaderIndex];

                        if (leader != null)
                        {
                            Log("[ ClientElectionTest ] ", ConsoleColor.Blue, $"*** Destroying Leader using index={leaderIndex}");
                            clientValid[leaderIndex] = false;

                            leader.DestroyAvatar();
                            leader.Disconnect();
                            leader.Logout();
                            leader.Close();

                            ++removedCount;
                        }
                    }
                }
                // Simulate the leader stop responding
                //else if (frameCount == LeaderLossCount)
                //{
                //    int leaderIndex = FindLeader(clients, clientValid, numClientInstances);

                //    if (leaderIndex >= 0)
                //    {
                //        Log("[ ClientElectionTest ] ", ConsoleColor.Blue, $"*** Simulate Leader Lost index={leaderIndex}");
                //        IMultiplayerTestClient leader = clients[leaderIndex];

                //        if (leader != null)
                //        {
                //            leader.SimulateLeaderLost();
                //        }
                //    }
                //}

                Thread.Sleep(MillisecondsPerFrame);
                ++frameCount;
            }

            for (int i = 0; i < NumClientInstances; ++i)
            {
                if (clientValid[i])
                {
                    Log("[ ClientElectionTest ] ", ConsoleColor.Blue, $"Destroy Client {i + 1} of {NumClientInstances}");

                    IMultiplayerTestClient client = clients[i];

                    client.DestroyAvatar();
                    client.Disconnect();
                    client.Logout();
                    client.Close();
                }
            }

            // Prevent stragler async events from trying to call invalid callbacks
            Systems.SystemsManager.Get().GetLogSystem().ClearAllCallbacks();

            // Merge each seperate client event log into one contiguous log
            MergeLogFiles(sessionDirectory, sessionName);
        }
#endif
    }
}


