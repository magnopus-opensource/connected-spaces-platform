// ------------------------------------------------------------------
// Copyright (c) Magnopus. All Rights Reserved.
// ------------------------------------------------------------------
#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <future>
#include <filesystem>
#include <ThirdParty/nlohmann/json.hpp>
#include "Olympus/Common/String.h"
#include "Olympus/OlympusFoundation.h"
#include "Olympus/Systems/SystemsManager.h"
#include "Olympus/Systems/Users/UserSystem.h"
#include "Olympus/Systems/Users/Authentication.h"
#include "Olympus/Systems/Spaces/Space.h"
#include "Olympus/Systems/Spaces/SpaceSystem.h"
#include "Olympus/Systems/GraphQL/GraphQL.h"
#include "Olympus/Systems/GraphQL/GraphQLSystem.h"
#include "Olympus/Multiplayer/MultiPlayerConnection.h"
#include "Olympus/Multiplayer/SpaceEntity.h"
#include "Olympus/Multiplayer/Components/AvatarSpaceComponent.h"
#include "Olympus/Systems/Assets/AssetSystem.h"
#include "Olympus/Systems/Assets/AssetCollection.h"
#include "Olympus/Systems/Assets/Asset.h"

using namespace std;
using namespace nlohmann;

const oly_common::String Tenant = "FOUNDATION_HELLO_WORLD";
oly_common::String CurrentSpaceId;
oly_multiplayer::MultiplayerConnection* MultiplayerConnection = nullptr;
oly_multiplayer::SpaceEntity* Avatar = nullptr;
oly_systems::AssetCollection AssetCollection;
oly_systems::Asset Asset;

bool StartupFoundation()
{
    const oly_common::String EndpointRootURI = "https://ogs-ostage.magnoboard.com";
	return oly::OlympusFoundation::Initialise(EndpointRootURI, Tenant);
}

void SetClientUserAgentInfo()
{
	oly::ClientUserAgent ClientHeaderInfo;
	ClientHeaderInfo.OlympusVersion = oly::OlympusFoundation::GetBuildID();
	ClientHeaderInfo.ClientSKU = "foundation-cPlusPlus-examples";
	ClientHeaderInfo.ClientEnvironment = "DEV";
	ClientHeaderInfo.ClientOS = "WIN64";
	ClientHeaderInfo.ClientVersion = "1.0";
	ClientHeaderInfo.CHSEnvironment = "ODEV";

	oly::OlympusFoundation::SetClientUserAgentInfo(ClientHeaderInfo);
}

bool ShutdownFoundation()
{
	return oly::OlympusFoundation::Shutdown();
}

void Signup()
{
	string Email, Password;
	cout << "\nPlease enter your email address to sign up:"  << endl;
	cin >> Email;
	cout << "Please enter your password to sign up:"  << endl;
	cin >> Password;

	promise<void> CallbackPromise;
	future<void> CallbackFuture = CallbackPromise.get_future();

	oly_systems::UserSystem* UserSystem = oly_systems::SystemsManager::Get().GetUserSystem();

	UserSystem->CreateUser("", "", Email.c_str(), Password.c_str(), false,
				"", "", [&](const oly_systems::ProfileResult& Result)
	{
		if (Result.GetResultCode() == oly_services::EResultCode::Success)
		{
			cout << "\nSuccessfully signed up as " + Email << endl;
			cout << "You should have received a verification email at " + Email << endl;
			cout << "Please restart this application once verified" << endl;
			CallbackPromise.set_value();
		}
		else if (Result.GetResultCode() == oly_services::EResultCode::Failed)
		{
			cout << "\nSign up failed. Please double check if have an account already and restart this application. " + Result.GetResponseBody()<< endl;
			CallbackPromise.set_value();
		}
	});

	CallbackFuture.wait();
}

