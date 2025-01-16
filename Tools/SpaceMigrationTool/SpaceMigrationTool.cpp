#include "CSP/CSPFoundation.h"
#include "CSP/Systems/Spaces/Space.h"
#include "CSP/Systems/Spaces/SpaceSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "CommandLineParser.h"
#include "Helpers.h"

#include <iostream>

bool Login(const std::string& UserEmail, const std::string& UserPassword)
{
    bool RetValue = true;
    ServiceResponseReceiver<csp::systems::LoginStateResult> ResponseReceiver;
    csp::systems::LoginStateResultCallback Callback = [&](const csp::systems::LoginStateResult& Result)
    {
        if (Result.GetResultCode() == csp::services::EResultCode::Failed)
        {
            std::cout << "Error: Login failed" << std::endl;
            RetValue = false;
        }

        ResponseReceiver.OnResult(Result);
    };

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto& UserSystem = *SystemsManager.GetUserSystem();
    UserSystem.Login("", UserEmail.c_str(), UserPassword.c_str(), Callback);
    ResponseReceiver.WaitForResult();

    return RetValue;
}

void GetSpacesForLoggedInUser(csp::common::Array<csp::systems::Space>& OutSpaces)
{
    csp::common::Array<csp::systems::Space> Spaces;

    ServiceResponseReceiver<csp::systems::SpacesResult> ResponseReceiver;
    csp::systems::SpacesResultCallback Callback = [&](const csp::systems::SpacesResult& Result)
    {
        if (Result.GetResultCode() == csp::services::EResultCode::Success)
        {
            auto& ResultSpaces = Result.GetSpaces();
            OutSpaces = csp::common::Array<csp::systems::Space>(ResultSpaces.Size());

            for (int idx = 0; idx < ResultSpaces.Size(); ++idx)
            {
                OutSpaces[idx] = ResultSpaces[idx];
            }
        }

        ResponseReceiver.OnResult(Result);
    };

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto& SpaceSystem = *SystemsManager.GetSpaceSystem();
    SpaceSystem.GetSpaces(Callback);
    ResponseReceiver.WaitForResult();
}

void ListSpacesForLoggedInUser()
{
    csp::common::Array<csp::systems::Space> UserSpaces;
    GetSpacesForLoggedInUser(UserSpaces);

    for (int idx = 0; idx < UserSpaces.Size(); ++idx)
    {
        std::cout << "Space Id: " << UserSpaces[idx].Id << " -> "
                  << "Space Name: " << UserSpaces[idx].Name << std::endl;
    }
}

bool GetSpace(const csp::common::String SpaceId, csp::systems::Space& OutSpaceInfo)
{
    bool RetValue = false;

    ServiceResponseReceiver<csp::systems::SpaceResult> ResponseReceiver;
    csp::systems::SpaceResultCallback Callback = [&](const csp::systems::SpaceResult& Result)
    {
        RetValue = (Result.GetResultCode() == csp::services::EResultCode::Success);

        if (Result.GetResultCode() == csp::services::EResultCode::Success)
        {
            OutSpaceInfo = Result.GetSpace();
        }

        ResponseReceiver.OnResult(Result);
    };

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto& SpaceSystem = *SystemsManager.GetSpaceSystem();
    SpaceSystem.GetSpace(SpaceId, Callback);
    ResponseReceiver.WaitForResult();

    return RetValue;
}

void MigrateSpace(const CommandLineParser& Parser)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto& SpaceSystem = *SystemsManager.GetSpaceSystem();

    if (Parser.SpaceId.empty())
    {
        std::cout << "Error: Empty Space Id provided";
        return;
    }

    csp::systems::Space RetrievedSpace;
    if (!GetSpace(Parser.SpaceId.c_str(), RetrievedSpace))
    {
        std::cout << "Error: Space retrieval failed. Migration has not been completed." << std::endl;
        return;
    }

    ServiceResponseReceiver<csp::systems::NullResult> MigrationResponseReceiver;
    csp::systems::NullResultCallback MigrationCallback = [&](const csp::systems::NullResult& MigrationResult)
    {
        if (MigrationResult.GetResultCode() == csp::services::EResultCode::Success)
        {
            std::cout << "The Space has been migrated successfully!";
        }
        else if (MigrationResult.GetResultCode() == csp::services::EResultCode::Failed)
        {
            std::cout << "Error: Space migration failed with error code " << MigrationResult.GetHttpResultCode() << std::endl;
        }

        MigrationResponseReceiver.OnResult(MigrationResult);
    };

    // NOTE - The space v2 migration functionality has been removed from Foundation.
    // We have elected to preserve the tool (as we expect it may be useful in the future)
    // and comment out the line that no longer exists in foundation.
    // SpaceSystem.MigrateVersion2Space(RetrievedSpace, MigrationCallback);

    MigrationResponseReceiver.WaitForResult();
}

int main(int argc, const char* argv[])
{
    CommandLineParser Parser;
    Parser.ParseCommandLine(argc, argv);

    if (Parser.IsShowHelpOperation)
    {
        return 0;
    }

    csp::CSPFoundation::Initialise(Parser.EndpointBaseURI.c_str(), Parser.Tenant.c_str());

    if (!Login(Parser.UserEmailAddress, Parser.UserPassword))
        return 0;

    if (Parser.IsListSpaceOperation)
    {
        ListSpacesForLoggedInUser();
    }
    else if (Parser.IsMigrateSpaceOperation)
    {
        MigrateSpace(Parser);
    }

    csp::CSPFoundation::Shutdown();

    return 0;
}