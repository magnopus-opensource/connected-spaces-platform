using System;
using System.Collections;
using System.Runtime.ExceptionServices;
using System.Threading.Tasks;

using Multiplayer = Olympus.Foundation.Multiplayer;


namespace Tests
{
    static class ExtensionMethods
    {
        public static IEnumerator RunAsCoroutine<T>(this Task<T> task)
        {
            while (!task.IsCompleted)
                yield return null;

            if (task.IsFaulted)
                ExceptionDispatchInfo.Capture(task.Exception).Throw();

            yield return null;
        }

        public static T As<T>(this Multiplayer.ComponentBase component) where T : Multiplayer.ComponentBase
        {
            return (T)Activator.CreateInstance(typeof(T), new[] { component });
        }
    }
}
