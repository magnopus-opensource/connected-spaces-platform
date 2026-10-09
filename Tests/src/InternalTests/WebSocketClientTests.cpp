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

#include "../PublicAPITests/UserSystemTestHelpers.h"
#include "CSP/CSPFoundation.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "Multiplayer/SignalR/POCOSignalRClient/POCOSignalRClient.h"
#include "PlatformTestUtils.h"
#include "Poco/Exception.h"
#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPRequestHandlerFactory.h"
#include "Poco/Net/HTTPServer.h"
#include "Poco/Net/HTTPServerParams.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Net/NetException.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/Net/SocketAddress.h"
#include "Poco/Net/WebSocket.h"
#include "TestHelpers.h"

#include "gtest/gtest.h"

#include <atomic>
#include <future>
#include <thread>
#include <vector>

using namespace csp::multiplayer;

// The following is in service of the regression test for OB-5497.
// We have defined a local WebSocket server so that we can test Send()/Stop() concurrency issues without relying on live services.
namespace
{

// Handles requests, draining frames that arrive until the connection is closed.
// The socket is kept alive for CSPWebSocketClientPOCO::Send() to write into while the call to Stop() is racing it.
class RaceTestWebSocketRequestHandler : public Poco::Net::HTTPRequestHandler
{
public:
    // This roughly mirrors the logic in CSPWebSocketClientPOCO::ReceiveThreadFunc()
    void handleRequest(Poco::Net::HTTPServerRequest& Request, Poco::Net::HTTPServerResponse& Response) override
    {
        try
        {
            Poco::Net::WebSocket Socket(Request, Response);
            Socket.setReceiveTimeout(Poco::Timespan(1, 0));

            char Buffer[4096];

            for (;;)
            {
                int Flags = 0;
                int Received = 0;

                try
                {
                    Received = Socket.receiveFrame(Buffer, sizeof(Buffer), Flags);
                }
                catch (const Poco::TimeoutException&)
                {
                    // Nothing to read right now. The client may still be about to send.
                    continue;
                }

                if (Received == 0 || (Flags & Poco::Net::WebSocket::FRAME_OP_BITMASK) == Poco::Net::WebSocket::FRAME_OP_CLOSE)
                {
                    break;
                }
            }
        }
        catch (const Poco::Exception&)
        {
            // Expected once the client's Stop() tears the connection down mid-test.
        }
    }
};

class RaceTestRequestHandlerFactory : public Poco::Net::HTTPRequestHandlerFactory
{
public:
    Poco::Net::HTTPRequestHandler* createRequestHandler(const Poco::Net::HTTPServerRequest&) override
    {
        return new RaceTestWebSocketRequestHandler();
    }
};

// A loopback-only WebSocket server that drains incoming data via the request handler.
class LocalWebSocketTestServer
{
public:
    LocalWebSocketTestServer()
        : Socket(Poco::Net::SocketAddress("127.0.0.1", 0))
        , Server(new RaceTestRequestHandlerFactory(), Socket, new Poco::Net::HTTPServerParams())
    {
        Server.start();
    }

    ~LocalWebSocketTestServer() { Server.stop(); }

    unsigned short Port() const { return Socket.address().port(); }

private:
    Poco::Net::ServerSocket Socket;
    Poco::Net::HTTPServer Server;
};

} // namespace

