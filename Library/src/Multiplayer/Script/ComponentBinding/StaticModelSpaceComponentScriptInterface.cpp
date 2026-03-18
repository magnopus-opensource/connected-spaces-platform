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

#include "Multiplayer/Script/ComponentBinding/StaticModelSpaceComponentScriptInterface.h"

#include "CSP/Multiplayer/Components/StaticModelSpaceComponent.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/Assets/RuntimeMaterialSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "Multiplayer/Script/RuntimeMaterialScriptInterface.h"

using namespace csp::systems;

namespace csp::multiplayer
{

StaticModelSpaceComponentScriptInterface::StaticModelSpaceComponentScriptInterface(StaticModelSpaceComponent* InComponent)
    : ComponentScriptInterface(InComponent)
{
}

DEFINE_SCRIPT_PROPERTY_STRING(StaticModelSpaceComponent, ExternalResourceAssetId);
DEFINE_SCRIPT_PROPERTY_STRING(StaticModelSpaceComponent, ExternalResourceAssetCollectionId);

DEFINE_SCRIPT_PROPERTY_VEC3(StaticModelSpaceComponent, Scale);
DEFINE_SCRIPT_PROPERTY_VEC3(StaticModelSpaceComponent, Position);
DEFINE_SCRIPT_PROPERTY_VEC4(StaticModelSpaceComponent, Rotation);

DEFINE_SCRIPT_PROPERTY_TYPE(StaticModelSpaceComponent, bool, bool, IsVisible);
DEFINE_SCRIPT_PROPERTY_TYPE(StaticModelSpaceComponent, bool, bool, IsARVisible);
DEFINE_SCRIPT_PROPERTY_TYPE(StaticModelSpaceComponent, bool, bool, IsVirtualVisible);

DEFINE_SCRIPT_PROPERTY_TYPE(StaticModelSpaceComponent, bool, bool, ShowAsHoldoutInAR);
DEFINE_SCRIPT_PROPERTY_TYPE(StaticModelSpaceComponent, bool, bool, ShowAsHoldoutInVirtual);

std::vector<std::string> StaticModelSpaceComponentScriptInterface::GetMaterialPaths() const
{
    std::vector<std::string> Result;
    const auto* StaticModelComponent = static_cast<const StaticModelSpaceComponent*>(Component);
    if (StaticModelComponent == nullptr)
    {
        return Result;
    }

    const auto Overrides = StaticModelComponent->GetMaterialOverrides();
    std::unique_ptr<csp::common::Array<csp::common::String>> Keys(const_cast<csp::common::Array<csp::common::String>*>(Overrides.Keys()));
    Result.reserve(Keys->Size());
    for (size_t Index = 0; Index < Keys->Size(); ++Index)
    {
        Result.emplace_back((*Keys)[Index].c_str());
    }
    return Result;
}

RuntimeMaterialScriptInterface* StaticModelSpaceComponentScriptInterface::GetMaterial(const std::string& MaterialPath) const
{
    const auto* StaticModelComponent = static_cast<const StaticModelSpaceComponent*>(Component);
    if (StaticModelComponent == nullptr || MaterialPath.empty())
    {
        return nullptr;
    }

    const auto Overrides = StaticModelComponent->GetMaterialOverrides();
    const auto MaterialIt = Overrides.Find(MaterialPath.c_str());
    if (MaterialIt == Overrides.end())
    {
        return nullptr;
    }

    const int32_t ComponentIndex = GetTypeIndexWithinParent();
    if (GetParentEntityId() == 0 || ComponentIndex < 0)
    {
        return nullptr;
    }

    return new RuntimeMaterialScriptInterface(csp::systems::SystemsManager::Get().GetRuntimeMaterialSystem(), GetParentEntityId(),
        csp::multiplayer::ComponentType::StaticModel, ComponentIndex, MaterialPath.c_str(), MaterialIt->second);
}

} // namespace csp::multiplayer
