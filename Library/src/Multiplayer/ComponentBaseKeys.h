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
constexpr const uint32_t MAX_RESERVED_COMPONENT_COUNT = 1024;

constexpr const uint32_t COMPONENT_BASE_KEYS_START = USHRT_MAX - MAX_RESERVED_COMPONENT_COUNT;
constexpr const uint32_t COMPONENT_BASE_KEYS_END = USHRT_MAX;

constexpr const uint32_t COMPONENT_KEY_NAME = COMPONENT_BASE_KEYS_START; // 64511
} // namespace csp::multiplayer
