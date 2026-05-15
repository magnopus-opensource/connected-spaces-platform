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
#include "CSP/CSPFoundation.h"

#include "../../Tools/WrapperGenerator/Output/C/generated_wrapper.h"
#include "CSP/Common/StringFormat.h"
#include "CSP/Common/fmt_Formatters.h"
#include "CSP/Systems/ServiceStatus.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/version.h"
#include "Common/UUIDGenerator.h"
#include "Common/Wrappers.h"
#include "Debug/Logging.h"
#include "Events/EventSystem.h"

#include <cstdio>
#include <fmt/format.h>
#if defined(CSP_ANDROID)
// Android specific LIB_NAME definition.
#if defined(DEBUG)
#define LIB_NAME "libConnectedSpacesPlatform_D.so"
#else
#define LIB_NAME "libConnectedSpacesPlatform.so"
#endif
#else
// All other platforms LIB_NAME definition.
#if defined(DEBUG)
#define LIB_NAME "ConnectedSpacesPlatform_D"
#else
#define LIB_NAME "ConnectedSpacesPlatform"
#endif
#endif

#if defined(CSP_WINDOWS)
// For SHGetKnownFolderPath
#include <Shlobj.h>
// For std::codecvt_utf8_utf16
#include <codecvt>
// For GetModuleHandle and GetProcAddress
#include <libloaderapi.h>

#define LOAD_OWN_MODULE() (void*)GetModuleHandleA(LIB_NAME)
#define GET_FUNCTION_ADDRESS(mod, name) (void*)GetProcAddress((HMODULE)(mod), name)
#elif defined(CSP_POSIX)
// For dlopen and dlsym
#include <dlfcn.h>
// For stat and mkdir
#include <sys/stat.h>
#include <sys/types.h>
// For PATH_MAX
#include <limits.h>

#if defined(CSP_ANDROID)
    // The normal behaviour is for a null pointer to return the symbol table for the currently loaded process,
    // but android diverges from that and passes the specific library name.
    // https://pubs.opengroup.org/onlinepubs/9699919799/functions/dlopen.html
    #define LOAD_OWN_MODULE() dlopen(LIB_NAME, RTLD_LAZY)
#else
    #define LOAD_OWN_MODULE() dlopen(nullptr, RTLD_LAZY)
#endif

#define GET_FUNCTION_ADDRESS(mod, name) dlsym((mod), name)

#elif defined(CSP_WASM)
#include <emscripten.h>
#endif

namespace
{

// We don't need these 3 functions for WASM, as we use localStorage instead of the filesystem to store the device ID
#if !defined(CSP_WASM)
bool FolderExists(std::string path)
{
#if defined(CSP_WINDOWS)
    auto attr = GetFileAttributesA(path.c_str());

    return (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY));
#else
    // All POSIX platforms should support stat
    struct stat Stat;

    return (stat(Path.c_str(), &Stat) == 0 && S_ISDIR(Stat.st_mode));
#endif
}

bool FileExists(std::string path)
{
#if defined(CSP_WINDOWS)
    auto attr = GetFileAttributesA(path.c_str());

    return (attr != INVALID_FILE_ATTRIBUTES && ((attr & FILE_ATTRIBUTE_NORMAL) || (attr & FILE_ATTRIBUTE_ARCHIVE)));
#else
    struct stat Stat;

    return (stat(Path.c_str(), &Stat) == 0 && S_ISREG(Stat.st_mode));
#endif
}

// Named "CreateFolder" to avoid conflicts with Win32's CreateDirectory macro
void CreateFolder(std::string path)
{
#if defined(CSP_WINDOWS)
    ::CreateDirectoryA(path.c_str(), NULL);
#else
    mkdir(Path.c_str(), 0777);
#endif
}
#endif

#if defined(CSP_WASM)
// clang-format off
EM_JS(char*, get_device_id, (), {
    const deviceId = window.localStorage.getItem('csp_foundation_deviceid');

    if (deviceId === null)
    {
        return null;
    }

    const length = lengthBytesUTF8(deviceId);
    var cString = _malloc(length + 1);
    stringToUTF8(deviceId, cString, length + 1);

    return cString;
});

