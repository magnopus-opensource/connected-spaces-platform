using System;
using System.Collections.Concurrent;
using System.Diagnostics;
using System.IO;
using System.Threading;

using ServiceWire;
using ServiceWire.NamedPipes;
using LogLevel = ServiceWire.LogLevel;

using Common = Csp.Common;
using Systems = Csp.Systems;
using Multiplayer = Csp.Multiplayer;

namespace MultiplayerTestClient
{
    static class FoundationExtensionMethods
    {
        public static T As<T>(this Multiplayer.ComponentBase component) where T : Multiplayer.ComponentBase
        {
            return (T)Activator.CreateInstance(typeof(T), new[] { component });
        }
    }

    public class MultiplayerTestClient : IMultiplayerTestClient
    {
        public static readonly string CHSEndpointBaseUri = "https://ogs.magnopus-dev.cloud";

        private NpHost nphost;
        private bool isRunning = false;
        private string clientId = string.Empty;
        private string sessionDirectory = string.Empty;
        private string sessionName = string.Empty;
        private long sessionStart = 0;

        private Multiplayer.SpaceEntity avatar;
        private Multiplayer.SpaceEntity scriptEntity;

        private Csp.Multiplayer.MultiplayerConnection connection;
        private Csp.Systems.LogSystem logSystem;

        public struct LogEvent
        {
            public DateTime timeStamp;
            public long ticks;
            public string clientId;
            public string message;
        }

        private ConcurrentQueue<LogEvent> eventQueue;
        private Thread eventLogThread;

        // Log event background thread
        private void EventLoggerThreadFunction()
        {
            while (isRunning)
            {
                LogEvent logEvent;
                if (eventQueue.TryDequeue(out logEvent))
                {
                    WriteLogEventToFile(logEvent);
                }
                else
                {
                    Thread.Sleep(1);
                }
            }
        }

        private void WriteLogEventToFile(LogEvent logEvent)
        {
            var fileName = $"{sessionDirectory}\\{sessionName}_{logEvent.clientId}.txt";

            if (!Directory.Exists(sessionDirectory))
            {
                Directory.CreateDirectory(sessionDirectory);
            }

            using (StreamWriter outputFile = new StreamWriter(fileName, append: true))
            {
                double ns = 1000000000.0 * (double)logEvent.ticks / Stopwatch.Frequency;
                string eventText = String.Format("[{0:D12}|{1}|{2}] {3}", (long)ns / 100, logEvent.clientId, logEvent.timeStamp.ToString("hh:mm:ss.fffffff"), logEvent.message);
                outputFile.WriteLine(eventText);
            }
        }

        private void LogHandler(object s, string e)
        {
            Log(e);
        }

        private void Log(string message)
        {
            if (isRunning)
            {
                LogEvent Event = new LogEvent
                {
                    timeStamp = DateTime.Now,
                    ticks = Stopwatch.GetTimestamp() - sessionStart,
                    clientId = this.clientId,
                    message = message
                };

                // Enqueue LogEvent to be processed by background thread
                eventQueue.Enqueue(Event);
            }
        }

        private static void InitialiseFoundationWithUserAgentInfo(string endpointRootURI)
        {
            Csp.CSPFoundation.Initialise(endpointRootURI,  "OKO_TESTS");
            var userAgentInfo = new Csp.ClientUserAgent
            {
                CSPVersion = Csp.CSPFoundation.GetVersion(),
                ClientOS = "CSharpTestsOS",
                ClientSKU = "MultiPlayerTestClient",
                ClientVersion = Csp.CSPFoundation.GetVersion(),
                ClientEnvironment = "ODev",
                CHSEnvironment = "oDev"
            };

            Csp.CSPFoundation.SetClientUserAgentInfo(userAgentInfo);
        }

        public MultiplayerTestClient(string pipeName, string sessionDirectory, string sessionName, long sessionStart)
        {
            isRunning = true;
            clientId = pipeName;
            this.sessionDirectory = sessionDirectory;
            this.sessionName = sessionName;
            this.sessionStart = sessionStart;

            eventQueue = new ConcurrentQueue<LogEvent>();
            eventLogThread = new Thread(EventLoggerThreadFunction);
            eventLogThread.Start();

            Log($"Starting Multiplayer Client Session - {sessionName}");

            var logger = new Logger(logLevel: LogLevel.Debug);
            var stats = new Stats();

            try
            {
                // Create ServiceWire NamedPipe connection for inter process comms
                nphost = new NpHost(pipeName, logger, stats);
                nphost.AddService<IMultiplayerTestClient>(this);

                nphost.Open();
            }
            catch
            {
                isRunning = false;
            }

            if (isRunning)
            {
                InitialiseFoundationWithUserAgentInfo(CHSEndpointBaseUri);

                logSystem = Systems.SystemsManager.Get().GetLogSystem();
                logSystem.OnLog += LogHandler;
            }
        }

