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
#include "CSP/Common/Interfaces/IRealtimeEngine.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"

#include <chrono>
#include <functional>

namespace csp::common
{
class LogSystem;
class IRealtimeEngine;
}

namespace csp::systems
{

class SystemsManager;
class UserSystem;
class SpaceSystem;
class AssetSystem;
class VoipSystem;
class ScriptSystem;
class PointOfInterestSystem;
class PointOfInterestInternalSystem;
class AnchorSystem;
class ApplicationSettingsSystem;
class SettingsSystem;
class GraphQLSystem;
class MaintenanceSystem;
class EventTicketingSystem;
class ECommerceSystem;
class QuotaSystem;
class SequenceSystem;
class HotspotSequenceSystem;
class ConversationSystemInternal;
class AnalyticsSystem;
class ExternalServiceProxySystem;

} // namespace csp::systems

namespace csp::multiplayer
{
class OnlineRealtimeEngine;
class OfflineRealtimeEngine;
} // namespace csp::multiplayer

namespace csp
{

class CSPFoundation;

} // namespace csp

namespace csp::web
{

class WebClient;

} // namespace csp::web

namespace csp::systems
{

/// @ingroup Systems
/// @brief Interface used to access each of the systems.
class CSP_API SystemsManager
{
    CSP_START_IGNORE
    /** @cond DO_NOT_DOCUMENT */
    friend class csp::CSPFoundation;
    friend class csp::multiplayer::ConversationSpaceComponent;
    /** @endcond */
    CSP_END_IGNORE

public:
    static SystemsManager& Get();

    /// @brief Retrieves user system.
    /// @return SpaceSystem : pointer to the user system class
    UserSystem* GetUserSystem();

    /// @brief Retrieves space system.
    /// @return SpaceSystem : pointer to the space system class
    SpaceSystem* GetSpaceSystem();

    /// @brief Retrieves asset system.
    /// @return SpaceSystem : pointer to the asset system class
    AssetSystem* GetAssetSystem();

    /// @brief Retrieves voip system.
    /// @return SpaceSystem : pointer to the voip system class
    VoipSystem* GetVoipSystem();

    /// @brief Retrieves script system.
    /// @return ScriptingSystem : pointer to the script system class
    ScriptSystem* GetScriptSystem();

    /// @brief Retrieves the Point Of Interest system.
    /// @return PointOfInterestSystem : pointer to the POI system class
    PointOfInterestSystem* GetPointOfInterestSystem();

    /// @brief Retrieves the Anchor system.
    /// @return AnchorSystem : pointer to the Anchor system class
    AnchorSystem* GetAnchorSystem();

    /// @brief Retrieves the Log system.
    /// @return LogSystem : pointer to the Log system class
    csp::common::LogSystem* GetLogSystem();

    /// @brief Retrieves the Application Settings system.
    /// @return ApplicationSettingsSystem : pointer to the Application Settings system class
    ApplicationSettingsSystem* GetApplicationSettingsSystem();

    /// @brief Retrieves the Settings system.
    /// @return SettingsSystem : pointer to the Settings system class
    SettingsSystem* GetSettingsSystem();

    /// @brief Retrieves the GraphQL system.
    /// @return GraphQLSystem : pointer to the GraphQL system class
    GraphQLSystem* GetGraphQLSystem();

    /// @brief Retrieves the Maintenance system.
    /// @return MaintenanceSystem : pointer to the Maintenance system class
    MaintenanceSystem* GetMaintenanceSystem();

    /// @brief Retrieves the Event Ticketing system.
    /// @return EventTicketingSystem : pointer to the Event Ticketing system class
    EventTicketingSystem* GetEventTicketingSystem();

    /// @brief Retrieves the ECommerce system.
    /// @return ECommerceSystem : pointer to the ECommerce system class
    ECommerceSystem* GetECommerceSystem();

    /// @brief Retrieves the Quota system.
    /// @return QuotaSystem : pointer to the Quota system class
    QuotaSystem* GetQuotaSystem();

    /// @brief Retrieves the Sequence system.
    /// @return SequenceSystem : pointer to the Sequence system class
    SequenceSystem* GetSequenceSystem();

    /// @brief Retrieves the HotspotSequenceSystem system.
    /// @return HotspotSequenceSystem : pointer to the HotspotSequenceSystem system class
    HotspotSequenceSystem* GetHotspotSequenceSystem();

    /// @brief Retrieves the AnalyticsSystem system.
    /// @return AnalyticsSystem : pointer to the AnalyticsSystem system class
    AnalyticsSystem* GetAnalyticsSystem();

    /// @brief Retrieves external services proxy system.
    /// @return ExternalServiceProxySystem : pointer to the external services proxy system class.
    ExternalServiceProxySystem* GetExternalServicesProxySystem();

    csp::multiplayer::MultiplayerConnection* GetMultiplayerConnection();

    csp::multiplayer::NetworkEventBus* GetEventBus();

    // Convenience methods for the moment. This will need to be broken at formal modularization, but the standard pattern it creates throughout
    // integrations/tests will no doubt be helpful in doing that anyhow, rather than having big constructors everywhere.
    CSP_NO_EXPORT csp::multiplayer::OnlineRealtimeEngine* MakeOnlineRealtimeEngine();
    CSP_NO_EXPORT csp::multiplayer::OfflineRealtimeEngine* MakeOfflineRealtimeEngine();
    CSP_NO_EXPORT csp::common::IRealtimeEngine* MakeRealtimeEngine(csp::common::RealtimeEngineType RealtimeEngineType);

private:
    SystemsManager();
    ~SystemsManager();

    ConversationSystemInternal* GetConversationSystem();

    // Optional SignalR inject, null means the systemsmanager will make one of its own
    static void Instantiate(csp::multiplayer::ISignalRConnection* SignalRInject = nullptr);
    static void Destroy();

    static SystemsManager* Instance;

    // Optional SignalR inject, null means the systemsmanager will make one of its own
    void CreateSystems(csp::multiplayer::ISignalRConnection* SignalRInject);
    void DestroySystems();

    csp::web::WebClient* WebClient;

    csp::multiplayer::MultiplayerConnection* MultiplayerConnection;
    csp::multiplayer::NetworkEventBus* NetworkEventBus;
    std::shared_ptr<csp::common::IRealtimeEngine> RealtimeEngine;
    UserSystem* UserSystem;
    SpaceSystem* SpaceSystem;
    AssetSystem* AssetSystem;
    ScriptSystem* ScriptSystem;
    VoipSystem* VoipSystem;
    PointOfInterestInternalSystem* PointOfInterestSystem;
    AnchorSystem* AnchorSystem;
    csp::common::LogSystem* LogSystem;
    ApplicationSettingsSystem* ApplicationSettingsSystem;
    SettingsSystem* SettingsSystem;
    GraphQLSystem* GraphQLSystem;
    MaintenanceSystem* MaintenanceSystem;
    EventTicketingSystem* EventTicketingSystem;
    ECommerceSystem* ECommerceSystem;
    QuotaSystem* QuotaSystem;
    SequenceSystem* SequenceSystem;
    HotspotSequenceSystem* HotspotSequenceSystem;
    ConversationSystemInternal* ConversationSystem;
    AnalyticsSystem* AnalyticsSystem;
    ExternalServiceProxySystem* ExternalServiceProxySystem;
};

} // namespace csp::systems
