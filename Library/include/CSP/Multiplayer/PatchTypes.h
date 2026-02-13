/*
 * Copyright 2025 Magnopus LLC

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

/// @brief Enum used to specify what part of a SpaceEntity was updated when deserialising.
/// Use this to determine which parts of an entity to copy values from when an update occurs.
/// It is a bitwise flag enum, so values are additive, the value may represent several flags.
enum SpaceEntityUpdateFlags
{
    UPDATE_FLAGS_NAME = 1,
    UPDATE_FLAGS_POSITION = 2,
    UPDATE_FLAGS_ROTATION = 4,
    UPDATE_FLAGS_SCALE = 8,
    UPDATE_FLAGS_COMPONENTS = 16,
    UPDATE_FLAGS_SELECTION_ID = 32,
    UPDATE_FLAGS_THIRD_PARTY_REF = 64,
    UPDATE_FLAGS_PARENT = 128,
    UPDATE_FLAGS_LOCK_TYPE = 256,
};

/// @brief This Enum should be used to determine what kind of operation the component update represents.
/// Update means properties on the component have updated, all need to be checked as we do not provide reference of specific property updates.
/// Add means the component is newly added, clients should ensure that this triggers appropriate instantiation of wrapping objects.
/// All properties for the component should be included.
/// Delete means the component has been marked for deletion. It is likely that some other clients will not have the component at the point this is
/// received. Any wrapping data objects should be deleted when this is received, and clients should cease updating this component as any call would
/// fail. The CSP representation of the component has been removed at this point.
enum class ComponentUpdateType
{
    Update,
    Add,
    Delete,
};

/// @brief Info class that specifies a type of update and the ID of a component the update is applied to.
class CSP_API ComponentUpdateInfo
{
public:
    uint16_t ComponentId;
    ComponentUpdateType UpdateType;

    bool operator==(const ComponentUpdateInfo& Other) const { return ComponentId == Other.ComponentId && UpdateType == Other.UpdateType; }
    bool operator!=(const ComponentUpdateInfo& Other) const { return !(*this == Other); }
};

} // namespace csp::multiplayer