EM_JS(void, set_device_id, (const char* cDeviceId), {
    var deviceId = UTF8ToString(cDeviceId);
    window.localStorage.setItem('csp_foundation_deviceid', deviceId);
});
// clang-format on
#endif

#if !defined(CSP_WASM)
std::string DeviceIdPath()
{
    // For all platforms, we want to guarantee the current user has read/write access
    // and to reduce public visibility of the file that holds the device ID
#if defined(CSP_WINDOWS)
    // On Windows, we store the device ID in %localappdata%
    PWSTR path;
    SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, 0, &path);
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;

    auto cspDataRoot = conv.to_bytes(std::wstring(path)) + "\\MagnopusCSP\\";

    CoTaskMemFree(path);
    return cspDataRoot;

#elif defined(CSP_ANDROID)
    // On Android, we store the device ID in the app's local storage directory
    FILE* CmdlineFile = fopen("/proc/self/cmdline", "r");

    char Path[256];
    fgets(Path, sizeof(Path), CmdlineFile);

    if (CmdlineFile != nullptr)
    {
        fclose(CmdlineFile);
    }

    return "/data/data/" + std::string(Path) + "/";

#elif defined(CSP_MACOSX) || defined(CSP_IOS)
    // On macOS and iOS, we store the device ID in the app's user library path
    const char* Home = getenv("HOME");
    return std::string(Home ? Home : "") + "/Library/MagnopusCSP/";

#elif defined(CSP_LINUX)

    const char* Home = getenv("HOME");
    return std::string(Home ? Home : "") + "/.local/share/MagnopusCSP/";

#else

#error "DeviceIdPath is not implemented"

#endif
}
#endif

std::string LoadDeviceId()
{
    // Use a unique code path for WASM to avoid using the awful async filesystem API
#if defined(CSP_WASM)
    auto DeviceId = get_device_id();

    if (DeviceId == nullptr)
    {
        auto GeneratedDeviceId = csp::GenerateUUID();
        set_device_id(GeneratedDeviceId.c_str());

        return GeneratedDeviceId;
    }

    auto DeviceIdString = std::string(DeviceId);
    free(DeviceId);

    return DeviceIdString;
#else
    const std::string cspDataRoot = DeviceIdPath();

    if (!FolderExists(cspDataRoot))
    {
        CreateFolder(cspDataRoot);
    }

    auto deviceIdFilePath = std::string(cspDataRoot) + "device.id";
    std::string deviceId;

    if (!FileExists(deviceIdFilePath))
    {
        FILE* file = FOPEN(deviceIdFilePath.c_str(), "w");
        assert(file != nullptr);

        deviceId = csp::GenerateUUID();

        fwrite(deviceId.c_str(), sizeof(char), 32, file);
        fflush(file);
        fclose(file);
    }
    else
    {
        FILE* file = FOPEN(deviceIdFilePath.c_str(), "r");
        assert(file != nullptr);

        char uuid[33];
        fread(uuid, sizeof(char), 32, file);
        fclose(file);
        uuid[32] = '\0';

        deviceId = uuid;
    }

    return deviceId;
#endif
}

} // namespace

