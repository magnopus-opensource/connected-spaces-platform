#pragma once

#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "Multiplayer/RealtimeEngineUtils.h"
#include "SpaceEntityStatePatcher.h"

namespace csp::multiplayer
{
// Helper function to generically set entity properties without duplicating a bunch of logic.
// This needs to live in a private include, as it relies on other non-exported includes.
template <typename P, typename V>
static bool SetProperty(
    SpaceEntity& entity, P& property, const V& value, SpaceEntityComponentKey key, SpaceEntityUpdateFlags flag, csp::common::LogSystem* logSystem)
{
    // Ensure we can modify the entity. The criteria for this can be found on the specific RealtimeEngine::IsEntityModifiable overloads.
    ModifiableStatus modifiable = entity.IsModifiable();
    if (modifiable != ModifiableStatus::Modifiable)
    {
        if (logSystem != nullptr)
        {
            logSystem->LogMsg(csp::common::LogLevel::Warning,
                fmt::format("Failed to set propery on entity: {0}, skipping update. Entity name: {1}",
                    RealtimeEngineUtils::ModifiableStatusToString(modifiable), entity.GetName())
                    .c_str());
        }

        return false;
    }

    if (entity.GetStatePatcher())
    {
        return entity.GetStatePatcher()->SetDirtyProperty(key, property, value);
    }
    else
    {
        // We need this logic here and in SetDirtyProperty to prevent callbacks from firing if the values are the same.
        if (property != static_cast<P>(value))
        {
            entity.SetPropertyDirect(property, value, flag, true);
            return true;
        }
        return false;
    }
}
}
