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

namespace
{

// Each member of the scene's "data" object, serialised back to json for the DTOs to parse.
struct SceneDataJson
{
    std::string Group;
    std::string Prototypes;
    std::string AssetDetails;
    std::string Sequences;
    std::string Anchors;
};

void FromJson(const csp::json::JsonDeserializer& Deserializer, SceneDataJson& Obj)
{
    Deserializer.EnterMember("data");

    Obj.Group = Deserializer.GetMemberAsString("group");
    Obj.Prototypes = Deserializer.GetMemberAsString("prototypes");
    Obj.AssetDetails = Deserializer.GetMemberAsString("assetDetails");
    Obj.Sequences = Deserializer.GetMemberAsString("sequences");
    Obj.Anchors = Deserializer.GetMemberAsString("anchors");

    Deserializer.ExitMember();
}

} // namespace

namespace csp::systems::mcs
{

SceneData ParseSceneData(const char* Json, csp::common::LogSystem&)
{
    SceneDataJson Members;
    SceneData Obj;

    if (!csp::json::JsonDeserializer::Deserialize(Json, Members))
    {
        return Obj;
    }

    Obj.Group.FromJson(Members.Group.c_str());

    csp::services::DtoArray<csp::services::generated::prototypeservice::PrototypeDto> PrototypesDto;
    PrototypesDto.FromJson(Members.Prototypes.c_str());

    Obj.Prototypes = PrototypesDto.GetArray();

    csp::services::DtoArray<csp::services::generated::prototypeservice::AssetDetailDto> AssetDetailsDto;
    AssetDetailsDto.FromJson(Members.AssetDetails.c_str());

    Obj.AssetDetails = AssetDetailsDto.GetArray();

    csp::services::DtoArray<csp::services::generated::aggregationservice::SequenceDto> SequencesDto;
    SequencesDto.FromJson(Members.Sequences.c_str());

    Obj.Sequences = SequencesDto.GetArray();

    csp::services::DtoArray<csp::services::generated::spatialdataservice::AnchorDto> AnchorssDto;
    AnchorssDto.FromJson(Members.Anchors.c_str());

    Obj.Anchors = AnchorssDto.GetArray();

    return Obj;
}

}