namespace csp
{

bool CSPFoundation::IsInitialised = false;
EndpointURIs* CSPFoundation::Endpoints = nullptr;
ClientUserAgent* CSPFoundation::ClientUserAgentInfo = nullptr;
csp::common::String* CSPFoundation::DeviceId = nullptr;
csp::common::String* CSPFoundation::ClientUserAgentString = nullptr;
csp::common::String* CSPFoundation::Tenant = nullptr;

// Default Feature Flag values should be defined here. Example shown below:
csp::common::Array<csp::FeatureFlag> csp::CSPFoundation::FeatureFlags;
// csp::common::Array<FeatureFlag> CSPFoundation::FeatureFlags = {
//     { EFeatureFlag::MyFeatureA, false, "Description of MyFeatureA" },
//     { EFeatureFlag::MyFeatureB, true, "Description of MyFeatureB" }
// };

namespace
{
    // Take the input endpoint to the cloud services, and get the multiplayer URIS
    std::string TranslateEndpointRootURIToMultiplayerRootUri(const std::string& endpointRootUri)
    {
        /* This is not the best, we've hard encoded how the root uri and the multiplayer uri relate, which is no real guarantee.
         * We should probably take the multiplayer too separately
         * Remember, this transformation needs to work on both "http://ogs-internal.cloud" sorts of formats, as well as "http://localhost:8081" sort
         * of formats
         *
         * Even if we're doing it this way, we should still be using a URI library to do all this transformation, not doing it as fragile raw strings.
         * We've got POCO, but we're wanting to remove it ... so didn't want to bleed it into CSPFoundation right now. :(
         */

        // Check if ogs exists, if so insert "-multiplayer" after it.
        std::string multiplayerRootUri = endpointRootUri;
        const std::string ogsFindTarget = "ogs";
        const size_t ogsFindPos = multiplayerRootUri.find(ogsFindTarget, 0);
        if (ogsFindPos != std::string::npos)
        {
            const std::string multiplayerServiceInsert = "-multiplayer";
            multiplayerRootUri.insert(ogsFindPos + ogsFindTarget.length(), multiplayerServiceInsert);
        }

        return multiplayerRootUri;
    }

    /// @brief Find the Reverse Proxy in service URI from Services Deployment Status.
    /// e.g. 'http://localhost:8081/mag-multiplayer/hubs/v1/multiplayer' -> 'mag-multiplayer'
    /// @return csp::common::String : empty string if not successful, otherwise the remaining stting.
    csp::common::String FindReverseProxy(const csp::systems::ServicesDeploymentStatus& servicesDeploymentStatus, const csp::common::String& uri)
    {
        for (auto const& service : servicesDeploymentStatus.Services)
        {
            if (uri.Contains(service.ReverseProxy))
                return service.ReverseProxy;
        }

        return "";
    }

    static constexpr auto DocumentationUri = "https://connected-spaces-platform.net/index.html";

    /// @brief Tries to find the ServiceStatus for a given reverse proxy.
    /// @return csp::systems::ServiceStatus* : nullptr if not found, otherwise a pointer to the ServiceStatus.
    const csp::systems::ServiceStatus* FindServiceStatus(
        const csp::systems::ServicesDeploymentStatus& servicesDeploymentStatus, const csp::common::String& uri)
    {
        // Finds the reverse proxy in the service's URI.
        // The reverse proxy is a unique identifier used to locate the service's deployment status.
        const auto reverseProxy = FindReverseProxy(servicesDeploymentStatus, uri);

        const auto serviceStatusIt = std::find_if(servicesDeploymentStatus.Services.begin(), servicesDeploymentStatus.Services.end(),
            [&reverseProxy](const csp::systems::ServiceStatus& status) { return status.ReverseProxy == reverseProxy; });

        if (serviceStatusIt == servicesDeploymentStatus.Services.end())
        {
            const auto message = fmt::format("Unable to resolve {} in Status Info", uri);
            CSP_LOG_MSG(csp::common::LogLevel::Error, message.c_str());
            return nullptr;
        }
        return &(*serviceStatusIt);
    }

    /// @brief Tries to find the VersionMetadata for an expected API version within a ServiceStatus.
    /// @return csp::systems::VersionMetadata* : nullptr if not found, otherwise a pointer to the VersionMetadata.
    const csp::systems::VersionMetadata* FindVersionMetadata(const csp::systems::ServiceStatus& serviceStatus, uint32_t expectedVersion)
    {
        const auto versionMetadataIt = std::find_if(serviceStatus.ApiVersions.begin(), serviceStatus.ApiVersions.end(),
            [expectedVersion](const csp::systems::VersionMetadata& metadata)
            { return metadata.Version.c_str() == fmt::format("v{}", expectedVersion); });

        if (versionMetadataIt == serviceStatus.ApiVersions.end())
        {
            return nullptr;
        }
        return &(*versionMetadataIt);
    }

