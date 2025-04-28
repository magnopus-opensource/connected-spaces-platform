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
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "Multiplayer/SignalR/POCOSignalRClient/POCOSignalRClient.h"
#include "PlatformTestUtils.h"
#include "Poco/Exception.h"
#include "TestHelpers.h"

#include "gtest/gtest.h"

using namespace csp::multiplayer;

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
    auto* WebSocket = WebSocketStart(csp::CSPFoundation::GetEndpoints().MultiplayerServiceURI);

    // Stop
    WebSocketStop(WebSocket);

    // Logout
    LogOut(UserSystem);
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
    auto* WebSocket = WebSocketStart(csp::CSPFoundation::GetEndpoints().MultiplayerServiceURI);

    // Send
    WebSocketSend(WebSocket, "test");

    // Stop
    WebSocketStop(WebSocket);

    // Logout
    LogOut(UserSystem);
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
    auto* WebSocket = WebSocketStart(csp::CSPFoundation::GetEndpoints().MultiplayerServiceURI);

    // Receive
    WebSocketSendReceive(WebSocket);

    // Stop
    WebSocketStop(WebSocket);

    // Logout
    LogOut(UserSystem);
}

/*
 * These tests test the POCO client specifically.
 * The motive was that we added the ability for the POCO client to point to localhost in order
 * to allow local testing, so there's logic to test there, mostly around port extraction.
 */

CSP_INTERNAL_TEST(CSPEngine, WebSocketClientTests, RegularMultiplayerServiceURI)
{
    const csp::EndpointURIs Endpoints = csp::CSPFoundation::CreateEndpointsFromRoot("https://ogs-internal.magnopus-dev.cloud");
    ASSERT_EQ(Endpoints.MultiplayerServiceURI, "https://ogs-multiplayer-internal.magnopus-dev.cloud/mag-multiplayer/hubs/v1/multiplayer");

    CSPWebSocketClientPOCO::ParsedURIInfo ParsedURI
        = CSPWebSocketClientPOCO::ParseMultiplayerServiceUriEndPoint(Endpoints.MultiplayerServiceURI.c_str());

    EXPECT_EQ(ParsedURI.Protocol, "https");
    EXPECT_EQ(ParsedURI.Domain, "ogs-multiplayer-internal.magnopus-dev.cloud");
    EXPECT_EQ(ParsedURI.Path, "/mag-multiplayer/hubs/v1/multiplayer");
    EXPECT_EQ(ParsedURI.Port, 443);
    EXPECT_EQ(ParsedURI.Endpoint, "https://ogs-multiplayer-internal.magnopus-dev.cloud/mag-multiplayer/hubs/v1/multiplayer");
}

CSP_INTERNAL_TEST(CSPEngine, WebSocketClientTests, LocalMultiplayerServiceURI)
{
    const csp::EndpointURIs Endpoints = csp::CSPFoundation::CreateEndpointsFromRoot("https://localhost:8081");
    ASSERT_EQ(Endpoints.MultiplayerServiceURI, "https://localhost:8081/mag-multiplayer/hubs/v1/multiplayer");

    CSPWebSocketClientPOCO::ParsedURIInfo ParsedURI
        = CSPWebSocketClientPOCO::ParseMultiplayerServiceUriEndPoint(Endpoints.MultiplayerServiceURI.c_str());

    EXPECT_EQ(ParsedURI.Protocol, "https");
    EXPECT_EQ(ParsedURI.Domain, "localhost");
    EXPECT_EQ(ParsedURI.Path, "/mag-multiplayer/hubs/v1/multiplayer");
    EXPECT_EQ(ParsedURI.Port, 8081);
    EXPECT_EQ(ParsedURI.Endpoint, "https://localhost:8081/mag-multiplayer/hubs/v1/multiplayer");
}

