/*
 * Copyright 2023 Magnopus LLC

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
#include "Multiplayer/Script/ComponentScriptMacros.h"

#include <string>
#include <vector>

namespace csp::multiplayer
{

class ComponentBase;

constexpr int64_t INVALID_COMPONENT_ID = -1;

class CSP_API ComponentScriptInterface
{
public:
    using Vector2 = std::vector<float>;
    using Vector3 = std::vector<float>;
    using Vector4 = std::vector<float>;

    ComponentScriptInterface(ComponentBase* InComponent = nullptr);
    virtual ~ComponentScriptInterface() = default;

    void SubscribeToPropertyChange(int32_t PropertyKey, std::string Message);
    void InvokeAction(std::string ActionId, std::string ActionParams);

    int64_t GetComponentId() const;
    int64_t GetComponentType() const;

    void SetComponentName(std::string name);
    std::string GetComponentName() const;

    void SendPropertyUpdate();

protected:
    ComponentBase* Component;
};

} // namespace csp::multiplayer
