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

#if defined(DEBUG)
#define LIB_NAME "ConnectedSpacesPlatform_D"
#else
#define LIB_NAME "ConnectedSpacesPlatform"
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
#elif defined(CSP_ANDROID)
// For dlopen and dlsym
#include <dlfcn.h>
// For fstat and mkdir
#include <sys/stat.h>

#define LOAD_OWN_MODULE() dlopen(nullptr, RTLD_LAZY)
#define GET_FUNCTION_ADDRESS(mod, name) dlsym((mod), name)
#elif defined(CSP_MACOSX) || defined(CSP_IOS)
// For dlopen and dlsym
#include <dlfcn.h>
// For stat and mkdir
#include <sys/stat.h>
// For PATH_MAX
#include <sys/syslimits.h>

#define LOAD_OWN_MODULE() dlopen(nullptr, RTLD_LAZY)
#define GET_FUNCTION_ADDRESS(mod, name) dlsym((mod), name)
#elif defined(CSP_WASM)
#include <emscripten.h>
#endif

namespace
{

// We don't need these 3 functions for WASM, as we use localStorage instead of the filesystem to store the device ID
#if !defined(CSP_WASM)
bool FolderExists(std::string Path)
{
#if defined(CSP_WINDOWS)
    auto Attr = GetFileAttributesA(Path.c_str());

    return (Attr != INVALID_FILE_ATTRIBUTES && (Attr & FILE_ATTRIBUTE_DIRECTORY));
#else
    // All POSIX platforms should support stat
    struct stat Stat;

    return (stat(Path.c_str(), &Stat) == 0 && S_ISDIR(Stat.st_mode));
#endif
}

bool FileExists(std::string Path)
{
#if defined(CSP_WINDOWS)
    auto Attr = GetFileAttributesA(Path.c_str());

    return (Attr != INVALID_FILE_ATTRIBUTES && ((Attr & FILE_ATTRIBUTE_NORMAL) || (Attr & FILE_ATTRIBUTE_ARCHIVE)));
#else
    struct stat Stat;

    return (stat(Path.c_str(), &Stat) == 0 && S_ISREG(Stat.st_mode));
#endif
}

// Named "CreateFolder" to avoid conflicts with Win32's CreateDirectory macro
void CreateFolder(std::string Path)
{
#if defined(CSP_WINDOWS)
    ::CreateDirectoryA(Path.c_str(), NULL);
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
// For all platforms, we want to guarantee the current user has read/write access and to reduce public visibility of the file that holds the
// device ID
#if defined(CSP_WINDOWS)
    // On Windows, we store the device ID in %localappdata%
    PWSTR Path;
    SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, 0, &Path);
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> Conv;

    auto CSPDataRoot = Conv.to_bytes(std::wstring(Path)) + "\\MagnopusCSP\\";

    CoTaskMemFree(Path);
#elif defined(CSP_ANDROID)
    // On Android, we store the device ID in the app's local storage directory
    FILE* CmdlineFile = fopen("/proc/self/cmdline", "r");

    char Path[256];
    fgets(Path, sizeof(Path), CmdlineFile);

    auto CSPDataRoot = "/data/data/" + std::string(Path) + "/";
#elif defined(CSP_MACOSX) || defined(CSP_IOS)
    // On macOS and iOS, we store the device ID in the app's user library path
    char CSPDataRoot[PATH_MAX];
    sprintf(CSPDataRoot, "%s/Library/MagnopusCSP/", getenv("HOME"));
#endif

    return CSPDataRoot;
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
    const std::string CSPDataRoot = DeviceIdPath();

    if (!FolderExists(CSPDataRoot))
    {
        CreateFolder(CSPDataRoot);
    }

    auto DeviceIdFilePath = std::string(CSPDataRoot) + "device.id";
    std::string DeviceId;

