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

#include "CSP/Systems/Assets/Material.h"

#include "CSP/Common/Array.h"

#include "Json/JsonSerializer.h"

void FromJson(const csp::json::JsonDeserializer& Deserializer, csp::systems::Material& Obj)
{
    // Name
    Deserializer.DeserializeMember("name", Obj.Name);

    // Shader Type
    uint32_t ShaderType;
    Deserializer.DeserializeMember("shaderType", ShaderType);

    Obj.Type = static_cast<csp::systems::EShaderType>(ShaderType);

    // Version
    Deserializer.DeserializeMember("version", Obj.Version);
}

namespace csp::systems
{

Material::Material(const csp::common::String& Name, const csp::common::String& InAssetCollectionId, const csp::common::String& InAssetId)
    : Name(Name)
    , Type(EShaderType::Standard)
    , CollectionId(InAssetCollectionId)
    , Id(InAssetId)
{
}

Material::Material(const csp::common::String& Name, const csp::common::String& InAssetCollectionId, const csp::common::String& InAssetId,
    EShaderType Type, const int Version)
    : Name(Name)
    , Type(Type)
    , Version(Version)
    , CollectionId(InAssetCollectionId)
    , Id(InAssetId)
{
}

const csp::common::String& Material::GetName() const { return Name; }

csp::systems::EShaderType Material::GetShaderType() const { return Type; }

int Material::GetVersion() const { return Version; }

const csp::common::String& Material::GetMaterialCollectionId() const { return CollectionId; }

const csp::common::String& Material::GetMaterialId() const { return Id; }

const Material* MaterialResult::GetMaterial() const { return Material; }

Material* MaterialResult::GetMaterial() { return Material; }

void MaterialResult::SetMaterial(csp::systems::Material* InMaterial) { Material = InMaterial; }

void MaterialResult::OnResponse(const csp::services::ApiResponseBase* /*ApiResponse*/) { }

const csp::common::Array<Material*>* MaterialsResult::GetMaterials() const { return &Materials; }

csp::common::Array<Material*>* MaterialsResult::GetMaterials() { return &Materials; }

void MaterialsResult::SetMaterials(const csp::common::Array<Material*>& InMaterials) { Materials = InMaterials; }

void MaterialsResult::OnResponse(const csp::services::ApiResponseBase* /*ApiResponse*/) { }

} // namespace csp::systems
