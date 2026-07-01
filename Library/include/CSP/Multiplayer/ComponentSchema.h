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
#include "CSP/Common/List.h"
#include "CSP/Common/Optional.h"
#include "CSP/Common/String.h"

#include <cstdint>

namespace csp::common
{
class LogSystem;
}

namespace csp::multiplayer
{

/// @brief A structural description of a component that can be interrogated at runtime
/// (i.e. to iterate over the properties) to facilitate registration and hydration (i.e. where a
/// serialised representation is reconstructed into this structure).
class CSP_API ComponentSchema
{
public:
    using TypeIdType = uint64_t;

    CSP_NO_EXPORT static csp::common::String ToJson(const ComponentSchema& Schema);
    CSP_NO_EXPORT static csp::common::Optional<ComponentSchema> FromJson(const csp::common::String& Json);

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

    bool operator==(const ComponentSchema& Other) const;
    bool operator!=(const ComponentSchema& Other) const;
};

CSP_START_IGNORE

/// @brief Parses a list of JSON documents into an array of component schemas.
/// Each document is expected to be a JSON array of schema objects.
/// Entries that fail to parse are skipped with a warning; valid entries in the same document are still returned.
/// @param JsonDocuments One or more JSON documents, each containing an array of schema objects.
/// The list is a wrapper generator workaround for passing large strings; in practice a single element is expected.
/// @param LogSystem Logger for reporting skipped entries.
/// @return An array containing all successfully parsed schemas.
csp::common::Array<ComponentSchema> ComponentSchemasFromJson(
    const csp::common::List<csp::common::String>& JsonDocuments, csp::common::LogSystem& LogSystem);

/// @brief Checks whether a schema update is compatible with an existing schema.
/// @param Original The existing schema to check against.
/// @param Updated The candidate updated schema.
/// @return True if the updated schema is a compatible update of the original, false otherwise.
///
/// @note The check is conservative. The updated schema must be a superset of the original without altering any existing property definitions. This
/// may be relaxed in future to permit changes that are unlikely to cause compatibility issues (e.g. updating a default value or exposing a property
/// to scripting where it wasn't previously).
bool IsCompatible(const ComponentSchema& Original, const ComponentSchema& Updated);

CSP_END_IGNORE

} // namespace csp::multiplayer