void Login()
{
	string Email, Password;
	cout << "\nPlease enter your email address to log in:"  << endl;
	cin >> Email;
	cout << "Please enter your password to log in:"  << endl;
	cin >> Password;

	promise<void> CallbackPromise;
	future<void> CallbackFuture = CallbackPromise.get_future();

	oly_systems::UserSystem* UserSystem = oly_systems::SystemsManager::Get().GetUserSystem();

	UserSystem->Login("", Email.c_str(), Password.c_str(), [&](const oly_systems::LoginStateResult& Result)
	{
		if (Result.GetResultCode() == oly_services::EResultCode::Success)
		{
			cout << "Successfully logged in as " + Email << endl;
			CallbackPromise.set_value();
		}
		else if (Result.GetResultCode() == oly_services::EResultCode::Failed)
		{
			cout << "Login failed. " + Result.GetResponseBody()<< endl;
			CallbackPromise.set_value();
		}
	});

	CallbackFuture.wait();
}

void Logout()
{
	promise<void> CallbackPromise;
	future<void> CallbackFuture = CallbackPromise.get_future();

	oly_systems::UserSystem* UserSystem = oly_systems::SystemsManager::Get().GetUserSystem();

	UserSystem->Logout([&](const oly_systems::LogoutResult& Result)
	{
		if (Result.GetResultCode() == oly_services::EResultCode::Success)
		{
			cout << "\nSuccessfully logged out" << endl;
			CallbackPromise.set_value();
		}
		else if (Result.GetResultCode() == oly_services::EResultCode::Failed)
		{
			cout << "\nLogout failed. " + Result.GetResponseBody()<< endl;
			CallbackPromise.set_value();
		}
	});

	CallbackFuture.wait();
}

void SearchSpaces()
{
	promise<void> CallbackPromise;
	future<void> CallbackFuture = CallbackPromise.get_future();

	//Query to get the first 10 spaces
	oly_common::String SpacesQuery =
		"spaces("
		"pagination: { limit: 10, skip: 0 }"
		"filters: {}"
		") {"
			"itemTotalCount,"
			"items{"
			"id: groupId,"
			"name,"
			"description"
			"}"
		"}"
	;

	oly_systems::GraphQLSystem* QuerySystem = oly_systems::SystemsManager::Get().GetGraphQLSystem();

	QuerySystem->RunQuery(SpacesQuery, [&](oly_systems::GraphQLResult& Result)
	{
		if (Result.GetResultCode() == oly_services::EResultCode::Success)
		{			
			json JsonData = json::parse(Result.GetResponse().c_str());
			int TotalSpacesCount = JsonData["data"]["spaces"]["itemTotalCount"];
			cout << "\nFound " + to_string(TotalSpacesCount) + " spaces in total"  << endl;
			CallbackPromise.set_value();
		}
	});

	CallbackFuture.wait();
}

void CreateSpace()
{
	promise<void> CallbackPromise;
	future<void> CallbackFuture = CallbackPromise.get_future();

	cout << "\nCreate Space: please specify a name for the new space" << endl;
	string SpaceName;
	cin >> SpaceName;

	oly_common::Map<oly_common::String, oly_common::String> TestMetadata;	
	oly_systems::SpaceSystem* SpaceSystem = oly_systems::SystemsManager::Get().GetSpaceSystem();

	SpaceSystem->CreateSpace(SpaceName.c_str(), "", oly_systems::SpaceType::Private, nullptr, 
							TestMetadata, nullptr, [&](const oly_systems::SpaceResult& Result)
	{
		if (Result.GetResultCode() == oly_services::EResultCode::Success)
		{
			string SpaceID = Result.GetSpace().Id.c_str();
			cout << "Created a new space called " + SpaceName + " and ID: " + SpaceID << endl;
			CallbackPromise.set_value();
		}
		else if (Result.GetResultCode() == oly_services::EResultCode::Failed)
		{
			cout << "Error: could not create the new space. " + Result.GetResponseBody()<< endl;
			CallbackPromise.set_value();
		}
	});

	CallbackFuture.wait();
}

void EnterSpace()
{
	promise<void> CallbackPromise;
	future<void> CallbackFuture = CallbackPromise.get_future();

	cout << "\nEnter Space: please specify the space ID to enter" << endl;
	string SpaceId;
	cin >> SpaceId;

	oly_systems::SpaceSystem* SpaceSystem = oly_systems::SystemsManager::Get().GetSpaceSystem();

	SpaceSystem->EnterSpace(SpaceId.c_str(), true, [&] (const oly_systems::EnterSpaceResult& Result)
	{
		if (Result.GetResultCode() == oly_services::EResultCode::Success)
		{
			cout << "Entered space with ID: " + SpaceId << endl;

			CurrentSpaceId = SpaceId.c_str();

			//Set Multiplayer Connection once entered a space
			MultiplayerConnection = Result.GetConnection();
		}
		else if (Result.GetResultCode() == oly_services::EResultCode::Failed)
		{
			cout << "Error: Could not enter space. " + Result.GetResponseBody()<< endl;
		}
		CallbackPromise.set_value();
	});

	CallbackFuture.wait();
}

