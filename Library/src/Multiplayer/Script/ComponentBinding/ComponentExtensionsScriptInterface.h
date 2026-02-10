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

#include "Multiplayer/Script/ComponentScriptInterface.h"

#include <string>
#include <variant>
#include <vector>

namespace csp::multiplayer
{

class ComponentExtensions;

class ComponentExtensionsScriptInterface
{
public:
    ComponentExtensionsScriptInterface(ComponentExtensions* InExtensions = nullptr);

    const std::variant<bool, int64_t, float, std::string, std::vector<float>> GetProperty(const std::string& Key);
    void SetProperty(const std::string& Key, const std::variant<int64_t, float, std::string, std::vector<float>, bool>& Value);
    bool HasProperty(const std::string& Key) const;

private:
    ComponentExtensions* Extensions;
};

} // namespace csp::multiplayer
