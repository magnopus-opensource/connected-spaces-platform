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

namespace msgpack_typeids
{

    enum ItemComponentData : uint64_t
    {
        BOOL,
        NULLABLE_BOOL,
        BOOL_ARRAY,
        NULLABLE_BOOL_ARRAY,
        UINT8,
        NULLABLE_UINT8,
        UINT8_ARRAY,
        NULLABLE_UINT8_ARRAY,
        INT32,
        NULLABLE_INT32,
        INT32_ARRAY,
        NULLABLE_INT32_ARRAY,
        UINT32,
        NULLABLE_UINT32,
        UINT32_ARRAY,
        NULLABLE_UINT32_ARRAY,
        INT64,
        NULLABLE_INT64,
        INT64_ARRAY,
        NULLABLE_INT64_ARRAY,
        UINT64,
        NULLABLE_UINT64,
        UINT64_ARRAY,
        NULLABLE_UINT64_ARRAY,
        FLOAT,
        NULLABLE_FLOAT,
        FLOAT_ARRAY,
        NULLABLE_FLOAT_ARRAY,
        DOUBLE,
        NULLABLE_DOUBLE,
        DOUBLE_ARRAY,
        NULLABLE_DOUBLE_ARRAY,
        STRING,
        STRING_ARRAY,
        DATETIMEOFFSET,
        NULLABLE_DATETIMEOFFSET,
        DATETIMEOFFSET_ARRAY,
        NULLABLE_DATETIMEOFFSET_ARRAY,
        TIMESPAN,
        NULLABLE_TIMESPAN,
        TIMESPAN_ARRAY,
        NULLABLE_TIMESPAN_ARRAY,
        GUID,
        NULLABLE_GUID,
        GUID_ARRAY,
        NULLABLE_GUID_ARRAY,
        INT16,
        NULLABLE_INT16,
        INT16_ARRAY,
        NULLABLE_INT16_ARRAY,
        UINT16,
        NULLABLE_UINT16,
        UINT16_ARRAY,
        NULLABLE_UINT16_ARRAY,
        UINT16_DICTIONARY,
        STRING_DICTIONARY
    };

} // namespace msgpack_typeids

class SequenceConstants
{
public:
    // For uniquely identifying sequences which relate to a space entity hierarchy.
    static csp::common::String GetHierarchyName();

    // Prefix needed when storing multiplayer unsigned integer ids in keys
    static csp::common::String GetIdPrefix();
};

} // namespace csp::multiplayer
