/*
 * GraphQLApi.h
 *
 *
 */

#pragma once
#include "Olympus/Services/WebService.h"

namespace oly_services
{

class ApiResponseBase;

} // namespace oly_services

namespace oly_systems
{

/// @ingroup GraphQL System
/// @brief Data class used to contain information when a Response is received from GraphQL

class OLY_API GraphQLResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    OLY_START_IGNORE
    friend class GraphQLSystem;
    OLY_END_IGNORE
    /** @endcond */
public:
    GraphQLResult() {};
    GraphQLResult(void*) {};

    /// @brief Retrieves response data from the GraphQL Server
    /// @return String : Data String as described above
    [[nodiscard]] const oly_common::String& GetResponse();

private:
    void OnResponse(const oly_services::ApiResponseBase* ApiResponse) override;
    oly_common::String GraphQLResponse;
};
// callback signatures
typedef std::function<void(GraphQLResult& Result)> GraphQLReceivedCallback;
} // namespace oly_systems
