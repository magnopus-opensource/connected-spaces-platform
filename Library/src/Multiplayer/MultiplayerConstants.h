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
#include "CSP/Common/String.h"

namespace csp::multiplayer
{

enum ComponentKeys : uint64_t
{
    PACKED_POD_REPLICATED_VALUES = 0,
    // we store strings as individual signalr components so that we can rely on fixed-width entries in the
    // backed buffer of replicated values (since all other types are POD types)
    START_STRING_REPLICATED_VALUES = 1,

    ENTITY_TYPE = 1000,
    ENTITY_CUSTOMTYPEID = 1004,

    ENTITY_POSITION = 1001,
    ENTITY_ROTATION = 1002,
    ENTITY_SCALE = 1003,
};

class SequenceConstants
{
public:
    // For uniquely identifying sequences which relate to a space entity hierarchy.
    static csp::common::String GetHierarchyName();

    // Prefix needed when storing multiplayer unsigned integer ids in keys
    static csp::common::String GetIdPrefix();
};

} // namespace csp::multiplayer
