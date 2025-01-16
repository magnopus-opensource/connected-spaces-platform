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
#include "Web/HttpRequest.h"

#include "Memory/Memory.h"
#include "Web/WebClient.h"

namespace csp::web
{

HttpRequest::HttpRequest(WebClient* InClient, ERequestVerb InVerb, const csp::web::Uri& InUri, HttpPayload& InPayload,
    IHttpResponseHandler* ResponseCallback, csp::common::CancellationToken& CancellationToken, bool CallbackIsAsync)
    : Client(InClient)
    , Verb(InVerb)
    , Uri(InUri)
    , Payload(InPayload)
    , Callback(ResponseCallback)
    , Response(this)
    , IsCallbackAsync(CallbackIsAsync)
    , IsAutoRetryEnabled(true)
    , RetryCount(0)
    , RefCount(0)
    , SendDelay(0)
{
    if (&CancellationToken == &csp::common::CancellationToken::Dummy())
    {
        this->CancellationToken = CSP_NEW csp::common::CancellationToken();
        OwnsCancellationToken = true;
    }
    else
    {
        this->CancellationToken = &CancellationToken;
        OwnsCancellationToken = false;
    }
}

HttpRequest::~HttpRequest()
{
    if (OwnsCancellationToken)
    {
        CSP_DELETE(CancellationToken);
    }

    if ((Callback != nullptr) && Callback->ShouldDelete())
    {
        CSP_DELETE(Callback);
    }
}

const ERequestVerb HttpRequest::GetVerb() const { return Verb; }

const csp::web::Uri& HttpRequest::GetUri() const { return Uri; }

HttpPayload& HttpRequest::GetMutablePayload() { return Payload; }

const HttpPayload& HttpRequest::GetPayload() const { return Payload; }

HttpResponse& HttpRequest::GetMutableResponse() { return Response; }

const HttpResponse& HttpRequest::GetResponse() const { return Response; }

IHttpResponseHandler* HttpRequest::GetCallback() const { return Callback; }

bool HttpRequest::GetIsCallbackAsync() const { return IsCallbackAsync; }

void HttpRequest::SetResponseCode(EResponseCodes InReponseCode) { Response.SetResponseCode(InReponseCode); }

void HttpRequest::SetResponseData(const char* Data, size_t DataLength) { Response.GetMutablePayload().SetContent(Data, DataLength); }

void HttpRequest::AllocateResponseData(size_t DataLength) { Response.GetMutablePayload().AllocateContent(DataLength); }

void HttpRequest::WriteResponseData(size_t Offset, const char* Data, size_t DataLength)
{
    Response.GetMutablePayload().WriteContent(Offset, Data, DataLength);
}

void HttpRequest::SetResponseProgress(float Progress)
{
    Response.GetProgress().SetProgressPercentage(Progress);

    if (Callback)
    {
        Callback->OnHttpProgress(*this);
    }
}

float HttpRequest::GetRequestProgressPercentage() const { return Progress.GetProgressPercentage(); }

float HttpRequest::GetResponseProgressPercentage() const { return Response.GetProgress().GetProgressPercentage(); }

void HttpRequest::SetRequestProgress(float InProgress)
{
    Progress.SetProgressPercentage(InProgress);

    if (Callback)
    {
        Callback->OnHttpProgress(*this);
    }
}

HttpProgress& HttpRequest::GetProgress() { return Progress; }

bool ResultCodeValidForRetry(csp::web::EResponseCodes Status)
{
    return (Status == csp::web::EResponseCodes::ResponseTooManyRequests // 429
        || Status == csp::web::EResponseCodes::ResponseRequestTimeout // 408
        || static_cast<int>(Status) >= 500 // 500
    );
}

/// @brief Retry this request
///
/// Issue this request again up to MaxRetries.
/// Note that we may want to make this more sophisticated in the future
/// with better retry algorithms, but lets start simple for now
///
/// @param MaxRetries Maximum number of times to retry before giving up
/// @return true if retry succeeded, false if retry limit was reached
bool HttpRequest::Retry(const uint32_t MaxRetries)
{
    if (ResultCodeValidForRetry(Response.GetResponseCode()) && RetryCount < MaxRetries)
    {
        ++RetryCount;

        // Re-issue the request
        Client->AddRequest(this, std::chrono::milliseconds(DefaultRetriesDelayInMs));

        return true;
    }

    return false;
}

/// @brief Auto retry if we get a ServiceUnavailable (503) response
///
/// Auto retry the response if we get a 503 response.  Note that ideally we
/// would wait for a bit here, and also probably some kind of back-off
/// We may also want to support Retry-After headers in responses
///
/// @param MaxRetries Maximum number of times to retry before giving up
/// @return true if we retied the request or false id not
bool HttpRequest::CheckForAutoRetry(const uint32_t MaxRetries)
{
    bool RetryIssued = false;
    EResponseCodes ErrorCodeValue = GetResponse().GetResponseCode();

    if (IsAutoRetryEnabled && (ErrorCodeValue != EResponseCodes::ResponseOK) && (ErrorCodeValue != EResponseCodes::ResponseCreated)
        && (ErrorCodeValue != EResponseCodes::ResponseNoContent))
    {
        RetryIssued = Retry(MaxRetries);
    }

    return RetryIssued;
}

void HttpRequest::IncRefCount() { ++RefCount; }

uint32_t HttpRequest::DecRefCount() { return --RefCount; }

uint32_t HttpRequest::GetRefCount() const { return RefCount; }

uint32_t HttpRequest::GetRetryCount() const { return RetryCount; }

void HttpRequest::SetSendDelay(const std::chrono::milliseconds InSendDelay) { SendDelay = InSendDelay; }

const std::chrono::milliseconds HttpRequest::GetSendDelay() { return SendDelay; }

void HttpRequest::EnableAutoRetry(bool Enable) { IsAutoRetryEnabled = Enable; }

void HttpRequest::Cancel() { CancellationToken->Cancel(); }

bool HttpRequest::Cancelled() { return CancellationToken->Cancelled(); }

void HttpRequest::RefreshAccessToken() { Payload.RefreshBearerToken(); }

} // namespace csp::web
