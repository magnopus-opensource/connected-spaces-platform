using System;

namespace CSharpTests
{
    [AttributeUsage(AttributeTargets.Method, Inherited = false, AllowMultiple = false)]
    sealed class TestAttribute : Attribute { }
}
