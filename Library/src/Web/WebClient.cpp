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
#include "WebClient.h"

#include "CSP/Systems/Users/UserSystem.h"
#include "Debug/Logging.h"
#include "Json.h"
#include "Memory/Memory.h"
#include "Services/ApiBase/ApiBase.h"

#include <chrono>

using namespace std::chrono;
using namespace std::chrono_literals;

namespace csp::web
{

WebClient::WebClient(const Port InPort, const ETransferProtocol Tp, bool AutoRefresh)
    : RootPort(InPort)
    , LoginState(nullptr)
    , UserSystem(nullptr)
    , RefreshNeeded(false)
    , RefreshStarted(false)
    , AutoRefreshEnabled(AutoRefresh)
#ifndef CSP_WASM
    , RequestCount(0)
    , ThreadPool(CSP_MAX_CONCURRENT_REQUESTS)
#endif
{
}

WebClient::~WebClient()
{
#ifdef CSP_WASM

    WasmRequestsMutex.lock();
    {
        // Cancel all in-flight requests
        while (!WasmRequests.IsEmpty())
        {
            auto WasmRequest = WasmRequests.Dequeue();
            WasmRequest.value()->Cancel();
        }
    }
    WasmRequestsMutex.unlock();

    WasmRequests.Close();

#else
    uint32_t WaitCounter = 0;
    const uint32_t kMaxWaitCounter = 10 * 10; // 10 seconds timeout

    RequestsMutex.lock();
    {
        // Cancel all in-flight requests
        for (auto* Request : Requests)
        {
            Request->Cancel();
        }
    }
    RequestsMutex.unlock();

    // Wait for all cancelled requests to be processed
    while ((RequestCount > 0) && (WaitCounter < kMaxWaitCounter))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        ++WaitCounter;
    }

    // Process all outstanding responses
    WaitCounter = 0;

    while ((RequestCount > 0) && (WaitCounter < kMaxWaitCounter))
    {
        ProcessResponses(RequestCount);

        // Guard against exiting while Requests are still in flight
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        ++WaitCounter;
    }

    if (WaitCounter == kMaxWaitCounter)
    {
        CSP_LOG_WARN_MSG("Web client timed out waiting for outstanding request on exit\n");
    }

    PollRequests.Close();

    ThreadPool.Shutdown();
#endif
}

void WebClient::RefreshIfExpired()
{
    if (!AutoRefreshEnabled || RefreshNeeded)
    {
        return;
    }

    if (LoginState == nullptr)
    {
        UserSystem = csp::systems::SystemsManager::Get().GetUserSystem();
        LoginState = &UserSystem->GetLoginState();
    }

    if (LoginState->RefreshNeeded())
    {
#ifdef CSP_WASM
        WasmRequestsMutex.lock();
        {
            RefreshNeeded = true;
        }
        WasmRequestsMutex.unlock();
#else
        RefreshNeeded = true;
#endif

        csp::systems::NullResultCallback NullResCallback = [this](const csp::systems::NullResult& NullRes)
        {
            if (NullRes.GetResultCode() == csp::systems::EResultCode::Success)
            {
#ifdef CSP_WASM
                WasmRequestsMutex.lock();
                {
                    while (!WasmRequests.IsEmpty())
                    {
                        auto WasmRequest = WasmRequests.Dequeue().value();
                        WasmRequest->RefreshAccessToken();
                        Send(*WasmRequest);
                    }

                    RefreshNeeded = false;
                }
                WasmRequestsMutex.unlock();
#else
                RefreshNeeded = false;
#endif
                RefreshStarted = false;
            }
            else if (NullRes.GetResultCode() == csp::systems::EResultCode::Failed)
            {
                assert(false && "User authentication token refresh failed!");
            }
        };

        UserSystem->RefreshSession(UserSystem->GetLoginState().UserId, csp::web::HttpAuth::GetRefreshToken(), NullResCallback);
    }
}

