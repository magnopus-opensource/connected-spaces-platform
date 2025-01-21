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
    : SystemBase(nullptr, nullptr)
    , GraphQLAPI(nullptr)
{
}

GraphQLSystem::GraphQLSystem(csp::web::WebClient* InWebClient)
    : SystemBase(InWebClient, nullptr)
{
    GraphQLAPI = CSP_NEW chs::GraphQLApi(InWebClient);
}

GraphQLSystem::~GraphQLSystem() { CSP_DELETE(GraphQLAPI); }

void GraphQLSystem::RunQuery(const csp::common::String QueryText, GraphQLReceivedCallback Callback)
{
    std::string QueryTextStr = QueryText.c_str();

    std::regex reg("\"");
    QueryTextStr = std::regex_replace(QueryTextStr, reg, "\\\"");
    std::string RequestBody = "{\"query\":\"query{" + QueryTextStr + "}\"}";

    RunRequest(csp::common::String(RequestBody.c_str()), Callback);
}

void GraphQLSystem::RunRequest(const csp::common::String RequestBody, GraphQLReceivedCallback Callback)
{
    std::ostringstream strm;
    std::string QueryTextStr = RequestBody.c_str();
    strm << QueryTextStr;

    csp::services::ResponseHandlerPtr GraphQLResponseHandler
        = GraphQLAPI->CreateHandler<GraphQLReceivedCallback, GraphQLResult, void, csp::services::NullDto>(Callback, nullptr);
    static_cast<chs::GraphQLApi*>(GraphQLAPI)->Query(strm.str().c_str(), GraphQLResponseHandler);
}

} // namespace csp::systems
