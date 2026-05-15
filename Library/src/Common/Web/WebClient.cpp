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
#include "CSP/Common/Interfaces/IAuthContext.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Common/fmt_Formatters.h"
#include "Json.h"
#include "Services/ApiBase/ApiBase.h"

#include <chrono>

using namespace std::chrono;
using namespace std::chrono_literals;

namespace csp::web
{

WebClient::WebClient(
    const Port inPort, const ETransferProtocol /*Tp*/, csp::common::IAuthContext& authContext, csp::common::LogSystem* logSystem, bool autoRefresh)
    : m_rootPort(inPort)
    , m_authContext { &authContext }
    , m_logSystem(logSystem)
    , m_refreshNeeded(false)
    , m_refreshStarted(false)
    , m_autoRefreshEnabled(autoRefresh)
#ifndef CSP_WASM
    , m_requestCount(0)
    , m_threadPool(CSP_MAX_CONCURRENT_REQUESTS)
#endif
{
}

WebClient::WebClient(const Port inPort, const ETransferProtocol /*Tp*/, csp::common::LogSystem* logSystem, bool autoRefresh)
    : m_rootPort(inPort)
    , m_authContext(nullptr)
    , m_logSystem(logSystem)
    , m_refreshNeeded(false)
    , m_refreshStarted(false)
    , m_autoRefreshEnabled(autoRefresh)
#ifndef CSP_WASM
    , m_requestCount(0)
    , m_threadPool(CSP_MAX_CONCURRENT_REQUESTS)
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
    uint32_t waitCounter = 0;
    const uint32_t kMaxWaitCounter = 10 * 10; // 10 seconds timeout

    m_requestsMutex.lock();
    {
        // Cancel all in-flight requests
        for (auto* request : m_requests)
        {
            request->Cancel();
        }
    }
    m_requestsMutex.unlock();

    // Wait for all cancelled requests to be processed
    while ((m_requestCount > 0) && (waitCounter < kMaxWaitCounter))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        ++waitCounter;
    }

    // Process all outstanding responses
    waitCounter = 0;

    while ((m_requestCount > 0) && (waitCounter < kMaxWaitCounter))
    {
        ProcessResponses(m_requestCount);

        // Guard against exiting while Requests are still in flight
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        ++waitCounter;
    }

    if (waitCounter == kMaxWaitCounter)
    {
        CSP_LOG_WARN_MSG("Web client timed out waiting for outstanding request on exit\n");
    }

    m_pollRequests.Close();

    m_threadPool.Shutdown();
#endif
}

void WebClient::RefreshIfExpired()
{
    if (!m_autoRefreshEnabled || m_refreshNeeded)
    {
        return;
    }

    if (m_authContext->GetLoginState().RefreshNeeded())
    {
#ifdef CSP_WASM
        WasmRequestsMutex.lock();
        {
            RefreshNeeded = true;
        }
        WasmRequestsMutex.unlock();
#else
        m_refreshNeeded = true;
#endif

        m_authContext->RefreshToken(
            [this](bool success)
            {
                if (success)
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
                    m_refreshNeeded = false;
#endif
                    m_refreshStarted = false;
                }
                else
                {
                    CSP_LOG_MSG(csp::common::LogLevel::Fatal, "User authentication token refresh failed!");

                    // reset the state of the web client to prevent indefinite freeze when enqueuing a request
                    m_refreshNeeded = true;
                    m_refreshStarted = false;
                }
            });
    }
}

void WebClient::SendRequest(ERequestVerb verb, const csp::web::Uri& inUri, HttpPayload& payload, IHttpResponseHandler* responseCallback,
    csp::common::CancellationToken& cancellationToken, bool asyncResponse)
{
    auto* request = new csp::web::HttpRequest(this, verb, inUri, payload, responseCallback, cancellationToken, asyncResponse);

    if (m_logSystem != nullptr && m_logSystem->GetSystemLevel() == csp::common::LogLevel::VeryVerbose)
    {
        m_logSystem->LogMsg(csp::common::LogLevel::VeryVerbose, fmt::format("{}", *request).c_str());
    }

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
    AddRequest(request);
#endif
}

