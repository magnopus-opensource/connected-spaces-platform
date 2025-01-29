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

UserSystem* SystemsManager::GetUserSystem() { return UserSystem; }

SpaceSystem* SystemsManager::GetSpaceSystem() { return SpaceSystem; }

AssetSystem* SystemsManager::GetAssetSystem() { return AssetSystem; }

ScriptSystem* SystemsManager::GetScriptSystem() { return ScriptSystem; }

VoipSystem* SystemsManager::GetVoipSystem() { return VoipSystem; }

PointOfInterestSystem* SystemsManager::GetPointOfInterestSystem() { return PointOfInterestSystem; }

AnchorSystem* SystemsManager::GetAnchorSystem() { return AnchorSystem; }

LogSystem* SystemsManager::GetLogSystem() { return LogSystem; }

SettingsSystem* SystemsManager::GetSettingsSystem() { return SettingsSystem; }

GraphQLSystem* SystemsManager::GetGraphQLSystem() { return GraphQLSystem; }

AnalyticsSystem* SystemsManager::GetAnalyticsSystem() { return AnalyticsSystem; }

MaintenanceSystem* SystemsManager::GetMaintenanceSystem() { return MaintenanceSystem; }

EventTicketingSystem* SystemsManager::GetEventTicketingSystem() { return EventTicketingSystem; }

ECommerceSystem* SystemsManager::GetECommerceSystem() { return ECommerceSystem; }

QuotaSystem* SystemsManager::GetQuotaSystem() { return QuotaSystem; }

OrganizationSystem* SystemsManager::GetOrganizationSystem() { return OrganizationSystem; }

SequenceSystem* SystemsManager::GetSequenceSystem() { return SequenceSystem; }

HotspotSequenceSystem* SystemsManager::GetHotspotSequenceSystem() { return HotspotSequenceSystem; }

csp::multiplayer::SpaceEntitySystem* SystemsManager::GetSpaceEntitySystem() { return SpaceEntitySystem; }

csp::multiplayer::MultiplayerConnection* SystemsManager::GetMultiplayerConnection() { return MultiplayerConnection; }

csp::multiplayer::EventBus* SystemsManager::GetEventBus() { return EventBus; }

SystemsManager::SystemsManager()
    : WebClient(nullptr)
    , MultiplayerConnection(nullptr)
    , EventBus(nullptr)
    , SpaceEntitySystem(nullptr)
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
    , SequenceSystem(nullptr)
    , HotspotSequenceSystem(nullptr)
{
}

SystemsManager::~SystemsManager() { DestroySystems(); }

void SystemsManager::CreateSystems()
{
    // Create Log system first, so we can log any startup issues in other systems
    LogSystem = CSP_NEW csp::systems::LogSystem();

#ifdef CSP_WASM
    WebClient = CSP_NEW csp::web::EmscriptenWebClient(80, csp::web::ETransferProtocol::HTTPS);
#else
    WebClient = CSP_NEW csp::web::POCOWebClient(80, csp::web::ETransferProtocol::HTTPS);
#endif
    ScriptSystem = CSP_NEW csp::systems::ScriptSystem();

    ScriptSystem->Initialise();

    MultiplayerConnection = CSP_NEW csp::multiplayer::MultiplayerConnection();
    EventBus = MultiplayerConnection->EventBusPtr;

    AnalyticsSystem = CSP_NEW csp::systems::AnalyticsSystem();
    UserSystem = CSP_NEW csp::systems::UserSystem(WebClient, EventBus);
    SpaceSystem = CSP_NEW csp::systems::SpaceSystem(WebClient);
    AssetSystem = CSP_NEW csp::systems::AssetSystem(WebClient, EventBus);
    AnchorSystem = CSP_NEW csp::systems::AnchorSystem(WebClient);
    PointOfInterestSystem = CSP_NEW csp::systems::PointOfInterestInternalSystem(WebClient);
    SettingsSystem = CSP_NEW csp::systems::SettingsSystem(WebClient);
    GraphQLSystem = CSP_NEW csp::systems::GraphQLSystem(WebClient);
    VoipSystem = CSP_NEW csp::systems::VoipSystem();
    MaintenanceSystem = CSP_NEW csp::systems::MaintenanceSystem(WebClient);
    EventTicketingSystem = CSP_NEW csp::systems::EventTicketingSystem(WebClient);
    ECommerceSystem = CSP_NEW csp::systems::ECommerceSystem(WebClient);
    QuotaSystem = CSP_NEW csp::systems::QuotaSystem(WebClient);
    OrganizationSystem = CSP_NEW csp::systems::OrganizationSystem(WebClient);
    SequenceSystem = CSP_NEW csp::systems::SequenceSystem(WebClient, EventBus);
    HotspotSequenceSystem = CSP_NEW csp::systems::HotspotSequenceSystem(SequenceSystem, SpaceSystem, EventBus);
    SpaceEntitySystem = CSP_NEW csp::multiplayer::SpaceEntitySystem(MultiplayerConnection);
}

void SystemsManager::DestroySystems()
{
    // Systems must be shut down in reverse order to CreateSystems() to ensure that any
    // dependencies continue to exist until each system is successfully shut down.
    CSP_DELETE(SpaceEntitySystem);
    CSP_DELETE(HotspotSequenceSystem);
    CSP_DELETE(SequenceSystem);
    CSP_DELETE(OrganizationSystem);
    CSP_DELETE(QuotaSystem);
    CSP_DELETE(ECommerceSystem);
    CSP_DELETE(EventTicketingSystem);
    CSP_DELETE(MaintenanceSystem);
    CSP_DELETE(VoipSystem);
    CSP_DELETE(GraphQLSystem);
    CSP_DELETE(SettingsSystem);
    CSP_DELETE(PointOfInterestSystem);
    CSP_DELETE(AnchorSystem);
    CSP_DELETE(AssetSystem);
    CSP_DELETE(SpaceSystem);
    CSP_DELETE(UserSystem);
    CSP_DELETE(AnalyticsSystem);
    CSP_DELETE(MultiplayerConnection);
    CSP_DELETE(ScriptSystem);

    CSP_DELETE(WebClient);
    CSP_DELETE(LogSystem);
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
