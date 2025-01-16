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
#pragma once

#include "CSP/Common/CancellationToken.h"
#include "Common/Queue.h"
#include "HttpAuth.h"
#include "HttpRequest.h"
#include "Uri.h"

#ifndef CSP_WASM
#include "Common/ThreadPool.h"
#endif

#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_set>

namespace csp::systems
{

class LoginState;
class SystemsManager;
class UserSystem;

} // namespace csp::systems

namespace csp::web
{

/**
                @defgroup   web Web Client
                @brief      Platform independent web client abstraction
 */

/**
                @namespace  csp::web
                @brief      Platform independent web client abstraction
                @details    Abstracts web requests and their responses from underlying platform implementation
 */

/// Maximum concurrent requests supported by the Web Request system
constexpr int CSP_MAX_CONCURRENT_REQUESTS = 4;

using Port = uint32_t;

enum class ETransferProtocol : uint8_t
{
    HTTP,
    HTTPS
};

/// @addtogroup web
/// @brief Web Client Base Class
///
/// Provides common functionality for all Web Request clients by abstracting requests and their responses and
/// provides methods of aynchronous or synchronous callbacks for the responses
///
class WebClient
{
    friend class HttpRequest;
    friend class csp::systems::SystemsManager;

public:
    WebClient(const Port InPort, const ETransferProtocol Tp, bool AutoRefresh = true);
    virtual ~WebClient();

    /// @brief Main method for sending a Http Request
    /// @param Verb Request type (GET, PUT, POST, DELETE)
    /// @param InUri The Uri of the request relative to the base Uri
    /// @param Payload Headers and body content
    /// @param ResponseCallback Pointer to callback for the response
    /// @param AsyncResponse Flag to indicate if the response should be issued asynchronously as soon as it's received
    void SendRequest(ERequestVerb Verb, const csp::web::Uri& InUri, HttpPayload& Payload, IHttpResponseHandler* ResponseCallback,
        csp::common::CancellationToken& CancellationToken, bool AsyncResponse = true);

#ifndef CSP_WASM
    /// @brief Manually poll for responses that have been flagged as non-async
    /// @param MaxNumResponses Maximum number of responses to process in this call
    void ProcessResponses(const uint32_t MaxNumResponses = 64);
#endif

    virtual std::string MD5Hash(const void* Data, const size_t Size) = 0;
    virtual void SetFileUploadContentFromFile(HttpPayload* Payload, const char* FilePath, const char* Version, const csp::common::String& MediaType)
        = 0;
    virtual void SetFileUploadContentFromString(HttpPayload* Payload, const csp::common::String& StringSource, const csp::common::String& FileName,
        const char* Version, const csp::common::String& MediaType)
        = 0;
    virtual void SetFileUploadContentFromBuffer(HttpPayload* Payload, const char* Buffer, size_t BufferLength, const csp::common::String& FileName,
        const char* Version, const csp::common::String& MediaType)
        = 0;

protected:
    /// @brief Send a http request
    /// @param Request Details of the web request headers and payload
    /// @return Response code and payload
    virtual void Send(HttpRequest& Request) = 0;

    const Port RootPort;

private:
    void AddRequest(HttpRequest* Request, std::chrono::milliseconds SendDelay = std::chrono::milliseconds(0));
    void RefreshIfExpired();
    void PrintClientErrorResponseMessages(const HttpResponse& Response);
    csp::systems::UserSystem* UserSystem;
    const csp::systems::LoginState* LoginState;
    std::atomic_bool RefreshNeeded, RefreshStarted;
    bool AutoRefreshEnabled;

#ifdef CSP_WASM
    csp::Queue<HttpRequest*> WasmRequests;
    std::mutex WasmRequestsMutex;
#else
    void ProcessRequest(HttpRequest* Request);
    void DestroyRequest(HttpRequest* Request);

    std::atomic_uint32_t RequestCount;
    csp::ThreadPool ThreadPool;
    csp::Queue<HttpRequest*> PollRequests;
    std::unordered_set<HttpRequest*> Requests;
    std::mutex RequestsMutex;
#endif
};

class WebClientException : public std::runtime_error
{
public:
    WebClientException(const char* what)
        : std::runtime_error(what)
    {
    }

    WebClientException(const std::string& what)
        : std::runtime_error(what)
    {
    }
};

} // namespace csp::web
