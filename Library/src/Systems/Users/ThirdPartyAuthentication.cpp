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
#include "CSP/Systems/Users/ThirdPartyAuthentication.h"

#include "CSP/Systems/Users/UserSystem.h"
#include "Services/ApiBase/ApiBase.h"
#include "Services/UserService/Dto.h"

using namespace std::chrono;
namespace chs = csp::services::generated::userservice;

namespace
{
void SocialProviderInfoDtoToProviderDetails(const chs::SocialProviderInfo& dto, csp::systems::ThirdPartyProviderDetails& providerDetails)
{
    if (dto.HasProviderName())
    {
        providerDetails.ProviderName = dto.GetProviderName();
    }

    if (dto.HasClientId())
    {
        providerDetails.ProviderClientId = dto.GetClientId();
    }

    if (dto.HasScopes())
    {
        const auto& scopes = dto.GetScopes();
        providerDetails.ProviderAuthScopes = csp::common::Array<csp::common::String>(scopes.size());

        for (size_t idx = 0; idx < scopes.size(); ++idx)
        {
            providerDetails.ProviderAuthScopes[idx] = scopes[idx];
        }
    }

    if (dto.HasAuthorizeEndpoint())
    {
        providerDetails.ProviderAuthURL = dto.GetAuthorizeEndpoint();
    }

    if (dto.HasThirdPartyAuthStateId())
    {
        providerDetails.ThirdPartyAuthStateId = dto.GetThirdPartyAuthStateId();
    }

    if (dto.HasRedirectUri())
    {
        providerDetails.ProviderRedirectURL = dto.GetRedirectUri();
    }
}
}; // namespace

namespace csp::systems
{

ThirdPartyProviderDetails& ProviderDetailsResult::GetDetails() { return m_providerDetails; }

const ThirdPartyProviderDetails& ProviderDetailsResult::GetDetails() const { return m_providerDetails; }

void ProviderDetailsResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* infoResponse = static_cast<chs::SocialProviderInfo*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        infoResponse->FromJson(response->GetPayload().GetContent());

        SocialProviderInfoDtoToProviderDetails(*infoResponse, m_providerDetails);
    }
}
} // namespace csp::systems
