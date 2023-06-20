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
	csp::services::ResponseHandlerPtr MaintenanceResponseHandler
		= MaintenanceAPI->CreateHandler<MaintenanceInfoCallback, MaintenanceInfoResult, void, csp::services::NullDto>(Callback, nullptr);
	static_cast<chs::MaintenanceApi*>(MaintenanceAPI)->Query(csp::CSPFoundation::GetClientUserAgentInfo().CHSEnvironment, MaintenanceResponseHandler);
}

void MaintenanceSystem::IsInsideMaintenanceWindow(BooleanResultCallback Callback)
{
	MaintenanceInfoCallback InternalCallback = [Callback](MaintenanceInfoResult& Result)
	{
		BooleanResult InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());
		if (Result.GetResultCode() == csp::services::EResultCode::Success)
		{
			const auto TimeNow				 = csp::common::DateTime::UtcTimeNow().GetTimePoint();
			const auto LatestMaintenanceInfo = Result.GetLatestMaintenanceInfo();
			if (csp::common::DateTime(LatestMaintenanceInfo.StartDateTimestamp).GetTimePoint() <= TimeNow
				&& csp::common::DateTime(LatestMaintenanceInfo.EndDateTimestamp).GetTimePoint() >= TimeNow)
			{
				InternalResult.SetValue(true);
			}
			else
			{
				InternalResult.SetValue(false);
			}
		}
		Callback(InternalResult);
	};

	GetMaintenanceInfo(InternalCallback);
}

} // namespace csp::systems
