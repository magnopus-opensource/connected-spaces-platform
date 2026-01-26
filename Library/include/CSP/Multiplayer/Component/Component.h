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
#include "CSP/Common/Map.h"
#include "CSP/Common/String.h"

namespace csp::common
{
class LogSystem;
class ReplicatedValue;
}

namespace csp::multiplayer
{
class SpaceEntity;

class Component
{
public:

    Component() = default;
    Component(const csp::common::String& Type, const csp::common::String& Name, SpaceEntity* Entity,
        const csp::common::Map<csp::common::String, csp::common::ReplicatedValue>& Properties, uint16_t Id, csp::common::LogSystem* LogSystem);

    const csp::common::String& GetType() const;

    const csp::common::String& GetName() const;

    void SetProperty(const csp::common::String& Name, const csp::common::ReplicatedValue& Value);
    const csp::common::ReplicatedValue& GetProperty(const csp::common::String& Name) const;

    const csp::common::Map<csp::common::String, csp::common::ReplicatedValue>* GetProperties() const;

    uint16_t GetId() const;


private:

    csp::common::String Type;
    csp::common::String Name;
    SpaceEntity* Entity;
    csp::common::LogSystem* LogSystem = nullptr;

    csp::common::Map<csp::common::String, csp::common::ReplicatedValue> Properties;
    uint16_t Id = 0;
};
}
