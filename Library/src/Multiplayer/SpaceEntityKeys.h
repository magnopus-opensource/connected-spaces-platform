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

constexpr const uint16_t COMPONENT_KEY_VIEW_ENTITYNAME = COMPONENT_KEYS_START_VIEWS; // 64511
constexpr const uint16_t COMPONENT_KEY_VIEW_POSITION = COMPONENT_KEYS_START_VIEWS + 1;
constexpr const uint16_t COMPONENT_KEY_VIEW_ROTATION = COMPONENT_KEYS_START_VIEWS + 2;
constexpr const uint16_t COMPONENT_KEY_VIEW_SCALE = COMPONENT_KEYS_START_VIEWS + 3;
constexpr const uint16_t COMPONENT_KEY_VIEW_SELECTEDCLIENTID = COMPONENT_KEYS_START_VIEWS + 4; // 64515
constexpr const uint16_t COMPONENT_KEY_VIEW_THIRDPARTYREF = COMPONENT_KEYS_START_VIEWS + 6;
constexpr const uint16_t COMPONENT_KEY_VIEW_THIRDPARTYPLATFORM = COMPONENT_KEYS_START_VIEWS + 7;

constexpr const uint16_t COMPONENT_KEY_COMPONENTTYPE = COMPONENT_KEYS_START_VIEWS + 5; // 64516

constexpr const uint16_t COMPONENT_KEY_START_COMPONENTS = 0;
constexpr const uint16_t COMPONENT_KEY_END_COMPONENTS = COMPONENT_KEYS_START_VIEWS - 1;

} // namespace csp::multiplayer
