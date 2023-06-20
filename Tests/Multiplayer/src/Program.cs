using System.Threading;

namespace MultiplayerTestClient
{
    class Program
    {
        static void Main(string[] args)
        {
            var pipeName = args[0];
            var testClient = new MultiplayerTestClient(pipeName);

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
