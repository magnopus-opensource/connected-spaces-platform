// ------------------------------------------------------------------
// Copyright (c) Magnopus. All Rights Reserved.
// ------------------------------------------------------------------
#pragma once

#include "Olympus/Common/String.h"
#include "Olympus/OlympusFoundation.h"
#include <iostream>

using namespace std;

const oly_common::String Tenant = "CSP_HELLO_WORLD";

int main()
{
    // Initialise Foundation
    const oly_common::String EndpointRootURI = "https://ogs.magnopus-stg.cloud";
    if (oly::OlympusFoundation::Initialise(EndpointRootURI, Tenant))
    {
        cout << "Foundation initalised" << endl;
    }
    else
    {
        cout << "Foundation failed to initialise" << endl;
    }

    if (oly::OlympusFoundation::GetIsInitialised())
    {
        // Shut down Foundation
        if (oly::OlympusFoundation::Shutdown)
        {
            cout << "Foundation shutdown" << endl;
        }
        else
        {

            cout << "Foundation failed to shutdown" << endl;
        }
    }
}