    if (!FileExists(DeviceIdFilePath))
    {
        FILE* File = FOPEN(DeviceIdFilePath.c_str(), "w");
        assert(File != nullptr);

        DeviceId = csp::GenerateUUID();

        fwrite(DeviceId.c_str(), sizeof(char), 32, File);
        fflush(File);
        fclose(File);
    }
    else
    {
        FILE* File = FOPEN(DeviceIdFilePath.c_str(), "r");
        assert(File != nullptr);

        char Uuid[33];
        fread(Uuid, sizeof(char), 32, File);
        fclose(File);
        Uuid[32] = '\0';

        DeviceId = Uuid;
    }

    return DeviceId;
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

namespace
{
    // Take the input endpoint to the cloud services, and get the multiplayer URIS
    std::string TranslateEndpointRootURIToMultiplayerServiceUri(const std::string& EndpointRootURI)
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
        std::string MultiplayerServiceURI = EndpointRootURI;
        const std::string OgsFindTarget = "ogs";
        const size_t OgsFindPos = MultiplayerServiceURI.find(OgsFindTarget, 0);
        if (OgsFindPos != std::string::npos)
        {
            const std::string MultiplayerServiceInsert = "-multiplayer";
            MultiplayerServiceURI.insert(OgsFindPos + OgsFindTarget.length(), MultiplayerServiceInsert);
        }

        // Append the hub location
        const std::string SignalRHubLocation = "/mag-multiplayer/hubs/v1/multiplayer";
        MultiplayerServiceURI = MultiplayerServiceURI + SignalRHubLocation;

        return MultiplayerServiceURI;
    }

    /// @brief Find the Reverse Proxy in service URI from Services Deployment Status.
    /// e.g. 'http://localhost:8081/mag-multiplayer/hubs/v1/multiplayer' -> 'mag-multiplayer'
    /// @return csp::common::String : empty string if not successful, otherwise the remaining stting.
    csp::common::String FindReverseProxy(const csp::systems::ServicesDeploymentStatus& ServicesDeploymentStatus, const csp::common::String& URI)
    {
        for (auto const& Service : ServicesDeploymentStatus.Services)
        {
            if (URI.Contains(Service.ReverseProxy))
                return Service.ReverseProxy;
        }

        return "";
    }

    static constexpr auto DocumentationUri = "https://connected-spaces-platform.net/index.html";

    /// @brief Tries to find the ServiceStatus for a given reverse proxy.
    /// @return csp::systems::ServiceStatus* : nullptr if not found, otherwise a pointer to the ServiceStatus.
    const csp::systems::ServiceStatus* FindServiceStatus(
        const csp::systems::ServicesDeploymentStatus& ServicesDeploymentStatus, const csp::common::String& URI)
    {
        // Finds the reverse proxy in the service's URI.
        // The reverse proxy is a unique identifier used to locate the service's deployment status.
        const auto ReverseProxy = FindReverseProxy(ServicesDeploymentStatus, URI);

        const auto ServiceStatusIt = std::find_if(ServicesDeploymentStatus.Services.begin(), ServicesDeploymentStatus.Services.end(),
            [&ReverseProxy](const csp::systems::ServiceStatus& status) { return status.ReverseProxy == ReverseProxy; });

        if (ServiceStatusIt == ServicesDeploymentStatus.Services.end())
        {
            const auto Message = fmt::format("Unable to resolve {} in Status Info", URI);
            CSP_LOG_MSG(csp::common::LogLevel::Error, Message.c_str());
            return nullptr;
        }
        return &(*ServiceStatusIt);
    }

    /// @brief Tries to find the VersionMetadata for an expected API version within a ServiceStatus.
    /// @return csp::systems::VersionMetadata* : nullptr if not found, otherwise a pointer to the VersionMetadata.
    const csp::systems::VersionMetadata* FindVersionMetadata(const csp::systems::ServiceStatus& ServiceStatus, uint32_t ExpectedVersion)
    {
        const auto VersionMetadataIt = std::find_if(ServiceStatus.ApiVersions.begin(), ServiceStatus.ApiVersions.end(),
            [ExpectedVersion](const csp::systems::VersionMetadata& metadata)
            { return metadata.Version.c_str() == fmt::format("v{}", ExpectedVersion); });

        if (VersionMetadataIt == ServiceStatus.ApiVersions.end())
        {
            return nullptr;
        }
        return &(*VersionMetadataIt);
    }

