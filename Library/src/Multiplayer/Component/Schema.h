/*
 * Copyright 2026 Magnopus LLC

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

#include "Property.h"

#include "Multiplayer/MCS/MCSTypes.h"

#include <cstdint>
#include <vector>

namespace csp::multiplayer::component
{

/// @brief A structural description of a component that can be interrogated at runtime
/// (i.e. to iterate over the properties) to facilitate registration and hydration (i.e. where a
/// serialised representation is reconstructed into this structure).
///
/// @tparam ComponentTypeId: The type of the stable ID value used to uniquely identify the specific component type this schema describes.
/// @tparam PropertyKey: The type of the stable ID values used to key individual properties within this schema.
template <typename ComponentTypeId, typename PropertyKey> struct Schema final
{
    static_assert(mcs::IsPackableIdV<ComponentTypeId, uint64_t>);

    using Property = Property<PropertyKey>;

    ComponentTypeId TypeId;
    std::vector<Property> Properties;
};

} // namespace csp::multiplayer::component
