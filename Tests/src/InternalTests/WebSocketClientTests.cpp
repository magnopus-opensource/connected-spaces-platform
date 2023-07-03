#include "../PublicAPITests/UserSystemTestHelpers.h"
#include "CSP/CSPFoundation.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "Multiplayer/SignalR/POCOSignalRClient/PocoSignalRClient.h"
#include "TestHelpers.h"
#include "Web/POCOWebClient/POCOWebClient.h"

#include "gtest/gtest.h"

using namespace csp::multiplayer;

CSP_INTERNAL_TEST(CSPEngine, WebSocketClientTests, SignalRClientStartStopTest)
{
	InitialiseFoundationWithUserAgentInfo(EndpointBaseURI);

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Start
	bool finished = false;

	auto Fn = [&](bool result)
	{
		EXPECT_TRUE(result);
		finished = true;
	};

	auto* WebSocketClient = CSP_NEW CSPWebSocketClientPOCO();
	WebSocketClient->Start(csp::CSPFoundation::GetEndpoints().MultiplayerServiceURI.c_str(), Fn);

	while (!finished)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	// Stop
	finished = false;

	WebSocketClient->Stop(Fn);

	// Cleanup
	LogOut(UserSystem);
}


CSP_INTERNAL_TEST(CSPEngine, WebSocketClientTests, SignalRClientSendTest)
{
	InitialiseFoundationWithUserAgentInfo(EndpointBaseURI);

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Start
	bool finished = false;

	auto Fn = [&](bool result)
	{
		EXPECT_TRUE(result);
		finished = true;
	};

	auto* WebSocketClient = CSP_NEW CSPWebSocketClientPOCO();
	WebSocketClient->Start(csp::CSPFoundation::GetEndpoints().MultiplayerServiceURI.c_str(), Fn);

	while (!finished)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	// Send
	finished = false;

	WebSocketClient->Send("test", Fn);

	// Stop
	finished = false;

	WebSocketClient->Stop(Fn);

	// Cleanup
	LogOut(UserSystem);
}

CSP_INTERNAL_TEST(CSPEngine, WebSocketClientTests, SignalRClientSendReceiveTest)
{
	InitialiseFoundationWithUserAgentInfo(EndpointBaseURI);

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();

	// Log in
	csp::common::String UserId;
	LogIn(UserSystem, UserId);

	// Start
	bool finished = false;

	auto Fn = [&](bool result)
	{
		EXPECT_TRUE(result);
		finished = true;
	};

	auto* WebSocketClient = CSP_NEW CSPWebSocketClientPOCO();
	WebSocketClient->Start(csp::CSPFoundation::GetEndpoints().MultiplayerServiceURI.c_str(), Fn);

	while (!finished)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	// Receive
	finished = false;

	bool finished2 = false;

	auto Fn2 = [&](const std::string& S, bool result)
	{
		EXPECT_TRUE(result);
		finished2 = true;
	};

	WebSocketClient->Receive(Fn2);

	// Ensure receive thread has started
	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	// Send
	finished = false;

	WebSocketClient->Send("{\"protocol\":\"messagepack\",\"version\":1}\x1e", Fn);

	while (!finished)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	finished = false;

	WebSocketClient->Send("\x2\x6", Fn);

	while (!finished)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	while (!finished2)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	// Stop
	finished = false;

	WebSocketClient->Stop(Fn);

	// Cleanup
	LogOut(UserSystem);
}