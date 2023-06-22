// ------------------------------------------------------------------
// Copyright (c) Magnopus. All Rights Reserved.
// ------------------------------------------------------------------

using Olympus.Foundation;
using UnityEngine;

public class HelloWorld : MonoBehaviour
{

    private const string endPointUri = "https://ogs-ostage.magnoboard.com";
    private const string TenantKey = "FOUNDATION_HELLO_WORLD";
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

        if (OlympusFoundation.Initialise(endPointUri, TenantKey))
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