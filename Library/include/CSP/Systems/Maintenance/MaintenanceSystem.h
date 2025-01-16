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

} // namespace csp::services

namespace csp::web
{

class WebClient;

} // namespace csp::web

namespace csp::memory
{

CSP_START_IGNORE
template <typename T> void Delete(T* Ptr);
CSP_END_IGNORE

} // namespace csp::memory

namespace csp::systems
{

/// @ingroup Maintenance System
/// @brief Public facing system that allows interfacing with the Maintenance Window Server.
/// This system can be used to query if there is currently a planned outage
/// and can also be used to check for up coming maintenances outages
class CSP_API MaintenanceSystem : public SystemBase
{
    CSP_START_IGNORE
    /** @cond DO_NOT_DOCUMENT */
    friend class SystemsManager;
    friend void csp::memory::Delete<MaintenanceSystem>(MaintenanceSystem* Ptr);
    /** @endcond */
    CSP_END_IGNORE

public:
    /// @brief Receives information on planned maintenances outages schedules for the future
    /// @param Callback MaintenanceInfoCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void GetMaintenanceInfo(const csp::common::String& MaintenanceURL, MaintenanceInfoCallback Callback);

private:
    MaintenanceSystem(); // This constructor is only provided to appease the wrapper generator and should not be used
    CSP_NO_EXPORT MaintenanceSystem(csp::web::WebClient* InWebClient);
    ~MaintenanceSystem();

    csp::services::ApiBase* MaintenanceAPI;
    bool AllowMaintenanceInfoRequests;
};

} // namespace csp::systems
