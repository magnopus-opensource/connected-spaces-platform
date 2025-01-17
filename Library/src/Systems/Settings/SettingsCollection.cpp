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

void SettingsDtoToSettingsCollection(const chs::SettingsDto& Dto, csp::systems::SettingsCollection& SettingsCollection)
{
    if (Dto.HasUserId())
    {
        SettingsCollection.UserId = Dto.GetUserId();
    }

    if (Dto.HasContext())
    {
        SettingsCollection.Context = Dto.GetContext();
    }

    if (Dto.HasSettings())
    {
        const auto& Settings = Dto.GetSettings();

        for (auto& Pair : Settings)
        {
            SettingsCollection.Settings[Pair.first] = Pair.second;
        }
    }
}

} // namespace

namespace csp::systems
{

AvatarType AvatarInfoResult::GetAvatarType() const { return Type; }

const csp::common::String& AvatarInfoResult::GetAvatarIdentifier() const { return Identifier; }

void AvatarInfoResult::SetAvatarType(AvatarType InValue) { Type = InValue; }

void AvatarInfoResult::SetAvatarIdentifier(const csp::common::String& InValue) { Identifier = InValue; }

const SettingsCollection& SettingsCollectionResult::GetSettingsCollection() const { return SettingsCollection; }

void SettingsCollectionResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto* SettingsResponse = static_cast<chs::SettingsDto*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        if (Response->GetPayload().GetContent().Length() > 0)
        {
            // Build the Dto from the response Json
            SettingsResponse->FromJson(Response->GetPayload().GetContent());

            SettingsDtoToSettingsCollection(*SettingsResponse, SettingsCollection);
        }
    }
}

} // namespace csp::systems
