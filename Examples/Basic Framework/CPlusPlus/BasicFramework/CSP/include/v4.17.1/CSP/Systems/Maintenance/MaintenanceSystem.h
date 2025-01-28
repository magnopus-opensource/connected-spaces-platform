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

#include "CSP/Systems/Maintenance/Maintenance.h"
#include "CSP/Systems/Spaces/Space.h"
#include "CSP/Systems/SystemBase.h"
#include "CSP/Systems/SystemsResult.h"

namespace csp::services
{

class ApiBase;

}

namespace csp::web
{

class WebClient;

}

namespace csp::systems
{

/// @ingroup Maintenance System
/// @brief Public facing system that allows interfacing with the Maintenance Window Server.
/// This system can be used to query if there is currently a planned outage
/// and can also be used to check for up coming maintenances outages
class CSP_API CSP_NO_DISPOSE MaintenanceSystem : public SystemBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class SystemsManager;
    /** @endcond */

public:
    ~MaintenanceSystem();

    /// @brief Receives information on planned maintenances outages schedules for the future
    /// @param Callback MaintenanceInfoCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetMaintenanceInfo(MaintenanceInfoCallback Callback);

private:
    MaintenanceSystem(); // This constructor is only provided to appease the wrapper generator and should not be used
    CSP_NO_EXPORT MaintenanceSystem(csp::web::WebClient* InWebClient);

    csp::services::ApiBase* MaintenanceAPI;
};

} // namespace csp::systems
