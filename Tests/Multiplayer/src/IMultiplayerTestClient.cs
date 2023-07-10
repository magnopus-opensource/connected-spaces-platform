
namespace MultiplayerTestClient
{
    /**
     * This is the interface for driving the MultiplayerTestClient Instance
     * from the test code. SeriviceWire handles the underlying comms between
     * the test code and the client instance for us
     */
    public interface IMultiplayerTestClient
    {
        bool Login(string Login, string Password);
        void ConnectToSpace(string SpaceId, bool EnableLeaderElection);
        void CreateAvatar(string avatarName, string avatarId);
        void DestroyAvatar();
        void Tick();
        
        void Disconnect();
        void Logout();
        
        void Close();
    }

}