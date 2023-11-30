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

namespace csp::multiplayer
{
CSP_START_IGNORE
// Entity script messages

/// @brief Mouse click or touch press.
constexpr const char* SCRIPT_MSG_ENTITY_CLICK = "entityClick";

/// @brief Called once per frame.
constexpr const char* SCRIPT_MSG_ENTITY_TICK = "entityTick";

/// @brief Called once all entities in a scene have been created on entering a space.
constexpr const char* SCRIPT_MSG_ENTITIES_LOADED = "entitiesLoaded";
CSP_END_IGNORE

} // namespace csp::multiplayer