// Regression test for OB-5497: Send() racing Stop() in CSPWebSocketClientPOCO.
//
// The bug was caused by connection_impl::send() only checking that the connection state is 'connected' before calling Send(). That state isn't set to
// 'disconnected' until Stop() has returned (via the m_close_callback, which is set in `connection_impl::start_transport`), so a Send() on one thread
// could pass the check while Stop() on another thread was closing and deleting the PocoWebSocket. Send() could then use a closed, deleted or null
// socket. Separately, it was observed in the Unreal client that concurrent Send() calls could interleave SSL_write calls on the same connection,
// which corrupted the SSL session ("ssl3_write_bytes: bad length").
//
// In production, Send() can be called concurrently from the main thread (e.g. ScopeLeadershipManager heartbeats and entity updates via
// CSPFoundation::Tick()), from the ReceiveThread (Invoke continuations) and from the SignalR scheduler (keep-alive pings). Stop() can be
// called from the ReceiveThread after a receive error, or from the SignalR scheduler when the server timeout elapses.
//
// This test performs multiple iterations, in which it connects a client to a local loopback WebSocket server, before several threads call Send() in a
// loop. Once a minimum number of sends have succeeded, Stop() is called while those sends are still in progress. We continue sending for a short
// period after Stop() has returned.
//
// The test passes if:
//  - No Error/Fatal messages are logged. Before the fix, a Send() that lost the race logged "Error: Failed to send data to socket.".
//  - No Send() that started after Stop() returned reports success.
//  - Stop() reports success.
//
// Stop() is called from the test thread, so it takes the join() branch, the same branch as a Stop() triggered by the server timeout.
// The detach() branch, where Stop() is called on the ReceiveThread, shares the same locked StopFlag and close/delete steps that race with
// Send(), so the branch taken doesn't affect what this test checks.
CSP_INTERNAL_TEST(CSPEngine, WebSocketClientTests, SendStopRaceConditionRegressionTest)
{
    csp::common::LogSystem LogSystem;

    // Prior to the fix, a Send() racing Stop() would log "Error: Failed to send data to socket." (or crash).
    // With the fix, a Send() that loses the race exits quietly, so any Error/Fatal log from the client is treated as a failure.
    std::atomic_int ErrorLogCount { 0 };
    LogSystem.SetSystemLevel(csp::common::LogLevel::Error);
    LogSystem.SetLogCallback([&ErrorLogCount](csp::common::LogLevel, const csp::common::String&) { ++ErrorLogCount; });

    // Construct a loopback-only WebSocket server that drains incoming data via the request handler.
    // The same server port is used by the client defined in each iteration below.
    LocalWebSocketTestServer Server;

    const std::string Uri = "http://127.0.0.1:" + std::to_string(Server.Port()) + "/";

    constexpr int Iterations = 10;
    constexpr int SenderThreadCount = 4;
    constexpr int MinSuccessesBeforeStop = 50;
    constexpr int MinSendsAfterStop = 50;
    constexpr std::chrono::milliseconds WaitTimeout { 5000 };

    // Spins until Predicate returns true or Timeout elapses. Returns whether Predicate was satisfied.
    const auto WaitUntil = [](const auto& Predicate, std::chrono::milliseconds Timeout)
    {
        const auto Deadline = std::chrono::steady_clock::now() + Timeout;

        while (!Predicate())
        {
            if (std::chrono::steady_clock::now() >= Deadline)
            {
                return false;
            }

            std::this_thread::yield();
        }

        return true;
    };

    // Loop over the connect and teardown cycle multiple times to increase the likelihood of encountering an issue.
    for (int Iteration = 0; Iteration < Iterations; ++Iteration)
    {
        CSPWebSocketClientPOCO Client(Uri, "", "", LogSystem);

        // Start client and wait for the handshake to complete suc
        std::promise<bool> StartedPromise;
        Client.Start(Uri, [&StartedPromise](bool Result) { StartedPromise.set_value(Result); });
        ASSERT_TRUE(StartedPromise.get_future().get());

        std::atomic_bool KeepSending { true };
        std::atomic_bool StopReturned { false };
        std::atomic_int SuccessCount { 0 };
        std::atomic_int SendsAfterStop { 0 };
        std::atomic_int SuccessesAfterStop { 0 };
        std::vector<std::thread> SenderThreads;

        // Launch multiple threads which repeatedly call Send() with a 512 byte payload while KeepSending == true
        for (int ThreadIndex = 0; ThreadIndex < SenderThreadCount; ++ThreadIndex)
        {
            SenderThreads.emplace_back(
                [&]()
                {
                    const std::string Payload(512, 'x');

                    while (KeepSending)
                    {
                        // Set before calling Send(), rather than inside the callback. Send() releases its lock before invoking the callback,
                        // so a Send() which completed before Stop() can invoke its callback after Stop() has returned.
                        const bool CalledAfterStop = StopReturned;

                        Client.Send(Payload,
                            [&, CalledAfterStop](bool Result)
                            {
                                if (!Result)
                                {
                                    return;
                                }

                                ++SuccessCount;

                                if (CalledAfterStop)
                                {
                                    ++SuccessesAfterStop;
                                }
                            });

                        if (CalledAfterStop)
                        {
                            ++SendsAfterStop;
                        }
                    }
                });
        }

        // Wait until sends are succeeding, so we know Stop() will be racing in-flight Send() calls.
        const bool SendsWereInFlight = WaitUntil([&]() { return SuccessCount >= MinSuccessesBeforeStop; }, WaitTimeout);

        // Call Stop() and block until it completes, while the sender threads are still calling Send().
        // In production, Stop() is usually called on the ReceiveThread (from websocket_transport::receive_loop() after a receive error),
        // which takes the detach() branch, whereas here it is called from the test thread and takes the join() branch.
        // That difference doesn't matter for this test: both branches share the parts of Stop() that race with Send(), ie setting
        // the StopFlag, then closing and deleting the PocoWebSocket under PocoWebSocketMutex. The race being tested is between those steps.
        // Send() does call from other threads, such as ScopeLeadershipManager::SendHeartbeatIfElectedScopeLeader() or entity creation via
        // CSPFoundation::Tick() on the main thread.
        std::promise<bool> StoppedPromise;
        Client.Stop([&StoppedPromise](bool Result) { StoppedPromise.set_value(Result); });
        const bool StopResult = StoppedPromise.get_future().get();
        StopReturned = true;

        // Keep sending after Stop() has returned to exercise the stopped path. Prior to the fix, this is where Send() would use a deleted
        // PocoWebSocket.
        const bool SentAfterStop = WaitUntil([&]() { return SendsAfterStop >= MinSendsAfterStop; }, WaitTimeout);

        KeepSending = false;

        for (std::thread& SenderThread : SenderThreads)
        {
            SenderThread.join();
        }

        // Assertions are made only after the sender threads have been joined, as a failed ASSERT would otherwise destroy joinable threads.
        ASSERT_TRUE(SendsWereInFlight) << "Timed out waiting for Send() to succeed before calling Stop()";
        EXPECT_TRUE(StopResult);
        ASSERT_TRUE(SentAfterStop) << "Timed out waiting for Send() calls after Stop() returned";
        EXPECT_EQ(SuccessesAfterStop, 0) << "Send() reported success after Stop() had returned";
        EXPECT_EQ(ErrorLogCount, 0) << "The client logged an error while Send() was racing Stop()";
    }
}

