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

ComponentSchemaRegistry MergeWithLegacyComponents(const csp::common::Array<ComponentSchema>& additionalComponents)
{
    const auto toPair = [](const ComponentSchema& schema) {
        return std::make_pair(schema.TypeId, schema);
    };

    auto registry = ComponentSchemaRegistry {
        toPair(StaticModelSpaceComponent::GetSchema()),
        toPair(AnimatedModelSpaceComponent::GetSchema()),
        toPair(VideoPlayerSpaceComponent::GetSchema()),
        toPair(ImageSpaceComponent::GetSchema()),
        toPair(ExternalLinkSpaceComponent::GetSchema()),
        toPair(AvatarSpaceComponent::GetSchema()),
        toPair(LightSpaceComponent::GetSchema()),
        toPair(ScriptSpaceComponent::GetSchema()),
        toPair(ButtonSpaceComponent::GetSchema()),
        toPair(CustomSpaceComponent::GetSchema()),
        toPair(PortalSpaceComponent::GetSchema()),
        toPair(ConversationSpaceComponent::GetSchema()),
        toPair(AudioSpaceComponent::GetSchema()),
        toPair(SplineSpaceComponent::GetSchema()),
        toPair(CollisionSpaceComponent::GetSchema()),
        toPair(ReflectionSpaceComponent::GetSchema()),
        toPair(FogSpaceComponent::GetSchema()),
        toPair(ECommerceSpaceComponent::GetSchema()),
        toPair(CinematicCameraSpaceComponent::GetSchema()),
        toPair(FiducialMarkerSpaceComponent::GetSchema()),
        toPair(GaussianSplatSpaceComponent::GetSchema()),
        toPair(TextSpaceComponent::GetSchema()),
        toPair(HotspotSpaceComponent::GetSchema()),
        toPair(ScreenSharingSpaceComponent::GetSchema()),
        toPair(AIChatbotSpaceComponent::GetSchema()), 
    };

    for (const auto& schema : additionalComponents)
    {
        registry[schema.TypeId] = schema;
    }

    return registry;
}

} // namespace csp::multiplayer
