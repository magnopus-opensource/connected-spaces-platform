// ------------------------------------------------------------------
// Copyright (c) Magnopus. All Rights Reserved.
// ------------------------------------------------------------------
#pragma once

#include <iostream>
#include "Olympus/Common/String.h"
#include "Olympus/OlympusFoundation.h"


using namespace std;


const oly_common::String Tenant = "FOUNDATION_HELLO_WORLD";


int main()
{
	//Initialise Foundation
	const oly_common::String EndpointRootURI = "https://ogs-ostage.magnoboard.com";
	if (oly::OlympusFoundation::Initialise(EndpointRootURI, Tenant))
	{
		cout << "Foundation initalised" << endl; 
	}
	else
	{
		cout << "Foundation failed to initialise" << endl; 
	}

	if(oly::OlympusFoundation::GetIsInitialised()){
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
