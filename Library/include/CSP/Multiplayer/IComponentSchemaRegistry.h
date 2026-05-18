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

#include "CSP/CSPCommon.h"
#include "CSP/Common/Array.h"
#include "CSP/Multiplayer/ComponentSchema.h"

namespace csp::multiplayer
{

/// @brief Read-only interface for querying registered component schemas.
class CSP_API IComponentSchemaRegistry
{
public:
    virtual ~IComponentSchemaRegistry() = default;

    /// @brief Returns all registered schemas.
    virtual csp::common::Array<ComponentSchema> GetAll() const = 0;

    /// @brief Finds the schema for the given TypeId.
    /// @return A pointer to the schema if found, otherwise nullptr.
    virtual const ComponentSchema* Find(uint64_t TypeId) const = 0;
};

} // namespace csp::multiplayer
