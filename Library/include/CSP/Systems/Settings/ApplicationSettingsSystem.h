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
#include "CSP/Systems/Settings/ApplicationSettings.h"
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

namespace csp::systems
{

/// @ingroup Application Settings System
/// @brief Public facing system that allows interfacing with Magnopus Connected Services' application settings service.
class CSP_API ApplicationSettingsSystem : public SystemBase
{
    CSP_START_IGNORE
    /** @cond DO_NOT_DOCUMENT */
    friend class SystemsManager;
    /** @endcond */
    CSP_END_IGNORE

public:
    /// @brief Asynchronously creates or updates application settings for a specific context.
    /// @param ApplicationSettings ApplicationSettings& : The settings object containing application name, context, and key-value pairs to be stored.
    /// @param Callback ApplicationSettingsResultCallback : Callback when asynchronous task finishes.
    CSP_NO_EXPORT CSP_ASYNC_RESULT void CreateSettingsByContext(
        const ApplicationSettings& ApplicationSettings, ApplicationSettingsResultCallback Callback);

    /// @brief Asynchronously retrieves application settings for a specific context.
    /// @param ApplicationName csp::common::String : The name of the application for which settings are requested.
    /// @param Context csp::common::String : The specific context whose settings should be retrieved.
    /// @param Keys csp::common::Optional<csp::common::String> : List of setting keys to retrieve. If not provided, all settings for the context are
    /// returned.
    /// @param Callback ApplicationSettingsResultCallback : Callback when asynchronous task finishes.
    CSP_ASYNC_RESULT void GetSettingsByContext(const csp::common::String& ApplicationName, const csp::common::String& Context,
        const csp::common::Optional<csp::common::Array<csp::common::String>>& Keys, ApplicationSettingsResultCallback Callback);

    /// @brief Asynchronously retrieves application settings for a specific context without requiring authentication.
    /// @param Tenant csp::common::String : The tenant identifier under which the application and settings are scoped.
    /// @param ApplicationName csp::common::String : The name of the application for which settings are requested.
    /// @param Context csp::common::String : The specific context whose settings should be retrieved.
    /// @param Keys csp::common::Optional<csp::common::String> : List of setting keys to retrieve. If not provided, all settings for the context are
    /// returned.
    /// @param Callback ApplicationSettingsResultCallback : Callback when asynchronous task finishes.
    CSP_ASYNC_RESULT void GetSettingsByContextAnonymous(const csp::common::String& Tenant, const csp::common::String& ApplicationName,
        const csp::common::String& Context, const csp::common::Optional<csp::common::Array<csp::common::String>>& Keys,
        ApplicationSettingsResultCallback Callback);

private:
    ApplicationSettingsSystem(); // This constructor is only provided to appease the wrapper generator and should not be used
    CSP_NO_EXPORT ApplicationSettingsSystem(csp::web::WebClient* InWebClient, csp::common::LogSystem& LogSystem);
    ~ApplicationSettingsSystem();

    // Application Settings Continuations
    async::task<ApplicationSettingsResult> CreateSettingsByContext(const ApplicationSettings& ApplicationSettings);
    async::task<ApplicationSettingsResult> GetSettingsByContext(const csp::common::String& ApplicationName, const csp::common::String& Context,
        const csp::common::Optional<csp::common::Array<csp::common::String>>& Keys);
    async::task<ApplicationSettingsResult> GetSettingsByContextAnonymous(const csp::common::String& Tenant,
        const csp::common::String& ApplicationName, const csp::common::String& Context,
        const csp::common::Optional<csp::common::Array<csp::common::String>>& Keys);

    csp::services::ApiBase* ApplicationSettingsAPI;
};

} // namespace csp::systems
