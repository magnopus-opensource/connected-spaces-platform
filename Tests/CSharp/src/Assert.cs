using System;
using System.Collections.Generic;
using System.Runtime.CompilerServices;


namespace CSharpTests
{
    static class Assert
    {
        [Obsolete("This is not the method you're looking for! Did you mean `AreEqual()`?", true)]
        public static new bool Equals(object first, object second)
        {
            throw new NotImplementedException();
        }

        public static void IsTrue(bool value, [CallerFilePath] string filePath = null, [CallerLineNumber] int line = 0, [CallerMemberName] string method = null)
        {
            if (!value)
                throw new AssertFailedException(filePath, line, method, "Expected value of 'true'");
        }

        public static void IsFalse(bool value, [CallerFilePath] string filePath = null, [CallerLineNumber] int line = 0, [CallerMemberName] string method = null)
        {
            if (value)
                throw new AssertFailedException(filePath, line, method, "Expected value of 'false'");
        }

        public static void AreEqual<T>(T first, T second, [CallerFilePath] string filePath = null, [CallerLineNumber] int line = 0, [CallerMemberName] string method = null)
        {
            if (!EqualityComparer<T>.Default.Equals(first, second))
                throw new AssertFailedException(filePath, line, method, $"Expected equality of values '{first}' and '{second}'");
        }
        
        public static void AreNotEqual<T>(T first, T second, [CallerFilePath] string filePath = null, [CallerLineNumber] int line = 0, [CallerMemberName] string method = null)
        {
            if (EqualityComparer<T>.Default.Equals(first, second))
                throw new AssertFailedException(filePath, line, method, $"Expected inequality of values");
        }

        public static void IsGreaterThan(long value, long target, [CallerFilePath] string filePath = null, [CallerLineNumber] int line = 0, [CallerMemberName] string method = null)
        {
            if (value <= target)
                throw new AssertFailedException(filePath, line, method, $"Expected value of '{value}' to be greater than '{target}'");
        }

        public static void IsGreaterThan(ulong value, ulong target, [CallerFilePath] string filePath = null, [CallerLineNumber] int line = 0, [CallerMemberName] string method = null)
        {
            if (value <= target)
                throw new AssertFailedException(filePath, line, method, $"Expected value of '{value}' to be greater than '{target}'");
        }

        public static void IsGreaterThan(double value, double target, [CallerFilePath] string filePath = null, [CallerLineNumber] int line = 0, [CallerMemberName] string method = null)
        {
            if (value <= target)
                throw new AssertFailedException(filePath, line, method, $"Expected value of '{value}' to be greater than '{target}'");
        }

        public static void IsGreaterOrEqualThan(long value, long target, [CallerFilePath] string filePath = null, [CallerLineNumber] int line = 0, [CallerMemberName] string method = null)
        {
            if (value < target)
                throw new AssertFailedException(filePath, line, method, $"Expected value of '{value}' to be greater than or equal to '{target}'");
        }

        public static void IsGreaterOrEqualThan(ulong value, ulong target, [CallerFilePath] string filePath = null, [CallerLineNumber] int line = 0, [CallerMemberName] string method = null)
        {
            if (value < target)
                throw new AssertFailedException(filePath, line, method, $"Expected value of '{value}' to be greater than or equal to '{target}'");
        }

        public static void IsGreaterOrEqualThan(double value, double target, [CallerFilePath] string filePath = null, [CallerLineNumber] int line = 0, [CallerMemberName] string method = null)
        {
            if (value < target)
                throw new AssertFailedException(filePath, line, method, $"Expected value of '{value}' to be greater than or equal to '{target}'");
        }

        public static void IsLessThan(long value, long target, [CallerFilePath] string filePath = null, [CallerLineNumber] int line = 0, [CallerMemberName] string method = null)
        {
            if (value >= target)
                throw new AssertFailedException(filePath, line, method, $"Expected value of '{value}' to be less than '{target}'");
        }

        public static void IsLessThan(ulong value, ulong target, [CallerFilePath] string filePath = null, [CallerLineNumber] int line = 0, [CallerMemberName] string method = null)
        {
            if (value >= target)
                throw new AssertFailedException(filePath, line, method, $"Expected value of '{value}' to be less than '{target}'");
        }

        public static void IsLessThan(double value, double target, [CallerFilePath] string filePath = null, [CallerLineNumber] int line = 0, [CallerMemberName] string method = null)
        {
            if (value >= target)
                throw new AssertFailedException(filePath, line, method, $"Expected value of '{value}' to be less than '{target}'");
        }

        public static void IsLessOrEqualThan(long value, long target, [CallerFilePath] string filePath = null, [CallerLineNumber] int line = 0, [CallerMemberName] string method = null)
        {
            if (value > target)
                throw new AssertFailedException(filePath, line, method, $"Expected value of '{value}' to be less than or equal to '{target}'");
        }

        public static void IsLessOrEqualThan(ulong value, ulong target, [CallerFilePath] string filePath = null, [CallerLineNumber] int line = 0, [CallerMemberName] string method = null)
        {
            if (value > target)
                throw new AssertFailedException(filePath, line, method, $"Expected value of '{value}' to be less than or equal to '{target}'");
        }

        public static void IsLessOrEqualThan(double value, double target, [CallerFilePath] string filePath = null, [CallerLineNumber] int line = 0, [CallerMemberName] string method = null)
        {
            if (value > target)
                throw new AssertFailedException(filePath, line, method, $"Expected value of '{value}' to be less than or equal to '{target}'");
        }
    }
}
