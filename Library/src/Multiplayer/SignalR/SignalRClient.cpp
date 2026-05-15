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

#include "SignalRClient.h"

#include "CSP/Common/Interfaces/IAuthContext.h"
#include "CSP/Common/String.h"
#include "Multiplayer/WebSocketClient.h"

#ifdef CSP_WASM
#include "Common/Web/EmscriptenWebClient/EmscriptenWebClient.h"
#else
#include "Common/Web/POCOWebClient/POCOWebClient.h"
#endif

#include <stdexcept>
#include <thread>

// Only used in this file for profiling macros. Still needs broken
#include "Debug/Logging.h"

using namespace signalr;

namespace csp::multiplayer
{

csp::multiplayer::IWebSocketClient* CSPWebSocketClientPtr = nullptr;

void SetWebSocketClient(IWebSocketClient* inCspWebSocketClientPtr) { CSPWebSocketClientPtr = inCspWebSocketClientPtr; }

CSPWebsocketClient::CSPWebsocketClient() noexcept { m_refreshInitialised = false; }

void CSPWebsocketClient::start(const std::string& url, std::function<void(std::exception_ptr)> callback)
{
    IWebSocketClient::CallbackHandler localCallback
        = [callback](bool ok) { ok ? callback(nullptr) : callback(std::make_exception_ptr(std::runtime_error("Socket Start Error"))); };

    CSPWebSocketClientPtr->Start(url, localCallback);
}

void CSPWebsocketClient::stop(std::function<void(std::exception_ptr)> callback)
{
    IWebSocketClient::CallbackHandler localCallback
        = [callback](bool ok) { ok ? callback(nullptr) : callback(std::make_exception_ptr(std::runtime_error("Socket Stop Error"))); };

    CSPWebSocketClientPtr->Stop(localCallback);
}

void CSPWebsocketClient::send(const std::string& payload, signalr::transfer_format /*format*/, std::function<void(std::exception_ptr)> callback)
{
    IWebSocketClient::CallbackHandler localCallback
        = [callback](bool ok) { ok ? callback(nullptr) : callback(std::make_exception_ptr(std::runtime_error("Socket Stop Error"))); };

    CSPWebSocketClientPtr->Send(payload, localCallback);
}

void CSPWebsocketClient::receive(std::function<void(const std::string&, std::exception_ptr)> callback)
{
    IWebSocketClient::ReceiveHandler localCallback = [callback](const std::string& message, bool ok)
    { ok ? callback(message, nullptr) : callback(message, std::make_exception_ptr(std::runtime_error("Socket Receive Error"))); };

    CSPWebSocketClientPtr->Receive(localCallback);
}

class SignalRResponseWaiter
{
public:
    /// @brief Wait for an event to occur
    /// @param IsDone Functional (function or lambda) that return true when an event occurs
    /// @param TimeOutInSeconds Maximum time to wait in Seconds
    /// @param SleepTimeMs (Optional) Millseconds to sleep for while waiting (default 100 ms)
    /// @return True if event occured or False if timeout period expired
    bool WaitFor(std::function<bool(void)> isDone, const std::chrono::seconds& timeOutInSeconds,
        const std::chrono::milliseconds sleepTimeMs = std::chrono::milliseconds(100))
    {
        using clock = std::chrono::system_clock;

        auto start = clock::now();
        clock::duration elapsed = clock::now() - start;

        const std::chrono::duration timeOut = timeOutInSeconds;

        while (!isDone() && (elapsed < timeOut))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepTimeMs));
            elapsed = clock::now() - start;
        }

        // Returns True if done event occured or False if we timeout
        return isDone();
    }
};

class SignalRResponseReceiver : public SignalRResponseWaiter, public csp::web::IHttpResponseHandler
{
public:
    SignalRResponseReceiver()
        : m_responseReceived(false)
        , m_threadId(std::this_thread::get_id())
    {
    }

    void OnHttpResponse(csp::web::HttpResponse& inResponse) override
    {
        m_response = inResponse;
        m_responseReceived = true;
    }

    bool WaitForResponse()
    {
        return WaitFor([this] { return IsResponseReceived(); }, std::chrono::seconds(5));
    }

    bool IsResponseReceived() const { return m_responseReceived; }

    csp::web::HttpResponse& GetResponse() { return m_response; }

private:
    csp::web::HttpResponse m_response;
    std::atomic<bool> m_responseReceived;
    std::thread::id m_threadId;
};

CSPHttpClient::CSPHttpClient(csp::common::IAuthContext& authContext)
{
// Passing null for the LogSystem to the POCO/Emscripten web client ctor to avoid logging high frequency multiplayer API exchange.
#ifdef CSP_WASM
    WebClientHttps = new csp::web::EmscriptenWebClient(443, csp::web::ETransferProtocol::HTTPS, AuthContext, nullptr);
#else
    m_webClientHttps = new csp::web::POCOWebClient(443, csp::web::ETransferProtocol::HTTPS, authContext, nullptr);
#endif
}

void CSPHttpClient::send(
    const std::string& /*url*/, const http_request& request, std::function<void(const http_response&, std::exception_ptr)> callback)
{
    CSP_PROFILE_SCOPED();

    SignalRResponseReceiver signalRConnectionReceiver;
    csp::web::HttpPayload payLoad;
    payLoad.AddHeader(CSP_TEXT("Content-Type"), CSP_TEXT("application/json"));
    payLoad.SetBearerToken();

    // Set headers here
    if (request.headers.size() > 0)
    {
        for (auto& header : request.headers)
        {
            payLoad.AddHeader(CSP_TEXT(header.first.c_str()), CSP_TEXT(header.second.c_str()));
        }
    }

    payLoad.AddContent(CSP_TEXT(request.content.c_str()));
    std::string uriString = "negotiate?negotiateVersion=1";

    m_webClientHttps->SendRequest(
        csp::web::ERequestVerb::POST, csp::web::Uri(uriString.c_str()), payLoad, &signalRConnectionReceiver, csp::common::CancellationToken::Dummy());
    // Sleep thread until response is received
    if (signalRConnectionReceiver.WaitForResponse())
    {
        if (signalRConnectionReceiver.GetResponse().GetResponseCode() == csp::web::EResponseCodes::ResponseOK)
        {
            std::string responseContent = signalRConnectionReceiver.GetResponse().GetPayload().GetContent().c_str();
            http_response receivedResponse
                = http_response(static_cast<int>(signalRConnectionReceiver.GetResponse().GetResponseCode()), responseContent);
            callback(receivedResponse, nullptr);
        }
        else
        {
            callback(http_response(), std::make_exception_ptr(std::runtime_error("unknown http method")));
        }
    }
    else
    {
        callback(http_response(), std::make_exception_ptr(std::runtime_error("unknown http method")));
    }
}

} // namespace csp::multiplayer
