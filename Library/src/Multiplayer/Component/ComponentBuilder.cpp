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

#include "CSP/Multiplayer/Component/ComponentBuilder.h"
#include "CSP/Multiplayer/Component/ComponentRegistry.h"

namespace csp::multiplayer
{
Component CreateComponent(
    const ComponentRegistry& Registry, std::string_view ComponentType, uint16_t ComponentId, SpaceEntity* Entity, csp::common::LogSystem* LogSystem)
{
    auto TemplateIt = std::find_if(Registry.GetTemplates().begin(), Registry.GetTemplates().end(),
        [ComponentType](const ComponentTemplate& Template) { return Template.Type == ComponentType.data(); });

    if (TemplateIt == Registry.GetTemplates().end())
    {
        // TODO: throw
    }

    csp::common::Map<csp::common::String, csp::common::ReplicatedValue> Properties;

    for (const auto& Prop : TemplateIt->Properties)
    {
        Properties[Prop.Name] = Prop.Value;
    }

    Component NewComponent { ComponentType.data(), Entity, Properties, ComponentId, LogSystem };
    return NewComponent;
}
}
