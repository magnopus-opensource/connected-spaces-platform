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

#include "CSP/Systems/Settings/SettingsCollection.h"

#include "Services/ApiBase/ApiBase.h"
#include "Services/UserService/Dto.h"

namespace chs = csp::services::generated::userservice;

namespace
{

void SettingsDtoToSettingsCollection(const chs::SettingsDto& dto, csp::common::SettingsCollection& settingsCollection)
{
    if (dto.HasUserId())
    {
        settingsCollection.UserId = dto.GetUserId();
    }

    if (dto.HasContext())
    {
        settingsCollection.Context = dto.GetContext();
    }

    if (dto.HasSettings())
    {
        const auto& settings = dto.GetSettings();

        for (auto& pair : settings)
        {
            settingsCollection.Settings[pair.first] = pair.second;
        }
    }
}

} // namespace

namespace csp::systems
{

AvatarType AvatarInfoResult::GetAvatarType() const { return m_type; }

const csp::common::String& AvatarInfoResult::GetAvatarIdentifier() const { return m_identifier; }

bool AvatarInfoResult::GetAvatarVisible() const { return m_avatarVisible; }

void AvatarInfoResult::SetAvatarType(AvatarType inValue) { m_type = inValue; }

void AvatarInfoResult::SetAvatarIdentifier(const csp::common::String& inValue) { m_identifier = inValue; }

void AvatarInfoResult::SetAvatarVisible(bool inValue) { m_avatarVisible = inValue; }

const csp::common::SettingsCollection& SettingsCollectionResult::GetSettingsCollection() const { return m_settingsCollection; }

void SettingsCollectionResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* settingsResponse = static_cast<chs::SettingsDto*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        if (response->GetPayload().GetContent().Length() > 0)
        {
            // Build the Dto from the response Json
            settingsResponse->FromJson(response->GetPayload().GetContent());

            SettingsDtoToSettingsCollection(*settingsResponse, m_settingsCollection);
        }
    }
}

} // namespace csp::systems
