/*
 * Copyright 2023 Magnopus LLC

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include "CSP/CSPFoundation.h"
#include "CSP/Systems/WebService.h"
#include "PublicTestBase.h"

#include <chrono>
#include <functional>
#include <gtest/gtest.h>
#include <iostream>
#include <thread>

inline const char* TESTS_CLIENT_SKU = "CPPTest";

// https://bitbucket.org/CadActive/workspace/snippets/GrBakB/macro-to-enable-namespaces-in-google-test

// A copy of GTEST_TEST_CLASS_NAME_, but with handling for namespace name.

#define NAMESPACE_GTEST_TEST_CLASS_NAME_(namespace_name, test_case_name, test_name) namespace_name##_##test_case_name##_##test_name##_Test

// A copy of GTEST_TEST_, but with handling for namespace name.

#define NAMESPACE_GTEST_TEST_(namespace_name, test_case_name, test_name, parent_class, parent_id)                                   \
	class NAMESPACE_GTEST_TEST_CLASS_NAME_(namespace_name, test_case_name, test_name) : public parent_class                         \
	{                                                                                                                               \
	public:                                                                                                                         \
		NAMESPACE_GTEST_TEST_CLASS_NAME_(namespace_name, test_case_name, test_name)()                                               \
		{                                                                                                                           \
		}                                                                                                                           \
                                                                                                                                    \
	private:                                                                                                                        \
		virtual void TestBody();                                                                                                    \
		static ::testing::TestInfo* const test_info_ GTEST_ATTRIBUTE_UNUSED_;                                                       \
		GTEST_DISALLOW_COPY_AND_ASSIGN_(NAMESPACE_GTEST_TEST_CLASS_NAME_(namespace_name, test_case_name, test_name));               \
	};                                                                                                                              \
                                                                                                                                    \
	::testing::TestInfo* const NAMESPACE_GTEST_TEST_CLASS_NAME_(namespace_name, test_case_name, test_name)::test_info_              \
		= ::testing::internal::MakeAndRegisterTestInfo(                                                                             \
			#namespace_name "." #test_case_name,                                                                                    \
			#test_name,                                                                                                             \
			NULL,                                                                                                                   \
			NULL, /* <-- Defines the test as "Namespace.Classname" */                                                               \
			::testing::internal::CodeLocation(__FILE__, __LINE__),                                                                  \
			(parent_id),                                                                                                            \
			parent_class::SetUpTestCase,                                                                                            \
			parent_class::TearDownTestCase,                                                                                         \
			new ::testing::internal::TestFactoryImpl<NAMESPACE_GTEST_TEST_CLASS_NAME_(namespace_name, test_case_name, test_name)>); \
	void NAMESPACE_GTEST_TEST_CLASS_NAME_(namespace_name, test_case_name, test_name)::TestBody()

// Custom macro for our unit tests so that we can declare the namespace needed in Visual Studio 2019 Test Explorer

#define CSP_PUBLIC_TEST(namespace_name, test_case_name, test_name) \
	NAMESPACE_GTEST_TEST_(namespace_name, test_case_name, test_name, ::PublicTestBase, ::testing::internal::GetTypeId<::PublicTestBase>())

#define CSP_INTERNAL_TEST(namespace_name, test_case_name, test_name) \
	NAMESPACE_GTEST_TEST_(namespace_name, test_case_name, test_name, ::testing::Test, ::testing::internal::GetTestTypeId())


/// Wait for a response from an aync event with a timeout
class ResponseWaiter
{
public:
	/// @brief Wait for an event to occur
	/// @param IsDone Functional (function or lambda) that return true when an event occurs
	/// @param TimeOutInSeconds Maximum time to wait in Seconds
	/// @param SleepTimeMs (Optional) Millseconds to sleep for while waiting (default 100 ms)
	/// @return True if event occured or False if timeout period expired
	static bool WaitFor(std::function<bool(void)> IsDone,
						const std::chrono::seconds& TimeOutInSeconds,
						const std::chrono::milliseconds SleepTimeMs = std::chrono::milliseconds(100))
	{
		using clock = std::chrono::system_clock;

		auto Start				= clock::now();
		clock::duration Elapsed = clock::now() - Start;

		const std::chrono::duration TimeOut = TimeOutInSeconds;

		while (!IsDone() && (Elapsed < TimeOut))
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(SleepTimeMs));
			Elapsed = clock::now() - Start;
		}

		// Returns True if done event occured or False if we timeout
		return IsDone();
	}
};

template <class ResultType> class ServiceResponseReceiver : public ResponseWaiter
{
public:
	ServiceResponseReceiver(csp::systems::EResultCode InExpectedResult = csp::systems::EResultCode::Success)
		: ExpectedResult(InExpectedResult), ResponseReceived(false)
	{
	}

	void OnResult(const ResultType& InResult)
	{
		if (InResult.GetResultCode() == csp::systems::EResultCode::InProgress)
			return;

		EXPECT_TRUE(InResult.GetResultCode() == ExpectedResult);
		ResponseReceived = true;
	}

	bool WaitForResult()
	{
		return WaitFor(
			[this]
			{
				return IsResponseReceived();
			},
			std::chrono::seconds(20));
	}

	bool IsResponseReceived() const
	{
		return ResponseReceived;
	}

private:
	csp::systems::EResultCode ExpectedResult;
	std::atomic<bool> ResponseReceived;
};

#if defined CSP_WINDOWS
	#define SPRINTF sprintf_s
#else
	#define SPRINTF sprintf
#endif

inline void PrintProgress(float Progress)
{
	unsigned int ProgressPercent = (unsigned int) (Progress + 0.5f);

	char ProgressString[256];
	SPRINTF(ProgressString, "Progress=%d%%\n", ProgressPercent);

	for (int i = 0; i < strlen(ProgressString); ++i)
		std::cerr << "\b";

	std::cerr << ProgressString;
}

inline void SetRandSeed()
{
	std::srand(static_cast<unsigned int>(std::time(nullptr)));
}
// This function creates a unique string by randomly selecting a values from a epoch time stamp and random values from a string
inline std::string GetUniqueString(int Length = 16)
{
	std::string str;
	const std::string Characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	const auto Epoch = std::to_string(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());

	for (int i = 0; i < Length / 2; i++)
	{
		int RandomNumber = rand();
		str += Epoch[RandomNumber % Epoch.length()];
		str += Characters[RandomNumber % Characters.length()];
	}

	return str;
}

inline void LogFatal(std::string Message)
{
	std::cerr << "[ ERROR    ] " << Message << std::endl;
	exit(1);
}

inline void InitialiseFoundationWithUserAgentInfo(const csp::common::String& EndpointRootURI)
{
	csp::CSPFoundation::Initialise(EndpointRootURI, "OKO_TESTS");

	csp::ClientUserAgent ClientHeaderInfo;
	ClientHeaderInfo.CSPVersion		   = csp::CSPFoundation::GetVersion();
	ClientHeaderInfo.ClientOS		   = "CPPTestsOS";
	ClientHeaderInfo.ClientSKU		   = TESTS_CLIENT_SKU;
	ClientHeaderInfo.ClientVersion	   = csp::CSPFoundation::GetVersion();
	ClientHeaderInfo.ClientEnvironment = "ODev";
	ClientHeaderInfo.CHSEnvironment	   = "oDev";

	csp::CSPFoundation::SetClientUserAgentInfo(ClientHeaderInfo);
}
