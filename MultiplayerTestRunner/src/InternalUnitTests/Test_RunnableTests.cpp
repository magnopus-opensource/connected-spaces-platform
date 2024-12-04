#include "../../include/TestIdentifiers.h"
#include "../LoginRAII.h"
#include "../RunnableTests/CreateAvatar.h"
#include "../SpaceRAII.h"
#include "../Utils.h"

#include <CSP/Common/String.h>
#include <gtest/gtest.h>

/*
 * The only purpose of this test suite is to execute the runnable tests themselves
 * We do no validation that they're doing what they should do, to do so would be
 * testing tests, which is too much testing for me.
 * Nonetheless, we should ensure they don't crash.
 */

namespace
{
/* Some tests only run if there's a credentials file */
std::optional<Utils::TestAccountCredentials> CredentialsFromFile()
{
	try
	{
		return Utils::LoadTestAccountCredentials();
	}
	catch (...)
	{
		return {};
	}
}
} // namespace

/* Initialze CSP before the suite begins with a fixture */
class RunnableTests : public ::testing::Test
{
protected:
	static void SetUpTestSuite()
	{
		Utils::InitialiseCSPWithUserAgentInfo(Utils::DEFAULT_TEST_ENDPOINT);
	}
};

TEST_F(RunnableTests, CreateAvatar)
{
	std::optional<Utils::TestAccountCredentials> Credentials = CredentialsFromFile();
	if (!Credentials.has_value())
	{
		GTEST_SKIP() << "No credentials file found, Skipping Test.";
	}

	// Login
	LoginRAII login {Credentials.value().DefaultLoginEmail, Credentials.value().DefaultLoginPassword};
	// Make a throwaway space
	SpaceRAII Space({});

	CreateAvatar::RunTest();
}