// ------------------------------------------------------------------
// Copyright (c) Magnopus. All Rights Reserved.
// ------------------------------------------------------------------

using Csp;
using UnityEngine;

public class HelloWorld : MonoBehaviour
{

    private const string endPointUri = "https://ogs-ostage.magnoboard.com";
    private const string TenantKey = "CSP_HELLO_WORLD";
    // Initialisation of Foundation
    private void Awake()
    {
        var userAgent = new ClientUserAgent()
        {
            CHSEnvironment = "OStage",
            ClientEnvironment = "Stage",
            ClientOS = SystemInfo.operatingSystem,
            ClientSKU = "csp-cSharp-examples",
            ClientVersion = "0.0.3",
            CSPVersion = CSPFoundation.GetBuildID()
        };

        if (CSPFoundation.Initialise(endPointUri, TenantKey))
        {
            Debug.Log("Successfully Initialised...");

        }
        else
        {
            Debug.Log("Failed to Initialise...");
        }

        if (CSPFoundation.GetIsInitialised())
        {
            if(CSPFoundation.Shutdown())
            {
                Debug.Log("Successfully Shutdown...");
            }else
            {
                Debug.Log("Failed to Shutdown...");
            }
            
        }
    }
}