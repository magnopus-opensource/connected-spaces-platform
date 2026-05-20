/*
 * Copyright 2025 Magnopus LLC

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

#ifndef CSP_WASM

#include "CSP/CSPFoundation.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "Common/Web/HttpAuth.h"
#include "PlatformTestUtils.h"
#include "TestHelpers.h"

#include "gtest/gtest.h"
#include <chrono>
#include <future>
#include <thread>

using namespace std::chrono_literals;

namespace
{

/// Helper: Calls CSPFoundation::Shutdown() and asserts it completes within the given timeout.
/// If shutdown hangs (e.g. due to mutex destruction while threads are still active), the test will fail
/// rather than blocking the test runner indefinitely.
void AssertShutdownCompletesWithinTimeout(std::chrono::seconds Timeout = std::chrono::seconds(5))
{
    auto ShutdownFuture = std::async(std::launch::async, [] { return csp::CSPFoundation::Shutdown(); });

    ASSERT_TRUE(ShutdownFuture.wait_for(Timeout) == std::future_status::ready)
        << "CSPFoundation::Shutdown() hung — possible mutex destruction while background threads still active";

    EXPECT_TRUE(ShutdownFuture.get());
}

} // namespace

/// @brief Tests that WebClient's RequestsMutex remains valid when ThreadPool workers are actively
/// executing HTTP requests during shutdown.
///
/// Regression test for: "mutex lock failed: Invalid argument" crash in Unity editor.
/// The crash occurred because WebClient was deleted while its ThreadPool workers still held
/// references to RequestsMutex. The fix intentionally leaks WebClient on shutdown so that
/// its mutexes remain valid for any in-flight background threads.
CSP_INTERNAL_TEST(CSPEngine, ShutdownTests, ShutdownWithActiveHttpThreadsDoesNotCrash)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();

    // Log in to get a valid auth context so HTTP requests will actually be dispatched
    // to the ThreadPool (not rejected at the auth layer).
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Fire multiple HTTP requests to ensure ThreadPool workers are actively executing
    // and holding RequestsMutex references. We intentionally do NOT wait for responses.
    for (int i = 0; i < 4; ++i)
    {
        UserSystem->GetProfileByUserId(UserId, [](const csp::systems::ProfileResult& /*Result*/) {
            // Response may or may not arrive before shutdown — that's fine.
        });
    }

    // Immediately shut down while HTTP requests are in-flight.
    // Without the fix, this would crash with "mutex lock failed: Invalid argument"
    // because deleting WebClient destroys RequestsMutex while workers still reference it.
    AssertShutdownCompletesWithinTimeout();
}

/// @brief Tests that MultiplayerConnection's internal mutexes (SignalR's connection_impl::m_stop_lock
/// etc.) remain valid when the SignalR detached scheduler thread is still running during shutdown.
///
/// Regression test for the same "mutex lock failed: Invalid argument" crash.
/// SignalR's scheduler creates a detached thread that outlives object destruction.
/// The fix intentionally leaks MultiplayerConnection so its mutexes remain valid.
CSP_INTERNAL_TEST(CSPEngine, ShutdownTests, ShutdownWithActiveMultiplayerThreadsDoesNotCrash)
{
    InitialiseFoundationWithUserAgentInfo(EndpointBaseURI());

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();

    // Log in to get valid credentials for the multiplayer connection.
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Start the multiplayer connection. This spawns SignalR's detached scheduler thread
    // which holds mutex references on MultiplayerConnection internals.
    auto* Connection = SystemsManager.GetMultiplayerConnection();

    std::promise<csp::multiplayer::ErrorCode> ConnectPromise;
    Connection->Connect(
        [&ConnectPromise](csp::multiplayer::ErrorCode Error) { ConnectPromise.set_value(Error); },
        csp::CSPFoundation::GetEndpoints().MultiplayerConnection.GetURI(),
        csp::web::HttpAuth::GetAccessToken(),
        csp::CSPFoundation::GetDeviceId());

    // Wait for connect to complete (or timeout after 10s).
    auto ConnectFuture = ConnectPromise.get_future();
    if (ConnectFuture.wait_for(10s) == std::future_status::ready)
    {
        // Connection may succeed or fail depending on environment — either is fine.
        // The important thing is that the SignalR scheduler thread is now running.
        auto ErrorCode = ConnectFuture.get();
        (void)ErrorCode;
    }

    // Immediately shut down WITHOUT disconnecting first.
    // Without the fix, this would crash with "mutex lock failed: Invalid argument"
    // because deleting MultiplayerConnection destroys SignalR's internal mutexes
    // while the detached scheduler thread still references them.
    AssertShutdownCompletesWithinTimeout();
}

#endif // CSP_WASM



