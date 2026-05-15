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

#include "Awaitable.h"
#include "CSP/Common/Interfaces/IAuthContext.h"
#include "CSP/CSPFoundation.h"
#include "CSP/Multiplayer/OnlineRealtimeEngine.h"
#include "CSP/Systems/WebService.h"
#include "PublicTestBase.h"

#include <chrono>
#include <functional>
#include <gtest/gtest.h>
#include <iostream>
#include <random>
#include <thread>
#include <future>
#include <filesystem>
#include <fstream>

using namespace std::chrono_literals;

inline const char* TESTS_CLIENT_SKU = "CPPTest";

// https://bitbucket.org/CadActive/workspace/snippets/GrBakB/macro-to-enable-namespaces-in-google-test

// A copy of GTEST_TEST_CLASS_NAME_, but with handling for namespace name.

#define NAMESPACE_GTEST_TEST_CLASS_NAME_(namespace_name, test_case_name, test_name) namespace_name##_##test_case_name##_##test_name##_Test

// A copy of GTEST_TEST_, but with handling for namespace name.

#define NAMESPACE_GTEST_TEST_(namespace_name, test_case_name, test_name, parent_class, parent_id)                                                    \
    class NAMESPACE_GTEST_TEST_CLASS_NAME_(namespace_name, test_case_name, test_name)                                                                \
        : public parent_class                                                                                                                        \
    {                                                                                                                                                \
    public:                                                                                                                                          \
        NAMESPACE_GTEST_TEST_CLASS_NAME_(namespace_name, test_case_name, test_name)() { }                                                            \
                                                                                                                                                     \
    private:                                                                                                                                         \
        virtual void TestBody();                                                                                                                     \
        static ::testing::TestInfo* const test_info_ GTEST_ATTRIBUTE_UNUSED_;                                                                        \
        GTEST_DISALLOW_COPY_AND_ASSIGN_(NAMESPACE_GTEST_TEST_CLASS_NAME_(namespace_name, test_case_name, test_name));                                \
    };                                                                                                                                               \
                                                                                                                                                     \
    ::testing::TestInfo* const NAMESPACE_GTEST_TEST_CLASS_NAME_(namespace_name, test_case_name, test_name)::test_info_                               \
        = ::testing::internal::MakeAndRegisterTestInfo(#namespace_name "." #test_case_name, #test_name, NULL,                                        \
            NULL, /* <-- Defines the test as "Namespace.Classname" */                                                                                \
            ::testing::internal::CodeLocation(__FILE__, __LINE__), (parent_id), parent_class::SetUpTestCase, parent_class::TearDownTestCase,         \
            new ::testing::internal::TestFactoryImpl<NAMESPACE_GTEST_TEST_CLASS_NAME_(namespace_name, test_case_name, test_name)>);                  \
    void NAMESPACE_GTEST_TEST_CLASS_NAME_(namespace_name, test_case_name, test_name)::TestBody()

// Custom macro for our unit tests so that we can declare the namespace needed in Visual Studio 2019 Test Explorer

#define CSP_PUBLIC_TEST(namespace_name, test_case_name, test_name)                                                                                   \
    NAMESPACE_GTEST_TEST_(namespace_name, test_case_name, test_name, ::PublicTestBase, ::testing::internal::GetTypeId<::PublicTestBase>())

#define CSP_PUBLIC_TEST_WITH_MOCKS(namespace_name, test_case_name, test_name)                                                                        \
    NAMESPACE_GTEST_TEST_(                                                                                                                           \
        namespace_name, test_case_name, test_name, ::PublicTestBaseWithMocks, ::testing::internal::GetTypeId<::PublicTestBaseWithMocks>())

