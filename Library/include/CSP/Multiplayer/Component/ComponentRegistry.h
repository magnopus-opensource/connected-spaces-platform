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
#include "CSP/Common/ReplicatedValue.h"
#include "CSP/Common/String.h"
#include "CSP/Common/Map.h"

#include "CSP/Multiplayer/Component/Component.h"

#include <vector>

namespace csp::json
{
class JsonDeserializer;
} // namespace csp::json

namespace csp::multiplayer
{
class ComponentRegistry;
}

void FromJson(const csp::json::JsonDeserializer& Deserializer, csp::multiplayer::ComponentRegistry& Registry);

namespace csp::multiplayer
{
class ComponentProperty
{
public:
    csp::common::String Name;
    csp::common::ReplicatedValue Value;
};

class ComponentTemplate
{
public:
    csp::common::String Type;
    csp::common::String Name;
    csp::common::String Id;
    csp::common::String Category;
    csp::common::String Description;
    csp::common::Array<csp::multiplayer::ComponentProperty> Properties;
};

class ComponentRegistry
{
public:
    /// Registers components from a json string
    bool RegisterComponents(const csp::common::String& ComponentsJson);

    const csp::common::Array<ComponentTemplate>& GetTemplates() const;

private:
    csp::common::Array<ComponentTemplate> Templates;

    friend void ::FromJson(const csp::json::JsonDeserializer& Deserializer, csp::multiplayer::ComponentRegistry& Registry);
};
}

void FromJson(const csp::json::JsonDeserializer& Deserializer, csp::multiplayer::ComponentProperty& Property);
void FromJson(const csp::json::JsonDeserializer& Deserializer, csp::multiplayer::ComponentTemplate& Template);
