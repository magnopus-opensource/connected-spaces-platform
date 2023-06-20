using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;

using NUnit.Framework;
using UnityEngine.TestTools;
using Debug = UnityEngine.Debug;
using Systems = Olympus.Foundation.Systems;


namespace Tests
{
    public class TestsBase
    {
        protected static readonly string chsEndpointBaseUri = "https://ogs-odev.magnoboard.com";
        protected static readonly Random rand = new Random();

        static readonly Stack<Func<IEnumerator>> cleanupFunctions = new Stack<Func<IEnumerator>>();

        protected static void PushCleanupFunction(Func<IEnumerator> function)
        {
            cleanupFunctions.Push(function);
        }

        protected static string GetUniqueHexString(int length = 8)
        {
            var sb = new StringBuilder(length * 2);

            for (int i = 0; i < length; i++)
            {
                sb.Append($"{(rand.Next() % 16):x}");
            }

            return sb.ToString();
        }

        protected static string GenerateUniqueString(string prefix)
        {
            return $"{prefix}-{GetUniqueHexString()}";
        }

        protected static void GetFoundationSystems(out Systems.UserSystem userSystem, out Systems.SpaceSystem spaceSystem, out Systems.AssetSystem assetSystem, out Systems.PointOfInterestSystem poiSystem)
        {
            var systemsManager = Systems.SystemsManager.Get();
            userSystem = systemsManager.GetUserSystem();
            spaceSystem = systemsManager.GetSpaceSystem();
            assetSystem = systemsManager.GetAssetSystem();
            poiSystem = systemsManager.GetPointOfInterestSystem();
        }

        [UnitySetUp]
        public IEnumerator SetUpTest()
        {
            cleanupFunctions.Clear();
            Olympus.Foundation.OlympusFoundation.Initialise(chsEndpointBaseUri);
            Debug.Log("Foundation initialized");

            yield return null;
        }

        [UnityTearDown]
        public IEnumerator TearDownTest()
        {
            while (cleanupFunctions.Count > 0)
                yield return cleanupFunctions.Pop()();

            Olympus.Foundation.OlympusFoundation.Shutdown();
            Debug.Log("Foundation uninitialized");

            yield return null;
        }
    }
}