#define CSP_INTERNAL_TEST(namespace_name, test_case_name, test_name)                                                                                 \
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
    static bool WaitFor(std::function<bool(void)> isDone, const std::chrono::seconds& timeOutInSeconds,
        const std::chrono::milliseconds sleepTimeMs = std::chrono::milliseconds(100))
    {
        using clock = std::chrono::system_clock;

        auto start = clock::now();
        clock::duration elapsed = clock::now() - start;

        const std::chrono::duration timeOut = timeOutInSeconds;

        while (!isDone() && (elapsed < timeOut))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepTimeMs));
            elapsed = clock::now() - start;
        }

        // Returns True if done event occured or False if we timeout
        return isDone();
    }
};

template <class ResultType> class ServiceResponseReceiver : public ResponseWaiter
{
public:
    ServiceResponseReceiver(csp::systems::EResultCode inExpectedResult = csp::systems::EResultCode::Success)
        : m_expectedResult(inExpectedResult)
        , m_responseReceived(false)
    {
    }

    void OnResult(const ResultType& inResult)
    {
        if (inResult.GetResultCode() == csp::systems::EResultCode::InProgress)
            return;

        EXPECT_TRUE(inResult.GetResultCode() == m_expectedResult);
        m_responseReceived = true;
    }

    bool WaitForResult()
    {
        return WaitFor([this] { return IsResponseReceived(); }, std::chrono::seconds(20));
    }

    bool IsResponseReceived() const { return m_responseReceived; }

private:
    csp::systems::EResultCode m_expectedResult;
    std::atomic<bool> m_responseReceived;
};

class TestAuthContext : public csp::common::IAuthContext
{
public:
    const csp::common::LoginState& GetLoginState() const override { return m_state; }
    void RefreshToken(std::function<void(bool)> success) override { success(true); }

private:
    csp::common::LoginState m_state;
};

#if defined CSP_WINDOWS
#define SPRINTF sprintf_s
#else
#define SPRINTF sprintf
#endif

inline void PrintProgress(float progress)
{
    unsigned int progressPercent = (unsigned int)(progress + 0.5f);

    char progressString[256];
    SPRINTF(progressString, "Progress=%d%%\n", progressPercent);

    for (size_t i = 0; i < strlen(progressString); ++i)
        std::cerr << "\b";

    std::cerr << progressString;
}

inline void SetRandSeed() { std::srand(static_cast<unsigned int>(std::time(nullptr))); }

inline double RandomUniformDouble()
{
    std::uniform_real_distribution<double> uniformDouble;

    // Seed using the current time.
    std::mt19937_64 rand;
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto currentNanoseconds = std::chrono::time_point_cast<std::chrono::nanoseconds>(currentTime);
    rand.seed(currentNanoseconds.time_since_epoch().count());
    return uniformDouble(rand);
}

inline double RandomRangeDouble(double min, double max)
{
    const double randomUniform = RandomUniformDouble();
    const double range = max - min;

    return (randomUniform * range) + min;
}

// This function creates a unique string by randomly selecting a values from a epoch time stamp and random values from a string
std::string GetUniqueString();

inline void LogFatal(std::string message)
{
    std::cerr << "[ ERROR    ] " << message << std::endl;
    exit(1);
}

inline const csp::ClientUserAgent& GetDefaultClientUserAgentInfo()
{
    static csp::ClientUserAgent clientHeaderInfo;
    clientHeaderInfo.CSPVersion = csp::CSPFoundation::GetVersion();
    clientHeaderInfo.ClientOS = "CPPTestsOS";
    clientHeaderInfo.ClientSKU = TESTS_CLIENT_SKU;
    clientHeaderInfo.ClientVersion = csp::CSPFoundation::GetVersion();
    clientHeaderInfo.ClientEnvironment = "ODev";
    clientHeaderInfo.CHSEnvironment = "oDev";
    return clientHeaderInfo;
}

inline void InitialiseFoundationWithUserAgentInfo(const csp::common::String& endpointRootUri, SignalRConnectionMock* signalRMock = nullptr, csp::web::WebClient* webClient = nullptr)
{
    csp::CSPFoundation::InitialiseWithInject(endpointRootUri, "OKO_TESTS", GetDefaultClientUserAgentInfo(), signalRMock, webClient, nullptr);
}

