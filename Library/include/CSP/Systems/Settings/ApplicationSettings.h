/*
 * Copyright 2025 Magnopus LLC
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

#include "CSP/CSPCommon.h"
#include "CSP/Common/Array.h"
#include "CSP/Common/Map.h"
#include "CSP/Common/Settings.h"
#include "CSP/Common/String.h"
#include "CSP/Systems/WebService.h"
#include "Json/JsonSerializer.h"

namespace csp::services
{

class ApiResponseBase;

CSP_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
CSP_END_IGNORE

} // namespace csp::services

namespace csp::systems
{

/// @ingroup Application Settings System
/// @brief Represents the result of a request for application settings.
class CSP_API ApplicationSettingsResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    const csp::common::ApplicationSettings& GetApplicationSettings() const;

    CSP_NO_EXPORT ApplicationSettingsResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode) { };

    CSP_NO_EXPORT ApplicationSettingsResult(
        csp::systems::EResultCode ResCode, csp::web::EResponseCodes HttpResCode, csp::systems::ERequestFailureReason Reason)
        : csp::systems::ResultBase(ResCode, static_cast<std::underlying_type<csp::web::EResponseCodes>::type>(HttpResCode), Reason) { };

private:
    ApplicationSettingsResult(void*) { };

    CSP_NO_EXPORT void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    csp::common::ApplicationSettings ApplicationSettings;
};

/// @brief Callback containing Application Settings.
/// @param Result ApplicationSettingsResult : result class
typedef std::function<void(const ApplicationSettingsResult& Result)> ApplicationSettingsResultCallback;

} // namespace csp::systems
