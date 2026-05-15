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

#include "CSP/Systems/Settings/ApplicationSettings.h"
#include "CSP/Common/fmt_Formatters.h"
#include "Common/Convert.h"
#include "Services/UserService/Api.h"
#include "Services/UserService/Dto.h"

#include "CSP/Systems/ContinuationUtils.h"

namespace chs = csp::services::generated::userservice;

namespace
{

void ApplicationSettingsDtoToApplicationSettings(const chs::ApplicationSettingsDto& dto, csp::common::ApplicationSettings& applicationSettings)
{
    if (dto.HasApplicationName())
        applicationSettings.ApplicationName = dto.GetApplicationName();

    if (dto.HasContext())
        applicationSettings.Context = dto.GetContext();

    if (dto.HasAllowAnonymous())
        applicationSettings.AllowAnonymous = dto.GetAllowAnonymous();

    if (dto.HasSettings())
        applicationSettings.Settings = Convert(dto.GetSettings());
}

} // namespace

namespace csp::systems
{

const csp::common::ApplicationSettings& ApplicationSettingsResult::GetApplicationSettings() const { return m_applicationSettings; }

void ApplicationSettingsResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* applicationSettingsResponse = static_cast<chs::ApplicationSettingsDto*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        if (response->GetPayload().GetContent().Length() > 0)
        {
            // Build the Dto from the response Json
            applicationSettingsResponse->FromJson(response->GetPayload().GetContent());

            ApplicationSettingsDtoToApplicationSettings(*applicationSettingsResponse, m_applicationSettings);
        }
    }
}

} // namespace csp::systems