void SetSelfMessaging()
{
	promise<void> CallbackPromise;
	future<void> CallbackFuture = CallbackPromise.get_future();

	MultiplayerConnection->SetAllowSelfMessagingFlag(true, [&] (bool IsSuccessful)
	{
		if(IsSuccessful)
		{
			cout << "\nAllowed this client to receive its own messages through multiplayer"<< endl;
		}
		else
		{
			cout << "\nError: Could not allow this client to receive every message it sends through multiplayer"<< endl;
		}
		CallbackPromise.set_value();
	});

	CallbackFuture.wait();
}

void CreateAvatarEntity()
{
	promise<void> CallbackPromise;
	future<void> CallbackFuture = CallbackPromise.get_future();

	oly_multiplayer::SpaceTransform InSpaceTransform = {oly_common::Vector3::Zero(), oly_common::Vector4::Zero(), oly_common::Vector3::One()};
	oly_multiplayer::SpaceEntitySystem* SpaceEntitySystem = MultiplayerConnection->GetSpaceEntitySystem();

	string AvatarName = "TestAvatar";
	SpaceEntitySystem->CreateAvatar(AvatarName.c_str(), InSpaceTransform, oly_multiplayer::AvatarState::Idle, "id",
									oly_multiplayer::AvatarPlayMode::Default, [&] (oly_multiplayer::SpaceEntity* AvatarSpaceEntity)
	{
		if(AvatarSpaceEntity != nullptr)
		{
			cout << "\nAvatar Entity created with name " + AvatarName << endl;
			Avatar = AvatarSpaceEntity;
		}
		else
		{
			cout << "\nError: Could not create an Avatar Entity"<< endl;
		}
		CallbackPromise.set_value();
	});

	CallbackFuture.wait();
}

int WaitForTimeoutCountMs;
int WaitForTimeoutLimit = 20000;

void MoveEntity(oly_multiplayer::SpaceEntity* Entity)
{
	bool EntityUpdated = false;

	//Set Entity Update Callback
	Entity->SetUpdateCallback([&](const oly_multiplayer::SpaceEntity* SpaceEntity, oly_multiplayer::SpaceEntityUpdateFlags UpdateFlags, 
									oly_common::Array<oly_multiplayer::ComponentUpdateInfo> ComponentUpdateInfo)
	{
		if (UpdateFlags & oly_multiplayer::SpaceEntityUpdateFlags::UPDATE_FLAGS_POSITION)
		{
			string SpaceEntityName = SpaceEntity->GetName().c_str();
			oly_common::Vector3 EntityPosition = SpaceEntity->GetTransform().Position;
			cout << "Received update from Entity " + SpaceEntityName + " : it moved to " + to_string(EntityPosition.X) + ", " + to_string(EntityPosition.Y) + ", " + to_string(EntityPosition.Z) << endl;
			EntityUpdated = true;
		}
	});

	//Move Entity
	oly_common::Vector3 EntityNewPosition = oly_common::Vector3{1.0f, 2.0f, 3.0f};
	Entity->SetPosition(EntityNewPosition);
	Entity->QueueUpdate();
	string EntityName = Entity->GetName().c_str();
	cout << "\nMoved Entity " + EntityName + " to " + to_string(EntityNewPosition.X) + ", " + to_string(EntityNewPosition.Y) + ", " + to_string(EntityNewPosition.Z) << endl;

	//Simulate "tick", which is needed for Multiplayer
	while (!EntityUpdated && WaitForTimeoutCountMs < WaitForTimeoutLimit)
	{
		oly_multiplayer::SpaceEntitySystem* EntitySystem = MultiplayerConnection->GetSpaceEntitySystem();
		EntitySystem->ProcessPendingEntityOperations();
		std::this_thread::sleep_for(50ms);
		WaitForTimeoutCountMs += 50;
	}
}

