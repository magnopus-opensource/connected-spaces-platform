#include "../PublicAPITests/UserSystemTestHelpers.h"
#include "CSP/CSPFoundation.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "TestHelpers.h"

#ifdef CSP_WASM
	#include "Multiplayer/SignalR/EmscriptenSignalRClient/EmscriptenSignalRClient.h"
	#include "Web/EmscriptenWebClient/EmscriptenWebClient.h"

	#include <emscripten/emscripten.h>
	#include <emscripten/fetch.h>
	#include <emscripten/threading.h>
#else
	#include "Multiplayer/SignalR/POCOSignalRClient/POCOSignalRClient.h"
	#include "Web/POCOWebClient/POCOWebClient.h"
#endif

#include "gtest/gtest.h"
#include <thread>

using namespace csp::multiplayer;

void PlatformTestsLogin(csp::systems::UserSystem* UserSystem)
{
#ifndef CSP_WASM
	csp::common::String Res;
	LogIn(UserSystem, Res);
#else

	bool finished = false;

	std::thread TestThread(
		[&]()
		{
			auto CB = [&](const csp::systems::LoginStateResult& Result)
			{
				if (Result.GetResultCode() == csp::services::EResultCode::InProgress)
				{
					return;
				}

				finished = true;
				EXPECT_EQ(Result.GetResultCode(), csp::services::EResultCode::Success);
			};

			UserSystem->Login("", DefaultLoginEmail, DefaultLoginPassword, CB);
		});

	while (!finished)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	TestThread.join();
#endif
}

void PlatformTestsLogout(csp::systems::UserSystem* UserSystem)
{
#ifndef CSP_WASM
	LogOut(UserSystem);
#else

	bool finished = false;

	std::thread TestThread(
		[&]()
		{
			auto CB = [&](const csp::systems::LogoutResult& Result)
			{
				if (Result.GetResultCode() == csp::services::EResultCode::InProgress)
				{
					return;
				}

				finished = true;
				EXPECT_EQ(Result.GetResultCode(), csp::services::EResultCode::Success);
			};

			UserSystem->Logout(CB);
		});

	while (!finished)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	TestThread.join();

#endif
}

csp::multiplayer::IWebSocketClient* WebSocketStart(const csp::common::String& Uri)
{
	bool finished = false;

	auto Fn = [&](bool result)
	{
		EXPECT_TRUE(result);
		finished = true;
	};

#ifdef CSP_WASM
	auto* WebSocketClient = CSP_NEW csp::multiplayer::CSPWebSocketClientEmscripten();

	std::thread TestThread(
		[&]()
		{
			WebSocketClient->Start(Uri.c_str(), Fn);
		});
#else
	auto* WebSocketClient = CSP_NEW csp::multiplayer::CSPWebSocketClientPOCO();
	WebSocketClient->Start(Uri.c_str(), Fn);
#endif

	while (!finished)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

#ifdef CSP_WASM
	TestThread.join();
#endif

	return WebSocketClient;
}

void WebSocketStop(csp::multiplayer::IWebSocketClient* WebSocketClient)
{
	bool finished = false;

	auto Fn = [&](bool result)
	{
		EXPECT_TRUE(result);
		finished = true;
	};

#ifdef CSP_WASM

	std::thread TestThread(
		[&]()
		{
			WebSocketClient->Stop(Fn);
		});
#else

	WebSocketClient->Stop(Fn);
#endif

	while (!finished)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

#ifdef CSP_WASM
	TestThread.join();
#endif
}

void WebSocketSend(csp::multiplayer::IWebSocketClient* WebSocketClient, const csp::common::String& Data)
{
	bool finished = false;

	auto Fn = [&](bool result)
	{
		EXPECT_TRUE(result);
		finished = true;
	};

#ifdef CSP_WASM

	std::thread TestThread(
		[&]()
		{
			WebSocketClient->Send(Data.c_str(), Fn);
		});
#else
	WebSocketClient->Send(Data.c_str(), Fn);
#endif

	while (!finished)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

#ifdef CSP_WASM
	TestThread.join();
#endif
}

void WebSocketSendReceive(csp::multiplayer::IWebSocketClient* WebSocketClient)
{
	bool finished2 = false;

	auto Fn2 = [&](const std::string& S, bool result)
	{
		EXPECT_TRUE(result);
		finished2 = true;
	};

#ifdef CSP_WASM

	std::thread TestThread(
		[&]()
		{
			WebSocketClient->Receive(Fn2);
		});
#else
	WebSocketClient->Receive(Fn2);
#endif

	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	WebSocketSend(WebSocketClient, "{\"protocol\":\"messagepack\",\"version\":1}\x1e");
	WebSocketSend(WebSocketClient, "\x2\x6");

	while (!finished2)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

#ifdef CSP_WASM
	TestThread.join();
#endif
}

void InitialiseFoundation2()
{
	InitialiseFoundationWithUserAgentInfo(EndpointBaseURI);
}

void InitialiseFoundation()
{
#ifndef CSP_WASM
	InitialiseFoundation2();
#else
	emscripten_sync_run_in_main_runtime_thread(EM_FUNC_SIG_V, InitialiseFoundation2);
#endif
}

CSP_INTERNAL_TEST(CSPEngine, WebSocketClientTests, SignalRClientStartStopTest)
{
	InitialiseFoundation();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();

	// Log in
	csp::common::String UserId;

	PlatformTestsLogin(UserSystem);

	// Start
	auto* WebSocket = WebSocketStart("wss://ogs-odev-internal.magnoboard.com/mag-multiplayer/hubs/v1/multiplayer?id=");

	// Stop
	WebSocketStop(WebSocket);

	// Logout
	PlatformTestsLogout(UserSystem);
}


CSP_INTERNAL_TEST(CSPEngine, WebSocketClientTests, SignalRClientSendTest)
{
	InitialiseFoundation();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();

	// Log in
	PlatformTestsLogin(UserSystem);

	// Start
	auto* WebSocket = WebSocketStart("wss://ogs-odev-internal.magnoboard.com/mag-multiplayer/hubs/v1/multiplayer?id=");

	WebSocketSend(WebSocket, "test");

	// Stop
	WebSocketStop(WebSocket);

	// Logout
	PlatformTestsLogout(UserSystem);
}

CSP_INTERNAL_TEST(CSPEngine, WebSocketClientTests, SignalRClientSendReceiveTest)
{
	InitialiseFoundation();

	auto& SystemsManager = csp::systems::SystemsManager::Get();
	auto* UserSystem	 = SystemsManager.GetUserSystem();

	// Log in
	PlatformTestsLogin(UserSystem);

	// Start
	auto* WebSocket = WebSocketStart("wss://ogs-odev-internal.magnoboard.com/mag-multiplayer/hubs/v1/multiplayer?id=");

	WebSocketSendReceive(WebSocket);

	// Stop
	WebSocketStop(WebSocket);

	// Logout
	PlatformTestsLogout(UserSystem);
}