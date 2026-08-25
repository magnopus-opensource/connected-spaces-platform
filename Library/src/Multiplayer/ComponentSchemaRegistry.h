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

#include "CSP/Common/Array.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/ComponentBase.h"
#include "Multiplayer/ComponentSchema.h"

#include <optional>
#include <unordered_map>

namespace csp::common
{
class LogSystem;
}

namespace csp::multiplayer::schema
{

std::optional<ComponentType> ToComponentType(uint64_t TypeId);

/// @brief The schemas for the components CSP itself defines.
csp::common::Array<ComponentSchema> GetBuiltInComponentSchemas();

/// @brief The built-in schemas as a JSON array, suitable for clients to read as a reference.
csp::common::String GetBuiltInComponentSchemasJson();

bool IsLegacyComponentTypeId(uint64_t TypeId);

/// @brief The engine-wide index of registered component schemas.
class ComponentSchemaRegistry final
{
public:
    ComponentSchemaRegistry(csp::common::LogSystem&, const csp::common::Array<ComponentSchema>& AdditionalComponents);

    ComponentSchemaRegistry(const ComponentSchemaRegistry&) = delete;
    ComponentSchemaRegistry(ComponentSchemaRegistry&&) = delete;
    ComponentSchemaRegistry& operator=(const ComponentSchemaRegistry&) = delete;
    ComponentSchemaRegistry& operator=(ComponentSchemaRegistry&&) = delete;

    /// @brief Returns all registered schemas.
    csp::common::Array<ComponentSchema> GetAll() const;

    /// @brief Finds the schema for the given TypeId.
    /// @return A pointer to the schema if found, otherwise nullptr.
    const ComponentSchema* Find(uint64_t TypeId) const;

private:
    std::unordered_map<ComponentSchema::TypeIdType, ComponentSchema> SchemaMap;
};

} // namespace csp::multiplayer::schema
