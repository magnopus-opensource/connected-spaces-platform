#pragma once

#include "Olympus/Common/String.h"
#include "Olympus/OlympusCommon.h"

namespace oly
{

class OLY_API EndpointURIs
{
public:
    oly_common::String UserServiceURI;
    oly_common::String PrototypeServiceURI;
    oly_common::String SpatialDataServiceURI;
    oly_common::String MultiplayerServiceURI;
    oly_common::String AggregationServiceURI;
};

class OLY_API ClientUserAgent
{
public:
    oly_common::String OlympusVersion;
    oly_common::String ClientOS;
    oly_common::String ClientSKU;
    oly_common::String ClientVersion;
    oly_common::String ClientEnvironment;
    oly_common::String CHSEnvironment;
};

class OLY_API OlympusFoundation
{
public:
    /**
     * @brief Sets the endpoints for the various services needed for foundation, passes over the client header information and initialises the systems
     * required for foundation to operate.
     * @param EndpointRootURI
     * @return bool for successful initialisation.
     */
    // TODO GB: ClientUserAgent should be provided here to ensure all WebRequests have CHS Analytic info

    /**
     * @brief Sets essential information for Foundation to run
     * @param EndpointRootURI oly_common::String : endpoint uri for Foundation to use
     * @param Tenant oly_common::String : client application name
     */
    static bool Initialise(const oly_common::String& EndpointRootURI, const oly_common::String& Tenant);
    /**
     * @brief Clears event queues and destroys foundation systems.
     * @return bool for successful shutdown.
     */
    static bool Shutdown();

    /**
     * @brief Ticks the event processing of foundation.
     */
    static void Tick();

    /**
     * @brief Gets the foundation version in use.
     * @return Returns the commit hash for the foundation build.
     */
    static const oly_common::String& GetVersion();
    /**
     * @brief Gets the foundation build type in use.
     * @return Returns the build type for the foundation build.
     */
    static const oly_common::String& GetBuildType();
    /**
     * @brief Gets the full foundation build ID in use.
     * @return Returns the build ID for the foundation build.
     */
    static const oly_common::String& GetBuildID();
    /**
     * @brief Gets the EntitySystemVersion number.
     * This represents the system used to parse data for Entities,
     * and is used to prevent conflicting entity data versions from being used together where we cannot parse both.
     * @return the system version number, a manually incremented counter that changes when significant breaking changes occur in the entity parsing
     * systems.
     */
    static int32_t GetEntitySystemVersion();
    /**
     * @brief Get the DeviceID.
     * @return a string representing the current device.
     */
    static const oly_common::String& GetDeviceId();

    /**
     * @brief Is this instance of Foundation initialised.
     * @return bool IsInitialised.
     */
    static bool GetIsInitialised();
    /**
     * @brief Gets a class containing all relevant endpoints currently set for Foundation.
     * @return The EndpointURIs class with current endpoint data.
     */
    static const EndpointURIs& GetEndpoints();
    /**
     * @brief Sets a class containing all relevant Client info currently set for Foundation.
     * @return The Client Info class with current Client Info data.
     */
    static void SetClientUserAgentInfo(const oly::ClientUserAgent& ClientUserAgentHeader);
    /**
     * @brief Gets a class containing all relevant Client info currently set for Foundation.
     * @return The Client Info class with current Client Info data.
     */
    static const ClientUserAgent& GetClientUserAgentInfo();
    /**
     * @brief Gets a string containing the CHS-formatted Client UserAgent header.
     * @return Returns Client UserAgent header.
     */
    static const oly_common::String& GetClientUserAgentString();

    static const oly_common::String& GetTenant();

private:
    static bool IsInitialised;
    static EndpointURIs* Endpoints;
    static ClientUserAgent* ClientUserAgentInfo;
    static oly_common::String* DeviceId;
    static oly_common::String* ClientUserAgentString;
    static oly_common::String* Tenant;

    static const uint32_t EntitySystemVersion = 3;
};

OLY_API void* GetFunctionAddress(const oly_common::String& Name);

} // namespace oly
