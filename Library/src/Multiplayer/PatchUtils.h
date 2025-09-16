#pragma once

#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "SpaceEntityStatePatcher.h"

namespace csp::multiplayer
{
// Helper function to generically set entity properties without duplicating a bunch of logic.
// This needs to live in a private include, as it relies on other non-exported includes.
template <typename P, typename V>
static bool SetProperty(
    SpaceEntity* Entity, P& Property, const V& Value, SpaceEntityComponentKey Key, SpaceEntityUpdateFlags Flag, csp::common::LogSystem* LogSystem)
{
    if (!Entity.IsModifiable())
    {
        if (LogSystem)
        {
            LogSystem->LogMsg(csp::common::LogLevel::Error,
                fmt::format("Entity is not modifiable, you can only modify entities that have transferable ownership, or which you already are the "
                            "owner of. Entity name: {}",
                    Entity->GetName())
                    .c_str());
        }

        return false;
    }

    if (Entity->GetStatePatcher())
    {
        return Entity->GetStatePatcher()->SetDirtyProperty(Key, Property, Value);
    }
    else
    {
        // We need this logic here and in SetDirtyProperty to prevent callbacks from firing if the values are the same.
        if (Property != static_cast<P>(Value))
        {
            Entity->SetPropertyDirect(Property, Value, Flag, true);
            return true;
        }
        return false;
    }
}
}
