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
#include "HttpPayload.h"
#include "HttpProgress.h"
#include "HttpResponse.h"
#include "Uri.h"

#include <atomic>
#include <chrono>

namespace csp::web
{

class WebClient;

// In some cases ERequestVerb::DELETE can clash with a win32 macro.
// To solve this, the following steps are taken:
//
// - store the current value of the macro
// - undefine the current definition
// - declare our enum
// - redefine the macro
#pragma push_macro("DELETE")
#undef DELETE
enum class ERequestVerb : uint8_t
{
    Get = 0,
    Put = 1,
    Post = 2,
    Delete = 3,
    Head = 4,
    Patch = 5,

    GET = Get,
    PUT = Put,
    POST = Post,
    DELETE = Delete,
    HEAD = Head,
    PATCH = Patch
};

#pragma pop_macro("DELETE")

constexpr uint32_t DefaultNumRequestRetries = 4;
constexpr uint32_t DefaultRetriesDelayInMs = 100;

class HttpRequest
{
public:
    HttpRequest(WebClient* inClient, ERequestVerb inVerb, const csp::web::Uri& inUri, HttpPayload& inPayload, IHttpResponseHandler* responseCallback,
        csp::common::CancellationToken& cancellationToken, bool callbackIsAsync = true);
    ~HttpRequest();

    ERequestVerb GetVerb() const;
    const csp::web::Uri& GetUri() const;
    HttpPayload& GetMutablePayload();
    const HttpPayload& GetPayload() const;
    HttpResponse& GetMutableResponse();
    const HttpResponse& GetResponse() const;

    IHttpResponseHandler* GetCallback() const;
    bool GetIsCallbackAsync() const;

    void SetResponseCode(EResponseCodes inReponseCode);
    void SetResponseData(const char* data, size_t dataLength);

    void AllocateResponseData(size_t dataLength);
    void WriteResponseData(size_t offset, const char* data, size_t dataLength);
    void SetResponseProgress(float progress);
    void SetRequestProgress(float progress);
    float GetRequestProgressPercentage() const;
    float GetResponseProgressPercentage() const;
    HttpProgress& GetProgress();

    bool CheckForAutoRetry(const uint32_t maxRetries = csp::web::DefaultNumRequestRetries);
    bool Retry(const uint32_t maxRetries = csp::web::DefaultNumRequestRetries);

    void EnableAutoRetry(bool enable);

    void IncRefCount();
    uint32_t DecRefCount();
    uint32_t GetRefCount() const;
    uint32_t GetRetryCount() const;

    void SetSendDelay(const std::chrono::milliseconds inSendDelay);
    std::chrono::milliseconds GetSendDelay();

    void Cancel();
    bool Cancelled();

    void RefreshAccessToken();

private:
    WebClient* m_client;

    ERequestVerb m_verb;
    csp::web::Uri m_uri;
    HttpPayload m_payload;

    IHttpResponseHandler* m_callback;
    HttpResponse m_response;

    bool m_isCallbackAsync;
    bool m_isAutoRetryEnabled;
    uint32_t m_retryCount;
    std::atomic_uint32_t m_refCount;

    std::chrono::milliseconds m_sendDelay;

    HttpProgress m_progress;
    csp::common::CancellationToken* m_cancellationToken;
    bool m_ownsCancellationToken;
};

} // namespace csp::web
