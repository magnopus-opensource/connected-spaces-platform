// ------------------------------------------------------------------
// Copyright (c) Magnopus. All Rights Reserved.
// ------------------------------------------------------------------
#pragma once

#include "CSP/CSPFoundation.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/Components/AvatarSpaceComponent.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Multiplayer/SpaceEntitySystem.h"
#include "CSP/Systems/Assets/Asset.h"
#include "CSP/Systems/Assets/AssetCollection.h"
#include "CSP/Systems/Assets/AssetSystem.h"
#include "CSP/Systems/GraphQL/GraphQL.h"
#include "CSP/Systems/GraphQL/GraphQLSystem.h"
#include "CSP/Systems/Spaces/Space.h"
#include "CSP/Systems/Spaces/SpaceSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/Authentication.h"
#include "CSP/Systems/Users/UserSystem.h"
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>

using namespace std;
using namespace nlohmann;

const csp::common::String Tenant = "CSP_HELLO_WORLD";
csp::common::String CurrentSpaceId;
csp::multiplayer::MultiplayerConnection* MultiplayerConnection = nullptr;
csp::multiplayer::SpaceEntity* Avatar = nullptr;
csp::systems::AssetCollection AssetCollection;
csp::systems::Asset Asset;

bool StartupCSPFoundation()
{
    const csp::common::String EndpointRootURI = "https://ogs.magnopus-stg.cloud";
    return csp::CSPFoundation::Initialise(EndpointRootURI, Tenant);
}

void SetClientUserAgentInfo()
{
    csp::ClientUserAgent ClientHeaderInfo;
    ClientHeaderInfo.CSPVersion = csp::CSPFoundation::GetBuildID();
    ClientHeaderInfo.ClientSKU = "foundation-cPlusPlus-examples";
    ClientHeaderInfo.ClientEnvironment = "oStage";
    ClientHeaderInfo.ClientOS = "WIN64";
    ClientHeaderInfo.ClientVersion = "1.0";
    ClientHeaderInfo.CHSEnvironment = "oStage";

    csp::CSPFoundation::SetClientUserAgentInfo(ClientHeaderInfo);
}

bool ShutdownCSPFoundation() { return csp::CSPFoundation::Shutdown(); }

void Signup()
{
    string Email, Password;
    cout << "\nPlease enter your email address to sign up:" << endl;
    cin >> Email;
    cout << "Please enter your password to sign up:" << endl;
    cin >> Password;

    promise<void> CallbackPromise;
    future<void> CallbackFuture = CallbackPromise.get_future();

    csp::systems::UserSystem* UserSystem = csp::systems::SystemsManager::Get().GetUserSystem();

    UserSystem->CreateUser("", "", Email.c_str(), Password.c_str(), false, true, "", "",
        [&](const csp::systems::ProfileResult& Result)
        {
            if (Result.GetResultCode() == csp::systems::EResultCode::Success)
            {
                cout << "\nSuccessfully signed up as " + Email << endl;
                cout << "You should have received a verification email at " + Email << endl;
                cout << "Please restart this application once verified" << endl;
                CallbackPromise.set_value();
            }
            else if (Result.GetResultCode() == csp::systems::EResultCode::Failed)
            {
                cout << "\nSign up failed. Please double check if have an account already and restart this application. " + Result.GetResponseBody()
                     << endl;
                CallbackPromise.set_value();
            }
        });

    CallbackFuture.wait();
}

void Login()
{
    string Email, Password;
    cout << "\nPlease enter your email address to log in:" << endl;
    cin >> Email;
    cout << "Please enter your password to log in:" << endl;
    cin >> Password;

    promise<void> CallbackPromise;
    future<void> CallbackFuture = CallbackPromise.get_future();

    csp::systems::UserSystem* UserSystem = csp::systems::SystemsManager::Get().GetUserSystem();

    UserSystem->Login("", Email.c_str(), Password.c_str(), true,
        [&](const csp::systems::LoginStateResult& Result)
        {
            if (Result.GetResultCode() == csp::systems::EResultCode::Success)
            {
                cout << "Successfully logged in as " + Email << endl;
                CallbackPromise.set_value();
            }
            else if (Result.GetResultCode() == csp::systems::EResultCode::Failed)
            {
                cout << "Login failed. " + Result.GetResponseBody() << endl;
                CallbackPromise.set_value();
            }
        });

    CallbackFuture.wait();
}