void WebClient::SendRequest(ERequestVerb Verb, const csp::web::Uri& InUri, HttpPayload& Payload, IHttpResponseHandler* ResponseCallback,
    csp::common::CancellationToken& CancellationToken, bool AsyncResponse)
{
    auto* Request = CSP_NEW csp::web::HttpRequest(this, Verb, InUri, Payload, ResponseCallback, CancellationToken, AsyncResponse);

#ifdef CSP_WASM
    RefreshIfExpired();

    WasmRequestsMutex.lock();
    {
        if (RefreshNeeded && Request->GetPayload().GetRequiresBearerToken())
        {
            WasmRequests.Enqueue(Request);
        }
        else
        {
            Request->RefreshAccessToken();
            Send(*Request);
        }
    }
    WasmRequestsMutex.unlock();
#else
    AddRequest(Request);
#endif
}

void WebClient::AddRequest(HttpRequest* Request, std::chrono::milliseconds SendDelay)
{
    RefreshIfExpired();

    if (Request)
    {
#ifdef CSP_WASM
        WasmRequestsMutex.lock();
        {
            if (RefreshNeeded && Request->GetPayload().GetRequiresBearerToken())
            {
                WasmRequests.Enqueue(Request);
            }
            else
            {
                Request->RefreshAccessToken();
                Send(*Request);
            }
        }
        WasmRequestsMutex.unlock();
#else
        RequestsMutex.lock();
        {
            Requests.emplace(Request);
        }
        RequestsMutex.unlock();

        ++RequestCount;
        Request->IncRefCount();
        Request->SetSendDelay(SendDelay);
        ThreadPool.Enqueue(
            [this, Request](void*)
            {
                while (RefreshStarted)
                {
                    std::this_thread::sleep_for(10ns);
                }

                if (RefreshNeeded && !RefreshStarted)
                {
                    RefreshStarted = true;
                }

                Request->RefreshAccessToken();

                ProcessRequest(Request);

                return nullptr;
            });
#endif
    }
}

#ifndef CSP_WASM
void WebClient::ProcessResponses(const uint32_t MaxNumResponses)
{
    uint32_t ResponseCount = 0;

    while ((PollRequests.IsEmpty() == false) && (ResponseCount < MaxNumResponses))
    {
        auto PollRequest = PollRequests.Dequeue();

        HttpRequest* Request = PollRequest.value();
        IHttpResponseHandler* Callback = Request->GetCallback();

        if (!Request->Cancelled() && Callback)
        {
            auto& Response = Request->GetMutableResponse();
            Callback->OnHttpResponse(Response);
        }

        DestroyRequest(Request);

        // In case responses are being constantly queued from another
        // thread, make sure we don't keep polling forever
        ++ResponseCount;
    }
}

void WebClient::ProcessRequest(HttpRequest* Request)
{
    if (Request)
    {
        auto& Payload = Request->GetMutablePayload();
        Payload.SetBearerToken();

        const std::chrono::milliseconds SendDelay = Request->GetSendDelay();

        if (SendDelay > std::chrono::milliseconds(0))
        {
            // Wait before sending if required (e.g. for Retries)
            std::this_thread::sleep_for(SendDelay);
        }

        try
        {
            if (!Request->Cancelled())
            {
                Send(*Request);
            }
            else
            {
                Request->SetRequestProgress(100.0f);
                Request->SetResponseProgress(100.0f);
                Request->SetResponseCode(EResponseCodes::ResponseRequestTimeout);
                std::string ResponseBody = "{\"errors\": {\"\": [\"Request was cancelled by user.\"]}}";
                Request->SetResponseData(ResponseBody.c_str(), ResponseBody.length());
                Request->EnableAutoRetry(false);
            }
        }
        catch (const WebClientException& Ex)
        {
            CSP_LOG_MSG(csp::systems::LogLevel::Error, Ex.what());

            Request->SetRequestProgress(100.0f);
            Request->SetResponseCode(EResponseCodes::ResponseServiceUnavailable);
            std::string ResponseBody = "{\"errors\": {\"\": [\"Server could not be contacted. Please check your internet connection.\"]}}";
            Request->SetResponseData(ResponseBody.c_str(), ResponseBody.length());
            Request->SetResponseProgress(100.0f);
        }

        auto& Response = Request->GetMutableResponse();

        // Attempt Auto-retry if needed
        bool RetryIssued = Request->CheckForAutoRetry();

        if (Request->GetCallback())
        {
            if (Request->GetIsCallbackAsync())
            {
                if (!RetryIssued)
                {
                    const uint16_t ResponseCode = static_cast<uint16_t>(Response.GetResponseCode());
                    if (ResponseCode >= 400)
                    {
                        PrintClientErrorResponseMessages(Response);
                    }

                    Request->GetCallback()->OnHttpResponse(Response);
                }

                DestroyRequest(Request);
            }
            else
            {
                if (!RetryIssued)
                {
                    const uint16_t ResponseCode = static_cast<uint16_t>(Response.GetResponseCode());
                    if (ResponseCode >= 400)
                    {
                        PrintClientErrorResponseMessages(Response);
                    }

                    // This request is marked to be polled, so add to the queue
                    // to be issued on the next call to WebClient::ProcessResponses()
                    PollRequests.Enqueue({ Request });
                }
            }
        }
        else
        {
            // No callback, so just destroy the request
            DestroyRequest(Request);
        }
    }
}