void WebClient::SetAuthContext(csp::common::IAuthContext& inAuthContext) { m_authContext = &inAuthContext; }

void WebClient::AddRequest(HttpRequest* request, [[maybe_unused]] std::chrono::milliseconds sendDelay)
{
    RefreshIfExpired();

    if (request)
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
        m_requestsMutex.lock();
        {
            m_requests.emplace(request);
        }
        m_requestsMutex.unlock();

        ++m_requestCount;
        request->IncRefCount();
        request->SetSendDelay(sendDelay);
        m_threadPool.Enqueue(
            [this, request](void*)
            {
                while (m_refreshStarted)
                {
                    std::this_thread::sleep_for(10ns);
                }

                if (m_refreshNeeded && !m_refreshStarted)
                {
                    m_refreshStarted = true;
                }

                request->RefreshAccessToken();

                ProcessRequest(request);

                return nullptr;
            });
#endif
    }
}

#ifndef CSP_WASM
void WebClient::ProcessResponses(const uint32_t maxNumResponses)
{
    uint32_t responseCount = 0;

    while ((m_pollRequests.IsEmpty() == false) && (responseCount < maxNumResponses))
    {
        auto pollRequest = m_pollRequests.Dequeue();

        HttpRequest* request = pollRequest.value();
        IHttpResponseHandler* callback = request->GetCallback();

        if (!request->Cancelled() && callback)
        {
            auto& response = request->GetMutableResponse();
            callback->OnHttpResponse(response);
        }

        DestroyRequest(request);

        // In case responses are being constantly queued from another
        // thread, make sure we don't keep polling forever
        ++responseCount;
    }
}

void WebClient::ProcessRequest(HttpRequest* request)
{
    if (request)
    {
        auto& payload = request->GetMutablePayload();
        payload.SetBearerToken();

        const std::chrono::milliseconds sendDelay = request->GetSendDelay();

        if (sendDelay > std::chrono::milliseconds(0))
        {
            // Wait before sending if required (e.g. for Retries)
            std::this_thread::sleep_for(sendDelay);
        }

        try
        {
            if (!request->Cancelled())
            {
                Send(*request);
            }
            else
            {
                request->SetRequestProgress(100.0f);
                request->SetResponseProgress(100.0f);
                request->SetResponseCode(EResponseCodes::ResponseRequestTimeout);
                std::string responseBody = "{\"errors\": {\"\": [\"Request was cancelled by user.\"]}}";
                request->SetResponseData(responseBody.c_str(), responseBody.length());
                request->EnableAutoRetry(false);
            }
        }
        catch (const WebClientException& ex)
        {
            CSP_LOG_MSG(csp::common::LogLevel::Error, ex.what());

            request->SetRequestProgress(100.0f);
            request->SetResponseCode(EResponseCodes::ResponseServiceUnavailable);
            std::string responseBody = "{\"errors\": {\"\": [\"Server could not be contacted. Please check your internet connection.\"]}}";
            request->SetResponseData(responseBody.c_str(), responseBody.length());
            request->SetResponseProgress(100.0f);
        }

        auto& response = request->GetMutableResponse();

        // Attempt Auto-retry if needed
        bool retryIssued = request->CheckForAutoRetry();

        if (request->GetCallback())
        {
            if (request->GetIsCallbackAsync())
            {
                if (!retryIssued)
                {
                    const uint16_t responseCode = static_cast<uint16_t>(response.GetResponseCode());
                    if (responseCode >= 400)
                    {
                        PrintClientErrorResponseMessages(response);
                    }

                    request->GetCallback()->OnHttpResponse(response);
                }

                DestroyRequest(request);
            }
            else
            {
                if (!retryIssued)
                {
                    const uint16_t responseCode = static_cast<uint16_t>(response.GetResponseCode());
                    if (responseCode >= 400)
                    {
                        PrintClientErrorResponseMessages(response);
                    }

                    // This request is marked to be polled, so add to the queue
                    // to be issued on the next call to WebClient::ProcessResponses()
                    m_pollRequests.Enqueue({ request });
                }
            }
        }
        else
        {
            // No callback, so just destroy the request
            DestroyRequest(request);
        }
    }
}

