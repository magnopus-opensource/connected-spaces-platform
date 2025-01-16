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

#include "CSP/Web/HTTPResponseCodes.h"
#include "HttpPayload.h"
#include "HttpProgress.h"

namespace csp::web
{

class HttpRequest;
class HttpResponse;

struct IHttpResponseHandler
{
    virtual void OnHttpProgress(HttpRequest& Request) {};
    virtual void OnHttpResponse(HttpResponse& Response) = 0;
    virtual bool ShouldDelete() const { return false; }

    virtual ~IHttpResponseHandler() = default;
};

class HttpResponse
{
public:
    HttpResponse();
    HttpResponse(HttpRequest* InRequest);
    ~HttpResponse();

    void SetResponseCode(EResponseCodes InReponseCode);
    EResponseCodes GetResponseCode() const;

    HttpPayload& GetMutablePayload();
    const HttpPayload& GetPayload() const;

    HttpRequest* GetRequest() const;

    void Reset();

    HttpProgress& GetProgress();
    const HttpProgress& GetProgress() const;

private:
    EResponseCodes ResponseCode;

    HttpRequest* Request;
    HttpPayload Payload;

    HttpProgress Progress;
};

} // namespace csp::web
