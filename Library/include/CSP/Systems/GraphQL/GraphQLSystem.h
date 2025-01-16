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

#include "CSP/Systems/GraphQL/GraphQL.h"
#include "CSP/Systems/SystemBase.h"
#include "CSP/Systems/SystemsResult.h"

namespace csp::services
{

class ApiBase;

} // namespace csp::services

namespace csp::web
{

class WebClient;

} // namespace csp::web

namespace csp::memory
{

CSP_START_IGNORE
template <typename T> void Delete(T* Ptr);
CSP_END_IGNORE

} // namespace csp::memory

namespace csp::systems
{

/// @ingroup GraphQL System
/// @brief Public facing system that allows interfacing with Magnopus Connect Services' GraphQL Server.
/// Offers methods for sending and receiving GraphQL Queries.
class CSP_API GraphQLSystem : public SystemBase
{
    CSP_START_IGNORE
    /** @cond DO_NOT_DOCUMENT */
    friend class SystemsManager;
    friend void csp::memory::Delete<GraphQLSystem>(GraphQLSystem* Ptr);
    /** @endcond */
    CSP_END_IGNORE

public:
    /// @brief Make a request to the Magnopus Connect Services' GraphQL Server, can contain a query, variables and operationName.
    /// @param RequestBody csp::common::String : graphql request body, JSON encoded string of full graphql request,
    /// can include a query, variables and operationName.
    /// @param Callback GraphQLReceivedCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void RunRequest(const csp::common::String RequestBody, GraphQLReceivedCallback ApiResponse);

    /// @brief Send basic query to the Magnopus Connect Services' GraphQL Server, must be a simple query, QueryText will be wrapped with a basic
    /// graphql request body. This function does not support variables. for variable use please see RunRequest.
    /// @param QueryText csp::common::String : graphql query string. will be wrapped in a basic graphql request,
    /// this paramater will be the value of request.query
    /// @param Callback GraphQLReceivedCallback : callback when asynchronous task finishes
    CSP_ASYNC_RESULT void RunQuery(const csp::common::String QueryText, GraphQLReceivedCallback ApiResponse);

private:
    GraphQLSystem(); // This constructor is only provided to appease the wrapper generator and should not be used
    CSP_NO_EXPORT GraphQLSystem(csp::web::WebClient* InWebClient);
    ~GraphQLSystem();

    csp::services::ApiBase* GraphQLAPI;
};

} // namespace csp::systems
