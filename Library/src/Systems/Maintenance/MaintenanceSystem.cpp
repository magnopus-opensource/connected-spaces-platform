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
#include "CSP/Systems/Maintenance/MaintenanceSystem.h"

#include "CSP/Systems/Users/UserSystem.h"
#include "Services/ApiBase/ApiBase.h"
#include "Web/MaintenanceApi/MaintenanceApi.h"



namespace chs = csp::systems::maintenanceservice;

namespace csp::systems
{

MaintenanceSystem::MaintenanceSystem() : SystemBase(), MaintenanceAPI(nullptr)
{
}

MaintenanceSystem::MaintenanceSystem(csp::web::WebClient* InWebClient) : SystemBase(InWebClient)
{
	MaintenanceAPI = CSP_NEW chs::MaintenanceApi(InWebClient);
}

MaintenanceSystem::~MaintenanceSystem()
{
	CSP_DELETE(MaintenanceAPI);
}

void MaintenanceSystem::GetMaintenanceInfo(MaintenanceInfoCallback Callback)
{
	const MaintenanceInfoCallback GetMaintenanceInfoCallback = [=](const MaintenanceInfoResult& Result)
	{
		if (Result.GetResultCode() == csp::systems::EResultCode::Success)
		{
			Callback(Result);
		}
		else if (Result.GetResultCode() == csp::systems::EResultCode::Failed)
		{
			Callback(MaintenanceInfoResult::Invalid());
		}
	};

	csp::services::ResponseHandlerPtr MaintenanceResponseHandler
		= MaintenanceAPI->CreateHandler<MaintenanceInfoCallback, MaintenanceInfoResult, void, csp::services::NullDto>(GetMaintenanceInfoCallback,
																													  nullptr);
	static_cast<chs::MaintenanceApi*>(MaintenanceAPI)->Query(csp::CSPFoundation::GetClientUserAgentInfo().CHSEnvironment, MaintenanceResponseHandler);
}

} // namespace csp::systems
