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

}


namespace csp::web
{

class WebClient;

}


namespace csp::systems
{

/// @ingroup GraphQL System
/// @brief Public facing system that allows interfacing with Magnopus Connect Services' GraphQL Server.
/// Offers methods for sending and receiving GraphQL Queries.
class CSP_API CSP_NO_DISPOSE GraphQLSystem : public SystemBase
{
	/** @cond DO_NOT_DOCUMENT */
	friend class SystemsManager;
	/** @endcond */

public:
	~GraphQLSystem();

	/// @brief Sends Query to the Magnopus Connect Services' GraphQL Server
	/// @param QueryText csp::common::String : graphql query string
	/// @param Callback GraphQLReceivedCallback : callback when asynchronous task finishes
	CSP_ASYNC_RESULT void RunQuery(const csp::common::String QueryText, GraphQLReceivedCallback ApiResponse);

private:
	GraphQLSystem(); // This constructor is only provided to appease the wrapper generator and should not be used
	CSP_NO_EXPORT GraphQLSystem(csp::web::WebClient* InWebClient);

	csp::services::ApiBase* GraphQLAPI;
};

} // namespace csp::systems
