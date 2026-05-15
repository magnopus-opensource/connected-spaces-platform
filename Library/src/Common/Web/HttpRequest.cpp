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
#include "Common/Web/HttpRequest.h"

#include "Common/Web/WebClient.h"

namespace csp::web
{

HttpRequest::HttpRequest(WebClient* inClient, ERequestVerb inVerb, const csp::web::Uri& inUri, HttpPayload& inPayload,
    IHttpResponseHandler* responseCallback, csp::common::CancellationToken& cancellationToken, bool callbackIsAsync)
    : m_client(inClient)
    , m_verb(inVerb)
    , m_uri(inUri)
    , m_payload(inPayload)
    , m_callback(responseCallback)
    , m_response(this)
    , m_isCallbackAsync(callbackIsAsync)
    , m_isAutoRetryEnabled(true)
    , m_retryCount(0)
    , m_refCount(0)
    , m_sendDelay(0)
{
    if (&cancellationToken == &csp::common::CancellationToken::Dummy())
    {
        this->m_cancellationToken = new csp::common::CancellationToken();
        m_ownsCancellationToken = true;
    }
    else
    {
        this->m_cancellationToken = &cancellationToken;
        m_ownsCancellationToken = false;
    }
}

HttpRequest::~HttpRequest()
{
    if (m_ownsCancellationToken)
    {
        delete (m_cancellationToken);
    }

    if ((m_callback != nullptr) && m_callback->ShouldDelete())
    {
        delete (m_callback);
    }
}

ERequestVerb HttpRequest::GetVerb() const { return m_verb; }

const csp::web::Uri& HttpRequest::GetUri() const { return m_uri; }

HttpPayload& HttpRequest::GetMutablePayload() { return m_payload; }

const HttpPayload& HttpRequest::GetPayload() const { return m_payload; }

HttpResponse& HttpRequest::GetMutableResponse() { return m_response; }

const HttpResponse& HttpRequest::GetResponse() const { return m_response; }

IHttpResponseHandler* HttpRequest::GetCallback() const { return m_callback; }

bool HttpRequest::GetIsCallbackAsync() const { return m_isCallbackAsync; }

void HttpRequest::SetResponseCode(EResponseCodes inReponseCode) { m_response.SetResponseCode(inReponseCode); }

void HttpRequest::SetResponseData(const char* data, size_t dataLength) { m_response.GetMutablePayload().SetContent(data, dataLength); }

void HttpRequest::AllocateResponseData(size_t dataLength) { m_response.GetMutablePayload().AllocateContent(dataLength); }

void HttpRequest::WriteResponseData(size_t offset, const char* data, size_t dataLength)
{
    m_response.GetMutablePayload().WriteContent(offset, data, dataLength);
}

void HttpRequest::SetResponseProgress(float reponseProgress)
{
    m_response.GetProgress().SetProgressPercentage(reponseProgress);

    if (m_callback)
    {
        m_callback->OnHttpProgress(*this);
    }
}

float HttpRequest::GetRequestProgressPercentage() const { return m_progress.GetProgressPercentage(); }

float HttpRequest::GetResponseProgressPercentage() const { return m_response.GetProgress().GetProgressPercentage(); }

void HttpRequest::SetRequestProgress(float inProgress)
{
    m_progress.SetProgressPercentage(inProgress);

    if (m_callback)
    {
        m_callback->OnHttpProgress(*this);
    }
}

HttpProgress& HttpRequest::GetProgress() { return m_progress; }

bool ResultCodeValidForRetry(csp::web::EResponseCodes status)
{
    return (status == csp::web::EResponseCodes::ResponseTooManyRequests // 429
        || status == csp::web::EResponseCodes::ResponseRequestTimeout // 408
        || static_cast<int>(status) >= 500 // 500
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
bool HttpRequest::Retry(const uint32_t maxRetries)
{
    if (ResultCodeValidForRetry(m_response.GetResponseCode()) && m_retryCount < maxRetries)
    {
        ++m_retryCount;

        // Re-issue the request
        m_client->AddRequest(this, std::chrono::milliseconds(DefaultRetriesDelayInMs));

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
bool HttpRequest::CheckForAutoRetry(const uint32_t maxRetries)
{
    bool retryIssued = false;
    EResponseCodes errorCodeValue = GetResponse().GetResponseCode();

    if (m_isAutoRetryEnabled && (errorCodeValue != EResponseCodes::ResponseOK) && (errorCodeValue != EResponseCodes::ResponseCreated)
        && (errorCodeValue != EResponseCodes::ResponseNoContent))
    {
        retryIssued = Retry(maxRetries);
    }

    return retryIssued;
}

void HttpRequest::IncRefCount() { ++m_refCount; }

uint32_t HttpRequest::DecRefCount() { return --m_refCount; }

uint32_t HttpRequest::GetRefCount() const { return m_refCount; }

uint32_t HttpRequest::GetRetryCount() const { return m_retryCount; }

void HttpRequest::SetSendDelay(const std::chrono::milliseconds inSendDelay) { m_sendDelay = inSendDelay; }

std::chrono::milliseconds HttpRequest::GetSendDelay() { return m_sendDelay; }

void HttpRequest::EnableAutoRetry(bool enable) { m_isAutoRetryEnabled = enable; }

void HttpRequest::Cancel() { m_cancellationToken->Cancel(); }

bool HttpRequest::Cancelled() { return m_cancellationToken->Cancelled(); }

void HttpRequest::RefreshAccessToken() { m_payload.RefreshBearerToken(); }

} // namespace csp::web
