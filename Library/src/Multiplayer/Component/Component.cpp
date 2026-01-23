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

#include "CSP/Multiplayer/Component/Component.h"
#include "CSP/Common/ReplicatedValue.h"

namespace csp::multiplayer
{
static const csp::common::ReplicatedValue NullValue;

Component::Component(const csp::common::String& Type, SpaceEntity* Entity,
    const csp::common::Map<csp::common::String, csp::common::ReplicatedValue>& Properties, uint16_t Id, csp::common::LogSystem* LogSystem)
    : Type { Type }
    , Entity { Entity }
    , Properties { Properties }
    , Id { Id }
    , LogSystem { LogSystem }
{
}

void Component::SetProperty(const csp::common::String& Name, const csp::common::ReplicatedValue& Value)
{
    if (Properties.HasKey(Name) == false)
    {
        // TODO: log
        return;
    }

    const auto& Prop = Properties[Name];

    if (Value.GetReplicatedValueType() != Prop.GetReplicatedValueType())
    {
        // TODO: log
        return;
    }

    Properties[Name] = Value;
}

const csp::common::ReplicatedValue& Component::GetProperty(const csp::common::String& Name) const
{
    if (Properties.HasKey(Name) == false)
    {
        // TODO: log
        return NullValue;
    }

    return Properties[Name];
}

const csp::common::Map<csp::common::String, csp::common::ReplicatedValue>* Component::GetProperties() const { return &Properties; }

uint16_t Component::GetId() const { return Id; }
}
