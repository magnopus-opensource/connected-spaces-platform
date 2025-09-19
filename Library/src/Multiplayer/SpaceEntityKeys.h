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

#include <climits>
#include <cstdint>

namespace csp::multiplayer
{
constexpr const uint16_t MAX_RESERVED_COMPONENT_COUNT = 1024;
constexpr const uint16_t COMPONENT_KEYS_START_VIEWS = USHRT_MAX - MAX_RESERVED_COMPONENT_COUNT;
constexpr const uint16_t COMPONENT_KEYS_END_VIEWS = USHRT_MAX;
constexpr const uint16_t COMPONENT_KEY_START_COMPONENTS = 0;
constexpr const uint16_t COMPONENT_KEY_END_COMPONENTS = COMPONENT_KEYS_START_VIEWS - 1;

// Its a shame we decided to use this value as the COMPONENTTYPE,
// as this doesnt relate to the rest of our entity properties,
// but resides in the same value range.
constexpr const uint16_t COMPONENT_KEY_COMPONENTTYPE = COMPONENT_KEYS_START_VIEWS + 5;

// These values are our unique entity property identifiers used when replicating.
// These only capture our entity properties (Name, Position etc.),
// even though our csp components are replicated using the same mechanism.
// The reason we handle these differently is because the component key values are dynamic,
// as they represent the index of our component, and not the type. This is due to being able to have multiple
// components of the same type on an entity, so we can't use the component type as a unique key.
enum class SpaceEntityComponentKey : uint16_t
{
    // Property Keys.
    Name = COMPONENT_KEYS_START_VIEWS,
    Position = COMPONENT_KEYS_START_VIEWS + 1,
    Rotation = COMPONENT_KEYS_START_VIEWS + 2,
    Scale = COMPONENT_KEYS_START_VIEWS + 3,
    SelectedClientId = COMPONENT_KEYS_START_VIEWS + 4,
    ThirdPartyRef = COMPONENT_KEYS_START_VIEWS + 6,
    ThirdPartyPlatform = COMPONENT_KEYS_START_VIEWS + 7,
    LockType = COMPONENT_KEYS_START_VIEWS + 8
};

} // namespace csp::multiplayer
