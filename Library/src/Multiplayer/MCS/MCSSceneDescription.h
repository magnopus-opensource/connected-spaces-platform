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

#include "MCSTypes.h"
#include "Services/AggregationService/Dto.h"
#include "Services/PrototypeService/Dto.h"
#include "Services/UserService/Dto.h"

#pragma once

namespace csp::multiplayer::mcs
{
class SceneDescription
{
public:
    csp::services::generated::userservice::GroupDto Group;
    std::vector<ObjectMessage> Objects;
    std::vector<csp::services::generated::prototypeservice::PrototypeDto> Prototypes;
    std::vector<csp::services::generated::prototypeservice::AssetDetailDto> AssetDetails;
    std::vector<csp::services::generated::aggregationservice::SequenceDto> Sequences;
};

}

void ToJson(csp::json::JsonSerializer& Deserializer, const csp::multiplayer::mcs::SceneDescription& Obj);
void FromJson(const csp::json::JsonDeserializer& Deserializer, csp::multiplayer::mcs::SceneDescription& Obj);
