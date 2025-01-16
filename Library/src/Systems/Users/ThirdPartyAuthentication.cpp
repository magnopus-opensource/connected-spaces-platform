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
void SocialProviderInfoDtoToProviderDetails(const chs::SocialProviderInfo& Dto, csp::systems::ThirdPartyProviderDetails& ProviderDetails)
{
    if (Dto.HasProviderName())
    {
        ProviderDetails.ProviderName = Dto.GetProviderName();
    }

    if (Dto.HasClientId())
    {
        ProviderDetails.ProviderClientId = Dto.GetClientId();
    }

    if (Dto.HasScopes())
    {
        const auto& Scopes = Dto.GetScopes();
        ProviderDetails.ProviderAuthScopes = csp::common::Array<csp::common::String>(Scopes.size());

        for (size_t idx = 0; idx < Scopes.size(); ++idx)
        {
            ProviderDetails.ProviderAuthScopes[idx] = Scopes[idx];
        }
    }

    if (Dto.HasAuthorizeEndpoint())
    {
        ProviderDetails.AuthoriseURL = Dto.GetAuthorizeEndpoint();
    }
}
}; // namespace

namespace csp::systems
{

ThirdPartyProviderDetails& ProviderDetailsResult::GetDetails() { return ProviderDetails; }

const ThirdPartyProviderDetails& ProviderDetailsResult::GetDetails() const { return ProviderDetails; }

void ProviderDetailsResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto* InfoResponse = static_cast<chs::SocialProviderInfo*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        InfoResponse->FromJson(Response->GetPayload().GetContent());

        SocialProviderInfoDtoToProviderDetails(*InfoResponse, ProviderDetails);
    }
}
} // namespace csp::systems