    /// @brief Handles validation for the "retired" state of a service.
    /// @return bool : true if the service is retired (and logs fatal), false otherwise.
    bool HandleRetiredState(const csp::systems::ServiceStatus& serviceStatus, uint32_t currentVersion)
    {
        const auto message = fmt::format("{} v{} has been retired, the latest version is {}. For more information please visit: {}",
            serviceStatus.Name, currentVersion, serviceStatus.CurrentApiVersion, DocumentationUri);
        CSP_LOG_MSG(csp::common::LogLevel::Fatal, message.c_str());
        return true;
    }

    /// @brief Handles validation for the "deprecated" state of a service.
    /// @return bool : true if the service is deprecated (and logs warning), false otherwise.
    bool HandleDeprecatedState(
        const csp::systems::ServiceStatus& serviceStatus, const csp::systems::VersionMetadata& versionMetadata, uint32_t currentVersion)
    {
        if (!versionMetadata.DeprecationDatetime.IsEmpty())
        {
            const auto message = fmt::format("{} v{} will be deprecated as of {}, the latest version is {}. For more information please visit: {}",
                serviceStatus.Name, currentVersion, versionMetadata.DeprecationDatetime, serviceStatus.CurrentApiVersion, DocumentationUri);
            CSP_LOG_MSG(csp::common::LogLevel::Warning, message.c_str());
            return true;
        }
        return false;
    }

    /// @brief Handles validation for the "available (older version)" state of a service.
    /// @return bool : true if a newer version is available (and logs info), false otherwise.
    bool HandleOlderVersionAvailableState(
        const csp::systems::ServiceStatus& serviceStatus, const csp::systems::VersionMetadata& versionMetadata, uint32_t currentVersion)
    {
        if (versionMetadata.Version != serviceStatus.CurrentApiVersion)
        {
            const auto message = fmt::format("{} v{} is not the latest available, the latest version is {}. For more information please visit: {}",
                serviceStatus.Name, currentVersion, serviceStatus.CurrentApiVersion, DocumentationUri);
            CSP_LOG_MSG(csp::common::LogLevel::Log, message.c_str());
            return true;
        }
        return false;
    }

    /// @brief Handles validation for the "latest (older version)" state of a service.
    /// @return bool : true if the latest version, false otherwise.
    bool HandleLatestVersionState(const csp::systems::ServiceStatus& serviceStatus, const csp::systems::VersionMetadata& versionMetadata)
    {
        if (versionMetadata.Version == serviceStatus.CurrentApiVersion)
        {
            return true;
        }
        return false;
    }

