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

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Start
    auto* webSocket = WebSocketStart(csp::CSPFoundation::GetEndpoints().MultiplayerConnection.GetURI(), csp::web::HttpAuth::GetAccessToken().c_str(),
        csp::CSPFoundation::GetDeviceId());

    // Stop
    WebSocketStop(webSocket);

    // Logout
    LogOut(userSystem);

    csp::CSPFoundation::Shutdown();
}

CSP_INTERNAL_TEST(CSPEngine, WebSocketClientTests, SignalRClientSendTest)
{
    // Initialise
    InitialiseFoundation();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Start
    auto* webSocket = WebSocketStart(csp::CSPFoundation::GetEndpoints().MultiplayerConnection.GetURI(), csp::web::HttpAuth::GetAccessToken().c_str(),
        csp::CSPFoundation::GetDeviceId());

    // Send
    WebSocketSend(webSocket, "test");

    // Stop
    WebSocketStop(webSocket);

    // Logout
    LogOut(userSystem);

    csp::CSPFoundation::Shutdown();
}

CSP_INTERNAL_TEST(CSPEngine, WebSocketClientTests, SignalRClientSendReceiveTest)
{
    // Initialise
    InitialiseFoundation();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Start
    auto* webSocket = WebSocketStart(csp::CSPFoundation::GetEndpoints().MultiplayerConnection.GetURI(), csp::web::HttpAuth::GetAccessToken().c_str(),
        csp::CSPFoundation::GetDeviceId());

    // Receive
    WebSocketSendReceive(webSocket);

    // Stop
    WebSocketStop(webSocket);

    // Logout
    LogOut(userSystem);

    csp::CSPFoundation::Shutdown();
}

/*
 * These tests test the POCO client specifically.
 * The motive was that we added the ability for the POCO client to point to localhost in order
 * to allow local testing, so there's logic to test there, mostly around port extraction.
 */

CSP_INTERNAL_TEST(CSPEngine, WebSocketClientTests, RegularMultiplayerServiceURI)
{
    const csp::EndpointURIs endpoints = csp::CSPFoundation::CreateEndpointsFromRoot("https://ogs-internal.magnopus-dev.cloud");
    ASSERT_EQ(endpoints.MultiplayerConnection.GetURI(), "https://ogs-multiplayer-internal.magnopus-dev.cloud/mag-multiplayer/hubs/v1/multiplayer");

    CSPWebSocketClientPOCO::ParsedURIInfo parsedUri
        = CSPWebSocketClientPOCO::ParseMultiplayerServiceUriEndPoint(endpoints.MultiplayerConnection.GetURI().c_str());

    EXPECT_EQ(parsedUri.Protocol, "https");
    EXPECT_EQ(parsedUri.Domain, "ogs-multiplayer-internal.magnopus-dev.cloud");
    EXPECT_EQ(parsedUri.Path, "/mag-multiplayer/hubs/v1/multiplayer");
    EXPECT_EQ(parsedUri.Port, 443);
    EXPECT_EQ(parsedUri.Endpoint, "https://ogs-multiplayer-internal.magnopus-dev.cloud/mag-multiplayer/hubs/v1/multiplayer");
}

CSP_INTERNAL_TEST(CSPEngine, WebSocketClientTests, LocalMultiplayerServiceURI)
{
    const csp::EndpointURIs endpoints = csp::CSPFoundation::CreateEndpointsFromRoot("https://localhost:8081");
    ASSERT_EQ(endpoints.MultiplayerConnection.GetURI(), "https://localhost:8081/mag-multiplayer/hubs/v1/multiplayer");

    CSPWebSocketClientPOCO::ParsedURIInfo parsedUri
        = CSPWebSocketClientPOCO::ParseMultiplayerServiceUriEndPoint(endpoints.MultiplayerConnection.GetURI().c_str());

    EXPECT_EQ(parsedUri.Protocol, "https");
    EXPECT_EQ(parsedUri.Domain, "localhost");
    EXPECT_EQ(parsedUri.Path, "/mag-multiplayer/hubs/v1/multiplayer");
    EXPECT_EQ(parsedUri.Port, 8081);
    EXPECT_EQ(parsedUri.Endpoint, "https://localhost:8081/mag-multiplayer/hubs/v1/multiplayer");
}

