using System;
using ServiceWire;
using ServiceWire.NamedPipes;
using LogLevel = ServiceWire.LogLevel;
using Systems = Csp.Systems;
using Multiplayer = Csp.Multiplayer;


namespace MultiplayerTestClient
{
    public class MultiplayerTestClient : IMultiplayerTestClient
    {
        public static readonly string CHSEndpointBaseUri = "https://ogs-odev.magnoboard.com";

        private string longLabel = string.Empty;
        private const int totalKilobytes = 140;
        private Random rand = new Random(DateTime.Now.Millisecond);

        private NpHost nphost = null;
        private bool isRunning = false;
        private string clientId = string.Empty;

        private Multiplayer.SpaceEntity avatar;
        
        private Csp.Multiplayer.MultiplayerConnection connection = null;

        public MultiplayerTestClient(string pipeName)
        {
            clientId = pipeName;

            Log("Starting Multiplayer Client");

            var logger = new Logger(logLevel: LogLevel.Debug);
            var stats = new Stats();

            isRunning = true;

            try
            {
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
                
                Systems.SystemsManager.Get().GetLogSystem().OnLog += (object s, string e) =>
                {
                    Log(e);
                };
            }
        }
        public bool IsRunning()
        {
            return isRunning;
        }

        public void Tick()
        {
            Csp.CSPFoundation.Tick();
        }

        public bool Login(string Login, string Password)
        {
            var systemsManager = Csp.Systems.SystemsManager.Get();
            var userSystem = systemsManager.GetUserSystem();

            using var result = userSystem.Login("", Login, Password).Result;
            var resCode = result.GetResultCode();

            Log("Login");

            return resCode == Csp.Services.EResultCode.Success;
        }

        public void Logout()
        {
            var systemsManager = Csp.Systems.SystemsManager.Get();
            var userSystem = systemsManager.GetUserSystem();

            Log("Logout");

            userSystem.Logout();
        }

        public void ConnectToSpace(string SpaceId, bool EnableLeaderElection)
        {
            connection = new Csp.Multiplayer.MultiplayerConnection(SpaceId);

            var entitySystem = connection.GetSpaceEntitySystem();

            if (EnableLeaderElection)
            {
                entitySystem.EnableLeaderElection();
            }

            var res = connection.Connect().Result;
            res = connection.InitialiseConnection().Result;
            Log("Multiplayer connected");

            entitySystem.OnEntityCreated += (s, e) => { Log("OnEntityCreated"); };
        }

        public void Disconnect()
        {
            var res = connection.Disconnect().Result;
            Log("Multiplayer disconnected");
        }

        public void CreateAvatar(string avatarName, string avatarId)
        {
            var entitySystem = connection.GetSpaceEntitySystem();

            var transform = new Multiplayer.SpaceTransform();
            var res = entitySystem.CreateAvatar(avatarName, transform, Multiplayer.AvatarState.Idle, avatarId, Multiplayer.AvatarPlayMode.Default).Result;

            avatar = res;
        }

        public void DestroyAvatar()
        {
            var entitySystem = connection.GetSpaceEntitySystem();
            entitySystem.DestroyEntity(avatar);
            avatar.Dispose();            
        }

        public void Close()
        {
            nphost.Close();
            isRunning = false;
            Log("Closing Multiplayer Client");
        }

        private static void InitialiseFoundationWithUserAgentInfo(string endpointRootURI)
        {
            Csp.CSPFoundation.Initialise(endpointRootURI, "OKO_TESTS");
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
        
        private void Log(string message)
        {
            var fg = Console.ForegroundColor;
            Console.ForegroundColor = ConsoleColor.Blue;
            Console.Write($"[ {clientId} ] ");
            Console.ForegroundColor = fg;
            Console.WriteLine(message);
        }
    }
}
