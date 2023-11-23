using System;

using Common = Csp.Common;
using Multiplayer = Csp.Multiplayer;
using Systems = Csp.Systems;

using CSharpTests;
using static CSharpTests.TestHelper;


namespace CSPEngine
{
    static class WrapperTests
    {
#if RUN_ALL_UNIT_TESTS || RUN_WRAPPER_TESTS || RUN_WRAPPER_ARRAYOFUINT16_TEST
        [Test]
        public static void ArrayOfUInt16Test()
        {
            // `using` ensures that `array.Dispose()` gets called once it goes out of scope
            using var array = new Common.Array<UInt16>(2);

            Assert.IsTrue(array.Size() == 2);

            array[0] = 42;
            array[1] = 1337;

            Assert.AreEqual(array[0], 42U);
            Assert.AreEqual(array[1], 1337U);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_WRAPPER_TESTS || RUN_WRAPPER_ARRAYOFSTRING_TEST
        [Test]
        public static void ArrayOfStringTest()
        {
            using var array = new Common.Array<string>(2);

            Assert.IsTrue(array.Size() == 2);

            array[0] = "forty two";
            array[1] = "elite";

            Assert.AreEqual(array[0], "forty two");
            Assert.AreEqual(array[1], "elite");
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_WRAPPER_TESTS || RUN_WRAPPER_ARRAYOFENUM_TEST
        [Test]
        public static void ArrayOfEnumTest()
        {
            using var array = new Common.Array<Systems.EAssetPlatform>(2);

            Assert.IsTrue(array.Size() == 2);

            array[0] = Systems.EAssetPlatform.DEFAULT;
            array[1] = (Systems.EAssetPlatform)42;

            Assert.AreEqual(array[0], Systems.EAssetPlatform.DEFAULT);
            Assert.AreEqual(array[1], (Systems.EAssetPlatform)42);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_WRAPPER_TESTS || RUN_WRAPPER_ARRAYOFCLASS_TEST
        [Test]
        public static void ArrayOfClassTest()
        {
            using var array = new Common.Array<Systems.Space>(1);

            Assert.IsTrue(array.Size() == 1);

            array[0] = new Systems.Space
            {
                Id = "TestSpaceId",
                Attributes = Systems.SpaceAttributes.Public
            };

            Assert.AreEqual(array[0].Id, "TestSpaceId");
            Assert.AreEqual(array[0].Attributes, Systems.SpaceAttributes.Public);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_WRAPPER_TESTS || RUN_WRAPPER_STRINGMAP_TEST
        [Test]
        public static void StringMapTest()
        {
            using var map = new Common.Map<string, string>();

            Assert.AreEqual(map.Size(), 0UL);

            map["the_answer_to_life"] = "forty two";

            Assert.AreEqual(map.Size(), 1UL);
            Assert.IsTrue(map.HasKey("the_answer_to_life"));
            Assert.AreEqual(map["the_answer_to_life"], "forty two");
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_WRAPPER_TESTS || RUN_WRAPPER_MAP_TEST
        [Test]
        public static void MapTest()
        {
            using var map = new Common.Map<uint, Multiplayer.ReplicatedValue>();

            Assert.AreEqual(map.Size(), 0UL);

            const uint item1Key = 42U;
            using var item1Val = new Multiplayer.ReplicatedValue("The answer to life, the universe, and everything");

            map[item1Key] = item1Val;

            Assert.AreEqual(map.Size(), 1UL);
            Assert.IsTrue(map.HasKey(item1Key));
            Assert.AreEqual(map[item1Key].GetReplicatedValueType(), Multiplayer.ReplicatedValueType.String);
            Assert.AreEqual(map[item1Key].GetString(), item1Val.GetString());

            const uint item2Key = 0x60FA57U;
            using var item2Val = new Multiplayer.ReplicatedValue(new Common.Vector3(1000f, 0f, 0f));

            map[item2Key] = item2Val;

            Assert.AreEqual(map.Size(), 2UL);
            Assert.IsTrue(map.HasKey(item2Key));
            Assert.AreEqual(map[item2Key].GetReplicatedValueType(), Multiplayer.ReplicatedValueType.Vector3);
            Assert.AreEqual(map[item2Key].GetVector3().X, item2Val.GetVector3().X);

            var keys = map.Keys();

            for (var i = 0U; i < keys.Size(); i++)
            {
                var key = keys[i];

                switch (key)
                {
                    case item1Key:
                        Assert.AreEqual(map[key].GetReplicatedValueType(), Multiplayer.ReplicatedValueType.String);
                        Assert.AreEqual(map[key].GetString(), item1Val.GetString());
                        break;
                    case item2Key:
                        Assert.AreEqual(map[key].GetReplicatedValueType(), Multiplayer.ReplicatedValueType.Vector3);
                        Assert.AreEqual(map[key].GetVector3().X, item2Val.GetVector3().X);
                        break;
                    default:
                        throw new Exception("Unknown map key!");
                }
            }
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_WRAPPER_TESTS || RUN_WRAPPER_REPLICATEDVALUE_VECTOR3_TEST
        [Test]
        public static void ReplicatedValue_Vector3Test()
        {
            using var val = new Multiplayer.ReplicatedValue(new Common.Vector3(1, 2, 3));

            Assert.AreEqual(val.GetReplicatedValueType(), Multiplayer.ReplicatedValueType.Vector3);
            Assert.AreEqual(val.GetVector3().X, 1);
            Assert.AreEqual(val.GetVector3().Y, 2);
            Assert.AreEqual(val.GetVector3().Z, 3);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_WRAPPER_TESTS || RUN_WRAPPER_GETDEVICEID_TEST
        [Test]
        public static void GetDeviceIdTest()
        {
            var deviceId1 = Csp.CSPFoundation.GetDeviceId();

            // Shutdown and re-initialise Foundation to verify we get the same DeviceID
            Csp.CSPFoundation.Shutdown();
            
            InitialiseFoundationWithUserAgentInfo(CHSEndpointBaseUri);

            var deviceId2 = Csp.CSPFoundation.GetDeviceId();

            Assert.AreEqual(deviceId1, deviceId2);
        }
#endif

#if RUN_ALL_UNIT_TESTS || RUN_WRAPPER_TESTS || RUN_WRAPPER_STOREARRAYELEMENT_TEST
        [Test]
        public static void StoreArrayElementTest()
        {
            Multiplayer.ReplicatedValue element;

            // Create array that goes out of scope after element has been stored
            {
                var array = new Common.Array<Multiplayer.ReplicatedValue>(1);
                array[0] = new Multiplayer.ReplicatedValue(new Common.Vector3(1, 2, 3));
                element = array[0];
            }

            // Force garbage collection
            GC.Collect();
            GC.WaitForPendingFinalizers();

            // Allocate a new array to try to force previous memory to be overwritten
            {
                var array = new Common.Array<Multiplayer.ReplicatedValue>(1);
                array[0] = new Multiplayer.ReplicatedValue(new Common.Vector3(4, 5, 6));
            }

            // Verify element
            var vector = element.GetVector3();
            Assert.AreEqual(vector.X, 1);
            Assert.AreEqual(vector.Y, 2);
            Assert.AreEqual(vector.Z, 3);
        }
#endif
    }
}
