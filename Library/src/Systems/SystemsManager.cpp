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

#include "CSP/CSPFoundation.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/OfflineRealtimeEngine.h"
#include "CSP/Multiplayer/OnlineRealtimeEngine.h"
#include "CSP/Systems/Analytics/AnalyticsSystem.h"
#include "CSP/Systems/Assets/AssetSystem.h"
#include "CSP/Systems/ECommerce/ECommerceSystem.h"
#include "CSP/Systems/EventTicketing/EventTicketingSystem.h"
#include "CSP/Systems/ExternalServices/ExternalServiceProxySystem.h"
#include "CSP/Systems/GraphQL/GraphQLSystem.h"
#include "CSP/Systems/HotspotSequence/HotspotSequenceSystem.h"
#include "CSP/Systems/Maintenance/MaintenanceSystem.h"
#include "CSP/Systems/Multiplayer/MultiplayerSystem.h"
#include "CSP/Systems/Quota/QuotaSystem.h"
#include "CSP/Systems/Script/ScriptSystem.h"
#include "CSP/Systems/Sequence/SequenceSystem.h"
#include "CSP/Systems/Settings/ApplicationSettingsSystem.h"
#include "CSP/Systems/Settings/SettingsSystem.h"
#include "CSP/Systems/Spaces/SpaceSystem.h"
#include "CSP/Systems/Spatial/AnchorSystem.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "CSP/Systems/Voip/VoipSystem.h"
#include "ECommerce/ECommerceSystemHelpers.h"
#include "Multiplayer/SignalR/SignalRClient.h"
#include "Multiplayer/SignalR/SignalRConnection.h"
#include "Systems/Conversation/ConversationSystemInternal.h"
#include "Systems/Spatial/PointOfInterestInternalSystem.h"
#include "signalrclient/signalr_value.h"

#ifdef CSP_WASM
#include "Common/Web/EmscriptenWebClient/EmscriptenWebClient.h"
#else
#include "Common/Web/POCOWebClient/POCOWebClient.h"
#endif

