#pragma once

#include "Olympus/Systems/GraphQL/GraphQL.h"
#include "Olympus/Systems/SystemBase.h"
#include "Olympus/Systems/SystemsResult.h"

namespace oly_services
{

class ApiBase;

}

namespace oly_web
{

class WebClient;

}

namespace oly_systems
{

/// @ingroup GraphQL System
/// @brief Public facing system that allows interfacing with CHS's GraphQL Server.
/// Offers methods for sending and receiving GraphQL Queries.

class OLY_API OLY_NO_DISPOSE GraphQLSystem : public SystemBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class SystemsManager;
    /** @endcond */

public:
    ~GraphQLSystem();

    /// @brief Sends Query to the CHS GraphQL Server
    /// @param QueryText oly_common::String : graphql query string
    /// @param Callback GraphQLReceivedCallback : callback when asynchronous task finishes
    OLY_ASYNC_RESULT void RunQuery(const oly_common::String QueryText, GraphQLReceivedCallback ApiResponse);

private:
    GraphQLSystem(); // This constructor is only provided to appease the wrapper generator and should not be used
    OLY_NO_EXPORT GraphQLSystem(oly_web::WebClient* InWebClient);

    oly_services::ApiBase* GraphQLAPI;
};

} // namespace oly_systems
