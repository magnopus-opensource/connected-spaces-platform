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
#include "Common/DateTime.h"
#include "Common/Web/WebClient.h"
#include "Services/PrototypeService/AssetFileDto.h"

namespace csp::web
{

class RemoteFileManager
{
public:
    RemoteFileManager(csp::web::WebClient* InWebClient);
    ~RemoteFileManager();

    void GetFile(
        const csp::common::String& FileUrl, csp::services::ResponseHandlerPtr ResponseHandler, csp::common::CancellationToken& CancellationToken);
    void GetResponseHeaders(const csp::common::String& Url, csp::services::ResponseHandlerPtr ResponseHandler);

private:
    csp::web::WebClient* WebClient;
};

} // namespace csp::web
