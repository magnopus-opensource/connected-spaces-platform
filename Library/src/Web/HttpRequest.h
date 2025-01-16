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

    GET = Get,
    PUT = Put,
    POST = Post,
    DELETE = Delete,
    HEAD = Head
};

#pragma pop_macro("DELETE")

constexpr uint32_t DefaultNumRequestRetries = 4;
constexpr uint32_t DefaultRetriesDelayInMs = 100;

class HttpRequest
{
public:
    HttpRequest(WebClient* InClient, ERequestVerb InVerb, const csp::web::Uri& InUri, HttpPayload& InPayload, IHttpResponseHandler* ResponseCallback,
        csp::common::CancellationToken& CancellationToken, bool CallbackIsAsync = true);
    ~HttpRequest();

    const ERequestVerb GetVerb() const;
    const csp::web::Uri& GetUri() const;
    HttpPayload& GetMutablePayload();
    const HttpPayload& GetPayload() const;
    HttpResponse& GetMutableResponse();
    const HttpResponse& GetResponse() const;

    IHttpResponseHandler* GetCallback() const;
    bool GetIsCallbackAsync() const;

    void SetResponseCode(EResponseCodes InReponseCode);
    void SetResponseData(const char* Data, size_t DataLength);

    void AllocateResponseData(size_t DataLength);
    void WriteResponseData(size_t Offset, const char* Data, size_t DataLength);
    void SetResponseProgress(float Progress);
    void SetRequestProgress(float Progress);
    float GetRequestProgressPercentage() const;
    float GetResponseProgressPercentage() const;
    HttpProgress& GetProgress();

    bool CheckForAutoRetry(const uint32_t MaxRetries = csp::web::DefaultNumRequestRetries);
    bool Retry(const uint32_t MaxRetries = csp::web::DefaultNumRequestRetries);

    void EnableAutoRetry(bool Enable);

    void IncRefCount();
    uint32_t DecRefCount();
    uint32_t GetRefCount() const;
    uint32_t GetRetryCount() const;

    void SetSendDelay(const std::chrono::milliseconds InSendDelay);
    const std::chrono::milliseconds GetSendDelay();

    void Cancel();
    bool Cancelled();

    void RefreshAccessToken();

private:
    WebClient* Client;

    ERequestVerb Verb;
    csp::web::Uri Uri;
    HttpPayload Payload;

    IHttpResponseHandler* Callback;
    HttpResponse Response;

    bool IsCallbackAsync;
    bool IsAutoRetryEnabled;
    uint32_t RetryCount;
    std::atomic_uint32_t RefCount;

    std::chrono::milliseconds SendDelay;

    HttpProgress Progress;
    csp::common::CancellationToken* CancellationToken;
    bool OwnsCancellationToken;
};

} // namespace csp::web