// The WebSocketClientTests will be reviewed as part of OF-1532.

CSP_INTERNAL_TEST(CSPEngine, WebSocketClientTests, SignalRClientStartStopTest)
{
    // Initialise
    InitialiseFoundation();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Start
    auto* WebSocket = WebSocketStart(csp::CSPFoundation::GetEndpoints().MultiplayerConnection.GetURI(), csp::web::HttpAuth::GetAccessToken().c_str(),
        csp::CSPFoundation::GetDeviceId());

    // Stop
    WebSocketStop(WebSocket);

    // Logout
    LogOut(UserSystem);

    csp::CSPFoundation::Shutdown();
}

CSP_INTERNAL_TEST(CSPEngine, WebSocketClientTests, SignalRClientSendTest)
{
    // Initialise
    InitialiseFoundation();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Start
    auto* WebSocket = WebSocketStart(csp::CSPFoundation::GetEndpoints().MultiplayerConnection.GetURI(), csp::web::HttpAuth::GetAccessToken().c_str(),
        csp::CSPFoundation::GetDeviceId());

    // Send
    WebSocketSend(WebSocket, "test");

    // Stop
    WebSocketStop(WebSocket);

    // Logout
    LogOut(UserSystem);

    csp::CSPFoundation::Shutdown();
}

