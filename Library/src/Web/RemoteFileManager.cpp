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

#include "Web/HttpAuth.h"
#include "Web/HttpPayload.h"
#include "Web/WebClient.h"

namespace csp::web
{

RemoteFileManager::RemoteFileManager(csp::web::WebClient* InWebClient)
    : WebClient(InWebClient)
{
}

RemoteFileManager::~RemoteFileManager() { }

void RemoteFileManager::GetFile(
    const csp::common::String& FileUrl, csp::services::ResponseHandlerPtr ResponseHandler, csp::common::CancellationToken& CancellationToken)
{
    csp::web::Uri GetUri(FileUrl);

    csp::web::HttpPayload Payload;
    Payload.AddHeader(CSP_TEXT("Content-Type"), CSP_TEXT("text/json"));

    WebClient->SendRequest(csp::web::ERequestVerb::GET, GetUri, Payload, ResponseHandler, CancellationToken);
}

void RemoteFileManager::GetResponseHeaders(const csp::common::String& Url, csp::services::ResponseHandlerPtr ResponseHandler)
{
    csp::web::Uri GetUri(Url);
    csp::web::HttpPayload Payload;

    WebClient->SendRequest(csp::web::ERequestVerb::HEAD, GetUri, Payload, ResponseHandler, csp::common::CancellationToken::Dummy());
}

} // namespace csp::web
