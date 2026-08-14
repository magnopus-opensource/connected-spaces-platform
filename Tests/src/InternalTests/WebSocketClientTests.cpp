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
// We have defined a local WebSocket server so that we can test Send()/Stop() concurrency issues without depending live services.
namespace
{

// Handles requests, draining frames that arrive until the connection is closed.
// The socket is kept alive for CSPWebSocketClientPOCO::Send to write into while the call to Stop is racing it.
class RaceTestWebSocketRequestHandler : public Poco::Net::HTTPRequestHandler
{
public:
    // This mimics the logic in CSPWebSocketClientPOCO::ReceiveThreadFunc()
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
                    // Nothing to read right now - the client may still be about to send. Keep waiting.
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

// The following is a regression test for OB-5497.
// The bug was caused by connection_impl::send() only checking 'connection_state == connected' before calling Send.
// - transport->send() > CSPWebsocketClient::send > CSPWebSocketClientPOCO::Send()
// However, ScopeLeadershipManager::SendHeartbeatIfElectedScopeLeader() and entity creation both use the main thread via CSPFoundation::Tick().
// The issue occurs when that connection state check in CSPWebSocketClientPOCO::Stop() passes while ReceiveThread is mid teardown. It is only after
// the call to CSPWebsocketClient::Stop() returns that the connection state is changed. This test replicates the behaviour by having multiple threads
// call Send in a loop and then calling Stop, which ultimately deletes and nulls the PocoWebSocket. Once Send/Stop are made to coordinate their access
// to PocoWebSocket, this test should pass cleanly and repeatably.
CSP_INTERNAL_TEST(CSPEngine, WebSocketClientTests, SendStopRaceConditionRegressionTest)
{
    csp::common::LogSystem LogSystem;
    // Construct a loopback-only WebSocket server that drains incoming data via the request handler.
    // The same server port is used by the client defined in each iteration below.
    LocalWebSocketTestServer Server;

    const std::string Uri = "http://127.0.0.1:" + std::to_string(Server.Port()) + "/";

    constexpr int Iterations = 10;
    constexpr int SenderThreadCount = 4;

    // Loop over the entire connect > race window > teardown cycle multiple times to increase the likelihood of encountering an issue.
    for (int Iteration = 0; Iteration < Iterations; ++Iteration)
    {
        CSPWebSocketClientPOCO Client(Uri, "", "", LogSystem);

        // Start client and wait for the handshake to complete successfully.
        std::promise<bool> StartedPromise;
        Client.Start(Uri, [&StartedPromise](bool Ok) { StartedPromise.set_value(Ok); });
        ASSERT_TRUE(StartedPromise.get_future().get());

        std::atomic_bool KeepSending { true };
        std::vector<std::thread> SenderThreads;

        // Launch multiple threads which repeatedly call Send() with a 512 byte payload while KeepSending == true
        for (int ThreadIndex = 0; ThreadIndex < SenderThreadCount; ++ThreadIndex)
        {
            SenderThreads.emplace_back(
                [&Client, &KeepSending]()
                {
                    const std::string Payload(512, 'x');

                    while (KeepSending)
                    {
                        // Prior to the fix for OB-5497, this Send() will hit the Assert in CSPWebSocketClientPOCO::Send() and throw an exception in
                        // the try/catch block that follows it. In release builds this assert will be compiled out.
                        // This results in the following error being logged, which matches what is being seen in the bug ticket logs:
                        // "Error: Failed to send data to socket."
                        Client.Send(Payload, [](bool) { });
                    }
                });
        }

        // Give the sending threads a moment to ensure calls are in-flight before calling Stop().
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

        // Call stop and block until it completes.
        // This is equivalent to the ReceiveThread calling Stop(), resulting in close() being called and the PocoWebSocket being deleted while
        // ScopeLeadershipManager::SendHeartbeatIfElectedScopeLeader or entity creation are calling Send on the main thread.
        std::promise<bool> StoppedPromise;
        Client.Stop([&StoppedPromise](bool Ok) { StoppedPromise.set_value(Ok); });
        StoppedPromise.get_future().get();

        // Now that Client Stop call has returned, signal threads to stop calling send and join them.
        KeepSending = false;

        // Prior to addressing the issues this is the window in which the unfixed Send() dereferences a freed or dangling PocoWebSocket.
        // The call to join is deliberate as the senders keep calling Send() for a little while after the pointer has already been deleted/nulled.
        for (std::thread& SenderThread : SenderThreads)
        {
            SenderThread.join();
        }
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
