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
#pragma once

#include "CSP/Common/CancellationToken.h"
#include "Services/ApiBase/ApiBase.h"

namespace csp::services
{

template <typename T, typename U, typename V, typename W> class ApiResponseHandler;

} // namespace csp::services

namespace csp::web
{

class WebClient;

}

namespace csp::systems::graphqlservice
{

class GraphQLApi : public csp::services::ApiBase
{
public:
    GraphQLApi(csp::web::WebClient* InWebClient);
    ~GraphQLApi();

    void Query(csp::common::String QueryText, csp::services::ApiResponseHandlerBase* ResponseHandler,
        csp::common::CancellationToken& CancellationToken = csp::common::CancellationToken::Dummy()) const;
};
} // namespace csp::systems::graphqlservice
