#include "Utils.h"

#include "../include/ErrorCodes.h"

#include <CSP/CSPFoundation.h>
#include <CSP/Common/String.h>
#include <CSP/Systems/WebService.h> //For resultbase
#include <filesystem>
#include <fstream>

namespace Utils
{

/*
 * Load the test account credentials from a location adjacent to the binary
 * Currently only provides 2 accounts, will need to be extended for tests that require more than 2 agents.
 */
TestAccountCredentials LoadTestAccountCredentials()
{
	if (!std::filesystem::exists("test_account_creds.txt"))
	{
		constexpr const char* msg
			= "test_account_creds.txt not found! This file must exist and must contain the following information:\n<DefaultLoginEmail> "
			  "<DefaultLoginPassword>\n<AlternativeLoginEmail> <AlternativeLoginPassword>\n<SuperUserLoginEmail> <SuperUserLoginPassword>";
		throw ExceptionWithCode(MultiplayerTestRunner::ErrorCodes::COULD_NOT_FIND_CREDENTIALS_FILE, msg);
	}

	std::ifstream CredsFile;
	CredsFile.open("test_account_creds.txt");

	std::string DefaultLoginEmail, DefaultLoginPassword, AlternativeLoginEmail, AlternativeLoginPassword, SuperUserLoginEmail, SuperUserLoginPassword;

	CredsFile >> DefaultLoginEmail >> DefaultLoginPassword;
	CredsFile >> AlternativeLoginEmail >> AlternativeLoginPassword;
	CredsFile >> SuperUserLoginEmail >> SuperUserLoginPassword;

	if (DefaultLoginEmail.empty() || DefaultLoginPassword.empty() || AlternativeLoginEmail.empty() || AlternativeLoginPassword.empty()
		|| SuperUserLoginEmail.empty() || SuperUserLoginPassword.empty())
	{
		constexpr const char* msg
			= "test_account_creds.txt must be in the following format:\n<DefaultLoginEmail> <DefaultLoginPassword>\n<AlternativeLoginEmail> "
			  "<AlternativeLoginPassword>\n<SuperUserLoginEmail> <SuperUserLoginPassword>";
		throw ExceptionWithCode(MultiplayerTestRunner::ErrorCodes::MALFORMED_CREDENTIALS_FILE, msg);
	}

	return TestAccountCredentials {DefaultLoginEmail.c_str(),
								   DefaultLoginPassword.c_str(),
								   AlternativeLoginEmail.c_str(),
								   AlternativeLoginPassword.c_str(),
								   SuperUserLoginEmail.c_str(),
								   SuperUserLoginPassword.c_str()};
}

void InitialiseCSPWithUserAgentInfo(const csp::common::String& EndpointRootURI)
{
	constexpr char* TESTS_CLIENT_SKU = "MultiplayerTestRunner";

	csp::CSPFoundation::Initialise(EndpointRootURI, "OKO_TESTS");

	csp::ClientUserAgent ClientHeaderInfo;
	ClientHeaderInfo.CSPVersion		   = csp::CSPFoundation::GetVersion();
	ClientHeaderInfo.ClientOS		   = "MultiplayerTestOS";
	ClientHeaderInfo.ClientSKU		   = TESTS_CLIENT_SKU;
	ClientHeaderInfo.ClientVersion	   = csp::CSPFoundation::GetVersion();
	ClientHeaderInfo.ClientEnvironment = "ODev";
	ClientHeaderInfo.CHSEnvironment	   = "oDev";

	csp::CSPFoundation::SetClientUserAgentInfo(ClientHeaderInfo);
}

std::string ToLowerCaseString(const std::string& input)
{
	std::string output = input;
	std::transform(input.cbegin(),
				   input.cend(),
				   output.begin(),
				   [](const unsigned char c)
				   {
					   return std::tolower(c);
				   });
	return output;
}

} // namespace Utils