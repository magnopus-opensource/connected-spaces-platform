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
#include "Web/MaintenanceApi/MaintenanceApi.h"

#include "CSP/CSPFoundation.h"
#include "CSP/Common/StringFormat.h"
#include "Web/HttpPayload.h"
#include "Web/WebClient.h"

namespace csp::systems::maintenanceservice
{
MaintenanceApi::MaintenanceApi(csp::web::WebClient* InWebClient)
    : ApiBase(InWebClient, &csp::CSPFoundation::GetEndpoints().AggregationServiceURI)
{
}

MaintenanceApi::~MaintenanceApi() { }

void MaintenanceApi::Query(const csp::common::String& MaintenanceURL, csp::services::ApiResponseHandlerBase* ResponseHandler,
    csp::common::CancellationToken& CancellationToken) const
{

    std::string MaintenanceURLLower = std::string(MaintenanceURL.c_str());
    csp::web::Uri Uri = csp::web::Uri(MaintenanceURLLower.c_str());

    csp::web::HttpPayload Payload;
    Payload.AddHeader(CSP_TEXT("Content-Type"), CSP_TEXT("application/octet-stream"));
    WebClient->SendRequest(csp::web::ERequestVerb::GET, Uri, Payload, ResponseHandler, CancellationToken);
}
} // namespace csp::systems::maintenanceservice
