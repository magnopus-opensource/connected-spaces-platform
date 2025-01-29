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

#include "CSP/CSPFoundation.h"
#include "CSP/Common/String.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Multiplayer/WebSocketClient.h"

#ifdef CSP_WASM
#include "Web/EmscriptenWebClient/EmscriptenWebClient.h"
#else
#include "Web/POCOWebClient/POCOWebClient.h"
#endif

#include <stdexcept>
#include <thread>

using namespace signalr;

namespace csp::multiplayer
{

csp::multiplayer::IWebSocketClient* CSPWebSocketClientPtr = nullptr;

void SetWebSocketClient(IWebSocketClient* InCSPWebSocketClientPtr) { CSPWebSocketClientPtr = InCSPWebSocketClientPtr; }

CSPWebsocketClient::CSPWebsocketClient() noexcept
    : UserSystem(nullptr)
    , LoginState(nullptr)
{
    RefreshInitialised = false;
}

void CSPWebsocketClient::start(const std::string& url, std::function<void(std::exception_ptr)> callback)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    UserSystem = SystemsManager.GetUserSystem();
    LoginState = &UserSystem->GetLoginState();

    IWebSocketClient::CallbackHandler LocalCallback
        = [callback](bool ok) { ok ? callback(nullptr) : callback(std::make_exception_ptr(std::runtime_error("Socket Start Error"))); };

    CSPWebSocketClientPtr->Start(url, LocalCallback);
}

void CSPWebsocketClient::stop(std::function<void(std::exception_ptr)> callback)
{
    IWebSocketClient::CallbackHandler LocalCallback
        = [callback](bool ok) { ok ? callback(nullptr) : callback(std::make_exception_ptr(std::runtime_error("Socket Stop Error"))); };

    CSPWebSocketClientPtr->Stop(LocalCallback);
}

void CSPWebsocketClient::send(const std::string& payload, signalr::transfer_format format, std::function<void(std::exception_ptr)> callback)
{
    IWebSocketClient::CallbackHandler LocalCallback
        = [callback](bool ok) { ok ? callback(nullptr) : callback(std::make_exception_ptr(std::runtime_error("Socket Stop Error"))); };

    CSPWebSocketClientPtr->Send(payload, LocalCallback);
}

void CSPWebsocketClient::receive(std::function<void(const std::string&, std::exception_ptr)> callback)
{
    IWebSocketClient::ReceiveHandler LocalCallback = [callback](const std::string& message, bool ok)
    { ok ? callback(message, nullptr) : callback(message, std::make_exception_ptr(std::runtime_error("Socket Receive Error"))); };

    CSPWebSocketClientPtr->Receive(LocalCallback);
}

class SignalRResponseWaiter
{
public:
    /// @brief Wait for an event to occur
    /// @param IsDone Functional (function or lambda) that return true when an event occurs
    /// @param TimeOutInSeconds Maximum time to wait in Seconds
    /// @param SleepTimeMs (Optional) Millseconds to sleep for while waiting (default 100 ms)
    /// @return True if event occured or False if timeout period expired
    bool WaitFor(std::function<bool(void)> IsDone, const std::chrono::seconds& TimeOutInSeconds,
        const std::chrono::milliseconds SleepTimeMs = std::chrono::milliseconds(100))
    {
        using clock = std::chrono::system_clock;

        auto Start = clock::now();
        clock::duration Elapsed = clock::now() - Start;

        const std::chrono::duration TimeOut = TimeOutInSeconds;

        while (!IsDone() && (Elapsed < TimeOut))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(SleepTimeMs));
            Elapsed = clock::now() - Start;
        }

        // Returns True if done event occured or False if we timeout
        return IsDone();
    }
};

class SignalRResponseReceiver : public SignalRResponseWaiter, public csp::web::IHttpResponseHandler
{
public:
    SignalRResponseReceiver()
        : ResponseReceived(false)
        , ThreadId(std::this_thread::get_id())
    {
    }

    void OnHttpResponse(csp::web::HttpResponse& InResponse) override
    {
        Response = InResponse;
        ResponseReceived = true;
    }

    bool WaitForResponse()
    {
        return WaitFor([this] { return IsResponseReceived(); }, std::chrono::seconds(5));
    }

    bool IsResponseReceived() const { return ResponseReceived; }

    csp::web::HttpResponse& GetResponse() { return Response; }

private:
    csp::web::HttpResponse Response;
    std::atomic<bool> ResponseReceived;
    std::thread::id ThreadId;
};

CSPHttpClient::CSPHttpClient()
{
#ifdef CSP_WASM
    WebClientHttps = CSP_NEW csp::web::EmscriptenWebClient(443, csp::web::ETransferProtocol::HTTPS);
#else
    WebClientHttps = CSP_NEW csp::web::POCOWebClient(443, csp::web::ETransferProtocol::HTTPS);
#endif
}

void CSPHttpClient::send(const std::string& url, const http_request& request, std::function<void(const http_response&, std::exception_ptr)> callback)
{
    CSP_PROFILE_SCOPED();

    SignalRResponseReceiver SignalRConnectionReceiver;
    csp::web::HttpPayload PayLoad;
    PayLoad.AddHeader(CSP_TEXT("Content-Type"), CSP_TEXT("application/json"));
    PayLoad.SetBearerToken();

    // Set headers here
    if (request.headers.size() > 0)
    {
        for (auto& header : request.headers)
        {
            PayLoad.AddHeader(CSP_TEXT(header.first.c_str()), CSP_TEXT(header.second.c_str()));
        }
    }

    PayLoad.AddContent(CSP_TEXT(request.content.c_str()));
    std::string UriString = "negotiate?negotiateVersion=1";

    WebClientHttps->SendRequest(
        csp::web::ERequestVerb::POST, csp::web::Uri(UriString.c_str()), PayLoad, &SignalRConnectionReceiver, csp::common::CancellationToken::Dummy());
    // Sleep thread until response is received
    if (SignalRConnectionReceiver.WaitForResponse())
    {
        if (SignalRConnectionReceiver.GetResponse().GetResponseCode() == csp::web::EResponseCodes::ResponseOK)
        {
            std::string ResponseContent = SignalRConnectionReceiver.GetResponse().GetPayload().GetContent().c_str();
            http_response ReceivedResponse
                = http_response(static_cast<int>(SignalRConnectionReceiver.GetResponse().GetResponseCode()), ResponseContent);
            callback(ReceivedResponse, nullptr);
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
