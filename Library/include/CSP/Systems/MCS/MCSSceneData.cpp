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

#include "MCSSceneData.h"
#include "Services/ApiBase/ApiBase.h"
#include "Json/JsonSerializer.h"

void ToJson(csp::json::JsonSerializer&, const csp::systems::mcs::SceneData&) { }

void FromJson(const csp::json::JsonDeserializer& Deserializer, csp::systems::mcs::SceneData& Obj)
{
    Deserializer.EnterMember("data");

    std::string GroupJson = Deserializer.GetMemberAsString("group");
    Obj.Group.FromJson(GroupJson.c_str());

    std::string PrototypesJson = Deserializer.GetMemberAsString("prototypes");
    csp::services::DtoArray<csp::services::generated::prototypeservice::PrototypeDto> PrototypesDto;
    PrototypesDto.FromJson(PrototypesJson.c_str());

    Obj.Prototypes = PrototypesDto.GetArray();

    std::string AssetDetailsJson = Deserializer.GetMemberAsString("assetDetails");
    csp::services::DtoArray<csp::services::generated::prototypeservice::AssetDetailDto> AssetDetailsDto;
    AssetDetailsDto.FromJson(AssetDetailsJson.c_str());

    Obj.AssetDetails = AssetDetailsDto.GetArray();

    std::string SequencesJson = Deserializer.GetMemberAsString("sequences");
    csp::services::DtoArray<csp::services::generated::aggregationservice::SequenceDto> SequencesDto;
    SequencesDto.FromJson(SequencesJson.c_str());

    Obj.Sequences = SequencesDto.GetArray();

    Deserializer.ExitMember();
}
