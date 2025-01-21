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
#include "CSP/Systems/GraphQL/GraphQL.h"

#include "Services/ApiBase/ApiBase.h"
#include "Web/GraphQLApi/GraphQLApi.h"

using namespace csp::systems::graphqlservice;

namespace csp::systems
{
const csp::common::String& GraphQLResult::GetResponse() { return GraphQLResponse; }
void GraphQLResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        GraphQLResponse = ApiResponse->GetResponse()->GetPayload().GetContent();
    }
};
} // namespace csp::systems