CSP_INTERNAL_TEST(CSPEngine, WebSocketClientTests, SignalRClientSendReceiveTest)
{
    // Initialise
    InitialiseFoundation();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Start
    auto* WebSocket = WebSocketStart(csp::CSPFoundation::GetEndpoints().MultiplayerConnection.GetURI(), csp::web::HttpAuth::GetAccessToken().c_str(),
        csp::CSPFoundation::GetDeviceId());

    // Receive
    WebSocketSendReceive(WebSocket);

    // Stop
    WebSocketStop(WebSocket);

    // Logout
    LogOut(UserSystem);

    csp::CSPFoundation::Shutdown();
}

/*
 * These tests test the POCO client specifically.
 * The motive was that we added the ability for the POCO client to point to localhost in order
 * to allow local testing, so there's logic to test there, mostly around port extraction.
 */

CSP_INTERNAL_TEST(CSPEngine, WebSocketClientTests, RegularMultiplayerServiceURI)
{
    const csp::EndpointURIs Endpoints = csp::CSPFoundation::CreateEndpointsFromRoot("https://ogs.magnopus-dev.cloud");
    ASSERT_EQ(Endpoints.MultiplayerConnection.GetURI(), "https://ogs-multiplayer.magnopus-dev.cloud/mag-multiplayer/hubs/v1/multiplayer");

    CSPWebSocketClientPOCO::ParsedURIInfo ParsedURI
        = CSPWebSocketClientPOCO::ParseMultiplayerServiceUriEndPoint(Endpoints.MultiplayerConnection.GetURI().c_str());

    EXPECT_EQ(ParsedURI.Protocol, "https");
    EXPECT_EQ(ParsedURI.Domain, "ogs-multiplayer.magnopus-dev.cloud");
    EXPECT_EQ(ParsedURI.Path, "/mag-multiplayer/hubs/v1/multiplayer");
    EXPECT_EQ(ParsedURI.Port, 443);
    EXPECT_EQ(ParsedURI.Endpoint, "https://ogs-multiplayer.magnopus-dev.cloud/mag-multiplayer/hubs/v1/multiplayer");
}

CSP_INTERNAL_TEST(CSPEngine, WebSocketClientTests, LocalMultiplayerServiceURI)
{
    const csp::EndpointURIs Endpoints = csp::CSPFoundation::CreateEndpointsFromRoot("https://localhost:8081");
    ASSERT_EQ(Endpoints.MultiplayerConnection.GetURI(), "https://localhost:8081/mag-multiplayer/hubs/v1/multiplayer");

    CSPWebSocketClientPOCO::ParsedURIInfo ParsedURI
        = CSPWebSocketClientPOCO::ParseMultiplayerServiceUriEndPoint(Endpoints.MultiplayerConnection.GetURI().c_str());

    EXPECT_EQ(ParsedURI.Protocol, "https");
    EXPECT_EQ(ParsedURI.Domain, "localhost");
    EXPECT_EQ(ParsedURI.Path, "/mag-multiplayer/hubs/v1/multiplayer");
    EXPECT_EQ(ParsedURI.Port, 8081);
    EXPECT_EQ(ParsedURI.Endpoint, "https://localhost:8081/mag-multiplayer/hubs/v1/multiplayer");
}

CSP_INTERNAL_TEST(CSPEngine, WebSocketClientTests, LocalMultiplayerServiceURIHttp)
{
    const csp::EndpointURIs Endpoints = csp::CSPFoundation::CreateEndpointsFromRoot("http://localhost");
    ASSERT_EQ(Endpoints.MultiplayerConnection.GetURI(), "http://localhost/mag-multiplayer/hubs/v1/multiplayer");

    CSPWebSocketClientPOCO::ParsedURIInfo ParsedURI
        = CSPWebSocketClientPOCO::ParseMultiplayerServiceUriEndPoint(Endpoints.MultiplayerConnection.GetURI().c_str());

    EXPECT_EQ(ParsedURI.Protocol, "http");
    EXPECT_EQ(ParsedURI.Domain, "localhost");
    EXPECT_EQ(ParsedURI.Path, "/mag-multiplayer/hubs/v1/multiplayer");
    EXPECT_EQ(ParsedURI.Port, 80);
    EXPECT_EQ(ParsedURI.Endpoint, "http://localhost/mag-multiplayer/hubs/v1/multiplayer");
}