void Logout()
{
    promise<void> CallbackPromise;
    future<void> CallbackFuture = CallbackPromise.get_future();

    csp::systems::UserSystem* UserSystem = csp::systems::SystemsManager::Get().GetUserSystem();

    UserSystem->Logout(
        [&](const csp::systems::LogoutResult& Result)
        {
            if (Result.GetResultCode() == csp::systems::EResultCode::Success)
            {
                cout << "\nSuccessfully logged out" << endl;
                CallbackPromise.set_value();
            }
            else if (Result.GetResultCode() == csp::systems::EResultCode::Failed)
            {
                cout << "\nLogout failed. " + Result.GetResponseBody() << endl;
                CallbackPromise.set_value();
            }
        });

    CallbackFuture.wait();
}

void SearchSpaces()
{
    promise<void> CallbackPromise;
    future<void> CallbackFuture = CallbackPromise.get_future();

    // Query to get the first 10 spaces
    csp::common::String SpacesQuery = "spaces("
                                      "pagination: { limit: 10, skip: 0 }"
                                      "filters: {}"
                                      ") {"
                                      "itemTotalCount,"
                                      "items{"
                                      "id: groupId,"
                                      "name,"
                                      "description"
                                      "}"
                                      "}";

    csp::systems::GraphQLSystem* QuerySystem = csp::systems::SystemsManager::Get().GetGraphQLSystem();

    QuerySystem->RunQuery(SpacesQuery,
        [&](csp::systems::GraphQLResult& Result)
        {
            if (Result.GetResultCode() == csp::systems::EResultCode::Success)
            {
                json JsonData = json::parse(Result.GetResponse().c_str());
                int TotalSpacesCount = JsonData["data"]["spaces"]["itemTotalCount"];
                cout << "\nFound " + to_string(TotalSpacesCount) + " spaces in total" << endl;
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
    cin.ignore();
    std::getline(cin, SpaceName);

    csp::systems::SystemsManager& SystemsManager = csp::systems::SystemsManager::Get();
    csp::systems::SpaceSystem* SpaceSystem = SystemsManager.GetSpaceSystem();

    csp::common::Map<csp::common::String, csp::common::String> TestMetadata = { { "spaceData", "myData" } };

    SpaceSystem->CreateSpace(SpaceName.c_str(), "", csp::systems::SpaceAttributes::Private, nullptr, TestMetadata, nullptr,
        [&CallbackPromise](const csp::systems::SpaceResult& Result)
        {
            if (Result.GetResultCode() == csp::systems::EResultCode::Success)
            {
                string SpaceID = Result.GetSpace().Id.c_str();
                string SpaceName = Result.GetSpace().Name.c_str();
                cout << "Created a new space called " + SpaceName + " and ID: " + SpaceID << endl;
                CallbackPromise.set_value();
            }
            else if (Result.GetResultCode() == csp::systems::EResultCode::Failed)
            {
                cout << "Error: could not create the new space. " + Result.GetResponseBody() << endl;
                CallbackPromise.set_value();
            }
        });

    CallbackFuture.wait();
}

void SetupConnection()
{
    promise<void> CallbackPromise;
    future<void> CallbackFuture = CallbackPromise.get_future();

    cout << "\nEnter Space: please specify the space ID to enter" << endl;
    string SpaceId;
    cin >> SpaceId;

    CurrentSpaceId = SpaceId.c_str();

    MultiplayerConnection = new csp::multiplayer::MultiplayerConnection(CurrentSpaceId);

    csp::multiplayer::SpaceEntitySystem* SpaceEntitySystem = MultiplayerConnection->GetSpaceEntitySystem();

    SpaceEntitySystem->SetEntityCreatedCallback(
        [](csp::multiplayer::SpaceEntity* Entity) { cout << "A new remote Entity has been created: " + Entity->GetName() << endl; });

    MultiplayerConnection->Connect(
        [&](bool IsOk)
        {
            if (IsOk)
            {
                MultiplayerConnection->InitialiseConnection([&](bool Ok) { cout << "Connection has been established." << endl; });
            }
            else
            {
                cout << "Error: could not create a new connection." << endl;
            }
            CallbackPromise.set_value();
        });

    CallbackFuture.wait();
}

void EnterSpace()
{
    promise<void> CallbackPromise;
    future<void> CallbackFuture = CallbackPromise.get_future();

    csp::systems::SpaceSystem* SpaceSystem = csp::systems::SystemsManager::Get().GetSpaceSystem();

    SpaceSystem->EnterSpace(CurrentSpaceId.c_str(),
        [&](const csp::systems::NullResult& Result)
        {
            if (Result.GetResultCode() == csp::systems::EResultCode::Success)
            {
                cout << "Entered space with ID: " + CurrentSpaceId << endl;
            }
            else if (Result.GetResultCode() == csp::systems::EResultCode::Failed)
            {
                cout << "Error: Could not enter space. " + Result.GetResponseBody() << endl;
            }
            CallbackPromise.set_value();
        });

    CallbackFuture.wait();
}

void CreateAvatarEntity()
{
    promise<void> CallbackPromise;
    future<void> CallbackFuture = CallbackPromise.get_future();

    csp::multiplayer::SpaceTransform InSpaceTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    csp::multiplayer::SpaceEntitySystem* SpaceEntitySystem = MultiplayerConnection->GetSpaceEntitySystem();

    string AvatarName = "TestAvatar";
    SpaceEntitySystem->CreateAvatar(AvatarName.c_str(), InSpaceTransform, csp::multiplayer::AvatarState::Idle, "id",
        csp::multiplayer::AvatarPlayMode::Default,
        [&](csp::multiplayer::SpaceEntity* AvatarSpaceEntity)
        {
            if (AvatarSpaceEntity != nullptr)
            {
                cout << "\nAvatar Entity created with name " + AvatarName << endl;
                Avatar = AvatarSpaceEntity;
            }
            else
            {
                cout << "\nError: Could not create an Avatar Entity" << endl;
            }
            CallbackPromise.set_value();
        });

    CallbackFuture.wait();
}

int WaitForTimeoutCountMs;
int WaitForTimeoutLimit = 20000;

void MoveEntity(csp::multiplayer::SpaceEntity* Entity)
{
    bool EntityUpdated = false;

    // Set Entity Update Callback
    Entity->SetUpdateCallback(
        [&](const csp::multiplayer::SpaceEntity* SpaceEntity, csp::multiplayer::SpaceEntityUpdateFlags UpdateFlags,
            csp::common::Array<csp::multiplayer::ComponentUpdateInfo> ComponentUpdateInfo)
        {
            if (UpdateFlags & csp::multiplayer::SpaceEntityUpdateFlags::UPDATE_FLAGS_POSITION)
            {
                string SpaceEntityName = SpaceEntity->GetName().c_str();
                csp::common::Vector3 EntityPosition = SpaceEntity->GetTransform().Position;
                cout << "Received update from Entity " + SpaceEntityName + " : it moved to " + to_string(EntityPosition.X) + ", "
                        + to_string(EntityPosition.Y) + ", " + to_string(EntityPosition.Z)
                     << endl;
                EntityUpdated = true;
            }
        });

    // Move Entity
    csp::common::Vector3 EntityNewPosition = csp::common::Vector3 { 1.0f, 2.0f, 3.0f };
    Entity->SetPosition(EntityNewPosition);
    Entity->QueueUpdate();
    string EntityName = Entity->GetName().c_str();
    cout << "\nMoved Entity " + EntityName + " to " + to_string(EntityNewPosition.X) + ", " + to_string(EntityNewPosition.Y) + ", "
            + to_string(EntityNewPosition.Z)
         << endl;

    // Simulate "tick", which is needed for Multiplayer
    while (!EntityUpdated && WaitForTimeoutCountMs < WaitForTimeoutLimit)
    {
        csp::multiplayer::SpaceEntitySystem* EntitySystem = MultiplayerConnection->GetSpaceEntitySystem();
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
    cin.ignore();
    std::getline(cin, AssetCollectionName);

    csp::systems::AssetSystem* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
    AssetSystem->CreateAssetCollection(CurrentSpaceId, nullptr, AssetCollectionName.c_str(), nullptr, csp::systems::EAssetCollectionType::DEFAULT,
        nullptr,
        [&](const csp::systems::AssetCollectionResult Result)
        {
            if (Result.GetResultCode() == csp::systems::EResultCode::Success)
            {
                AssetCollection = Result.GetAssetCollection();
                cout << "Created a new Asset Collection called " + AssetCollection.Name + ".ID: " + AssetCollection.Id << endl;
                CallbackPromise.set_value();
            }
            else if (Result.GetResultCode() == csp::systems::EResultCode::Failed)
            {
                cout << "Error: Could not create a new Asset Collection. " + Result.GetResponseBody() << endl;
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
    cin.ignore();
    std::getline(cin, AssetName);

    csp::systems::AssetSystem* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
    AssetSystem->CreateAsset(AssetCollection, AssetName.c_str(), nullptr, nullptr, csp::systems::EAssetType::IMAGE,
        [&](const csp::systems::AssetResult Result)
        {
            if (Result.GetResultCode() == csp::systems::EResultCode::Success)
            {
                Asset = Result.GetAsset();
                cout << "Created a new Asset called " + Asset.Name + ". ID: " + Asset.Id << endl;
                CallbackPromise.set_value();
            }
            else if (Result.GetResultCode() == csp::systems::EResultCode::Failed)
            {
                cout << "Error: Could not create a new Asset. " + Result.GetResponseBody() << endl;
                CallbackPromise.set_value();
            }
        });

    CallbackFuture.wait();
}

void UploadAsset()
{
    promise<void> CallbackPromise;
    future<void> CallbackFuture = CallbackPromise.get_future();

    // Create Asset Data Source from file
    string SolutionPath = _SOLUTION_DIR;
    filesystem::path FilePath = std::filesystem::absolute(SolutionPath + "TestAsset/TestImage.png");

    csp::systems::FileAssetDataSource AssetDataSource;
    AssetDataSource.FilePath = FilePath.u8string().c_str();
    AssetDataSource.SetMimeType("image/png");

    // Upload Asset
    csp::systems::AssetSystem* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();

    AssetSystem->UploadAssetData(AssetCollection, Asset, AssetDataSource,
        [&](const csp::systems::UriResult& Result)
        {
            if (Result.GetResultCode() == csp::systems::EResultCode::Success)
            {
                cout << "\nUploaded Test Asset from path: " + AssetDataSource.FilePath << endl;
                CallbackPromise.set_value();
            }
            else if (Result.GetResultCode() == csp::systems::EResultCode::Failed)
            {
                cout << "\nError: Could not upload Test Asset. " + Result.GetResponseBody() << endl;
                CallbackPromise.set_value();
            }
        });

    CallbackFuture.wait();
}

void DeleteAsset()
{
    promise<void> CallbackPromise;
    future<void> CallbackFuture = CallbackPromise.get_future();

    csp::systems::AssetSystem* AssetSystem = csp::systems::SystemsManager::Get().GetAssetSystem();
    AssetSystem->DeleteAsset(AssetCollection, Asset,
        [&](const csp::systems::NullResult Result)
        {
            if (Result.GetResultCode() == csp::systems::EResultCode::Success)
            {
                cout << "\nDeleted Asset called " + Asset.Name + ". ID: " + Asset.Id << endl;
                CallbackPromise.set_value();
            }
            else if (Result.GetResultCode() == csp::systems::EResultCode::Failed)
            {
                cout << "\nError: Could not delete Asset. " + Result.GetResponseBody() << endl;
                CallbackPromise.set_value();
            }
        });

    CallbackFuture.wait();
}

void ExitSpace()
{
    csp::systems::SpaceSystem* SpaceSystem = csp::systems::SystemsManager::Get().GetSpaceSystem();

    SpaceSystem->ExitSpace();
    cout << "\nExited space" << endl;
}

void DeleteSpace()
{
    promise<void> CallbackPromise;
    future<void> CallbackFuture = CallbackPromise.get_future();

    cout << "\nDelete Space: please specify the ID of the space to delete:" << endl;
    string SpaceId;
    cin >> SpaceId;

    csp::systems::SpaceSystem* SpaceSystem = csp::systems::SystemsManager::Get().GetSpaceSystem();

    SpaceSystem->DeleteSpace(SpaceId.c_str(),
        [&](const csp::systems::NullResult& Result)
        {
            if (Result.GetResultCode() == csp::systems::EResultCode::Success)
            {
                cout << "Deleted space with ID: " + SpaceId << endl;
                CallbackPromise.set_value();
            }
            else if (Result.GetResultCode() == csp::systems::EResultCode::Failed)
            {
                cout << "Error: could not delete the space. " + Result.GetResponseBody() << endl;
                CallbackPromise.set_value();
            }
        });

    CallbackFuture.wait();
}

int main()
{
    // Initialise CSP Foundation
    if (StartupCSPFoundation())
    {
        cout << "Welcome to the Connected Spaces Platform (CSP)! \nThis is a simple Hello World example to demonstrate basic CSP functionality."
             << endl;
        SetClientUserAgentInfo();
    }
    else
    {
        cout << "Error: The Connected Spaces Platform (CSP) could not be initialized." << endl;
        return 1;
    }

    // Signup if we don't have an account yet
    cout << "\nDo you already have an account to login with using the tenant " + Tenant + "?" << endl;
    string SignUpAnswer;
    while (SignUpAnswer != "Y" && SignUpAnswer != "y")
    {
        cout << "(Please answer Y for yes or N for no)" << endl;
        cin >> SignUpAnswer;
        if (SignUpAnswer == "N" || SignUpAnswer == "n")
        {
            Signup();
            // After sign up, exit application because the user either needs to verify email or sign up failed
            return 0;
        }
    }

    // Login
    Login();

    // Search spaces through GraphQL query
    SearchSpaces();

    // Create a new space if wanted
    cout << "\nDo you want to create a new space?" << endl;
    string CreateSpaceAnswer;
    while (CreateSpaceAnswer != "N" && CreateSpaceAnswer != "n")
    {
        cout << "(Please answer Y for yes or N for no)" << endl;
        cin >> CreateSpaceAnswer;
        if (CreateSpaceAnswer == "Y" || CreateSpaceAnswer == "y")
        {
            CreateSpace();
            break;
        }
    }

    // Set up a multiplayer connection
    SetupConnection();

    // Enter an existing space
    EnterSpace();

    if (MultiplayerConnection != nullptr)
    {
        // Create an Avatar
        CreateAvatarEntity();

        // Move the Avatar
        if (Avatar != nullptr)
        {
            MoveEntity(Avatar);
        }

        // Add/delete an Asset to the space
        CreateAssetCollection();
        CreateAsset();
        UploadAsset();
        DeleteAsset();

        // Exit space
        ExitSpace();
    }

    // Delete a space if wanted
    cout << "\nDo you want to delete a space?" << endl;
    string DeleteSpaceAnswer;
    while (DeleteSpaceAnswer != "N" && DeleteSpaceAnswer != "n")
    {
        cout << "(Please answer Y for yes or N for no)" << endl;
        cin >> DeleteSpaceAnswer;
        if (DeleteSpaceAnswer == "Y" || DeleteSpaceAnswer == "y")
        {
            DeleteSpace();
            break;
        }
    }

    // Logout
    Logout();

    // Shut down CSP Foundation
    if (ShutdownCSPFoundation())
    {
        cout << "\nCSP Foundation shut down" << endl;
    }
    else
    {
        cout << "\nError: CSP Foundation could not shut down" << endl;
        return 1;
    }
}
