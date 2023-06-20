#include "CommandLineParser.h"
#include "Helpers.h"
#include "Olympus/OlympusFoundation.h"
#include "Olympus/Systems/Spaces/Space.h"
#include "Olympus/Systems/Spaces/SpaceSystem.h"
#include "Olympus/Systems/SystemsManager.h"
#include "Olympus/Systems/Users/UserSystem.h"

#include <iostream>


bool Login(const std::string& UserEmail, const std::string& UserPassword)
{
	bool RetValue = true;
	ServiceResponseReceiver<oly_systems::LoginStateResult> ResponseReceiver;
	oly_systems::LoginStateResultCallback Callback = [&](const oly_systems::LoginStateResult& Result)
	{
		if (Result.GetResultCode() == oly_services::EResultCode::Failed)
		{
			std::cout << "Error: Login failed" << std::endl;
			RetValue = false;
		}

		ResponseReceiver.OnResult(Result);
	};

	auto& SystemsManager = oly_systems::SystemsManager::Get();
	auto& UserSystem	 = *SystemsManager.GetUserSystem();
	UserSystem.Login("", UserEmail.c_str(), UserPassword.c_str(), Callback);
	ResponseReceiver.WaitForResult();

	return RetValue;
}

void GetSpacesForLoggedInUser(oly_common::Array<oly_systems::Space>& OutSpaces)
{
	oly_common::Array<oly_systems::Space> Spaces;

	ServiceResponseReceiver<oly_systems::SpacesResult> ResponseReceiver;
	oly_systems::SpacesResultCallback Callback = [&](const oly_systems::SpacesResult& Result)
	{
		if (Result.GetResultCode() == oly_services::EResultCode::Success)
		{
			auto& ResultSpaces = *Result.GetSpaces();
			OutSpaces		   = oly_common::Array<oly_systems::Space>(ResultSpaces.Size());

			for (int idx = 0; idx < ResultSpaces.Size(); ++idx)
			{
				OutSpaces[idx] = ResultSpaces[idx];
			}
		}

		ResponseReceiver.OnResult(Result);
	};

	auto& SystemsManager = oly_systems::SystemsManager::Get();
	auto& SpaceSystem	 = *SystemsManager.GetSpaceSystem();
	SpaceSystem.GetSpaces(Callback);
	ResponseReceiver.WaitForResult();
}

void ListSpacesForLoggedInUser()
{
	oly_common::Array<oly_systems::Space> UserSpaces;
	GetSpacesForLoggedInUser(UserSpaces);

	for (int idx = 0; idx < UserSpaces.Size(); ++idx)
	{
		std::cout << "Space Id: " << UserSpaces[idx].Id << " -> "
				  << "Space Name: " << UserSpaces[idx].Name << std::endl;
	}
}

bool GetSpace(const oly_common::String SpaceId, oly_systems::Space& OutSpaceInfo)
{
	bool RetValue = false;

	ServiceResponseReceiver<oly_systems::SpaceResult> ResponseReceiver;
	oly_systems::SpaceResultCallback Callback = [&](const oly_systems::SpaceResult& Result)
	{
		RetValue = (Result.GetResultCode() == oly_services::EResultCode::Success);

		if (Result.GetResultCode() == oly_services::EResultCode::Success)
		{
			OutSpaceInfo = Result.GetSpace();
		}

		ResponseReceiver.OnResult(Result);
	};

	auto& SystemsManager = oly_systems::SystemsManager::Get();
	auto& SpaceSystem	 = *SystemsManager.GetSpaceSystem();
	SpaceSystem.GetSpace(SpaceId, Callback);
	ResponseReceiver.WaitForResult();

	return RetValue;
}

void MigrateSpace(const CommandLineParser& Parser)
{
	auto& SystemsManager = oly_systems::SystemsManager::Get();
	auto& SpaceSystem	 = *SystemsManager.GetSpaceSystem();

	if (Parser.SpaceId.empty())
	{
		std::cout << "Error: Empty Space Id provided";
		return;
	}

	oly_systems::Space RetrievedSpace;
	if (!GetSpace(Parser.SpaceId.c_str(), RetrievedSpace))
	{
		std::cout << "Error: Space retrieval failed. Migration has not been completed." << std::endl;
		return;
	}

	ServiceResponseReceiver<oly_systems::NullResult> MigrationResponseReceiver;
	oly_systems::NullResultCallback MigrationCallback = [&](const oly_systems::NullResult& MigrationResult)
	{
		if (MigrationResult.GetResultCode() == oly_services::EResultCode::Success)
		{
			std::cout << "The Space has been migrated successfully!";
		}
		else if (MigrationResult.GetResultCode() == oly_services::EResultCode::Failed)
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

	oly::OlympusFoundation::Initialise(Parser.EndpointBaseURI.c_str());

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

	oly::OlympusFoundation::Shutdown();

	return 0;
}