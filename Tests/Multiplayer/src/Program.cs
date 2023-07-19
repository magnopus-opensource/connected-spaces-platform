using System.Threading;

namespace MultiplayerTestClient
{
    class Program
    {
        static void Main(string[] args)
        {
            // NamedPipe name used by ServiceWire for inter process comms
            var pipeName = args[0];
            // Working directory to store log files
            var sessionDirectory = args[1];
            // Unique session name based on start time and date of test run
            var sessionName = args[2];
            // 100ns resolution start time from system timer
            // used to synchronise timestamps across processes
            long sessionStart = long.Parse(args[3]);

            var testClient = new MultiplayerTestClient(pipeName, sessionDirectory, sessionName, sessionStart);

            while (testClient.IsRunning())
            {
                Thread.Sleep(100);
                
                // @note that we *could* automatically call Tick
                // here, but I opted to not do it give the tests
                // control of when and how often they want to
                // tick the remote clients
            }
        }
    }
}