    /// @brief Handles validation for the "retired" state of a service.
    /// @return bool : true if the service is retired (and logs fatal), false otherwise.
    bool HandleRetiredState(const csp::systems::ServiceStatus& ServiceStatus, uint32_t CurrentVersion)
    {
        const auto Message = fmt::format("{} v{} has been retired, the latest version is {}. For more information please visit: {}",
            ServiceStatus.Name, CurrentVersion, ServiceStatus.CurrentApiVersion, DocumentationUri);
        CSP_LOG_MSG(csp::common::LogLevel::Fatal, Message.c_str());
        return true;
    }

    /// @brief Handles validation for the "deprecated" state of a service.
    /// @return bool : true if the service is deprecated (and logs warning), false otherwise.
    bool HandleDeprecatedState(
        const csp::systems::ServiceStatus& ServiceStatus, const csp::systems::VersionMetadata& versionMetadata, uint32_t CurrentVersion)
    {
        if (!versionMetadata.DeprecationDatetime.IsEmpty())
        {
            const auto Message = fmt::format("{} v{} will be deprecated as of {}, the latest version is {}. For more information please visit: {}",
                ServiceStatus.Name, CurrentVersion, versionMetadata.DeprecationDatetime, ServiceStatus.CurrentApiVersion, DocumentationUri);
            CSP_LOG_MSG(csp::common::LogLevel::Warning, Message.c_str());
            return true;
        }
        return false;
    }

    /// @brief Handles validation for the "available (older version)" state of a service.
    /// @return bool : true if a newer version is available (and logs info), false otherwise.
    bool HandleOlderVersionAvailableState(
        const csp::systems::ServiceStatus& ServiceStatus, const csp::systems::VersionMetadata& VersionMetadata, uint32_t CurrentVersion)
    {
        if (VersionMetadata.Version != ServiceStatus.CurrentApiVersion)
        {
            const auto Message = fmt::format("{} v{} is not the latest available, the latest version is {}. For more information please visit: {}",
                ServiceStatus.Name, CurrentVersion, ServiceStatus.CurrentApiVersion, DocumentationUri);
            CSP_LOG_MSG(csp::common::LogLevel::Log, Message.c_str());
            return true;
        }
        return false;
    }

    /// @brief Handles validation for the "latest (older version)" state of a service.
    /// @return bool : true if the latest version, false otherwise.
    bool HandleLatestVersionState(const csp::systems::ServiceStatus& ServiceStatus, const csp::systems::VersionMetadata& VersionMetadata)
    {
        if (VersionMetadata.Version == ServiceStatus.CurrentApiVersion)
        {
            return true;
        }
        return false;
    }
}

EndpointURIs CSPFoundation::CreateEndpointsFromRoot(const csp::common::String& EndpointRootURI)
{
    // remove last char if a slash
    std::string RootURI(EndpointRootURI.c_str());
    while (RootURI.rbegin() != RootURI.rend() && (*RootURI.rbegin() == '\\' || *RootURI.rbegin() == '/'))
    {
        RootURI.resize(RootURI.length() - 1);
    }

    const std::string UserServiceURI = RootURI + "/mag-user";
    const std::string PrototypeServiceURI = RootURI + "/mag-prototype";
    const std::string SpatialDataServiceURI = RootURI + "/mag-spatialdata";
    const std::string AggregationServiceURI = RootURI + "/oly-aggregation";
    const std::string TrackingServiceURI = RootURI + "/mag-tracking";

    const std::string MultiplayerServiceURI = TranslateEndpointRootURIToMultiplayerServiceUri(RootURI);

    EndpointURIs EndpointsURI;
    EndpointsURI.UserService = ServiceDefinition(CSP_TEXT(UserServiceURI.c_str()), 1U);
    EndpointsURI.PrototypeService = ServiceDefinition(CSP_TEXT(PrototypeServiceURI.c_str()), 1U);
    EndpointsURI.SpatialDataService = ServiceDefinition(CSP_TEXT(SpatialDataServiceURI.c_str()), 1U);
    EndpointsURI.AggregationService = ServiceDefinition(CSP_TEXT(AggregationServiceURI.c_str()), 1U);
    EndpointsURI.TrackingService = ServiceDefinition(CSP_TEXT(TrackingServiceURI.c_str()), 1U);
    EndpointsURI.MultiplayerService = ServiceDefinition(CSP_TEXT(MultiplayerServiceURI.c_str()), 1U);

    return EndpointsURI;
}

