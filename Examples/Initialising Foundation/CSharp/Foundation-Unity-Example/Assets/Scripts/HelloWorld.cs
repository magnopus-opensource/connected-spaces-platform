// ------------------------------------------------------------------
// Copyright (c) Magnopus. All Rights Reserved.
// ------------------------------------------------------------------

using Olympus.Foundation;

using FoundationCommon = Olympus.Foundation.Common;
using FoundationSystems = Olympus.Foundation.Systems;
using FoundationAsset = Olympus.Foundation.Systems.Asset;
using UnityEngine;

public class HelloWorld : MonoBehaviour
{

    private const string endPointUri = "https://ogs-ostage.magnoboard.com";
    private const string OKOTenant = "FOUNDATION_HELLO_WORLD";
    // Initialisation of Foundation
    private void Awake()
    {
        var userAgent = new ClientUserAgent()
        {
            CHSEnvironment = "OStage",
            ClientEnvironment = "Stage",
            ClientOS = SystemInfo.operatingSystem,
            ClientSKU = "foundation-cSharp-examples",
            ClientVersion = "0.0.1",
            OlympusVersion = OlympusFoundation.GetBuildID()
        };

        if (OlympusFoundation.Initialise(endPointUri, OKOTenant))
        {
            Debug.Log("Successfully Initialised...");

        }
        else
        {
            Debug.Log("Failed to Initialise...");
        }

        if (OlympusFoundation.GetIsInitialised())
        {
            if(OlympusFoundation.Shutdown())
            {
                Debug.Log("Successfully Shutdown...");
            }else
            {
                Debug.Log("Failed to Shutdown...");
            }
            
        }
    }
}