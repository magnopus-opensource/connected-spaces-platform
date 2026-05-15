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
#include "Debug/Logging.h"
#include "Services/ApiBase/ApiBase.h"
#include "Systems/ResultHelpers.h"
#include "Web/MaintenanceApi/MaintenanceApi.h"

namespace chs = csp::systems::maintenanceservice;

namespace csp::systems
{

MaintenanceSystem::MaintenanceSystem()
    : SystemBase(nullptr, nullptr, nullptr)
    , m_maintenanceApi(nullptr)
{
    m_allowMaintenanceInfoRequests = true;
}

MaintenanceSystem::MaintenanceSystem(csp::web::WebClient* inWebClient, csp::common::LogSystem& logSystem)
    : SystemBase(inWebClient, nullptr, &logSystem)
{
    m_maintenanceApi = new chs::MaintenanceApi(inWebClient);
    m_allowMaintenanceInfoRequests = true;
}

MaintenanceSystem::~MaintenanceSystem() { delete (m_maintenanceApi); }

void MaintenanceSystem::GetMaintenanceInfo(const csp::common::String& maintenanceUrl, MaintenanceInfoCallback callback)
{
    if (m_allowMaintenanceInfoRequests == true)
    {
        const MaintenanceInfoCallback getMaintenanceInfoCallback = [this, callback](const MaintenanceInfoResult& result)
        {
            if (result.GetResultCode() == csp::systems::EResultCode::InProgress)
            {
                return;
            }

            if (result.GetResultCode() == csp::systems::EResultCode::Failed)
            {
                MaintenanceInfoResult res(csp::systems::EResultCode::Failed, result.GetHttpResultCode());
                m_allowMaintenanceInfoRequests = false;
                CSP_LOG_ERROR_MSG("Failed to Get Maintenance Information: Maintenance system disabled. Check Maintenance URL, and restart");
                INVOKE_IF_NOT_NULL(callback, res);

                return;
            }

            INVOKE_IF_NOT_NULL(callback, result);
        };
        csp::services::ResponseHandlerPtr maintenanceResponseHandler
            = m_maintenanceApi->CreateHandler<MaintenanceInfoCallback, MaintenanceInfoResult, void, csp::services::NullDto>(
                getMaintenanceInfoCallback, nullptr);
        static_cast<chs::MaintenanceApi*>(m_maintenanceApi)->Query(maintenanceUrl, maintenanceResponseHandler);
    }
}

} // namespace csp::systems