namespace csp::systems
{

SystemsManager* SystemsManager::Instance = nullptr;

SystemsManager& SystemsManager::Get()
{
    assert(SystemsManager::Instance && "Please call csp::CSPFoundation::Initialize() before calling csp::services::GetSystemsManager().");

    return *SystemsManager::Instance;
}

UserSystem* SystemsManager::GetUserSystem() { return m_userSystem; }

SpaceSystem* SystemsManager::GetSpaceSystem() { return m_spaceSystem; }

AssetSystem* SystemsManager::GetAssetSystem() { return m_assetSystem; }

ScriptSystem* SystemsManager::GetScriptSystem() { return m_scriptSystem; }

VoipSystem* SystemsManager::GetVoipSystem() { return m_voipSystem; }

PointOfInterestSystem* SystemsManager::GetPointOfInterestSystem() { return m_pointOfInterestSystem; }

AnchorSystem* SystemsManager::GetAnchorSystem() { return m_anchorSystem; }

csp::common::LogSystem* SystemsManager::GetLogSystem() { return m_logSystem; }

ApplicationSettingsSystem* SystemsManager::GetApplicationSettingsSystem() { return m_applicationSettingsSystem; }

SettingsSystem* SystemsManager::GetSettingsSystem() { return m_settingsSystem; }

GraphQLSystem* SystemsManager::GetGraphQLSystem() { return m_graphQlSystem; }

MaintenanceSystem* SystemsManager::GetMaintenanceSystem() { return m_maintenanceSystem; }

EventTicketingSystem* SystemsManager::GetEventTicketingSystem() { return m_eventTicketingSystem; }

ECommerceSystem* SystemsManager::GetECommerceSystem() { return m_eCommerceSystem; }

QuotaSystem* SystemsManager::GetQuotaSystem() { return m_quotaSystem; }

SequenceSystem* SystemsManager::GetSequenceSystem() { return m_sequenceSystem; }

HotspotSequenceSystem* SystemsManager::GetHotspotSequenceSystem() { return m_hotspotSequenceSystem; }

AnalyticsSystem* SystemsManager::GetAnalyticsSystem() { return m_analyticsSystem; }

ExternalServiceProxySystem* SystemsManager::GetExternalServicesProxySystem() { return m_externalServiceProxySystem; }

MultiplayerSystem* SystemsManager::GetMultiplayerSystem() { return m_multiplayerSystem; }

csp::multiplayer::MultiplayerConnection* SystemsManager::GetMultiplayerConnection() { return m_multiplayerConnection; }

csp::multiplayer::NetworkEventBus* SystemsManager::GetEventBus() { return &m_multiplayerConnection->GetEventBus(); }

csp::multiplayer::OnlineRealtimeEngine* SystemsManager::MakeOnlineRealtimeEngine()
{
    return new csp::multiplayer::OnlineRealtimeEngine { *GetMultiplayerConnection(), *GetLogSystem(), *GetEventBus(), *GetScriptSystem() };
}

csp::multiplayer::OfflineRealtimeEngine* SystemsManager::MakeOfflineRealtimeEngine()
{
    return new csp::multiplayer::OfflineRealtimeEngine { *GetLogSystem(), *GetScriptSystem() };
}

csp::common::IRealtimeEngine* SystemsManager::MakeRealtimeEngine(csp::common::RealtimeEngineType realtimeEngineType)
{
    switch (realtimeEngineType)
    {
    case RealtimeEngineType::Online:
        return MakeOnlineRealtimeEngine();
    case RealtimeEngineType::Offline:
        return MakeOfflineRealtimeEngine();
    default:
        throw std::runtime_error("Unknown Realtime Engine Type");
    }
}

SystemsManager::SystemsManager()
    : m_webClient(nullptr)
    , m_multiplayerConnection(nullptr)
    , m_realtimeEngine(nullptr)
    , m_userSystem(nullptr)
    , m_spaceSystem(nullptr)
    , m_assetSystem(nullptr)
    , m_scriptSystem(nullptr)
    , m_voipSystem(nullptr)
    , m_pointOfInterestSystem(nullptr)
    , m_anchorSystem(nullptr)
    , m_logSystem(nullptr)
    , m_applicationSettingsSystem(nullptr)
    , m_settingsSystem(nullptr)
    , m_graphQlSystem(nullptr)
    , m_maintenanceSystem(nullptr)
    , m_eventTicketingSystem(nullptr)
    , m_eCommerceSystem(nullptr)
    , m_quotaSystem(nullptr)
    , m_sequenceSystem(nullptr)
    , m_hotspotSequenceSystem(nullptr)
    , m_conversationSystem(nullptr)
    , m_analyticsSystem(nullptr)
    , m_externalServiceProxySystem(nullptr)
    , m_multiplayerSystem(nullptr)
{
}

SystemsManager::~SystemsManager() { DestroySystems(); }

ConversationSystemInternal* SystemsManager::GetConversationSystem() { return m_conversationSystem; }

void SystemsManager::CreateSystems(csp::multiplayer::ISignalRConnection* signalRInject, csp::web::WebClient* webClientInject)
{
    // Create Log system first, so we can log any startup issues in other systems
    m_logSystem = new csp::common::LogSystem();

    if (webClientInject)
    {
        m_webClient = webClientInject;
    }
    else
    {
#ifdef CSP_WASM
        WebClient = new csp::web::EmscriptenWebClient(80, csp::web::ETransferProtocol::HTTPS, LogSystem);
#else
        m_webClient = new csp::web::POCOWebClient(80, csp::web::ETransferProtocol::HTTPS, m_logSystem);
#endif
    }

    // Emergency Fix: We have a circular dependency issue here due to SignalR requiring the AuthContext for construction. To get around this
    // we pass nullptr for the NetworkEventBus and then set it after it has been constructed below.
    m_userSystem = new csp::systems::UserSystem(m_webClient, nullptr, *m_logSystem);

    m_webClient->SetAuthContext(m_userSystem->GetAuthContext());

    m_scriptSystem = new csp::systems::ScriptSystem();
    m_scriptSystem->Initialise();

    // At the moment, the inject is for mocking behaviour. In the future this will probably not even be instantiated here at all.
    auto* signalRConnection
        = (signalRInject == nullptr) ? csp::multiplayer::MultiplayerConnection::MakeSignalRConnection(m_userSystem->GetAuthContext()) : signalRInject;

    m_multiplayerConnection = new csp::multiplayer::MultiplayerConnection(*m_logSystem, *signalRConnection);

    // Set the NetworkEventBus now that it has been initialized.
    m_userSystem->SetNetworkEventBus(m_multiplayerConnection->GetEventBus());

    m_voipSystem = new csp::systems::VoipSystem();

    // SystemBase inheritors

    m_spaceSystem = new csp::systems::SpaceSystem(m_webClient, m_multiplayerConnection->GetEventBus(), m_userSystem, *m_logSystem);
    m_assetSystem = new csp::systems::AssetSystem(m_webClient, m_multiplayerConnection->GetEventBus(), m_userSystem->GetAuthContext(), *m_logSystem);
    m_anchorSystem = new csp::systems::AnchorSystem(m_webClient, *m_logSystem);
    m_pointOfInterestSystem = new csp::systems::PointOfInterestInternalSystem(m_webClient, *m_logSystem);
    m_applicationSettingsSystem = new csp::systems::ApplicationSettingsSystem(m_webClient, *m_logSystem);
    m_settingsSystem = new csp::systems::SettingsSystem(m_webClient, *m_logSystem);
    m_graphQlSystem = new csp::systems::GraphQLSystem(m_webClient, *m_logSystem);
    m_maintenanceSystem = new csp::systems::MaintenanceSystem(m_webClient, *m_logSystem);
    m_eventTicketingSystem = new csp::systems::EventTicketingSystem(m_webClient, *m_logSystem);
    m_eCommerceSystem = new csp::systems::ECommerceSystem(m_webClient, *m_logSystem);
    m_quotaSystem = new csp::systems::QuotaSystem(m_webClient, *m_logSystem, m_userSystem->GetAuthContext());
    m_sequenceSystem = new csp::systems::SequenceSystem(m_webClient, m_multiplayerConnection->GetEventBus(), *m_logSystem);
    m_hotspotSequenceSystem = new csp::systems::HotspotSequenceSystem(m_sequenceSystem, m_spaceSystem, m_multiplayerConnection->GetEventBus(), *m_logSystem);
    m_conversationSystem
        = new csp::systems::ConversationSystemInternal(m_assetSystem, m_spaceSystem, m_userSystem, m_multiplayerConnection->GetEventBus(), *m_logSystem);
    m_analyticsSystem = new csp::systems::AnalyticsSystem(m_webClient, &(csp::CSPFoundation::GetClientUserAgentInfo()), *m_logSystem);
    m_externalServiceProxySystem = new csp::systems::ExternalServiceProxySystem(m_webClient, *m_logSystem);
    m_multiplayerSystem = new csp::systems::MultiplayerSystem(m_webClient, *m_spaceSystem, *m_logSystem);
    m_spaceSystem->SetMultiplayerSystem(*m_multiplayerSystem);
}

void SystemsManager::DestroySystems()
{
    // Systems must be shut down in reverse order to CreateSystems() to ensure that any
    // dependencies continue to exist until each system is successfully shut down.
    delete m_externalServiceProxySystem;
    delete m_analyticsSystem;
    delete m_conversationSystem;
    delete m_hotspotSequenceSystem;
    delete m_sequenceSystem;
    delete m_quotaSystem;
    delete m_eCommerceSystem;
    delete m_eventTicketingSystem;
    delete m_maintenanceSystem;
    delete m_graphQlSystem;
    delete m_settingsSystem;
    delete m_applicationSettingsSystem;
    delete m_pointOfInterestSystem;
    delete m_anchorSystem;
    delete m_assetSystem;
    delete m_spaceSystem;
    delete m_userSystem;
    delete m_voipSystem;
    delete m_multiplayerConnection; // Also deletes NetworkEventBus
    delete m_scriptSystem;
    delete m_multiplayerSystem;
    delete m_webClient;
    delete m_logSystem;
}

void SystemsManager::Instantiate(csp::multiplayer::ISignalRConnection* signalRInject, csp::web::WebClient* webClientInject)
{
    Instance = new SystemsManager();
    Instance->CreateSystems(signalRInject, webClientInject);
}

void SystemsManager::Destroy()
{
    assert(Instance && "Please call csp::CSPFoundation::Initialize() before calling csp::CSPFoundation::Shutdown().");
    delete (Instance);
    Instance = nullptr;
}

} // namespace csp::systems
