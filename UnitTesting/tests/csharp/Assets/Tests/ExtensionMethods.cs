using System;


namespace Csp.Tests
{
    public static class ExtensionMethods
    {
        public static bool NearlyEquals(this float a, float b)
        {
            var diff = Math.Abs(a - b);

            return diff < float.Epsilon;
        }

        public static bool NearlyEquals(this double a, double b)
        {
            var diff = Math.Abs(a - b);

            return diff < float.Epsilon;
        }
    }
}