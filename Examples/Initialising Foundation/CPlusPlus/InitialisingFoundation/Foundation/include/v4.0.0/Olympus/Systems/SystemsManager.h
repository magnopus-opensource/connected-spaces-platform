#pragma once

#include "Olympus/OlympusCommon.h"

#include <chrono>
#include <functional>

namespace oly_systems
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

} // namespace oly_systems

namespace oly
{

class OlympusFoundation;

}

namespace oly_memory
{

OLY_START_IGNORE
template <typename T> void Delete(T*);
OLY_END_IGNORE

} // namespace oly_memory

namespace oly_web
{

class WebClient;

}

namespace oly_systems
{

/// @ingroup Systems
/// @brief Interface used to access each of the systems.
class OLY_API OLY_NO_DISPOSE SystemsManager
{
    /** @cond DO_NOT_DOCUMENT */
    friend class oly::OlympusFoundation;
    friend void oly_memory::Delete(SystemsManager*);
    /** @endcond */

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

    GraphQLSystem* GetGraphQLSystem();

    AnalyticsSystem* GetAnalyticsSystem();

private:
    SystemsManager();
    ~SystemsManager();

    static void Instantiate();
    static void Destroy();

    static SystemsManager* Instance;

    void CreateSystems();
    void DestroySystems();

    oly_web::WebClient* WebClient;

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
};

} // namespace oly_systems
