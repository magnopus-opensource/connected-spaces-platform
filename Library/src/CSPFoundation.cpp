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
#include "CSP/Systems/SystemsManager.h"
#include "CSP/version.h"
#include "Common/UUIDGenerator.h"
#include "Common/Wrappers.h"
#include "Debug/Logging.h"
#include "Events/EventSystem.h"

#include <cstdio>

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

	#define LOAD_OWN_MODULE()				(void*) GetModuleHandleA(LIB_NAME)
	#define GET_FUNCTION_ADDRESS(mod, name) (void*) GetProcAddress((HMODULE) (mod), name)
#elif defined(CSP_ANDROID)
	// For dlopen and dlsym
	#include <dlfcn.h>
	// For fstat and mkdir
	#include <sys/stat.h>

	#define LOAD_OWN_MODULE()				dlopen(nullptr, RTLD_LAZY)
	#define GET_FUNCTION_ADDRESS(mod, name) dlsym((mod), name)
#elif defined(CSP_MACOSX) || defined(CSP_IOS)
	// For dlopen and dlsym
	#include <dlfcn.h>
	// For stat and mkdir
	#include <sys/stat.h>
	// For PATH_MAX
	#include <sys/syslimits.h>

	#define LOAD_OWN_MODULE()				dlopen(nullptr, RTLD_LAZY)
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
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t> > Conv;

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

bool CSPFoundation::IsInitialised						  = false;
EndpointURIs* CSPFoundation::Endpoints					  = nullptr;
ClientUserAgent* CSPFoundation::ClientUserAgentInfo		  = nullptr;
csp::common::String* CSPFoundation::DeviceId			  = nullptr;
csp::common::String* CSPFoundation::ClientUserAgentString = nullptr;
csp::common::String* CSPFoundation::Tenant				  = nullptr;

bool CSPFoundation::Initialise(const csp::common::String& EndpointRootURI, const csp::common::String& InTenant)
{
	if (IsInitialised)
	{
		return false;
	}

	// remove last char if a slash
	std::string RootURI(EndpointRootURI.c_str());
	while (RootURI.rbegin() != RootURI.rend() && (*RootURI.rbegin() == '\\' || *RootURI.rbegin() == '/'))
	{
		RootURI.resize(RootURI.length() - 1);
	}

	Tenant = CSP_NEW csp::common::String(InTenant);

	const std::string UserServiceURI		= RootURI + "/mag-user";
	const std::string PrototypeServiceURI	= RootURI + "/mag-prototype";
	const std::string SpatialDataServiceURI = RootURI + "/mag-spatialdata";
	const std::string MultiplayerServiceURI = RootURI + "/mag-multiplayer/hubs/v1/multiplayer";
	const std::string AggregationServiceURI = RootURI + "/oly-aggregation";
	const std::string TrackingServiceURI	= RootURI + "/mag-tracking";


	Endpoints			  = CSP_NEW EndpointURIs();
	ClientUserAgentInfo	  = CSP_NEW ClientUserAgent();
	DeviceId			  = CSP_NEW csp::common::String("");
	ClientUserAgentString = CSP_NEW csp::common::String("");

	Endpoints->UserServiceURI		 = CSP_TEXT(UserServiceURI.c_str());
	Endpoints->PrototypeServiceURI	 = CSP_TEXT(PrototypeServiceURI.c_str());
	Endpoints->SpatialDataServiceURI = CSP_TEXT(SpatialDataServiceURI.c_str());
	Endpoints->MultiplayerServiceURI = CSP_TEXT(MultiplayerServiceURI.c_str());
	Endpoints->AggregationServiceURI = CSP_TEXT(AggregationServiceURI.c_str());
	Endpoints->TrackingServiceURI	 = CSP_TEXT(TrackingServiceURI.c_str());

	csp::systems::SystemsManager::Instantiate();

	*DeviceId	  = LoadDeviceId().c_str();
	IsInitialised = true;

	// Initialises ClientAgentHeaderInfo with default values in case the client doesn't call SetClientUserAgentInfo().
	ClientUserAgent ClientAgentHeaderInfo;
	ClientAgentHeaderInfo.CSPVersion		= CSP_TEXT("CSPVersionUnset");
	ClientAgentHeaderInfo.ClientOS			= CSP_TEXT("ClientOSUnset");
	ClientAgentHeaderInfo.ClientSKU			= CSP_TEXT("ClientSKUUnset");
	ClientAgentHeaderInfo.ClientVersion		= CSP_TEXT("ClientVersionUnset");
	ClientAgentHeaderInfo.ClientEnvironment = CSP_TEXT("ClientBuildTypeUnset");
	ClientAgentHeaderInfo.CHSEnvironment	= CSP_TEXT("CHSEnvironmentUnset");

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

	CSP_DELETE(Tenant);
	CSP_DELETE(Endpoints);
	CSP_DELETE(ClientUserAgentInfo);
	CSP_DELETE(DeviceId);
	CSP_DELETE(ClientUserAgentString);

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

const csp::common::String& CSPFoundation::GetDeviceId()
{
	return *DeviceId;
}

bool CSPFoundation::GetIsInitialised()
{
	return IsInitialised;
}

const EndpointURIs& CSPFoundation::GetEndpoints()
{
	return *Endpoints;
}

const ClientUserAgent& CSPFoundation::GetClientUserAgentInfo()
{
	return *ClientUserAgentInfo;
}

const csp::common::String& CSPFoundation::GetClientUserAgentString()
{
	return *ClientUserAgentString;
}

const csp::common::String& CSPFoundation::GetTenant()
{
	return *Tenant;
}

void CSPFoundation::SetClientUserAgentInfo(const csp::ClientUserAgent& ClientUserAgentHeader)
{
	ClientUserAgentInfo->CSPVersion			= CSP_TEXT(ClientUserAgentHeader.CSPVersion);
	ClientUserAgentInfo->ClientOS			= CSP_TEXT(ClientUserAgentHeader.ClientOS);
	ClientUserAgentInfo->ClientSKU			= CSP_TEXT(ClientUserAgentHeader.ClientSKU);
	ClientUserAgentInfo->ClientVersion		= CSP_TEXT(ClientUserAgentHeader.ClientVersion);
	ClientUserAgentInfo->ClientEnvironment	= CSP_TEXT(ClientUserAgentHeader.ClientEnvironment);
	ClientUserAgentInfo->CHSEnvironment		= CSP_TEXT(ClientUserAgentHeader.CHSEnvironment);
	const char* ClientUserAgentHeaderFormat = "%s/%s(%s) Foundation/%s(%s) CHS(%s) CSP/%s(%s)";

	*ClientUserAgentString = csp::common::StringFormat(ClientUserAgentHeaderFormat,
													   GetClientUserAgentInfo().ClientSKU.c_str(),
													   GetClientUserAgentInfo().ClientVersion.c_str(),
													   GetClientUserAgentInfo().ClientEnvironment.c_str(),
													   GetVersion().c_str(),
													   GetBuildType().c_str(),
													   GetClientUserAgentInfo().CHSEnvironment.c_str(),
													   GetClientUserAgentInfo().CSPVersion.c_str(),
													   GetClientUserAgentInfo().ClientOS.c_str());
}


void Free(void* Pointer)
{
	CSP_FREE(Pointer);
}

void* ModuleHandle = nullptr;

void* GetFunctionAddress(const csp::common::String& Name)
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
