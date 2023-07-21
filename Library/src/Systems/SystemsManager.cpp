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
#include "CSP/Systems/SystemsManager.h"

#include "CSP/Systems/Analytics/AnalyticsSystem.h"
#include "CSP/Systems/Assets/AssetSystem.h"
#include "CSP/Systems/EventTicketing/EventTicketingSystem.h"
#include "CSP/Systems/GraphQL/GraphQLSystem.h"
#include "CSP/Systems/Log/LogSystem.h"
#include "CSP/Systems/Maintenance/MaintenanceSystem.h"
#include "CSP/Systems/Script/ScriptSystem.h"
#include "CSP/Systems/Settings/SettingsSystem.h"
#include "CSP/Systems/Spaces/SpaceSystem.h"
#include "CSP/Systems/Spatial/AnchorSystem.h"
#include "CSP/Systems/Voip/VoipSystem.h"
#include "Memory/Memory.h"
#include "Systems/Spatial/PointOfInterestInternalSystem.h"
#include "Systems/Users/UserSystem.internal.h"

#ifdef CSP_WASM
	#include "Web/EmscriptenWebClient/EmscriptenWebClient.h"
#else
	#include "Web/POCOWebClient/POCOWebClient.h"
#endif


namespace csp::systems
{

SystemsManager* SystemsManager::Instance = nullptr;

SystemsManager& SystemsManager::Get()
{
	assert(SystemsManager::Instance && "Please call csp::CSPFoundation::Initialize() before calling csp::services::GetSystemsManager().");

	return *SystemsManager::Instance;
}

UserSystem* SystemsManager::GetUserSystem()
{
	return UserSystem;
}

SpaceSystem* SystemsManager::GetSpaceSystem()
{
	return SpaceSystem;
}

AssetSystem* SystemsManager::GetAssetSystem()
{
	return AssetSystem;
}

ScriptSystem* SystemsManager::GetScriptSystem()
{
	return ScriptSystem;
}

VoipSystem* SystemsManager::GetVoipSystem()
{
	return VoipSystem;
}

PointOfInterestSystem* SystemsManager::GetPointOfInterestSystem()
{
	return PointOfInterestSystem;
}

AnchorSystem* SystemsManager::GetAnchorSystem()
{
	return AnchorSystem;
}

LogSystem* SystemsManager::GetLogSystem()
{
	return LogSystem;
}

SettingsSystem* SystemsManager::GetSettingsSystem()
{
	return SettingsSystem;
}

GraphQLSystem* SystemsManager::GetGraphQLSystem()
{
	return GraphQLSystem;
}

AnalyticsSystem* SystemsManager::GetAnalyticsSystem()
{
	return AnalyticsSystem;
}

MaintenanceSystem* SystemsManager::GetMaintenanceSystem()
{
	return MaintenanceSystem;
}

EventTicketingSystem* SystemsManager::GetEventTicketingSystem()
{
	return EventTicketingSystem;
}

SystemsManager::SystemsManager()
	: WebClient(nullptr)
	, UserSystem(nullptr)
	, SpaceSystem(nullptr)
	, AssetSystem(nullptr)
	, ScriptSystem(nullptr)
	, VoipSystem(nullptr)
	, PointOfInterestSystem(nullptr)
	, AnchorSystem(nullptr)
	, LogSystem(nullptr)
	, SettingsSystem(nullptr)
	, GraphQLSystem(nullptr)
	, AnalyticsSystem(nullptr)
	, MaintenanceSystem(nullptr)
	, EventTicketingSystem(nullptr)
{
}

SystemsManager::~SystemsManager()
{
	DestroySystems();
}

void SystemsManager::CreateSystems()
{
	// Create Log system first, so we can log any startup issues in other systems
	LogSystem = CSP_NEW csp::systems::LogSystem();

#ifdef CSP_WASM
	WebClient = CSP_NEW csp::web::EmscriptenWebClient(80, csp::web::ETransferProtocol::HTTPS);
#else
	WebClient = CSP_NEW csp::web::POCOWebClient(80, csp::web::ETransferProtocol::HTTPS);
#endif

	AnalyticsSystem = CSP_NEW csp::systems::AnalyticsSystem();

	UserSystem			  = CSP_NEW csp::systems_internal::UserSystem(WebClient);
	SpaceSystem			  = CSP_NEW csp::systems::SpaceSystem(WebClient);
	AssetSystem			  = CSP_NEW csp::systems::AssetSystem(WebClient);
	AnchorSystem		  = CSP_NEW csp::systems::AnchorSystem(WebClient);
	PointOfInterestSystem = CSP_NEW csp::systems::PointOfInterestInternalSystem(WebClient);
	SettingsSystem		  = CSP_NEW csp::systems::SettingsSystem(WebClient);
	GraphQLSystem		  = CSP_NEW csp::systems::GraphQLSystem(WebClient);
	ScriptSystem		  = CSP_NEW csp::systems::ScriptSystem();
	VoipSystem			  = CSP_NEW csp::systems::VoipSystem();
	MaintenanceSystem	  = CSP_NEW csp::systems::MaintenanceSystem(WebClient);
	EventTicketingSystem  = CSP_NEW csp::systems::EventTicketingSystem(WebClient);
}

void SystemsManager::DestroySystems()
{
	CSP_DELETE(UserSystem);
	CSP_DELETE(SpaceSystem);
	CSP_DELETE(AssetSystem);
	CSP_DELETE(VoipSystem);
	CSP_DELETE(PointOfInterestSystem);
	CSP_DELETE(AnchorSystem);
	CSP_DELETE(ScriptSystem);
	CSP_DELETE(SettingsSystem);
	CSP_DELETE(GraphQLSystem);

	CSP_DELETE(AnalyticsSystem);
	CSP_DELETE(WebClient);
	CSP_DELETE(LogSystem);
	CSP_DELETE(MaintenanceSystem);
	CSP_DELETE(EventTicketingSystem);
}

void SystemsManager::Instantiate()
{
	assert(!Instance && "Please call csp::CSPFoundation::Shutdown() before calling csp::CSPFoundation::Initialize() again.");
	Instance = CSP_NEW SystemsManager();
	Instance->CreateSystems();
}

void SystemsManager::Destroy()
{
	assert(Instance && "Please call csp::CSPFoundation::Initialize() before calling csp::CSPFoundation::Shutdown().");
	CSP_DELETE(Instance);
	Instance = nullptr;
}

} // namespace csp::systems
