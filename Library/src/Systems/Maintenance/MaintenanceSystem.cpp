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
#include "CallHelpers.h"
#include "Services/ApiBase/ApiBase.h"
#include "Systems/ResultHelpers.h"
#include "Web/MaintenanceApi/MaintenanceApi.h"


namespace chs = csp::systems::maintenanceservice;


namespace csp::systems
{

MaintenanceSystem::MaintenanceSystem() : SystemBase(), MaintenanceAPI(nullptr)
{
	Running = true;
}

MaintenanceSystem::MaintenanceSystem(csp::web::WebClient* InWebClient) : SystemBase(InWebClient)
{
	MaintenanceAPI = CSP_NEW chs::MaintenanceApi(InWebClient);
	Running		   = true;
}

MaintenanceSystem::~MaintenanceSystem()
{
	CSP_DELETE(MaintenanceAPI);
}

void MaintenanceSystem::GetMaintenanceInfo(MaintenanceInfoCallback Callback)
{
	if (Running == true)
	{
		const MaintenanceInfoCallback GetMaintenanceInfoCallback = [this, Callback](const MaintenanceInfoResult& Result)
		{
			if (Result.GetResultCode() == csp::systems::EResultCode::InProgress)
			{
				return;
			}

			if (Result.GetResultCode() == csp::systems::EResultCode::Failed)
			{
				MaintenanceInfoResult res(csp::systems::EResultCode::Failed, Result.GetHttpResultCode());
				Running = false;
				INVOKE_IF_NOT_NULL(Callback, res);

				return;
			}

			INVOKE_IF_NOT_NULL(Callback, Result);
		};
		csp::services::ResponseHandlerPtr MaintenanceResponseHandler
			= MaintenanceAPI->CreateHandler<MaintenanceInfoCallback, MaintenanceInfoResult, void, csp::services::NullDto>(GetMaintenanceInfoCallback,
																														  nullptr);
		static_cast<chs::MaintenanceApi*>(MaintenanceAPI)
			->Query(csp::CSPFoundation::GetClientUserAgentInfo().MaintainanceWindowURI, MaintenanceResponseHandler);
	}
}

} // namespace csp::systems
