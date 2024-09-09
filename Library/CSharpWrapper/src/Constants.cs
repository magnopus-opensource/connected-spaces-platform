namespace Csp
{
    internal class Constants
    {
#if !UNITY_EDITOR && (UNITY_IOS || UNITY_VISIONOS)
        internal const string DllName = "__Internal";
#elif DEBUG && !UNITY_EDITOR_OSX && !UNITY_STANDALONE_OSX
        internal const string DllName = "ConnectedSpacesPlatform_D";
#else
        internal const string DllName = "ConnectedSpacesPlatform";
#endif
    }
}
