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
#include "Web/HttpResponse.h"

namespace csp::web
{

HttpResponse::HttpResponse()
    : ResponseCode(EResponseCodes::ResponseNotFound)
    , Request(nullptr)
{
}

HttpResponse::HttpResponse(HttpRequest* InRequest)
    : ResponseCode(EResponseCodes::ResponseNotFound)
    , Request(InRequest)
{
}

HttpResponse::~HttpResponse() { }

void HttpResponse::SetResponseCode(EResponseCodes InReponseCode) { ResponseCode = InReponseCode; }

EResponseCodes HttpResponse::GetResponseCode() const { return ResponseCode; }

HttpPayload& HttpResponse::GetMutablePayload() { return Payload; }

const HttpPayload& HttpResponse::GetPayload() const { return Payload; }

HttpRequest* HttpResponse::GetRequest() const { return Request; }

void HttpResponse::Reset()
{
    ResponseCode = EResponseCodes::ResponseNotFound;
    Payload.Reset();
}

HttpProgress& HttpResponse::GetProgress() { return Progress; }

const HttpProgress& HttpResponse::GetProgress() const { return Progress; }

} // namespace csp::web
