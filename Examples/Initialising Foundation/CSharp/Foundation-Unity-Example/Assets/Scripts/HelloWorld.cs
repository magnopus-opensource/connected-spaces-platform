// ------------------------------------------------------------------
// Copyright (c) Magnopus. All Rights Reserved.
// ------------------------------------------------------------------

using Olympus.Foundation;
using Olympus.Foundation.Multiplayer;
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;
using FoundationCommon = Olympus.Foundation.Common;
using FoundationSystems = Olympus.Foundation.Systems;
using FoundationAsset = Olympus.Foundation.Systems.Asset;
using UnityEngine;
using UnityEngine.Windows;

public class HelloWorld : MonoBehaviour
{
    [SerializeField] private AccountUI accountUI;

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

        if (OlympusFoundation.Initialised())
        {
            if(OlympusFoundation.shutdown())
            {
                Debug.Log("Successfully Shutdown...");
            }else
            {
                Debug.Log("Failed to Shutdown...");
            }
            
        }
    }
}