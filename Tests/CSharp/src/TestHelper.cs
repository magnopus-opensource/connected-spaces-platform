using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Text;

using MultiplayerTestClient;
using ServiceWire.NamedPipes;

using Systems = Csp.Systems;


namespace CSharpTests
{
    static class TestHelper
    {
        public static readonly string CHSEndpointBaseUri = "https://ogs-odev-internal.magnoboard.com";
        public static readonly Random Rand = new Random();

        static readonly Stack<Action> cleanupFunctions = new Stack<Action>();
        static Systems.LogSystem LogSystem;

        [Conditional("DEBUG")]
        public static void LogDebug(string message)
        {
            Debug.WriteLine(message);
            var fg = Console.ForegroundColor;
            Console.ForegroundColor = ConsoleColor.DarkYellow;
            Console.Write("[ DEBUG    ] ");
            Console.ForegroundColor = fg;
            Console.WriteLine(message);
        }

        public static void LogError(string message)
        {
            var fg = Console.ForegroundColor;
            Console.ForegroundColor = ConsoleColor.Red;
            Console.Write("[ ERROR    ] ");
            Console.ForegroundColor = fg;
            Console.WriteLine(message);
        }

        /// <summary>
        /// Logs a fatal error and exits the application.
        /// <para>DOES NOT RETURN</para>
        /// </summary>
        /// <param name="message"></param>
        public static void LogFatal(string message)
        {
            LogError(message);

            if (Debugger.IsAttached)
            {
                Console.WriteLine("\nPress any key to close this window . . .");
                Console.ReadKey();

                Environment.Exit(1);
            }
        }
        
        public static void Log(string tag, ConsoleColor tagColour, string message)
        {
            var fg = Console.ForegroundColor;
            Console.ForegroundColor = tagColour;
            Console.Write($"{ tag } ");
            Console.ForegroundColor = fg;
            Console.WriteLine(message);
        }

        public static void PushCleanupFunction(Action function)
        {
            cleanupFunctions.Push(function);
        }

        static void OnLogEvent(object s, string e)
        {
            Log("[ ::EVENT  ]", ConsoleColor.Blue, e);
        }

        static void OnLogMessage(object s, string e)
        {
            Log("[ ::LOG    ]", ConsoleColor.Blue, e);
        }

        public static void StartupTest()
        {
            cleanupFunctions.Clear();
            InitialiseFoundationWithUserAgentInfo(CHSEndpointBaseUri);

            LogSystem = Systems.SystemsManager.Get().GetLogSystem();
            GC.SuppressFinalize(LogSystem);
            LogSystem.OnEvent += OnLogEvent;
            LogSystem.OnLog += OnLogMessage;

            LogDebug("Foundation initialized");
        }

        public static void CleanupTest()
        {
            while (cleanupFunctions.Count > 0)
                cleanupFunctions.Pop()();

            LogSystem.ClearAllCallbacks();
            Csp.CSPFoundation.Shutdown();
            LogDebug("Foundation uninitialized");

            LogSystem = null;
        }

        public static string GetUniqueHexString(int length = 16)
        {
            var sb = new StringBuilder(length * 2);

            for (int i = 0; i < length; i++)
            {
                sb.Append($"{(Rand.Next() % 16):x}");
            }

            return sb.ToString();
        }

        public static string GenerateUniqueString(string prefix)
        {
            return $"{prefix}-{GetUniqueHexString()}";
        }

        public static void GetFoundationSystems(out Systems.UserSystem userSystem, out Systems.SpaceSystem spaceSystem, out Systems.AssetSystem assetSystem, out Systems.PointOfInterestSystem poiSystem,
                                                out Systems.AnchorSystem anchorSystem, out Systems.GraphQLSystem graphQLSystem, out Systems.SettingsSystem settingsSystem, out Systems.MaintenanceSystem maintenanceSystem, 
                                                out Systems.EventTicketingSystem eventTicketingSystem,out Systems.ECommerceSystem eCommerceSystem)
        {
            var systemsManager = Systems.SystemsManager.Get();
            userSystem = systemsManager.GetUserSystem();
            spaceSystem = systemsManager.GetSpaceSystem();
            assetSystem = systemsManager.GetAssetSystem();
            poiSystem = systemsManager.GetPointOfInterestSystem();
            anchorSystem = systemsManager.GetAnchorSystem();
            graphQLSystem = systemsManager.GetGraphQLSystem();
            settingsSystem = systemsManager.GetSettingsSystem();
            maintenanceSystem = systemsManager.GetMaintenanceSystem();
            eventTicketingSystem = systemsManager.GetEventTicketingSystem();
            eCommerceSystem = systemsManager.GetECommerceSystem();

        }