void WebClient::DestroyRequest(HttpRequest* request)
{
    m_requestsMutex.lock();
    {
        m_requests.erase(request);
    }
    m_requestsMutex.unlock();

    --m_requestCount;

    if (request->DecRefCount() == 0)
    {
        delete (request);
    }
}
#endif

// This makes an attempt to parse out errors from known JSON error response structures but will fallback to
// logging the full error response if it cannot find the structured errors.
void WebClient::PrintClientErrorResponseMessages(const HttpResponse& response)
{
    const uint16_t responseCode = static_cast<uint16_t>(response.GetResponseCode());
    const csp::common::String& responsePayload = response.GetPayload().GetContent();

    csp::common::String verb = "";
    switch (response.GetRequest()->GetVerb())
    {
    case ERequestVerb::Get:
        verb = "GET";
        break;
    case ERequestVerb::Post:
        verb = "POST";
        break;
    case ERequestVerb::Put:
        verb = "PUT";
        break;
    case ERequestVerb::Delete:
        verb = "DELETE";
        break;
    case ERequestVerb::Head:
        verb = "HEAD";
        break;
    case ERequestVerb::Patch:
        verb = "PATCH";
        break;
    default:
        break;
    }

    if (responsePayload.IsEmpty())
    {
        CSP_LOG_ERROR_FORMAT("Services request %s %s has returned a failed response (%i) but with no payload/error message.", verb.c_str(),
            response.GetRequest()->GetUri().GetAsString(), responseCode);
        return;
    }

    csp::common::List<csp::common::String> errors;

    if (response.GetPayload().IsJsonPayload())
    {
        rapidjson::Document responseJson;
        responseJson.Parse(responsePayload.c_str());

        // Since is is possible to have responses in various different structures this is extra cautious in
        // not assuming the structure and always checking before accessing any element. If there is doubt
        // the full response will be printed later.
        if (responseJson.HasMember("errors"))
        {
            if (responseJson["errors"].IsArray())
            {
                const auto& responseArray = responseJson["errors"].GetArray();

                for (uint32_t i = 0; i < responseArray.Size(); i++)
                {
                    if (responseArray[i].IsObject())
                    {
                        const auto& responseError = responseArray[i].GetObject();
                        if (responseError.HasMember("message") && responseError["message"].IsString())
                        {
                            errors.Append(responseError["message"].GetString());
                        }
                        else
                        {
                            errors.Append(JsonObjectToString(responseError));
                        }
                    }
                }
            }
            else
            {
                errors.Append(JsonObjectToString(responseJson["errors"]));
            }
        }
        else if (responseJson.HasMember("error"))
        {
            errors.Append(JsonObjectToString(responseJson["error"]));
        }
    }

    // If the response was not JSON or errors were not found as expected, log the full response payload.
    if (errors.Size() == 0)
    {
        CSP_LOG_ERROR_FORMAT("Services request %s %s has returned a failed response (%i) with payload/error message: %s", verb.c_str(),
            response.GetRequest()->GetUri().GetAsString(), responseCode, responsePayload.c_str());
    }
    else
    {
        for (size_t i = 0; i < errors.Size(); ++i)
        {
            CSP_LOG_ERROR_FORMAT("Services request %s %s has returned a failed response (%i) with payload/error message: %s", verb.c_str(),
                response.GetRequest()->GetUri().GetAsString(), responseCode, errors[i].c_str());
        }
    }
}

} // namespace csp::web