bool CSPFoundation::Initialise(const csp::common::String& EndpointRootURI, const csp::common::String& InTenant)
{
    if (IsInitialised)
    {
        return false;
    }

    Tenant = new csp::common::String(InTenant);

    Endpoints = new EndpointURIs(CreateEndpointsFromRoot(EndpointRootURI));
    ClientUserAgentInfo = new ClientUserAgent();
    DeviceId = new csp::common::String("");
    ClientUserAgentString = new csp::common::String("");

    csp::systems::SystemsManager::Instantiate();

    *DeviceId = LoadDeviceId().c_str();
    IsInitialised = true;

    // Initialises ClientAgentHeaderInfo with default values in case the client doesn't call SetClientUserAgentInfo().
    ClientUserAgent ClientAgentHeaderInfo;
    ClientAgentHeaderInfo.CSPVersion = CSP_TEXT("CSPVersionUnset");
    ClientAgentHeaderInfo.ClientOS = CSP_TEXT("ClientOSUnset");
    ClientAgentHeaderInfo.ClientSKU = CSP_TEXT("ClientSKUUnset");
    ClientAgentHeaderInfo.ClientVersion = CSP_TEXT("ClientVersionUnset");
    ClientAgentHeaderInfo.ClientEnvironment = CSP_TEXT("ClientBuildTypeUnset");
    ClientAgentHeaderInfo.CHSEnvironment = CSP_TEXT("CHSEnvironmentUnset");

    SetClientUserAgentInfo(ClientAgentHeaderInfo);

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

    csp::events::Event* TickEvent = csp::events::EventSystem::Get().AllocateEvent(csp::events::FOUNDATION_TICK_EVENT_ID);
    csp::events::EventSystem::Get().EnqueueEvent(TickEvent);

    csp::events::EventSystem::Get().ProcessEvents();
}

const csp::common::String& CSPFoundation::GetVersion()
{
    static csp::common::String Version(CSP_FOUNDATION_COMMIT_ID);

    return Version;
}

const csp::common::String& CSPFoundation::GetBuildType()
{
    static csp::common::String BuildType(CSP_FOUNDATION_BUILD_TYPE);

    return BuildType;
}

const csp::common::String& CSPFoundation::GetBuildID()
{
    static csp::common::String BuildID(CSP_FOUNDATION_BUILD_ID);

    return BuildID;
}

const csp::common::String& CSPFoundation::GetDeviceId() { return *DeviceId; }

bool CSPFoundation::GetIsInitialised() { return IsInitialised; }

const EndpointURIs& CSPFoundation::GetEndpoints() { return *Endpoints; }

const ClientUserAgent& CSPFoundation::GetClientUserAgentInfo() { return *ClientUserAgentInfo; }

const csp::common::String& CSPFoundation::GetClientUserAgentString() { return *ClientUserAgentString; }

const csp::common::String& CSPFoundation::GetTenant() { return *Tenant; }

