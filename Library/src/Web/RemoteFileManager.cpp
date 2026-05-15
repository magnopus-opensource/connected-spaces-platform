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
#include "Web/RemoteFileManager.h"

#include "CSP/Common/Interfaces/IAuthContext.h"
#include "Common/Web/HttpAuth.h"
#include "Common/Web/HttpPayload.h"
#include "Common/Web/WebClient.h"

#include <fmt/format.h>

namespace
{
csp::common::Optional<csp::common::String> ConstructAuthorizationHeader(const csp::common::IAuthContext& inAuthContext)
{
    csp::common::Optional<csp::common::String> bearerToken;

    if (inAuthContext.GetLoginState().State == csp::common::ELoginState::LoggedIn)
    {
        csp::common::String accessToken = inAuthContext.GetLoginState().AccessToken;
        bearerToken = csp::common::String(fmt::format("Bearer {}", accessToken.c_str()).c_str());
    }

    return bearerToken;
}
} // namespace

namespace csp::web
{

RemoteFileManager::RemoteFileManager(csp::web::WebClient* inWebClient, const csp::common::IAuthContext& inAuthContext)
    : m_webClient(inWebClient)
    , m_authContext(inAuthContext)
{
}

RemoteFileManager::~RemoteFileManager() { }

void RemoteFileManager::GetFile(
    const csp::common::String& fileUrl, csp::services::ResponseHandlerPtr responseHandler, csp::common::CancellationToken& cancellationToken)
{
    csp::web::Uri getUri(fileUrl);

    csp::web::HttpPayload payload;
    payload.AddHeader(CSP_TEXT("Content-Type"), CSP_TEXT("text/json"));

    auto bearerToken = ConstructAuthorizationHeader(m_authContext);

    if (bearerToken.HasValue())
    {
        payload.AddHeader(CSP_TEXT("Authorization"), *bearerToken);
    }

    m_webClient->SendRequest(csp::web::ERequestVerb::GET, getUri, payload, responseHandler, cancellationToken);
}

void RemoteFileManager::GetResponseHeaders(const csp::common::String& url, csp::services::ResponseHandlerPtr responseHandler)
{
    csp::web::Uri getUri(url);
    csp::web::HttpPayload payload;

    auto bearerToken = ConstructAuthorizationHeader(m_authContext);

    if (bearerToken.HasValue())
    {
        payload.AddHeader(CSP_TEXT("Authorization"), *bearerToken);
    }

    m_webClient->SendRequest(csp::web::ERequestVerb::HEAD, getUri, payload, responseHandler, csp::common::CancellationToken::Dummy());
}

} // namespace csp::web
