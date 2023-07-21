namespace MultiplayerTestClient
{
    /**
     * This is the interface for driving the MultiplayerTestClient Instance
     * from the test code. SeriviceWire handles the underlying comms between
     * the test code and the client instance for us
     */
    public interface IMultiplayerTestClient
    {
        public abstract bool Login(string login, string password);
        public abstract void ConnectToSpace(string spaceId);
        public abstract void CreateAvatar(string avatarName, string avatarId);
        public abstract void DestroyAvatar();
        public abstract void Tick(int frameCount);

        public abstract bool IsTheLeader();

        public abstract void CreateMultiProcessTestScript();
        public abstract void SimulateLeaderLost();

        public abstract void Disconnect();
        public abstract void Logout();

        public abstract void Close();
    }

}