inline void InitialiseFoundationWithUserAgentInfoAndFeatureFlags(const csp::common::String& endpointRootUri,
    const csp::common::Optional<csp::common::Array<csp::FeatureFlag>>& featureFlags, SignalRConnectionMock* signalRMock = nullptr, csp::web::WebClient* webClient = nullptr)
{
    csp::CSPFoundation::InitialiseWithInject(endpointRootUri, "OKO_TESTS", GetDefaultClientUserAgentInfo(), signalRMock, webClient, featureFlags);
}

inline void WaitForCallback(bool& callbackCalled, int maxTextTimeSeconds = 20)
{
    // Wait for message
    auto start = std::chrono::steady_clock::now();
    auto current = std::chrono::steady_clock::now();
    int64_t testTime = 0;

    while (callbackCalled == false && testTime < maxTextTimeSeconds)
    {
        std::this_thread::sleep_for(50ms);

        current = std::chrono::steady_clock::now();
        testTime = std::chrono::duration_cast<std::chrono::seconds>(current - start).count();
    }

    if (callbackCalled == false)
    {
        printf("Test timed out - Callback wasn't called\n");
    }
}

template <typename T> inline bool WaitForFuture(const std::future<T>& future, int maxWaitTimeSeconds = 20)
{
    auto status = future.wait_for(std::chrono::seconds(maxWaitTimeSeconds));
    return status == std::future_status::ready;
}


inline void ProcessPendingIfOnline(csp::common::IRealtimeEngine& realtimeEngine)
{
    if (realtimeEngine.GetRealtimeEngineType() == csp::common::RealtimeEngineType::Online)
    {
        static_cast<csp::multiplayer::OnlineRealtimeEngine&>(realtimeEngine).ProcessPendingEntityOperations();
    }
}

inline void WaitForCallbackWithUpdate(bool& callbackCalled, csp::common::IRealtimeEngine* entitySystem, int maxTextTimeSeconds = 20)
{
    // Wait for message
    auto start = std::chrono::steady_clock::now();
    auto current = std::chrono::steady_clock::now();
    int64_t testTime = 0;

    // Call at least once
    ProcessPendingIfOnline(*entitySystem);

    while (callbackCalled == false && testTime < maxTextTimeSeconds)
    {
        ProcessPendingIfOnline(*entitySystem);

        std::this_thread::sleep_for(50ms);

        current = std::chrono::steady_clock::now();
        testTime = std::chrono::duration_cast<std::chrono::seconds>(current - start).count();
    }

    if (callbackCalled == false)
    {
        printf("Test timed out - Callback wasn't called\n");
    }
}

inline csp::multiplayer::SpaceEntity* CreateTestObject(csp::common::IRealtimeEngine* entitySystem, csp::common::String name = "Object")
{
    csp::multiplayer::SpaceTransform objectTransform { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
    auto [CreatedObject] = AWAIT(entitySystem, CreateEntity, name, objectTransform, csp::common::Optional<uint64_t> {});
    EXPECT_TRUE(CreatedObject != nullptr);
    return CreatedObject;
}

inline std::optional<std::vector<unsigned char>> OpenFile(const std::string& filePath)
{
    auto absoluteFilePath = std::filesystem::absolute(filePath);
    if (!std::filesystem::exists(absoluteFilePath))
    {
        return std::nullopt;
    }

    const auto fileSize = std::filesystem::file_size(absoluteFilePath);

    std::ifstream file(absoluteFilePath, std::ios::binary);
    if (!file)
    {
        return std::nullopt;
    }

    if (!std::filesystem::is_regular_file(absoluteFilePath))
    {
        return std::nullopt;
    }

    std::vector<unsigned char> fileData(static_cast<size_t>(fileSize));

    if (!file.read(reinterpret_cast<char*>(fileData.data()), static_cast<std::streamsize>(fileData.size())))
    {
        return std::nullopt;
    }
    
    return fileData;
}
