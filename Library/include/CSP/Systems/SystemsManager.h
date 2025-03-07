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
#include "CSP/Multiplayer/MultiPlayerConnection.h"

#include <chrono>
#include <functional>

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
class LogSystem;
class SettingsSystem;
class GraphQLSystem;
class AnalyticsSystem;
class MaintenanceSystem;
class EventTicketingSystem;
class ECommerceSystem;
class QuotaSystem;
class SequenceSystem;
class HotspotSequenceSystem;

} // namespace csp::systems

namespace csp
{

class CSPFoundation;

} // namespace csp

namespace csp::web
{

class WebClient;

} // namespace csp::web

namespace csp::memory
{

CSP_START_IGNORE
template <typename T> void Delete(T* Ptr);
CSP_END_IGNORE

} // namespace csp::memory

namespace csp::systems
{

/// @ingroup Systems
/// @brief Interface used to access each of the systems.
class CSP_API SystemsManager
{
    CSP_START_IGNORE
    /** @cond DO_NOT_DOCUMENT */
    friend class csp::CSPFoundation;
    friend void csp::memory::Delete<SystemsManager>(SystemsManager* Ptr);
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
    LogSystem* GetLogSystem();

    /// @brief Retrieves the Settings system.
    /// @return SettingsSystem : pointer to the Settings system class
    SettingsSystem* GetSettingsSystem();

    /// @brief Retrieves the GraphQL system.
    /// @return GraphQLSystem : pointer to the GraphQL system class
    GraphQLSystem* GetGraphQLSystem();

    /// @brief Retrieves the Analytics system.
    /// @return AnalyticsSystem : pointer to the Analytics system class
    AnalyticsSystem* GetAnalyticsSystem();

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

    csp::multiplayer::SpaceEntitySystem* GetSpaceEntitySystem();

    csp::multiplayer::MultiplayerConnection* GetMultiplayerConnection();

    csp::multiplayer::EventBus* GetEventBus();

private:
    SystemsManager();
    ~SystemsManager();

    static void Instantiate();
    static void Destroy();

    static SystemsManager* Instance;

    void CreateSystems();
    void DestroySystems();

    csp::web::WebClient* WebClient;

    csp::multiplayer::MultiplayerConnection* MultiplayerConnection;
    csp::multiplayer::EventBus* EventBus;
    csp::multiplayer::SpaceEntitySystem* SpaceEntitySystem;
    UserSystem* UserSystem;
    SpaceSystem* SpaceSystem;
    AssetSystem* AssetSystem;
    ScriptSystem* ScriptSystem;
    VoipSystem* VoipSystem;
    PointOfInterestInternalSystem* PointOfInterestSystem;
    AnchorSystem* AnchorSystem;
    LogSystem* LogSystem;
    SettingsSystem* SettingsSystem;
    GraphQLSystem* GraphQLSystem;
    AnalyticsSystem* AnalyticsSystem;
    MaintenanceSystem* MaintenanceSystem;
    EventTicketingSystem* EventTicketingSystem;
    ECommerceSystem* ECommerceSystem;
    QuotaSystem* QuotaSystem;
    SequenceSystem* SequenceSystem;
    HotspotSequenceSystem* HotspotSequenceSystem;
};

} // namespace csp::systems