bool ServiceDefinition::CheckPrerequisites(const csp::systems::ServicesDeploymentStatus& ServicesDeploymentStatus) const
{
    // Evaluate State: Service Not Found (Highest Priority)
    // Attempt to find the overall status of the service within the provided deployment status.
    // If the service's reverse proxy is not found in the deployment status,
    // it implies the service is not deployed or recognized by the system.
    // This is a critical failure, and the prerequisite check immediately returns false.
    const auto ServiceStatus = FindServiceStatus(ServicesDeploymentStatus, this->GetURI());
    if (!ServiceStatus)
    {
        return false;
    }

    // Retrieve the specific version metadata for this service definition's version.
    const auto VersionMetadata = FindVersionMetadata(*ServiceStatus, this->GetVersion());

    // Evaluate State: Retired (Second Highest Priority)
    // A service version is considered 'retired' if the currently expected version
    // (from this ServiceDefinition's configuration) is no longer listed or supported
    // in the live service's API versions. This often means the version has been
    // completely removed from the live environment and is no longer operational.
    // This is a fatal condition, and the prerequisite check immediately returns false.
    if (!VersionMetadata)
    {
        HandleRetiredState(*ServiceStatus, this->GetVersion());
        return false;
    }

    // Evaluate State: Deprecated (Third Highest Priority)
    // A service version is 'deprecated' if it is still active and functional,
    // but its continued use is discouraged. It typically means the service
    // will be retired at a future date (indicated by DeprecationDatetime)
    // and clients should migrate to a newer version.
    // For prerequisite checks, a deprecated service is generally still considered
    // 'valid enough to run', but a warning is logged to inform the user.
    if (HandleDeprecatedState(*ServiceStatus, *VersionMetadata, this->GetVersion()))
    {
        return true;
    }

    // Evaluate State: Older Version Available (Fourth Highest Priority)
    // This state indicates that the service version being used is functional
    // but is not the absolute latest version available on the live system.
    // It implies there's a newer, fully supported version that clients could upgrade to.
    // This is typically an informational message, not a blocking error for prerequisites.
    if (HandleOlderVersionAvailableState(*ServiceStatus, *VersionMetadata, this->GetVersion()))
    {
        return true;
    }

    // Evaluate State: Latest Version (Lowest Priority)
    // If none of the above conditions (service not found, retired, deprecated, or older version)
    // are met, then the service's current version is the latest and fully supported version
    // available on the live system. This is the ideal and expected state.
    if (HandleLatestVersionState(*ServiceStatus, *VersionMetadata))
    {
        return true;
    }

    // Fallback in the event that the service's state could not be definitively validated.
    CSP_LOG_MSG(csp::common::LogLevel::Error, "ServiceDefinition::CheckPrerequisites: Unable to validate service state.");
    return false;
}

void CSPFoundation::SetClientUserAgentInfo(const csp::ClientUserAgent& ClientUserAgentHeader)
{
    ClientUserAgentInfo->CSPVersion = CSP_TEXT(ClientUserAgentHeader.CSPVersion);
    ClientUserAgentInfo->ClientOS = CSP_TEXT(ClientUserAgentHeader.ClientOS);
    ClientUserAgentInfo->ClientSKU = CSP_TEXT(ClientUserAgentHeader.ClientSKU);
    ClientUserAgentInfo->ClientVersion = CSP_TEXT(ClientUserAgentHeader.ClientVersion);
    ClientUserAgentInfo->ClientEnvironment = CSP_TEXT(ClientUserAgentHeader.ClientEnvironment);
    ClientUserAgentInfo->CHSEnvironment = CSP_TEXT(ClientUserAgentHeader.CHSEnvironment);
    const char* ClientUserAgentHeaderFormat = "%s/%s(%s) CSP/%s(%s) CHS(%s) CSP/%s(%s)";

    *ClientUserAgentString = csp::common::StringFormat(ClientUserAgentHeaderFormat, GetClientUserAgentInfo().ClientSKU.c_str(),
        GetClientUserAgentInfo().ClientVersion.c_str(), GetClientUserAgentInfo().ClientEnvironment.c_str(), GetVersion().c_str(),
        GetBuildType().c_str(), GetClientUserAgentInfo().CHSEnvironment.c_str(), GetClientUserAgentInfo().CSPVersion.c_str(),
        GetClientUserAgentInfo().ClientOS.c_str());
}

void Free(void* Pointer) { std::free(Pointer); }

void* ModuleHandle = nullptr;

void* GetFunctionAddress([[maybe_unused]] const csp::common::String& Name)
{
#if defined(CSP_WASM)
    return nullptr;
#else
    if (ModuleHandle == nullptr)
    {
        ModuleHandle = LOAD_OWN_MODULE();
        assert(ModuleHandle != nullptr);
    }

    return GET_FUNCTION_ADDRESS(ModuleHandle, Name.c_str());
#endif
}

} // namespace csp
