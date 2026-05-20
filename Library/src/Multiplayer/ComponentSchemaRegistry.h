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

#include "CSP/Multiplayer/ComponentBase.h"
#include "CSP/Multiplayer/IComponentSchemaRegistry.h"

#include <optional>
#include <unordered_map>

namespace csp::common
{
class LogSystem;
}

namespace csp::multiplayer
{

std::optional<ComponentType> ToComponentType(uint64_t TypeId);

bool IsLegacyComponentTypeId(uint64_t TypeId);

class ComponentSchemaRegistryImpl final : public IComponentSchemaRegistry
{
public:
    ComponentSchemaRegistryImpl(csp::common::LogSystem&, const csp::common::Array<ComponentSchema>& AdditionalComponents);

    csp::common::Array<ComponentSchema> GetAll() const override;
    const ComponentSchema* Find(uint64_t TypeId) const override;

private:
    std::unordered_map<ComponentSchema::TypeIdType, ComponentSchema> SchemaMap;
};

} // namespace csp::multiplayer