        public bool IsRunning()
        {
            return isRunning;
        }

        // Functions below implement IMultiplayerTestClient

        public void Tick(int frameCount)
        {
            if (!isRunning)
                return;

            Csp.CSPFoundation.Tick();

            if (connection != null)
            {
                var entitySystem = Systems.SystemsManager.Get().GetSpaceEntitySystem();

                if (entitySystem != null)
                {
                    Log($"Tick Frame {frameCount} - Leader is {entitySystem.GetLeaderId()}");
                }
            }
            else
            {
                Log($"Tick Frame {frameCount} - Connection is null");
            }
        }

        // Is this client the current leader?
        public bool IsTheLeader()
        {
            if (connection != null)
            {
                var entitySystem = Systems.SystemsManager.Get().GetSpaceEntitySystem();

                if (entitySystem != null)
                {
                    return entitySystem.GetLeaderId() == connection.GetClientId();
                }
            }

            return false;
        }

        public bool Login(string login, string password)
        {
            var systemsManager = Csp.Systems.SystemsManager.Get();
            var userSystem = systemsManager.GetUserSystem();

            using var result = userSystem.Login("", login, password, null).Result;
            var resCode = result.GetResultCode();

            Log($"Login using username '{login}'");

            return resCode == Csp.Systems.EResultCode.Success;
        }

        public void Logout()
        {
            var systemsManager = Csp.Systems.SystemsManager.Get();
            var userSystem = systemsManager.GetUserSystem();

            Log("Logout");

            userSystem.Logout();
        }

        public void ConnectToSpace(string SpaceId)
        {

            var entitySystem = Systems.SystemsManager.Get().GetSpaceEntitySystem();

            entitySystem.OnEntityCreated += (s, e) => { Log("OnEntityCreated"); };
        }

        public void CreateAvatar(string avatarName, string avatarId)
        {
            var entitySystem = Systems.SystemsManager.Get().GetSpaceEntitySystem();

            var transform = new Multiplayer.SpaceTransform();
            var res = entitySystem.CreateAvatar(avatarName, transform, Multiplayer.AvatarState.Idle, avatarId, Multiplayer.AvatarPlayMode.Default).Result;

            avatar = res;
        }

        public void DestroyAvatar()
        {
            var entitySystem = Systems.SystemsManager.Get().GetSpaceEntitySystem();
            entitySystem.DestroyEntity(avatar);
            avatar.Dispose();
        }

        private void CreateObject(Multiplayer.SpaceEntitySystem entitySystem, string name, out Multiplayer.SpaceEntity entity)
        {
            var transform = new Multiplayer.SpaceTransform();
            var res = entitySystem.CreateObject(name, transform).Result;
            transform.Dispose();

            entity = res;
            var outEntity = entity;
        }

        public void CreateMultiProcessTestScript()
        {
            Log("Create Test Script");

            var entitySystem = Systems.SystemsManager.Get().GetSpaceEntitySystem();

            // we'll be using this in a few places below as part of the test, so we declare it upfront
            const string ScriptText = @"
                var entities = TheEntitySystem.getEntities();
                var entityIndex = TheEntitySystem.getIndexOfEntity(ThisEntity.id);

                globalThis.onTick = () =>
                {
                    OKO.Log(""TestScript onTick called"");
                    var model = entities[entityIndex].getAnimatedModelComponents()[0];
                    model.position = [10, 10, 10];
                }

                ThisEntity.subscribeToMessage(""entityTick"", ""onTick"");
            ";

            // Create an AnimatedModelComponent and have the script update it's position
            const string name = "ScriptTestObject";
            CreateObject(entitySystem, name, out var entity);

            var component = entity.AddComponent(Multiplayer.ComponentType.AnimatedModel);
            component = entity.AddComponent(Multiplayer.ComponentType.ScriptData);

            var scriptComponent = component.As<Multiplayer.ScriptSpaceComponent>();

            scriptComponent.SetScriptSource(ScriptText);
            entity.GetScript().Invoke();

            entity.QueueUpdate();
            entitySystem.ProcessPendingEntityOperations();

            scriptEntity = entity;
        }

        public void SimulateLeaderLost()
        {
            Log("Simulate Leader Lost");
            var array = new Common.Array<Multiplayer.ReplicatedValue>();
            connection.SendNetworkEvent("DebugLeaderDropoutMessage", array);
            array.Dispose();
        }

        public void Close()
        {
            Log("Closing Multiplayer Client");
            nphost.Close();
            isRunning = false;

            // Prevent stragler async events from trying to call invalid callbacks
            logSystem.ClearAllCallbacks();
            logSystem = null;
        }
    }
}