void CreateAssetCollection()
{
	promise<void> CallbackPromise;
	future<void> CallbackFuture = CallbackPromise.get_future();

	cout << "\nCreate Asset Collection: please enter a unique name" << endl;
	string AssetCollectionName;
	cin >> AssetCollectionName;

	oly_systems::AssetSystem* AssetSystem = oly_systems::SystemsManager::Get().GetAssetSystem();
	AssetSystem->CreateAssetCollection(CurrentSpaceId, nullptr, AssetCollectionName.c_str(),
									nullptr, oly_systems::EAssetCollectionType::DEFAULT, nullptr, 
									[&](const oly_systems::AssetCollectionResult Result)
	{
		if(Result.GetResultCode() == oly_services::EResultCode::Success)
		{
			AssetCollection = Result.GetAssetCollection();
			cout << "Created a new Asset Collection called " + AssetCollection.Name + ".ID: " + AssetCollection.Id << endl;
			CallbackPromise.set_value();
		}
		else if (Result.GetResultCode() == oly_services::EResultCode::Failed)
		{
			cout << "Error: Could not create a new Asset Collection. " + Result.GetResponseBody()<< endl;
		}
	});

	CallbackFuture.wait();
}

void CreateAsset()
{
	promise<void> CallbackPromise;
	future<void> CallbackFuture = CallbackPromise.get_future();

	cout << "\nCreate Asset: please enter a unique name" << endl;
	string AssetName;
	cin >> AssetName;

	oly_systems::AssetSystem* AssetSystem = oly_systems::SystemsManager::Get().GetAssetSystem();
	AssetSystem->CreateAsset(AssetCollection, AssetName.c_str(), nullptr, nullptr,
							oly_systems::EAssetType::IMAGE, [&](const oly_systems::AssetResult Result)
	{
		if(Result.GetResultCode() == oly_services::EResultCode::Success)
		{
			Asset = Result.GetAsset();
			cout << "Created a new Asset called " + Asset.Name + ". ID: " + Asset.Id << endl;
			CallbackPromise.set_value();
		}
		else if (Result.GetResultCode() == oly_services::EResultCode::Failed)
		{
			cout << "Error: Could not create a new Asset. " + Result.GetResponseBody()<< endl;
			CallbackPromise.set_value();
		}
	});

	CallbackFuture.wait();
}

void UploadAsset()
{
	promise<void> CallbackPromise;
	future<void> CallbackFuture = CallbackPromise.get_future();

	//Create Asset Data Source from file
	string SolutionPath = _SOLUTION_DIR;
	filesystem::path FilePath = std::filesystem::absolute(SolutionPath + "TestAsset/TestImage.png");

	oly_systems::FileAssetDataSource AssetDataSource;
	AssetDataSource.FilePath = FilePath.u8string().c_str();
	AssetDataSource.SetMimeType("image/png");

	//Upload Asset
	oly_systems::AssetSystem* AssetSystem = oly_systems::SystemsManager::Get().GetAssetSystem();

	AssetSystem->UploadAssetData(AssetCollection, Asset, AssetDataSource, [&](const oly_systems::UriResult& Result)
	{
		if(Result.GetResultCode() == oly_services::EResultCode::Success)
		{
			cout << "\nUploaded Test Asset from path: " + AssetDataSource.FilePath << endl;
			CallbackPromise.set_value();
		}
		else if(Result.GetResultCode() == oly_services::EResultCode::Failed)
		{
			cout << "\nError: Could not upload Test Asset. " + Result.GetResponseBody()<< endl;
			CallbackPromise.set_value();
		}
	});

	CallbackFuture.wait();
}

void DeleteAsset()
{
	promise<void> CallbackPromise;
	future<void> CallbackFuture = CallbackPromise.get_future();

	oly_systems::AssetSystem* AssetSystem = oly_systems::SystemsManager::Get().GetAssetSystem();
	AssetSystem->DeleteAsset(AssetCollection, Asset, [&](const oly_systems::NullResult Result)
	{
		if(Result.GetResultCode() == oly_services::EResultCode::Success)
		{
			cout << "\nDeleted Asset called " + Asset.Name + ". ID: " + Asset.Id << endl;
			CallbackPromise.set_value();
		}
		else if (Result.GetResultCode() == oly_services::EResultCode::Failed)
		{
			cout << "\nError: Could not delete Asset. " + Result.GetResponseBody()<< endl;
			CallbackPromise.set_value();
		}
	});

	CallbackFuture.wait();
}

