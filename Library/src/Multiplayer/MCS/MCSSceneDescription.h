/*
 * Copyright 2025 Magnopus LLC

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

#include "MCSTypes.h"

namespace csp::multiplayer::mcs
{
/// @brief Internal mcs data structure which represents objects in a scene.
/// This is created through the deserialization of a csp scene json file.
class SceneDescription
{
public:
    std::vector<ObjectMessage> Objects;
};

}

void FromJson(const csp::json::JsonDeserializer& Deserializer, csp::multiplayer::mcs::SceneDescription& Obj);
