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

#include "ComponentProperty.h"

#include "CSP/CSPCommon.h"
#include "CSP/Common/Array.h"
#include "CSP/Common/Map.h"
#include "CSP/Common/String.h"

#include <cstdint>

namespace csp::multiplayer
{

/// @brief A structural description of a component that can be interrogated at runtime
/// (i.e. to iterate over the properties) to facilitate registration and hydration (i.e. where a
/// serialised representation is reconstructed into this structure).
class CSP_API ComponentSchema
{
public:
    using TypeIdType = uint64_t;

    /// @brief A globally unique ID for identifiying this component type. Will ultimately be
    /// serialized and used in messages sent over the multiplayer connection.
    TypeIdType TypeId;

    /// @brief A human-readable name describing this component (in `PascalCase`).
    /// Must be unique across components registered with the engine. This name will be used for
    /// generating script bindings i.e. when named `Audio`, a function with the name
    /// `getAudioComponents` is available on the entity in scripts.
    csp::common::String Name;

    /// @brief The properties of this component
    csp::common::Array<ComponentProperty> Properties;
};

using ComponentSchemaRegistry = csp::common::Map<csp::multiplayer::ComponentSchema::TypeIdType, csp::multiplayer::ComponentSchema>;

} // namespace csp::multiplayer
