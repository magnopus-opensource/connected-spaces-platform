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

#include "CSP/Multiplayer/MultiplayerConnection.h"
#include "CSP/Systems/Analytics/AnalyticsSystem.h"
#include "CSP/Systems/Assets/AssetSystem.h"
#include "CSP/Systems/ECommerce/ECommerceSystem.h"
#include "CSP/Systems/EventTicketing/EventTicketingSystem.h"
#include "CSP/Systems/GraphQL/GraphQLSystem.h"
#include "CSP/Systems/HotspotSequence/HotspotSequenceSystem.h"
#include "CSP/Systems/Log/LogSystem.h"
#include "CSP/Systems/Maintenance/MaintenanceSystem.h"
#include "CSP/Systems/Organizations/OrganizationSystem.h"
#include "CSP/Systems/Quota/QuotaSystem.h"
#include "CSP/Systems/Script/ScriptSystem.h"
#include "CSP/Systems/Sequence/SequenceSystem.h"
#include "CSP/Systems/Settings/SettingsSystem.h"
#include "CSP/Systems/Spaces/SpaceSystem.h"
#include "CSP/Systems/Spatial/AnchorSystem.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "CSP/Systems/Voip/VoipSystem.h"
#include "ECommerce/ECommerceSystemHelpers.h"
#include "Memory/Memory.h"
#include "Systems/Spatial/PointOfInterestInternalSystem.h"
#include "signalrclient/signalr_value.h"

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

ECommerceSystem* SystemsManager::GetECommerceSystem()
{
	return ECommerceSystem;
}

QuotaSystem* SystemsManager::GetQuotaSystem()
{
	return QuotaSystem;
}

OrganizationSystem* SystemsManager::GetOrganizationSystem()
{
	return OrganizationSystem;
}

SequenceSystem* SystemsManager::GetSequenceSystem()
{
	return SequenceSystem;
}

HotspotSequenceSystem* SystemsManager::GetHotspotSequenceSystem()
{
	return HotspotSequenceSystem;
}

csp::multiplayer::SpaceEntitySystem* SystemsManager::GetSpaceEntitySystem()
{
	return SpaceEntitySystem;
}

csp::multiplayer::MultiplayerConnection* SystemsManager::GetMultiplayerConnection()
{
	return MultiplayerConnection;
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
	, ECommerceSystem(nullptr)
	, QuotaSystem(nullptr)
	, OrganizationSystem(nullptr)
	, MultiplayerConnection(nullptr)
	, SpaceEntitySystem(nullptr)
	, SequenceSystem(nullptr)
	, HotspotSequenceSystem(nullptr)
{
}

SystemsManager::~SystemsManager()
{
	DestroySystems();
}

void SystemsManager::CreateSystems()
{
	// Create Log system first, so we can log any startup issues in other systems
	LogSystem = new csp::systems::LogSystem();

#ifdef CSP_WASM
	WebClient = new csp::web::EmscriptenWebClient(80, csp::web::ETransferProtocol::HTTPS);
#else
	WebClient = new csp::web::POCOWebClient(80, csp::web::ETransferProtocol::HTTPS);
#endif
	ScriptSystem = new csp::systems::ScriptSystem();

	ScriptSystem->Initialise();

	MultiplayerConnection = new csp::multiplayer::MultiplayerConnection();

	AnalyticsSystem		  = new csp::systems::AnalyticsSystem();
	UserSystem			  = new csp::systems::UserSystem(WebClient);
	SpaceSystem			  = new csp::systems::SpaceSystem(WebClient);
	AssetSystem			  = new csp::systems::AssetSystem(WebClient);
	AnchorSystem		  = new csp::systems::AnchorSystem(WebClient);
	PointOfInterestSystem = new csp::systems::PointOfInterestInternalSystem(WebClient);
	SettingsSystem		  = new csp::systems::SettingsSystem(WebClient);
	GraphQLSystem		  = new csp::systems::GraphQLSystem(WebClient);
	VoipSystem			  = new csp::systems::VoipSystem();
	MaintenanceSystem	  = new csp::systems::MaintenanceSystem(WebClient);
	EventTicketingSystem  = new csp::systems::EventTicketingSystem(WebClient);
	ECommerceSystem		  = new csp::systems::ECommerceSystem(WebClient);
	QuotaSystem			  = new csp::systems::QuotaSystem(WebClient);
	OrganizationSystem	  = new csp::systems::OrganizationSystem(WebClient);
	SequenceSystem		  = new csp::systems::SequenceSystem(WebClient);
	HotspotSequenceSystem = new csp::systems::HotspotSequenceSystem(SequenceSystem, SpaceSystem);
	SpaceEntitySystem	  = new csp::multiplayer::SpaceEntitySystem(MultiplayerConnection);
}

void SystemsManager::DestroySystems()
{
	// Systems must be shut down in reverse order to CreateSystems() to ensure that any
	// dependencies continue to exist until each system is successfully shut down.
	delete (SpaceEntitySystem);
	delete (OrganizationSystem);
	delete (QuotaSystem);
	delete (ECommerceSystem);
	delete (EventTicketingSystem);
	delete (MaintenanceSystem);
	delete (VoipSystem);
	delete (GraphQLSystem);
	delete (SettingsSystem);
	delete (PointOfInterestSystem);
	delete (AnchorSystem);
	delete (AssetSystem);
	delete (AnalyticsSystem);
	delete (MultiplayerConnection);
	delete (ScriptSystem);
	delete (SpaceSystem);
	delete (UserSystem);
	delete (HotspotSequenceSystem);

	delete (WebClient);
	delete (LogSystem);
}

void SystemsManager::Instantiate()
{
	assert(!Instance && "Please call csp::CSPFoundation::Shutdown() before calling csp::CSPFoundation::Initialize() again.");
	Instance = new SystemsManager();
	Instance->CreateSystems();
}

void SystemsManager::Destroy()
{
	assert(Instance && "Please call csp::CSPFoundation::Initialize() before calling csp::CSPFoundation::Shutdown().");
	delete (Instance);
	Instance = nullptr;
}

} // namespace csp::systems