void WebClient::DestroyRequest(HttpRequest* Request)
{
    RequestsMutex.lock();
    {
        Requests.erase(Request);
    }
    RequestsMutex.unlock();

    --RequestCount;

    if (Request->DecRefCount() == 0)
    {
        CSP_DELETE(Request);
    }
}
#endif

// This makes an attempt to parse out errors from known JSON error response structures but will fallback to
// logging the full error response if it cannot find the structured errors.
void WebClient::PrintClientErrorResponseMessages(const HttpResponse& Response)
{
    const uint16_t ResponseCode = static_cast<uint16_t>(Response.GetResponseCode());
    const csp::common::String& ResponsePayload = Response.GetPayload().GetContent();

    csp::common::String Verb = "";
    switch (Response.GetRequest()->GetVerb())
    {
    case ERequestVerb::Get:
        Verb = "GET";
        break;
    case ERequestVerb::Post:
        Verb = "POST";
        break;
    case ERequestVerb::Put:
        Verb = "PUT";
        break;
    case ERequestVerb::Delete:
        Verb = "DELETE";
        break;
    case ERequestVerb::Head:
        Verb = "HEAD";
        break;
    default:
        break;
    }

    if (ResponsePayload.IsEmpty())
    {
        CSP_LOG_ERROR_FORMAT("Services request %s %s has returned a failed response (%i) but with no payload/error message.", Verb.c_str(),
            Response.GetRequest()->GetUri().GetAsString(), ResponseCode);
        return;
    }

    csp::common::List<csp::common::String> Errors;

    if (Response.GetPayload().IsJsonPayload())
    {
        rapidjson::Document ResponseJson;
        ResponseJson.Parse(ResponsePayload.c_str());

        // Since is is possible to have responses in various different structures this is extra cautious in
        // not assuming the structure and always checking before accessing any element. If there is doubt
        // the full response will be printed later.
        if (ResponseJson.HasMember("errors"))
        {
            if (ResponseJson["errors"].IsArray())
            {
                const auto& ResponseArray = ResponseJson["errors"].GetArray();

                for (uint32_t i = 0; i < ResponseArray.Size(); i++)
                {
                    if (ResponseArray[i].IsObject())
                    {
                        const auto& ResponseError = ResponseArray[i].GetObject();
                        if (ResponseError.HasMember("message") && ResponseError["message"].IsString())
                        {
                            Errors.Append(ResponseError["message"].GetString());
                        }
                        else
                        {
                            Errors.Append(JsonObjectToString(ResponseError));
                        }
                    }
                }
            }
            else
            {
                Errors.Append(JsonObjectToString(ResponseJson["errors"]));
            }
        }
        else if (ResponseJson.HasMember("error"))
        {
            Errors.Append(JsonObjectToString(ResponseJson["error"]));
        }
    }

    // If the response was not JSON or errors were not found as expected, log the full response payload.
    if (Errors.Size() == 0)
    {
        CSP_LOG_ERROR_FORMAT("Services request %s %s has returned a failed response (%i) with payload/error message: %s", Verb.c_str(),
            Response.GetRequest()->GetUri().GetAsString(), ResponseCode, ResponsePayload.c_str());
    }
    else
    {
        for (auto i = 0; i < Errors.Size(); ++i)
        {
            CSP_LOG_ERROR_FORMAT("Services request %s %s has returned a failed response (%i) with payload/error message: %s", Verb.c_str(),
                Response.GetRequest()->GetUri().GetAsString(), ResponseCode, Errors[i].c_str());
        }
    }
}
} // namespace csp::web
