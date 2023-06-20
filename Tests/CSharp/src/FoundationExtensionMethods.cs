using System;

using Common = Csp.Common;
using Multiplayer = Csp.Multiplayer;


namespace CSPEngine
{
    static class FoundationExtensionMethods
    {
        public static Common.Array<T> ToFoundationArray<T>(this T[] array)
        {
            if (array == null)
                throw new ArgumentNullException("array");

            var r = new Common.Array<T>((ulong)array.Length);

            for (int i = 0; i < array.Length; i++)
                r[(ulong)i] = array[i];

            return r;
        }

        public static T As<T>(this Multiplayer.ComponentBase component) where T : Multiplayer.ComponentBase
        {
            return (T)Activator.CreateInstance(typeof(T), new[] { component });
        }
    }
}