CSP_INTERNAL_TEST(CSPEngine, WebSocketClientTests, LocalVariantMultiplayerServiceURI)
{
    const csp::EndpointURIs Endpoints = csp::CSPFoundation::CreateEndpointsFromRoot("https://127.0.0.1:8081");
    ASSERT_EQ(Endpoints.MultiplayerConnection.GetURI(), "https://127.0.0.1:8081/mag-multiplayer/hubs/v1/multiplayer");

    CSPWebSocketClientPOCO::ParsedURIInfo ParsedURI
        = CSPWebSocketClientPOCO::ParseMultiplayerServiceUriEndPoint(Endpoints.MultiplayerConnection.GetURI().c_str());

    EXPECT_EQ(ParsedURI.Protocol, "https");
    EXPECT_EQ(ParsedURI.Domain, "127.0.0.1");
    EXPECT_EQ(ParsedURI.Path, "/mag-multiplayer/hubs/v1/multiplayer");
    EXPECT_EQ(ParsedURI.Port, 8081);
    EXPECT_EQ(ParsedURI.Endpoint, "https://127.0.0.1:8081/mag-multiplayer/hubs/v1/multiplayer");
}

CSP_INTERNAL_TEST(CSPEngine, WebSocketClientTests, LocalMultiplayerServiceURINoScheme)
{
    const csp::EndpointURIs Endpoints = csp::CSPFoundation::CreateEndpointsFromRoot("localhost:8081");
    ASSERT_EQ(Endpoints.MultiplayerConnection.GetURI(), "localhost:8081/mag-multiplayer/hubs/v1/multiplayer");

    EXPECT_THROW(CSPWebSocketClientPOCO::ParseMultiplayerServiceUriEndPoint(Endpoints.MultiplayerConnection.GetURI().c_str()), std::runtime_error);
}

CSP_INTERNAL_TEST(CSPEngine, WebSocketClientTests, LocalNoPortMultiplayerServiceURI)
{
    const csp::EndpointURIs Endpoints = csp::CSPFoundation::CreateEndpointsFromRoot("https://localhost");
    ASSERT_EQ(Endpoints.MultiplayerConnection.GetURI(), "https://localhost/mag-multiplayer/hubs/v1/multiplayer");

    CSPWebSocketClientPOCO::ParsedURIInfo ParsedURI
        = CSPWebSocketClientPOCO::ParseMultiplayerServiceUriEndPoint(Endpoints.MultiplayerConnection.GetURI().c_str());

    EXPECT_EQ(ParsedURI.Protocol, "https");
    EXPECT_EQ(ParsedURI.Domain, "localhost");
    EXPECT_EQ(ParsedURI.Path, "/mag-multiplayer/hubs/v1/multiplayer");
    EXPECT_EQ(ParsedURI.Port, 443);
    EXPECT_EQ(ParsedURI.Endpoint, "https://localhost/mag-multiplayer/hubs/v1/multiplayer");
}

CSP_INTERNAL_TEST(CSPEngine, WebSocketClientTests, LocalMalformedMultiplayerServiceURI)
{
    const csp::EndpointURIs Endpoints = csp::CSPFoundation::CreateEndpointsFromRoot("https://localhost:notanumber");
    ASSERT_EQ(Endpoints.MultiplayerConnection.GetURI(), "https://localhost:notanumber/mag-multiplayer/hubs/v1/multiplayer");

    EXPECT_THROW(CSPWebSocketClientPOCO::ParseMultiplayerServiceUriEndPoint(Endpoints.MultiplayerConnection.GetURI().c_str()), Poco::SyntaxException);
}