        public static Dictionary<string, object> ParseJsonObject(string data)
        {
            var json = new Dictionary<string, object>();
            var index = data.IndexOf('{') + 1;

            // Skips whitespace and returns the next character in the string
            char NextChar()
            {
                var c = data[index++];

                while (char.IsWhiteSpace(c))
                    c = data[index++];

                return c;
            }

            // Parses the next string of characters and returns it as the appropriate type
            object ReadValue()
            {
                var c = NextChar();
                object value;

                switch (c)
                {
                    case '{':   // Object
                        {
                            var _start = index - 1;
                            var _scope = 1;

                            while (_scope > 0)
                            {
                                c = data[index++];

                                if (c == '{')
                                    _scope++;
                                else if (c == '}')
                                    _scope--;
                            }

                            var _end = index;
                            var inner = data.Substring(_start, _end - _start);
                            value = ParseJsonObject(inner);
                        }
                        break;
                    case '[':   // Array
                        var list = new List<object>();

                        while (c != ']')
                        {
                            list.Add(ReadValue());
                            c = NextChar();
                        }

                        value = list;
                        break;
                    case '"':   // String
                        {
                            var _start = index;
                            c = data[index++];

                            while (!(c == '"' && data[index - 1] != '\\'))
                                c = data[index++];

                            var _end = index - 1;
                            value = data.Substring(_start, _end - _start);
                        }
                        break;
                    default:
                        {
                            var _start = index - 1;
                            c = data[index++];

                            while (!char.IsWhiteSpace(c))
                            {
                                if (c == ',' || c == '}')
                                {
                                    index--;

                                    break;
                                }

                                c = data[index++];
                            }

                            var _end = index;
                            var inner = data.Substring(_start, _end - _start);

                            if (inner == "true")
                                value = true;
                            else if (inner == "false")
                                value = false;
                            else if (double.TryParse(inner, out var result))
                                value = result;
                            else
                                value = null;
                        }
                        break;
                }

                return value;
            }

            for (; ; )
            {
                var c = NextChar();

                if (c == '}')
                    break;

                if (c == ',')
                    c = NextChar();

                if (c != '"')
                    throw new System.Exception($"Expected '\"'. Got '{c}'.");

                var start = index;
                c = data[index++];

                while (!(c == '"' && data[index - 1] != '\\'))
                    c = data[index++];

                var end = index - 1;
                var key = data.Substring(start, end - start);

                c = NextChar();

                if (c != ':')
                    throw new System.Exception($"Expected ':'. Got '{c}'.");

                json[key] = ReadValue();
            }

            return json;
        }

        public static void InitialiseFoundationWithUserAgentInfo(string endpointRootURI)
        {
            Csp.CSPFoundation.Initialise(endpointRootURI, "OKO_TESTS");
            var userAgentInfo = new Csp.ClientUserAgent
            {
                CSPVersion  = Csp.CSPFoundation.GetVersion(),
                ClientOS		  = "CSharpTestsOS",
                ClientSKU		  = "CSharpTest",
                ClientVersion	  = Csp.CSPFoundation.GetVersion(),
                ClientEnvironment = "ODev",
                CHSEnvironment  = "oDev"
            };

            Csp.CSPFoundation.SetClientUserAgentInfo(userAgentInfo);
        }

        public static IMultiplayerTestClient CreateMultiplayerTestClient(string ClientId, string SessionDirectory, string SessionName, long SessionStart)
        {
            Process multiPlayerTestClient = new Process();

            multiPlayerTestClient.StartInfo.FileName = "MultiplayerTestClient.exe";

            string ArgsString = String.Format("{0} {1} {2} {3}", ClientId, SessionDirectory, SessionName, SessionStart);

            // Pass the client process a handle to the server.
            multiPlayerTestClient.StartInfo.Arguments = ArgsString;
            multiPlayerTestClient.StartInfo.UseShellExecute = false;
            multiPlayerTestClient.Start();

            var client = new NpClient<IMultiplayerTestClient>(new NpEndPoint(ClientId));

            return client.Proxy;
        }
    }
}
