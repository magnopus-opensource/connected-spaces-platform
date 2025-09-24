using System;
using System.Numerics;

class Program
{
    // Keep delegate alive
    static ConnectedSpacesPlatform.EntityCreatedCallbackFP onCreated = OnEntityCreated;

    static void Main()
    {
        // With your change: default ctor exists.
        var eng = new OfflineRealtimeEngine();

        var pos = new Vector3(0, 0, 0);
        var rot = new Vector4(0, 0, 0, 1);
        var scl = new Vector3(1, 1, 1);
        var xform = new SpaceTransform(pos, rot, scl);

        long parentOrMinusOne = -1; // -1 == no parent
        CSPRealtime.OfflineRealtimeEngine_CreateEntity(eng, "Box", xform, parentOrMinusOne, onCreated);

        // If CreateEntity is async, you may need to keep the process alive long enough to see the callback.
        Console.WriteLine("CreateEntity called.");
    }

    static void OnEntityCreated(global::csp.multiplayer.SpaceEntity entityPtr)
    {
        // If SpaceEntity is wrapped by SWIG (header included), you'll get a proxy.
        // If not, this may be an opaque handle. Adjust as needed.
        Console.WriteLine($"Entity created: {(entityPtr == null ? "null" : "non-null")}");
    }
}