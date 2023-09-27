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
MaintenanceApi::MaintenanceApi(csp::web::WebClient* InWebClient) : ApiBase(InWebClient, &csp::CSPFoundation::GetEndpoints().AggregationServiceURI)
{
}

MaintenanceApi::~MaintenanceApi()
{
}

void MaintenanceApi::Query(csp::common::String CHSEnvironment,
						   csp::services::ApiResponseHandlerBase* ResponseHandler,
						   csp::common::CancellationToken& CancellationToken) const
{

	// S3 bucket URLS are case Sensitive so the CHS Environment name is set to lower case.
	std::string CHSEnvironmentLower = std::string(CHSEnvironment.c_str());
	std::transform(CHSEnvironmentLower.begin(), CHSEnvironmentLower.end(), CHSEnvironmentLower.begin(), ::tolower);
	csp::web::Uri Uri;

	if (CHSEnvironmentLower != "oprod")
	{
		Uri = csp::web::Uri(
			csp::common::StringFormat("https://maintenance-windows.magnoboard.com/%s/maintenance-windows.json", CHSEnvironmentLower.c_str()).c_str());
	}
	else
	{
		Uri = csp::web::Uri(
			csp::common::StringFormat("https://maintenance-windows.magnolympus.com/%s/maintenance-windows.json", CHSEnvironmentLower.c_str())
				.c_str());
	}

	csp::web::HttpPayload Payload;
	Payload.AddHeader(CSP_TEXT("Content-Type"), CSP_TEXT("application/octet-stream"));
	WebClient->SendRequest(csp::web::ERequestVerb::GET, Uri, Payload, ResponseHandler, CancellationToken);
}
} // namespace csp::systems::maintenanceservice
