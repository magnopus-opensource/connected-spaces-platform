#pragma once

namespace oly_multiplayer
{
OLY_START_IGNORE
// Entity script messages

// Mouse click or touch press
constexpr const char* SCRIPT_MSG_ENTITY_CLICK = "entityClick";
// Called once per frame
constexpr const char* SCRIPT_MSG_ENTITY_TICK = "entityTick";
// Called once all entities in a scene have been created on entering a space
constexpr const char* SCRIPT_MSG_ENTITIES_LOADED = "entitiesLoaded";
OLY_END_IGNORE

} // namespace oly_multiplayer
