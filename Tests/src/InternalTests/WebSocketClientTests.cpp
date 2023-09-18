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

#ifdef RUN_PLATFORM_TESTS

	#include "../PublicAPITests/UserSystemTestHelpers.h"
	#include "CSP/CSPFoundation.h"
	#include "CSP/Systems/SystemsManager.h"
	#include "CSP/Systems/Users/UserSystem.h"
	#include "PlatformTestUtils.h"
	#include "TestHelpers.h"

	#include "gtest/gtest.h"

using namespace csp::multiplayer;

CSP_INTERNAL_TEST(CSPEngine, WebSocketClientTests, SignalRClientStartStopTest)
{
	// Initialise
	InitialiseFoundation();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Start
	auto* WebSocket = WebSocketStart("wss://ogs-odev-internal.magnoboard.com/mag-multiplayer/hubs/v1/multiplayer?id=");

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
	auto* UserSystem	 = SystemsManager.GetUserSystem();

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Start
	auto* WebSocket = WebSocketStart("wss://ogs-odev-internal.magnoboard.com/mag-multiplayer/hubs/v1/multiplayer?id=");

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
	auto* UserSystem	 = SystemsManager.GetUserSystem();

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Start
	auto* WebSocket = WebSocketStart("wss://ogs-odev-internal.magnoboard.com/mag-multiplayer/hubs/v1/multiplayer?id=");

	// Receive
	WebSocketSendReceive(WebSocket);

	// Stop
	WebSocketStop(WebSocket);

	// Logout
	LogOut(UserSystem);
}
#endif