CSP_INTERNAL_TEST(CSPEngine, WebSocketClientTests, LocalMultiplayerServiceURIHttp)
{
    const csp::EndpointURIs endpoints = csp::CSPFoundation::CreateEndpointsFromRoot("http://localhost");
    ASSERT_EQ(endpoints.MultiplayerConnection.GetURI(), "http://localhost/mag-multiplayer/hubs/v1/multiplayer");

    CSPWebSocketClientPOCO::ParsedURIInfo parsedUri
        = CSPWebSocketClientPOCO::ParseMultiplayerServiceUriEndPoint(endpoints.MultiplayerConnection.GetURI().c_str());

    EXPECT_EQ(parsedUri.Protocol, "http");
    EXPECT_EQ(parsedUri.Domain, "localhost");
    EXPECT_EQ(parsedUri.Path, "/mag-multiplayer/hubs/v1/multiplayer");
    EXPECT_EQ(parsedUri.Port, 80);
    EXPECT_EQ(parsedUri.Endpoint, "http://localhost/mag-multiplayer/hubs/v1/multiplayer");
}

CSP_INTERNAL_TEST(CSPEngine, WebSocketClientTests, LocalVariantMultiplayerServiceURI)
{
    const csp::EndpointURIs endpoints = csp::CSPFoundation::CreateEndpointsFromRoot("https://127.0.0.1:8081");
    ASSERT_EQ(endpoints.MultiplayerConnection.GetURI(), "https://127.0.0.1:8081/mag-multiplayer/hubs/v1/multiplayer");

    CSPWebSocketClientPOCO::ParsedURIInfo parsedUri
        = CSPWebSocketClientPOCO::ParseMultiplayerServiceUriEndPoint(endpoints.MultiplayerConnection.GetURI().c_str());

    EXPECT_EQ(parsedUri.Protocol, "https");
    EXPECT_EQ(parsedUri.Domain, "127.0.0.1");
    EXPECT_EQ(parsedUri.Path, "/mag-multiplayer/hubs/v1/multiplayer");
    EXPECT_EQ(parsedUri.Port, 8081);
    EXPECT_EQ(parsedUri.Endpoint, "https://127.0.0.1:8081/mag-multiplayer/hubs/v1/multiplayer");
}

CSP_INTERNAL_TEST(CSPEngine, WebSocketClientTests, LocalMultiplayerServiceURINoScheme)
{
    const csp::EndpointURIs endpoints = csp::CSPFoundation::CreateEndpointsFromRoot("localhost:8081");
    ASSERT_EQ(endpoints.MultiplayerConnection.GetURI(), "localhost:8081/mag-multiplayer/hubs/v1/multiplayer");

    EXPECT_THROW(CSPWebSocketClientPOCO::ParseMultiplayerServiceUriEndPoint(endpoints.MultiplayerConnection.GetURI().c_str()), std::runtime_error);
}

CSP_INTERNAL_TEST(CSPEngine, WebSocketClientTests, LocalNoPortMultiplayerServiceURI)
{
    const csp::EndpointURIs endpoints = csp::CSPFoundation::CreateEndpointsFromRoot("https://localhost");
    ASSERT_EQ(endpoints.MultiplayerConnection.GetURI(), "https://localhost/mag-multiplayer/hubs/v1/multiplayer");

    CSPWebSocketClientPOCO::ParsedURIInfo parsedUri
        = CSPWebSocketClientPOCO::ParseMultiplayerServiceUriEndPoint(endpoints.MultiplayerConnection.GetURI().c_str());

    EXPECT_EQ(parsedUri.Protocol, "https");
    EXPECT_EQ(parsedUri.Domain, "localhost");
    EXPECT_EQ(parsedUri.Path, "/mag-multiplayer/hubs/v1/multiplayer");
    EXPECT_EQ(parsedUri.Port, 443);
    EXPECT_EQ(parsedUri.Endpoint, "https://localhost/mag-multiplayer/hubs/v1/multiplayer");
}

CSP_INTERNAL_TEST(CSPEngine, WebSocketClientTests, LocalMalformedMultiplayerServiceURI)
{
    const csp::EndpointURIs endpoints = csp::CSPFoundation::CreateEndpointsFromRoot("https://localhost:notanumber");
    ASSERT_EQ(endpoints.MultiplayerConnection.GetURI(), "https://localhost:notanumber/mag-multiplayer/hubs/v1/multiplayer");

    EXPECT_THROW(CSPWebSocketClientPOCO::ParseMultiplayerServiceUriEndPoint(endpoints.MultiplayerConnection.GetURI().c_str()), Poco::SyntaxException);
}
