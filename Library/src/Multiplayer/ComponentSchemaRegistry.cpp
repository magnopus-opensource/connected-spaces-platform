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

namespace csp::multiplayer
{

ComponentSchemaRegistry MergeWithLegacyComponents(const csp::common::Array<ComponentSchema>& AdditionalComponents)
{
    const auto ToPair = [](const ComponentSchema& Schema) {
        return std::make_pair(Schema.TypeId, Schema);
    };

    auto Registry = ComponentSchemaRegistry {
        ToPair(StaticModelSpaceComponent::GetSchema()),
        ToPair(AnimatedModelSpaceComponent::GetSchema()),
        ToPair(VideoPlayerSpaceComponent::GetSchema()),
        ToPair(ImageSpaceComponent::GetSchema()),
        ToPair(ExternalLinkSpaceComponent::GetSchema()),
        ToPair(AvatarSpaceComponent::GetSchema()),
        ToPair(LightSpaceComponent::GetSchema()),
        ToPair(ScriptSpaceComponent::GetSchema()),
        ToPair(ButtonSpaceComponent::GetSchema()),
        ToPair(CustomSpaceComponent::GetSchema()),
        ToPair(PortalSpaceComponent::GetSchema()),
        ToPair(ConversationSpaceComponent::GetSchema()),
        ToPair(AudioSpaceComponent::GetSchema()),
        ToPair(SplineSpaceComponent::GetSchema()),
        ToPair(CollisionSpaceComponent::GetSchema()),
        ToPair(ReflectionSpaceComponent::GetSchema()),
        ToPair(FogSpaceComponent::GetSchema()),
        ToPair(ECommerceSpaceComponent::GetSchema()),
        ToPair(CinematicCameraSpaceComponent::GetSchema()),
        ToPair(FiducialMarkerSpaceComponent::GetSchema()),
        ToPair(GaussianSplatSpaceComponent::GetSchema()),
        ToPair(TextSpaceComponent::GetSchema()),
        ToPair(HotspotSpaceComponent::GetSchema()),
        ToPair(ScreenSharingSpaceComponent::GetSchema()),
        ToPair(AIChatbotSpaceComponent::GetSchema()), 
    };

    for (const auto& Schema : AdditionalComponents)
    {
        Registry[Schema.TypeId] = Schema;
    }

    return Registry;
}

} // namespace csp::multiplayer
