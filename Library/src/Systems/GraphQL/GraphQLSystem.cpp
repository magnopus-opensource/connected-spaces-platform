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
#include "CSP/Systems/GraphQL/GraphQLSystem.h"

#include "CSP/Systems/Users/UserSystem.h"
#include "Services/ApiBase/ApiBase.h"
#include "Web/GraphQLApi/GraphQLApi.h"

#include <sstream>

namespace chs = csp::systems::graphqlservice;

namespace csp::systems
{

GraphQLSystem::GraphQLSystem()
    : SystemBase(nullptr, nullptr, nullptr)
    , m_graphQlapi(nullptr)
{
}

GraphQLSystem::GraphQLSystem(csp::web::WebClient* inWebClient, csp::common::LogSystem& logSystem)
    : SystemBase(inWebClient, nullptr, &logSystem)
{
    m_graphQlapi = new chs::GraphQLApi(inWebClient);
}

GraphQLSystem::~GraphQLSystem() { delete (m_graphQlapi); }

void GraphQLSystem::RunQuery(const csp::common::String queryText, GraphQLReceivedCallback callback)
{
    std::string queryTextStr = queryText.c_str();

    std::regex reg("\"");
    queryTextStr = std::regex_replace(queryTextStr, reg, "\\\"");
    std::string requestBody = "{\"query\":\"query{" + queryTextStr + "}\"}";

    RunRequest(csp::common::String(requestBody.c_str()), callback);
}

void GraphQLSystem::RunRequest(const csp::common::String requestBody, GraphQLReceivedCallback callback)
{
    std::ostringstream strm;
    std::string queryTextStr = requestBody.c_str();
    strm << queryTextStr;

    csp::services::ResponseHandlerPtr graphQlResponseHandler
        = m_graphQlapi->CreateHandler<GraphQLReceivedCallback, GraphQLResult, void, csp::services::NullDto>(callback, nullptr);
    static_cast<chs::GraphQLApi*>(m_graphQlapi)->Query(strm.str().c_str(), graphQlResponseHandler);
}

} // namespace csp::systems
