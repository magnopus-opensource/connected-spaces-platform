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

#include "ComponentSchemaRegistry.h"

#include "CSP/Multiplayer/ComponentBase.h"

#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Multiplayer/Components/AIChatbotComponent.h"
#include "CSP/Multiplayer/Components/AnimatedModelSpaceComponent.h"
#include "CSP/Multiplayer/Components/AudioSpaceComponent.h"
#include "CSP/Multiplayer/Components/AvatarSpaceComponent.h"
#include "CSP/Multiplayer/Components/ButtonSpaceComponent.h"
#include "CSP/Multiplayer/Components/CinematicCameraSpaceComponent.h"
#include "CSP/Multiplayer/Components/CollisionSpaceComponent.h"
#include "CSP/Multiplayer/Components/ConversationSpaceComponent.h"
#include "CSP/Multiplayer/Components/CustomSpaceComponent.h"
#include "CSP/Multiplayer/Components/ECommerceSpaceComponent.h"
#include "CSP/Multiplayer/Components/ExternalLinkSpaceComponent.h"
#include "CSP/Multiplayer/Components/FiducialMarkerSpaceComponent.h"
#include "CSP/Multiplayer/Components/FogSpaceComponent.h"
#include "CSP/Multiplayer/Components/GaussianSplatSpaceComponent.h"
#include "CSP/Multiplayer/Components/HotspotSpaceComponent.h"
#include "CSP/Multiplayer/Components/ImageSpaceComponent.h"
#include "CSP/Multiplayer/Components/LightSpaceComponent.h"
#include "CSP/Multiplayer/Components/PortalSpaceComponent.h"
#include "CSP/Multiplayer/Components/ReflectionSpaceComponent.h"
#include "CSP/Multiplayer/Components/ScreenSharingSpaceComponent.h"
#include "CSP/Multiplayer/Components/ScriptSpaceComponent.h"
#include "CSP/Multiplayer/Components/SplineSpaceComponent.h"
#include "CSP/Multiplayer/Components/StaticModelSpaceComponent.h"
#include "CSP/Multiplayer/Components/TextSpaceComponent.h"
#include "CSP/Multiplayer/Components/VideoPlayerSpaceComponent.h"

#include <fmt/format.h>

#include <limits>
#include <type_traits>

namespace csp::multiplayer
{

ComponentSchemaRegistryImpl::ComponentSchemaRegistryImpl(
    csp::common::LogSystem& LogSystem, const csp::common::Array<ComponentSchema>& AdditionalComponents)
{
    const auto AddSchema = [this, &LogSystem](const ComponentSchema& Schema)
    {
        const auto Result = SchemaMap.insert_or_assign(Schema.TypeId, Schema);
        const auto DidReplace = !Result.second;

        if (DidReplace)
        {
            LogSystem.LogMsg(
                csp::common::LogLevel::Warning, fmt::format("Replaced a previously registered schema for TypeId: {}", Schema.TypeId).c_str());
        }
    };

    AddSchema(StaticModelSpaceComponent::GetSchema());
    AddSchema(AnimatedModelSpaceComponent::GetSchema());
    AddSchema(VideoPlayerSpaceComponent::GetSchema());
    AddSchema(ImageSpaceComponent::GetSchema());
    AddSchema(ExternalLinkSpaceComponent::GetSchema());
    AddSchema(AvatarSpaceComponent::GetSchema());
    AddSchema(LightSpaceComponent::GetSchema());
    AddSchema(ScriptSpaceComponent::GetSchema());
    AddSchema(ButtonSpaceComponent::GetSchema());
    AddSchema(CustomSpaceComponent::GetSchema());
    AddSchema(PortalSpaceComponent::GetSchema());
    AddSchema(ConversationSpaceComponent::GetSchema());
    AddSchema(AudioSpaceComponent::GetSchema());
    AddSchema(SplineSpaceComponent::GetSchema());
    AddSchema(CollisionSpaceComponent::GetSchema());
    AddSchema(ReflectionSpaceComponent::GetSchema());
    AddSchema(FogSpaceComponent::GetSchema());
    AddSchema(ECommerceSpaceComponent::GetSchema());
    AddSchema(CinematicCameraSpaceComponent::GetSchema());
    AddSchema(FiducialMarkerSpaceComponent::GetSchema());
    AddSchema(GaussianSplatSpaceComponent::GetSchema());
    AddSchema(TextSpaceComponent::GetSchema());
    AddSchema(HotspotSpaceComponent::GetSchema());
    AddSchema(ScreenSharingSpaceComponent::GetSchema());
    AddSchema(AIChatbotSpaceComponent::GetSchema());

    for (const auto& Schema : AdditionalComponents)
    {
        if (const auto It = SchemaMap.find(Schema.TypeId); It != SchemaMap.end() && !IsCompatible(It->second, Schema, &LogSystem))
        {
            LogSystem.LogMsg(csp::common::LogLevel::Warning,
                fmt::format("Injected schema for TypeId {} is not compatible with the built-in schema and will be ignored.", Schema.TypeId).c_str());
            continue;
        }

        AddSchema(Schema);
    }
}

csp::common::Array<ComponentSchema> ComponentSchemaRegistryImpl::GetAll() const
{
    csp::common::Array<ComponentSchema> Result(SchemaMap.size());
    size_t Index = 0;

    for (const auto& [TypeId, Schema] : SchemaMap)
    {
        Result[Index++] = Schema;
    }

    return Result;
}

const ComponentSchema* ComponentSchemaRegistryImpl::Find(uint64_t TypeId) const
{
    const auto It = SchemaMap.find(TypeId);
    return It != SchemaMap.end() ? &It->second : nullptr;
}

std::optional<ComponentType> ToComponentType(uint64_t TypeId)
{
    using Underlying = std::underlying_type_t<ComponentType>;
    static_assert(std::is_unsigned_v<Underlying>);

    if (TypeId > static_cast<uint64_t>(std::numeric_limits<Underlying>::max()))
    {
        return std::nullopt;
    }

    switch (static_cast<ComponentType>(TypeId))
    {
    case ComponentType::Invalid:
    case ComponentType::Core:
    case ComponentType::UIController_DEPRECATED:
    case ComponentType::StaticModel:
    case ComponentType::AnimatedModel:
    case ComponentType::MediaSurface_DEPRECATED:
    case ComponentType::VideoPlayer:
    case ComponentType::ImageSequencer_DEPRECATED:
    case ComponentType::ExternalLink:
    case ComponentType::AvatarData:
    case ComponentType::Light:
    case ComponentType::Button:
    case ComponentType::Image:
    case ComponentType::ScriptData:
    case ComponentType::Custom:
    case ComponentType::Conversation:
    case ComponentType::Portal:
    case ComponentType::Audio:
    case ComponentType::Spline:
    case ComponentType::Collision:
    case ComponentType::Reflection:
    case ComponentType::Fog:
    case ComponentType::ECommerce:
    case ComponentType::FiducialMarker:
    case ComponentType::GaussianSplat:
    case ComponentType::Text:
    case ComponentType::Hotspot:
    case ComponentType::CinematicCamera:
    case ComponentType::ScreenSharing:
    case ComponentType::AIChatbot:
    case ComponentType::Delete:
        return static_cast<ComponentType>(TypeId);
    }

    return std::nullopt;
}

bool IsLegacyComponentTypeId(uint64_t TypeId) { return ToComponentType(TypeId).has_value(); }

} // namespace csp::multiplayer
