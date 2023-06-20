using System;


namespace CSharpTests
{
    class AssertFailedException : Exception
    {
        readonly public string FilePath;
        readonly public int Line;
        readonly public string Method;
        readonly public string Description;

        public AssertFailedException(string filePath, int line, string method, string description)
        {
            FilePath = filePath;
            Line = line;
            Method = method;
            Description = description;
        }
    }
}