void ExitSpace()
{
	oly_systems::SpaceSystem* SpaceSystem = oly_systems::SystemsManager::Get().GetSpaceSystem();

	SpaceSystem->ExitSpace();
	cout << "\nExited space"  << endl;
}

void DeleteSpace()
{
	promise<void> CallbackPromise;
	future<void> CallbackFuture = CallbackPromise.get_future();

	cout << "\nDelete Space: please specify the ID of the space to delete:" << endl;
	string SpaceId;
	cin >> SpaceId;

	oly_systems::SpaceSystem* SpaceSystem = oly_systems::SystemsManager::Get().GetSpaceSystem();

	SpaceSystem->DeleteSpace(SpaceId.c_str(), [&](const oly_systems::NullResult& Result)
	{
		if (Result.GetResultCode() == oly_services::EResultCode::Success)
		{
			cout << "Deleted space with ID: " + SpaceId << endl;
			CallbackPromise.set_value();
		}
		else if (Result.GetResultCode() == oly_services::EResultCode::Failed)
		{
			cout << "Error: could not delete the space. " + Result.GetResponseBody()<< endl;
			CallbackPromise.set_value();
		}
	});

	CallbackFuture.wait();
}

int main()
{
	//Initialise Foundation
	if(StartupFoundation())
	{
		cout << "Welcome to Foundation! \nThis is a simple Hello World example to demonstrate basic Foundation functionality."  << endl;
		SetClientUserAgentInfo();
	}
	else
	{
		cout << "Error: Foundation could not be initialized."  << endl;
		return 1;
	}

	//Signup if we don't have an account yet
	cout << "\nDo you already have an account to login with using the tenant " + Tenant + "?"  << endl;
	string SignUpAnswer;
	while (SignUpAnswer != "Y" && SignUpAnswer != "y")
	{
		cout << "(Please answer Y for yes or N for no)" << endl;
		cin >> SignUpAnswer;
		if(SignUpAnswer == "N" || SignUpAnswer == "n")
		{
			Signup();
			//After sign up, exit application because the user either needs to verify email or sign up failed
			return 0;
		}
	}

	//Login
	Login();

	//Search spaces through GraphQL query
	SearchSpaces();

	//Create a new space if wanted
	cout << "\nDo you want to create a new space?" << endl;
	string CreateSpaceAnswer;
	while (CreateSpaceAnswer != "N" && CreateSpaceAnswer != "n")
	{
		cout << "(Please answer Y for yes or N for no)" << endl;
		cin >> CreateSpaceAnswer;
		if(CreateSpaceAnswer == "Y" || CreateSpaceAnswer == "y")
		{
			CreateSpace();
			break;
		}
	}

	//Enter an existing space
	EnterSpace();

	if(MultiplayerConnection != nullptr)
	{
		//For this example we want to demonstrate that we are able to
		//receive multiplayer updates, even the ones we send
		SetSelfMessaging();

		//Create an Avatar
		CreateAvatarEntity();

		//Move the Avatar
		if(Avatar != nullptr)
		{
			MoveEntity(Avatar);
		}

		//Add/delete an Asset to the space
		CreateAssetCollection();
		CreateAsset();
		UploadAsset();
		DeleteAsset();

		//Exit space
		ExitSpace();
	}

	//Delete a space if wanted
	cout << "\nDo you want to delete a space?" << endl;
	string DeleteSpaceAnswer;
	while (DeleteSpaceAnswer != "N" && DeleteSpaceAnswer != "n")
	{
		cout << "(Please answer Y for yes or N for no)" << endl;
		cin >> DeleteSpaceAnswer;
		if(DeleteSpaceAnswer == "Y" || DeleteSpaceAnswer == "y")
		{
			DeleteSpace();
			break;
		}
	}

	//Logout
	Logout();

	//Shut down Foundation
	if(ShutdownFoundation())
	{
		cout << "\nFoundation shut down" << endl;
	}
	else
	{
		cout << "\nError: Foundation could not shut down"  << endl;
		return 1;
	}
}
