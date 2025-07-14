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

#include "CSP/CSPCommon.h"
#include "CSP/Common/String.h"

#include <string>

namespace csp::systems
{
class StatusInfo;
} // namespace csp::systems

namespace csp
{

/// @brief Represents definition for identifying and versioning an external service endpoint..
class CSP_API ServiceDefinition
{
public:
    ServiceDefinition()
        : URI("")
        , Version(0)
    {
    }

    ServiceDefinition(const csp::common::String& InURI, const uint32_t InVersion)
        : URI(InURI)
        , Version(InVersion)
    {
    }

    /// @brief Gets the URI for the service endpoint.
    /// @return csp::common::String : URI of the service endpoint.
    csp::common::String GetURI() const { return URI; }
    /// @brief Sets the URI for the service endpoint.
    /// @param InURI csp::common::String : URI for service endpoint.
    void SetURI(const csp::common::String& InURI) { URI = InURI; }

    /// @brief Gets the current version of the service endpoint.
    /// @return int32_t : Representing the current version of the service endpoint.
    int32_t GetVersion() const { return Version; }
    /// @brief Sets the current Version for the service endpoint.
    /// @param InVersion uint32_t : Version for service endpoint.
    CSP_NO_EXPORT void SetVersion(const uint32_t InVersion) { Version = InVersion; }

private:
    csp::common::String URI;
    uint32_t Version;
};

/// @brief Holds supported endpoint uris used by Foundation.
class CSP_API EndpointURIs
{
public:
    ServiceDefinition UserService;
    ServiceDefinition PrototypeService;
    ServiceDefinition SpatialDataService;
    ServiceDefinition MultiplayerService;
    ServiceDefinition AggregationService;
    ServiceDefinition TrackingService;
    ServiceDefinition MaintenanceWindow;
};

/// @brief Holds client data used in requests for all Magnopus Serives.
class CSP_API ClientUserAgent
{
public:
    // @brief Foundation version.
    csp::common::String CSPVersion;

    // @brief Operating system of the client.
    csp::common::String ClientOS;

    // @brief Client project code.
    csp::common::String ClientSKU;

    // @brief Client application version.
    csp::common::String ClientVersion;

    // @brief Build type of the client. e.g DEVELOPMENT.
    csp::common::String ClientEnvironment;

    // @brief Magnopus services environment. e.g odev.
    csp::common::String CHSEnvironment;
};

/// @brief Main entry point for interacting with Foundation.
/// Provides functionality for initialising, shutting down and managing essential information for the Foundation instance to run.
class CSP_API CSPFoundation
{
public:
    /// @brief Sets the endpoints for the various services needed for foundation, passes over the client header information and initialises the
    /// systems required for foundation to operate.
    /// @param EndpointRootURI csp::common::String : Root URI for service endpoints
    /// @param Tenant csp::common::String : Tenant for Magnopus Services. Data is not shared between tenants so clients using separate tenants cannot
    /// interact with each other.
    /// @return bool : True for successful initialisation.
    static bool Initialise(const csp::common::String& EndpointRootURI, const csp::common::String& Tenant);

    /// @brief This should be used at the end of the application lifecycle.
    /// Clears event queues and destroys foundation systems.
    /// After shutdown, no other Foundation functions should be called until Initialise is called again.
    /// @return bool : True for successful shutdown
    static bool Shutdown();

    /// @brief Ticks the event processing of foundation.
    /// This should only be called once per frame from the client application.
    static void Tick();

    /// @brief Gets the foundation version in use.
    /// @return const csp::common::String& : Returns the commit hash for the foundation build
    static const csp::common::String& GetVersion();

    /// @brief Gets the foundation build type in use.
    /// @return const csp::common::String& : Returns the build type for the foundation build (DEV or REL)
    static const csp::common::String& GetBuildType();

    /// @brief Gets the full foundation build ID in use.
    /// Generated at project generation time using the lastest commit timestamp and hash.
    /// @return const csp::common::String : Returns the build ID for the foundation build
    static const csp::common::String& GetBuildID();

    /// @brief Unique identifier for the current device.
    /// Used internally by certain user authentication endpoints.
    /// @return csp::common::String& : A string representing the current device
    static const csp::common::String& GetDeviceId();

    /// @brief Is this instance of Foundation initialised.
    /// @return bool : Returns false before Initialise and after Shutdown
    static bool GetIsInitialised();

    /// @brief Gets endpoints used for all Magnopus service.
    /// Used internally to setup all System classes.
    /// @return const EndpointURIs& : The EndpointURIs class with current endpoint data
    static const EndpointURIs& GetEndpoints();

    /// @brief Create an EndpointsURIs object containing URIs to the various services needed by CSP.
    /// @param EndpointRootURI csp::common::String: URI to the root of the cloud services.
    /// @return EndpointURIs class with deduced endpoint URIs.
    static EndpointURIs CreateEndpointsFromRoot(const csp::common::String& EndpointRootURI);

    /// @brief Sets a class containing all relevant Client info currently set for Foundation.
    /// Used internally to generate ClientUserAgentString.
    /// @param The Client Info class with current Client Info data
    static void SetClientUserAgentInfo(const csp::ClientUserAgent& ClientUserAgentHeader);

    /// @brief Gets a class containing all relevant Client info currently set for Foundation.
    /// @return const ClientUserAgent& : The Client Info class with current Client Info data
    static const ClientUserAgent& GetClientUserAgentInfo();

    /// @brief Gets a string containing the Client UserAgent header information.
    /// Used internally in headers for all Magnopus Service requests.
    /// @return csp::common::String& : Returns Client UserAgent header
    static const csp::common::String& GetClientUserAgentString();

    /// @brief Gets the tenant that foundation is currently using, based on what was provided during initialisation.
    /// @return csp::common::String&
    static const csp::common::String& GetTenant();

    /// @brief Compares the given service definition against status information to evaluate state differences.
    /// This function analyzes the provided `ServiceDefinition` and compares it with the corresponding
    /// `StatusInfo` to determine the differences between the two. The comparison is performed with
    /// respect to a defined set of service states: 'Latest', 'Deployed', 'Deprecated', and 'Retired'.
    /// @param ServiceDefinition The service definition to be evaluated.
    /// @param StatusInfo The current status information against which the service definition is compared.
    /// @return bool : true if all services are available, false otherwise
    CSP_NO_EXPORT static bool ResolveServiceDefinition(const ServiceDefinition& ServiceDefinition, const csp::systems::StatusInfo& StatusInfo);

private:
    static bool IsInitialised;
    static EndpointURIs* Endpoints;
    static ClientUserAgent* ClientUserAgentInfo;
    static csp::common::String* DeviceId;
    static csp::common::String* ClientUserAgentString;
    static csp::common::String* Tenant;
};

// Helper function to get function address for templates from wrappers
CSP_API void* GetFunctionAddress(const csp::common::String& Name);

// Helper function to free allocated memory from wrappers
CSP_API void Free(void* Pointer);

} // namespace csp
