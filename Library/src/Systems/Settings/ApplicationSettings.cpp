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

void ApplicationSettingsDtoToApplicationSettings(const chs::ApplicationSettingsDto& Dto, csp::systems::ApplicationSettings& ApplicationSettings)
{
    if (Dto.HasApplicationName())
        ApplicationSettings.ApplicationName = Dto.GetApplicationName();

    if (Dto.HasContext())
        ApplicationSettings.Context = Dto.GetContext();

    if (Dto.HasAllowAnonymous())
        ApplicationSettings.AllowAnonymous = Dto.GetAllowAnonymous();

    if (Dto.HasSettings())
        ApplicationSettings.Settings = Convert(Dto.GetSettings());
}

} // namespace

namespace csp::systems
{

const ApplicationSettings& ApplicationSettingsResult::GetApplicationSettings() const { return ApplicationSettings; }

void ApplicationSettingsResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto* ApplicationSettingsResponse = static_cast<chs::ApplicationSettingsDto*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        if (Response->GetPayload().GetContent().Length() > 0)
        {
            // Build the Dto from the response Json
            ApplicationSettingsResponse->FromJson(Response->GetPayload().GetContent());

            ApplicationSettingsDtoToApplicationSettings(*ApplicationSettingsResponse, ApplicationSettings);
        }
    }
}

} // namespace csp::systems

void ToJson(csp::json::JsonSerializer& Serializer, const csp::systems::ApplicationSettings& Obj)
{
    Serializer.SerializeMember("applicationName", Obj.ApplicationName);
    Serializer.SerializeMember("context", Obj.Context);
    Serializer.SerializeMember("allowAnonymous", Obj.AllowAnonymous);
    Serializer.SerializeMember("settings", Obj.Settings);
}