    void ApplyFeatureFlagOverrides(
        const csp::common::Optional<csp::common::Array<FeatureFlag>>& featureFlagOverrides, csp::common::Array<csp::FeatureFlag>& featureFlags)
    {
        if (!featureFlagOverrides.HasValue())
        {
            return;
        }

        for (const auto& overrideFlag : *featureFlagOverrides)
        {
            bool featureFlagFound = false;
            for (auto& flag : featureFlags)
            {
                if (flag.Type == overrideFlag.Type)
                {
                    flag.Enabled = overrideFlag.Enabled;
                    featureFlagFound = true;
                    break;
                }
            }
            if (!featureFlagFound)
            {
                CSP_LOG_MSG(csp::common::LogLevel::Warning,
                    fmt::format("Unknown feature flag passed with integer value: {}", static_cast<int>(overrideFlag.Type)).c_str());
            }
        }
    }
}

EndpointURIs CSPFoundation::CreateEndpointsFromRoot(const csp::common::String& endpointRootUri)
{
    // remove last char if a slash
    std::string rootUri(endpointRootUri.c_str());
    while (rootUri.rbegin() != rootUri.rend() && (*rootUri.rbegin() == '\\' || *rootUri.rbegin() == '/'))
    {
        rootUri.resize(rootUri.length() - 1);
    }

    const std::string userServiceUri = rootUri + "/mag-user";
    const std::string prototypeServiceUri = rootUri + "/mag-prototype";
    const std::string spatialDataServiceUri = rootUri + "/mag-spatialdata";
    const std::string aggregationServiceUri = rootUri + "/oly-aggregation";
    const std::string trackingServiceUri = rootUri + "/mag-tracking";

    const std::string multiplayerRootUri = TranslateEndpointRootURIToMultiplayerRootUri(rootUri);

    const std::string multiplayerConnectionUri = multiplayerRootUri + "/mag-multiplayer/hubs/v1/multiplayer";
    const std::string multiplayerServiceUri = multiplayerRootUri + "/mag-multiplayer";

    EndpointURIs endpointsUri;
    endpointsUri.UserService = ServiceDefinition(CSP_TEXT(userServiceUri.c_str()), 1U);
    endpointsUri.PrototypeService = ServiceDefinition(CSP_TEXT(prototypeServiceUri.c_str()), 1U);
    endpointsUri.SpatialDataService = ServiceDefinition(CSP_TEXT(spatialDataServiceUri.c_str()), 1U);
    endpointsUri.AggregationService = ServiceDefinition(CSP_TEXT(aggregationServiceUri.c_str()), 1U);
    endpointsUri.TrackingService = ServiceDefinition(CSP_TEXT(trackingServiceUri.c_str()), 1U);
    endpointsUri.MultiplayerService = ServiceDefinition(CSP_TEXT(multiplayerServiceUri.c_str()), 1U);
    endpointsUri.MultiplayerConnection = ServiceDefinition(CSP_TEXT(multiplayerConnectionUri.c_str()), 1U);

    return endpointsUri;
}

bool CSPFoundation::Initialise(const csp::common::String& endpointRootUri, const csp::common::String& inTenant,
    const csp::ClientUserAgent& clientUserAgentHeader, const csp::common::Optional<csp::common::Array<FeatureFlag>>& featureFlagOverrides)
{
    // Nullptr means the signalRConnection will be internally instantiated
    return InitialiseWithInject(endpointRootUri, inTenant, clientUserAgentHeader, nullptr, nullptr, featureFlagOverrides);
}

bool CSPFoundation::InitialiseWithInject(const csp::common::String& endpointRootUri, const csp::common::String& inTenant,
    const csp::ClientUserAgent& clientUserAgentHeader, csp::multiplayer::ISignalRConnection* signalRInject, csp::web::WebClient* webClientInject,
    const csp::common::Optional<csp::common::Array<FeatureFlag>>& featureFlagOverrides)
{
    assert(!IsInitialised && "Please call csp::CSPFoundation::Shutdown() before calling csp::CSPFoundation::Initialize() again.");

    Tenant = new csp::common::String(inTenant);

    Endpoints = new EndpointURIs(CreateEndpointsFromRoot(endpointRootUri));
    ClientUserAgentInfo = new ClientUserAgent();
    DeviceId = new csp::common::String("");
    ClientUserAgentString = new csp::common::String("");

    SetClientUserAgentInfo(clientUserAgentHeader);

    // Apply feature flag overrides
    ApplyFeatureFlagOverrides(featureFlagOverrides, FeatureFlags);

    csp::systems::SystemsManager::Instantiate(signalRInject, webClientInject);

    *DeviceId = LoadDeviceId().c_str();
    IsInitialised = true;

    return true;
}

bool CSPFoundation::Shutdown()
{
    if (!IsInitialised)
    {
        return false;
    }

    IsInitialised = false;

    // Clear unprocessed events before shutting down
    csp::events::EventSystem::Get().ProcessEvents();
    csp::events::EventSystem::Get().UnRegisterAllListeners();
    csp::systems::SystemsManager::Destroy();

    delete (Tenant);
    delete (Endpoints);
    delete (ClientUserAgentInfo);
    delete (DeviceId);
    delete (ClientUserAgentString);

    return true;
}

void CSPFoundation::Tick()
{
    if (!IsInitialised)
    {
        return;
    }

    CSP_PROFILE_SCOPED();

    csp::events::Event* tickEvent = csp::events::EventSystem::Get().AllocateEvent(csp::events::FOUNDATION_TICK_EVENT_ID);
    csp::events::EventSystem::Get().EnqueueEvent(tickEvent);

    csp::events::EventSystem::Get().ProcessEvents();
}

const csp::common::String& CSPFoundation::GetVersion()
{
    static csp::common::String version(CSP_FOUNDATION_COMMIT_ID);

    return version;
}

const csp::common::String& CSPFoundation::GetBuildType()
{
    static csp::common::String buildType(CSP_FOUNDATION_BUILD_TYPE);

    return buildType;
}

const csp::common::String& CSPFoundation::GetBuildID()
{
    static csp::common::String buildId(CSP_FOUNDATION_BUILD_ID);

    return buildId;
}

const csp::common::String& CSPFoundation::GetDeviceId() { return *DeviceId; }

bool CSPFoundation::GetIsInitialised() { return IsInitialised; }

const EndpointURIs& CSPFoundation::GetEndpoints() { return *Endpoints; }

const ClientUserAgent& CSPFoundation::GetClientUserAgentInfo() { return *ClientUserAgentInfo; }

const csp::common::String& CSPFoundation::GetClientUserAgentString() { return *ClientUserAgentString; }

const csp::common::String& CSPFoundation::GetTenant() { return *Tenant; }

bool CSPFoundation::IsFeatureEnabled(EFeatureFlag flag)
{
    auto it = std::find_if(FeatureFlags.begin(), FeatureFlags.end(), [flag](const FeatureFlag& featureFlag) { return featureFlag.Type == flag; });

    if (it != FeatureFlags.end())
    {
        return it->Enabled;
    }
    else
    {
        CSP_LOG_MSG(
            csp::common::LogLevel::Warning, fmt::format("Unknown feature flag queried with integer value: {}", static_cast<int>(flag)).c_str());

        return false;
    }
}

const csp::common::Array<FeatureFlag>& CSPFoundation::GetFeatureFlags() { return FeatureFlags; }

csp::common::String CSPFoundation::GetFeatureFlagDescription(EFeatureFlag flag)
{
    auto it = std::find_if(FeatureFlags.begin(), FeatureFlags.end(), [flag](const FeatureFlag& featureFlag) { return featureFlag.Type == flag; });

    if (it != FeatureFlags.end())
    {
        return it->GetDescription();
    }
    else
    {
        CSP_LOG_MSG(csp::common::LogLevel::Warning,
            fmt::format("Unknown feature flag description requested with integer value: {}", static_cast<int>(flag)).c_str());

        return "";
    }
}

void CSPFoundation::__AddFeatureFlagForTesting(EFeatureFlag type, bool isEnabled, const csp::common::String description)
{
    csp::common::Array<FeatureFlag> newFeatureFlags(FeatureFlags.Size() + 1);

    for (size_t i = 0; i < FeatureFlags.Size(); i++)
    {
        newFeatureFlags[i] = FeatureFlags[i];
    }

    newFeatureFlags[FeatureFlags.Size()] = { type, isEnabled, description };

    FeatureFlags = newFeatureFlags;
}

void CSPFoundation::__ResetFeatureFlagsForTesting() { CSPFoundation::FeatureFlags = csp::common::Array<csp::FeatureFlag>(); }

bool ServiceDefinition::CheckPrerequisites(const csp::systems::ServicesDeploymentStatus& servicesDeploymentStatus) const
{
    // Evaluate State: Service Not Found (Highest Priority)
    // Attempt to find the overall status of the service within the provided deployment status.
    // If the service's reverse proxy is not found in the deployment status,
    // it implies the service is not deployed or recognized by the system.
    // This is a critical failure, and the prerequisite check immediately returns false.
    const auto serviceStatus = FindServiceStatus(servicesDeploymentStatus, this->GetURI());
    if (!serviceStatus)
    {
        return false;
    }

    // Retrieve the specific version metadata for this service definition's version.
    const auto versionMetadata = FindVersionMetadata(*serviceStatus, this->GetVersion());

    // Evaluate State: Retired (Second Highest Priority)
    // A service version is considered 'retired' if the currently expected version
    // (from this ServiceDefinition's configuration) is no longer listed or supported
    // in the live service's API versions. This often means the version has been
    // completely removed from the live environment and is no longer operational.
    // This is a fatal condition, and the prerequisite check immediately returns false.
    if (!versionMetadata)
    {
        HandleRetiredState(*serviceStatus, this->GetVersion());
        return false;
    }

    // Evaluate State: Deprecated (Third Highest Priority)
    // A service version is 'deprecated' if it is still active and functional,
    // but its continued use is discouraged. It typically means the service
    // will be retired at a future date (indicated by DeprecationDatetime)
    // and clients should migrate to a newer version.
    // For prerequisite checks, a deprecated service is generally still considered
    // 'valid enough to run', but a warning is logged to inform the user.
    if (HandleDeprecatedState(*serviceStatus, *versionMetadata, this->GetVersion()))
    {
        return true;
    }

    // Evaluate State: Older Version Available (Fourth Highest Priority)
    // This state indicates that the service version being used is functional
    // but is not the absolute latest version available on the live system.
    // It implies there's a newer, fully supported version that clients could upgrade to.
    // This is typically an informational message, not a blocking error for prerequisites.
    if (HandleOlderVersionAvailableState(*serviceStatus, *versionMetadata, this->GetVersion()))
    {
        return true;
    }

    // Evaluate State: Latest Version (Lowest Priority)
    // If none of the above conditions (service not found, retired, deprecated, or older version)
    // are met, then the service's current version is the latest and fully supported version
    // available on the live system. This is the ideal and expected state.
    if (HandleLatestVersionState(*serviceStatus, *versionMetadata))
    {
        return true;
    }

    // Fallback in the event that the service's state could not be definitively validated.
    CSP_LOG_MSG(csp::common::LogLevel::Error, "ServiceDefinition::CheckPrerequisites: Unable to validate service state.");
    return false;
}

void CSPFoundation::SetClientUserAgentInfo(const csp::ClientUserAgent& clientUserAgentHeader)
{
    ClientUserAgentInfo->CSPVersion = CSP_TEXT(clientUserAgentHeader.CSPVersion);
    ClientUserAgentInfo->ClientOS = CSP_TEXT(clientUserAgentHeader.ClientOS);
    ClientUserAgentInfo->ClientSKU = CSP_TEXT(clientUserAgentHeader.ClientSKU);
    ClientUserAgentInfo->ClientVersion = CSP_TEXT(clientUserAgentHeader.ClientVersion);
    ClientUserAgentInfo->ClientEnvironment = CSP_TEXT(clientUserAgentHeader.ClientEnvironment);
    ClientUserAgentInfo->CHSEnvironment = CSP_TEXT(clientUserAgentHeader.CHSEnvironment);
    const char* clientUserAgentHeaderFormat = "%s/%s(%s) CSP/%s(%s) CHS(%s) CSP/%s(%s)";

    *ClientUserAgentString = csp::common::StringFormat(clientUserAgentHeaderFormat, GetClientUserAgentInfo().ClientSKU.c_str(),
        GetClientUserAgentInfo().ClientVersion.c_str(), GetClientUserAgentInfo().ClientEnvironment.c_str(), GetVersion().c_str(),
        GetBuildType().c_str(), GetClientUserAgentInfo().CHSEnvironment.c_str(), GetClientUserAgentInfo().CSPVersion.c_str(),
        GetClientUserAgentInfo().ClientOS.c_str());
}

void Free(void* pointer) { std::free(pointer); }

void* ModuleHandle = nullptr;

void* GetFunctionAddress([[maybe_unused]] const csp::common::String& name)
{
#if defined(CSP_WASM)
    return nullptr;
#else
    if (ModuleHandle == nullptr)
    {
        ModuleHandle = LOAD_OWN_MODULE();
        assert(ModuleHandle != nullptr);
    }

    return GET_FUNCTION_ADDRESS(ModuleHandle, name.c_str());
#endif
}

} // namespace csp
