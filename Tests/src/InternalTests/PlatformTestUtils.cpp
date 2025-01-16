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

#include "PlatformTestUtils.h"

#include "Debug/Logging.h"
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

namespace
{
void InitialiseFoundationInternal() { InitialiseFoundationWithUserAgentInfo(EndpointBaseURI()); }
} // namespace

void PlatformTestWait(bool& finished)
{
    std::chrono::seconds total(10);
    std::chrono::milliseconds current(0);

    while (!finished && total > current)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        current += std::chrono::milliseconds(10);
    }
}

csp::multiplayer::IWebSocketClient* WebSocketStart(const csp::common::String& Uri)
{
    bool finished = false;

    auto Fn = [&](bool result)
    {
        finished = true;
        EXPECT_TRUE(result);
    };

#ifdef CSP_WASM
    auto* WebSocketClient = CSP_NEW csp::multiplayer::CSPWebSocketClientEmscripten();

    std::thread TestThread([&]() { WebSocketClient->Start(Uri.c_str(), Fn); });
#else
    auto* WebSocketClient = CSP_NEW csp::multiplayer::CSPWebSocketClientPOCO();
    WebSocketClient->Start(Uri.c_str(), Fn);
#endif

    PlatformTestWait(finished);

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
        finished = true;
        EXPECT_TRUE(result);
    };

#ifdef CSP_WASM

    std::thread TestThread([&]() { WebSocketClient->Stop(Fn); });
#else

    WebSocketClient->Stop(Fn);
#endif

    PlatformTestWait(finished);

#ifdef CSP_WASM
    TestThread.join();
#endif
}

void WebSocketSend(csp::multiplayer::IWebSocketClient* WebSocketClient, const csp::common::String& Data)
{
    bool finished = false;

    auto Fn = [&](bool result)
    {
        finished = true;
        EXPECT_TRUE(result);
    };

#ifdef CSP_WASM

    std::thread TestThread([&]() { WebSocketClient->Send(Data.c_str(), Fn); });
#else
    WebSocketClient->Send(Data.c_str(), Fn);
#endif

    PlatformTestWait(finished);

#ifdef CSP_WASM
    TestThread.join();
#endif
}

void WebSocketSendReceive(csp::multiplayer::IWebSocketClient* WebSocketClient)
{
    bool finished2 = false;

    auto Fn2 = [&](const std::string& S, bool result)
    {
        finished2 = true;
        EXPECT_TRUE(result);
    };

#ifdef CSP_WASM

    std::thread TestThread([&]() { WebSocketClient->Receive(Fn2); });
#else
    WebSocketClient->Receive(Fn2);
#endif

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    WebSocketSend(WebSocketClient, "{\"protocol\":\"messagepack\",\"version\":1}\x1e");
    WebSocketSend(WebSocketClient, "\x2\x6");

    PlatformTestWait(finished2);

#ifdef CSP_WASM
    TestThread.join();
#endif
}

void InitialiseFoundation()
{
#ifndef CSP_WASM
    InitialiseFoundationInternal();
#else
    // wasm tests are called from a pthread so we need to run setup on the main thread
    emscripten_sync_run_in_main_runtime_thread(EM_FUNC_SIG_V, InitialiseFoundationInternal);

    // remove logging on wasm to prevent corruption of the output xml report as this is sent through stdout
    csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(csp::systems::LogLevel::NoLogging);
#endif
}