CSP_INTERNAL_TEST(CSPEngine, WebSocketClientTests, LocalMultiplayerServiceURIHttp)
{
    const csp::EndpointURIs Endpoints = csp::CSPFoundation::CreateEndpointsFromRoot("http://localhost");
    ASSERT_EQ(Endpoints.MultiplayerServiceURI, "http://localhost/mag-multiplayer/hubs/v1/multiplayer");

    CSPWebSocketClientPOCO::ParsedURIInfo ParsedURI
        = CSPWebSocketClientPOCO::ParseMultiplayerServiceUriEndPoint(Endpoints.MultiplayerServiceURI.c_str());

    EXPECT_EQ(ParsedURI.Protocol, "http");
    EXPECT_EQ(ParsedURI.Domain, "localhost");
    EXPECT_EQ(ParsedURI.Path, "/mag-multiplayer/hubs/v1/multiplayer");
    EXPECT_EQ(ParsedURI.Port, 80);
    EXPECT_EQ(ParsedURI.Endpoint, "http://localhost/mag-multiplayer/hubs/v1/multiplayer");
}

CSP_INTERNAL_TEST(CSPEngine, WebSocketClientTests, LocalVariantMultiplayerServiceURI)
{
    const csp::EndpointURIs Endpoints = csp::CSPFoundation::CreateEndpointsFromRoot("https://127.0.0.1:8081");
    ASSERT_EQ(Endpoints.MultiplayerServiceURI, "https://127.0.0.1:8081/mag-multiplayer/hubs/v1/multiplayer");

    CSPWebSocketClientPOCO::ParsedURIInfo ParsedURI
        = CSPWebSocketClientPOCO::ParseMultiplayerServiceUriEndPoint(Endpoints.MultiplayerServiceURI.c_str());

    EXPECT_EQ(ParsedURI.Protocol, "https");
    EXPECT_EQ(ParsedURI.Domain, "127.0.0.1");
    EXPECT_EQ(ParsedURI.Path, "/mag-multiplayer/hubs/v1/multiplayer");
    EXPECT_EQ(ParsedURI.Port, 8081);
    EXPECT_EQ(ParsedURI.Endpoint, "https://127.0.0.1:8081/mag-multiplayer/hubs/v1/multiplayer");
}

CSP_INTERNAL_TEST(CSPEngine, WebSocketClientTests, LocalMultiplayerServiceURINoScheme)
{
    const csp::EndpointURIs Endpoints = csp::CSPFoundation::CreateEndpointsFromRoot("localhost:8081");
    ASSERT_EQ(Endpoints.MultiplayerServiceURI, "localhost:8081/mag-multiplayer/hubs/v1/multiplayer");

    EXPECT_THROW(CSPWebSocketClientPOCO::ParseMultiplayerServiceUriEndPoint(Endpoints.MultiplayerServiceURI.c_str()), std::runtime_error);
}

CSP_INTERNAL_TEST(CSPEngine, WebSocketClientTests, LocalNoPortMultiplayerServiceURI)
{
    const csp::EndpointURIs Endpoints = csp::CSPFoundation::CreateEndpointsFromRoot("https://localhost");
    ASSERT_EQ(Endpoints.MultiplayerServiceURI, "https://localhost/mag-multiplayer/hubs/v1/multiplayer");

    CSPWebSocketClientPOCO::ParsedURIInfo ParsedURI
        = CSPWebSocketClientPOCO::ParseMultiplayerServiceUriEndPoint(Endpoints.MultiplayerServiceURI.c_str());

    EXPECT_EQ(ParsedURI.Protocol, "https");
    EXPECT_EQ(ParsedURI.Domain, "localhost");
    EXPECT_EQ(ParsedURI.Path, "/mag-multiplayer/hubs/v1/multiplayer");
    EXPECT_EQ(ParsedURI.Port, 443);
    EXPECT_EQ(ParsedURI.Endpoint, "https://localhost/mag-multiplayer/hubs/v1/multiplayer");
}

CSP_INTERNAL_TEST(CSPEngine, WebSocketClientTests, LocalMalformedMultiplayerServiceURI)
{
    const csp::EndpointURIs Endpoints = csp::CSPFoundation::CreateEndpointsFromRoot("https://localhost:notanumber");
    ASSERT_EQ(Endpoints.MultiplayerServiceURI, "https://localhost:notanumber/mag-multiplayer/hubs/v1/multiplayer");

    EXPECT_THROW(CSPWebSocketClientPOCO::ParseMultiplayerServiceUriEndPoint(Endpoints.MultiplayerServiceURI.c_str()), Poco::SyntaxException);
}
