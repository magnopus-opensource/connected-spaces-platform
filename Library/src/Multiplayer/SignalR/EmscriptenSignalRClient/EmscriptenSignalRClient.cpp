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
#include "EmscriptenSignalRClient.h"

#include "Debug/Logging.h"
#include "Web/HttpAuth.h"

#include <assert.h>
#include <iostream>

#ifdef ENABLE_EMS_SOCKET_LOGGING
#define EMS_LOG(STR) FOUNDATION_LOG_MSG(csp::systems::LogLevel::VeryVerbose, STR)
#define EMS_FORMATTED_LOG(FORMAT_STR, ...) FOUNDATION_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, FORMAT_STR, ##__VA_ARGS__)
#else
#define EMS_LOG(STR)
#define EMS_FORMATTED_LOG(FORMAT_STR, ...)
#endif

namespace csp::multiplayer
{

constexpr const unsigned short SOCKET_CLOSE_CODE = 1000; // TODO random number for now
constexpr const char* SOCKET_CLOSE_REASON = "Close";
constexpr const char* URL_QUERY_SEPARATOR = "?";

EM_BOOL onSocketOpened(int EventType, const EmscriptenWebSocketOpenEvent* WebsocketEvent, void* UserData)
{
    EMS_LOG("EMS onSocketOpened");

    auto WebSocketClient = static_cast<CSPWebSocketClientEmscripten*>(UserData);

    auto& StartCallbackThread = WebSocketClient->GetStartCallbackThread();
    StartCallbackThread = std::thread([WebSocketClient]() { WebSocketClient->StartCallbackThreadFunc(true); });
    StartCallbackThread.detach();

    return EM_TRUE;
}

EM_BOOL onSocketError(int EventType, const EmscriptenWebSocketErrorEvent* WebsocketEvent, void* UserData)
{
    // TODO handle the socket error
    EMS_LOG("EMS onSocketError");

    auto WebSocketClient = static_cast<CSPWebSocketClientEmscripten*>(UserData);

    if (WebSocketClient->getReceiveCallback())
    {
        (*WebSocketClient->getReceiveCallback())("", false);
    }

    return EM_TRUE;
}

EM_BOOL onSocketClosed(int EventType, const EmscriptenWebSocketCloseEvent* WebsocketEvent, void* UserData)
{
    EMS_FORMATTED_LOG("EMS onSocketClosed Reason: %s", WebsocketEvent->reason);

    auto WebSocketClient = static_cast<CSPWebSocketClientEmscripten*>(UserData);

    if (WebSocketClient->getReceiveCallback())
    {
        (*WebSocketClient->getReceiveCallback())("", false);
    }

    emscripten_websocket_delete(WebsocketEvent->socket);

    return EM_TRUE;
}

/*
   Runs on main thread
 */
EM_BOOL onDataReceived(int EventType, const EmscriptenWebSocketMessageEvent* WebsocketEvent, void* UserData)
{
    EMS_FORMATTED_LOG("EMS onDataReceived NumBytes: %d, isText: %d", WebsocketEvent->numBytes, (int)WebsocketEvent->isText);

    if (WebsocketEvent->numBytes == 0)
    {
        std::cerr << "Socket closed by remote host." << std::endl;
        return EM_FALSE;
    }

    auto WebSocketClient = static_cast<CSPWebSocketClientEmscripten*>(UserData);
    auto ReceivedByteCount = WebsocketEvent->numBytes;
    auto Idx = 0;

    while (ReceivedByteCount > 0)
    {
        const auto ProcessedByteCount
            = WebSocketClient->ProcessReceivedMessage(WebsocketEvent->data + Idx, ReceivedByteCount, WebsocketEvent->isText);
        Idx += ProcessedByteCount;
        ReceivedByteCount -= ProcessedByteCount;
    }

    return EM_TRUE;
}

CSPWebSocketClientEmscripten::CSPWebSocketClientEmscripten() noexcept
    : ReceivedHandshake(false)
{
}

void CSPWebSocketClientEmscripten::Start(const std::string& Url, CallbackHandler Callback)
{
    EMS_LOG("EMS Start");

    if (!emscripten_websocket_is_supported())
    {
        Callback(false);
        return;
    }

    std::string wsConnectURL = GetWebSocketConnectURL(Url);

    EmscriptenWebSocketCreateAttributes ws_attrs = { wsConnectURL.c_str(), nullptr, EM_TRUE };

    Socket = emscripten_websocket_new(&ws_attrs);
    emscripten_websocket_set_onopen_callback(Socket, this, onSocketOpened);
    emscripten_websocket_set_onerror_callback(Socket, this, onSocketError);
    emscripten_websocket_set_onclose_callback(Socket, this, onSocketClosed);
    emscripten_websocket_set_onmessage_callback(Socket, this, onDataReceived);

    // the callback will be executed from a different thread than main
    StartCallback = Callback;
}

void CSPWebSocketClientEmscripten::Stop(CallbackHandler Callback)
{
    EMS_LOG("EMS Stop");

    EMSCRIPTEN_RESULT result = emscripten_websocket_close(Socket, SOCKET_CLOSE_CODE, SOCKET_CLOSE_REASON);
    EMSCRIPTEN_RESULT result2 = emscripten_websocket_delete(Socket);

    Socket = NULL;

    if (Callback)
    {
        Callback(EMSCRIPTEN_RESULT_SUCCESS == result && EMSCRIPTEN_RESULT_SUCCESS == result2);
    }
}

void CSPWebSocketClientEmscripten::Send(const std::string& Message, CallbackHandler Callback)
{
    EMS_FORMATTED_LOG("EMS Send %s", Message.c_str());

    EMSCRIPTEN_RESULT result = emscripten_websocket_send_binary(Socket, const_cast<char*>(Message.data()), Message.size());
    if (result)
    {
        EMS_FORMATTED_LOG("Failed to send data: %d", result);
    }

    Callback(EMSCRIPTEN_RESULT_SUCCESS == result);
}

void CSPWebSocketClientEmscripten::Receive(ReceiveHandler Callback)
{
    EMS_LOG("EMS Receive");
    ReceiveCallback = Callback;
}

std::thread& CSPWebSocketClientEmscripten::GetStartCallbackThread() { return StartCallbackThread; }

void CSPWebSocketClientEmscripten::StartCallbackThreadFunc(bool CallbackResult) { StartCallback(CallbackResult); }

std::string CSPWebSocketClientEmscripten::GetWebSocketConnectURL(const std::string& InitialUrl)
{
    std::string WebSocketConnectURL;
    auto QueryParamPos = InitialUrl.rfind(URL_QUERY_SEPARATOR);
    if (std::string::npos != QueryParamPos)
    {
        std::string WebSocketEndpoint = InitialUrl.substr(0, QueryParamPos);

        WebSocketConnectURL = WebSocketEndpoint + "?access_token=" + std::string(csp::web::HttpAuth::GetAccessToken().c_str())
            + "&X-DeviceUDID=" + std::string(csp::CSPFoundation::GetDeviceId().c_str());
        EMS_FORMATTED_LOG("WebSocket connect URL: %s", WebSocketConnectURL.c_str());
    }
    else
    {
        assert(false && "The initialURL is not in the expected format");
    }

    return WebSocketConnectURL;
}

size_t CSPWebSocketClientEmscripten::ProcessReceivedMessage(uint8_t* RecvData, uint32_t NumRecvBytes, EM_BOOL IsPlainText)
{
    std::string CallbackMessage;
    bool CallbackHasData = false;

    char* RecvBuffer = reinterpret_cast<char*>(RecvData);

    assert(!(IsPlainText) && "The JSON hub protocol is not supported!");

    size_t ProcessedByteCount = 0;

    // Handshake needs to be handled differently as it is in JSON format
    if (!ReceivedHandshake)
    {
        int Idx;
        for (Idx = 0; Idx < NumRecvBytes; ++Idx)
        {
            // JSON messages are terminated with the 0x1E character
            if (RecvBuffer[Idx] == 0x1E)
            {
                break;
            }
        }

        assert(!(Idx == NumRecvBytes) && "Message terminator was not found");

        CallbackMessage = std::string(RecvBuffer, Idx + 1);
        ReceivedHandshake = true;
        CallbackHasData = true;

        ProcessedByteCount = Idx + 1;
    }
    else
    {
        auto Length = 0;
        int Idx;

        for (Idx = 0; Idx < 5; ++Idx)
        {
            Length |= (RecvBuffer[Idx] & 0x7F) << (Idx * 7);

            if ((RecvBuffer[Idx] & 0x80) == 0)
            {
                ++Idx;
                break;
            }
        }

        if (Length > NumRecvBytes - Idx)
        {
            assert(false && "We have not received the entire SignalR message");
        }

        CallbackMessage = std::string(RecvBuffer, Length + Idx);
        CallbackHasData = true;

        ProcessedByteCount = Length + Idx;
    }

    // Call the callback
    if (ReceiveCallback)
    {
        ReceiveCallback(CallbackMessage, CallbackHasData);
    }

    return ProcessedByteCount;
}

IWebSocketClient::ReceiveHandler* CSPWebSocketClientEmscripten::getReceiveCallback() { return &ReceiveCallback; }

} // namespace csp::multiplayer
