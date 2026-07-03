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

namespace
{

    const auto AllSchemas = csp::common::Array<ComponentSchema> {
        // [0] StaticModel
        ComponentSchema {
            static_cast<ComponentSchema::TypeIdType>(ComponentType::StaticModel),
            "StaticModel",
            csp::common::Array<ComponentProperty> {
                {
                    static_cast<ComponentProperty::KeyType>(StaticModelPropertyKeys::ExternalResourceAssetId),
                    "externalResourceAssetId",
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(StaticModelPropertyKeys::ExternalResourceAssetCollectionId),
                    "externalResourceAssetCollectionId",
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(StaticModelPropertyKeys::MaterialOverrides),
                    {}, // not exposed to scripting
                    csp::common::Map<csp::common::String, csp::common::ReplicatedValue>(),
                },
                {
                    static_cast<ComponentProperty::KeyType>(StaticModelPropertyKeys::Position),
                    "position",
                    csp::common::Vector3::Zero(),
                },
                {
                    static_cast<ComponentProperty::KeyType>(StaticModelPropertyKeys::Rotation),
                    "rotation",
                    csp::common::Vector4::Identity(),
                },
                {
                    static_cast<ComponentProperty::KeyType>(StaticModelPropertyKeys::Scale),
                    "scale",
                    csp::common::Vector3::One(),
                },
                {
                    static_cast<ComponentProperty::KeyType>(StaticModelPropertyKeys::IsVisible),
                    "isVisible",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(StaticModelPropertyKeys::IsARVisible),
                    "isARVisible",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(StaticModelPropertyKeys::ThirdPartyComponentRef),
                    {}, // not exposed to scripting
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(StaticModelPropertyKeys::IsShadowCaster),
                    "isShadowCaster",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(StaticModelPropertyKeys::IsVirtualVisible),
                    "isVirtualVisible",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(StaticModelPropertyKeys::ShowAsHoldoutInAR),
                    "showAsHoldoutInAR",
                    false,
                },
                {
                    static_cast<ComponentProperty::KeyType>(StaticModelPropertyKeys::ShowAsHoldoutInVirtual),
                    "showAsHoldoutInVirtual",
                    false,
                },
            },
        },
        // [1] AnimatedModel
        ComponentSchema {
            static_cast<ComponentSchema::TypeIdType>(ComponentType::AnimatedModel),
            "AnimatedModel",
            csp::common::Array<ComponentProperty> {
                {
                    static_cast<ComponentProperty::KeyType>(AnimatedModelPropertyKeys::ExternalResourceAssetId),
                    "externalResourceAssetId",
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(AnimatedModelPropertyKeys::ExternalResourceAssetCollectionId),
                    "externalResourceAssetCollectionId",
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(AnimatedModelPropertyKeys::MaterialOverrides),
                    {}, // not exposed to scripting
                    csp::common::Map<csp::common::String, csp::common::ReplicatedValue>(),
                },
                {
                    static_cast<ComponentProperty::KeyType>(AnimatedModelPropertyKeys::Position),
                    "position",
                    csp::common::Vector3 { 0, 0, 0 },
                },
                {
                    static_cast<ComponentProperty::KeyType>(AnimatedModelPropertyKeys::Rotation),
                    "rotation",
                    csp::common::Vector4 { 0, 0, 0, 1 },
                },
                {
                    static_cast<ComponentProperty::KeyType>(AnimatedModelPropertyKeys::Scale),
                    "scale",
                    csp::common::Vector3 { 1, 1, 1 },
                },
                {
                    static_cast<ComponentProperty::KeyType>(AnimatedModelPropertyKeys::IsLoopPlayback),
                    "isLoopPlayback",
                    false,
                },
                {
                    static_cast<ComponentProperty::KeyType>(AnimatedModelPropertyKeys::IsPlaying),
                    "isPlaying",
                    false,
                },
                {
                    static_cast<ComponentProperty::KeyType>(AnimatedModelPropertyKeys::IsVisible),
                    "isVisible",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(AnimatedModelPropertyKeys::AnimationIndex),
                    "animationIndex",
                    static_cast<int64_t>(-1),
                },
                {
                    static_cast<ComponentProperty::KeyType>(AnimatedModelPropertyKeys::IsARVisible),
                    "isARVisible",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(AnimatedModelPropertyKeys::ThirdPartyComponentRef),
                    {}, // not exposed to scripting
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(AnimatedModelPropertyKeys::IsShadowCaster),
                    "isShadowCaster",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(AnimatedModelPropertyKeys::IsVirtualVisible),
                    "isVirtualVisible",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(AnimatedModelPropertyKeys::ShowAsHoldoutInAR),
                    "showAsHoldoutInAR",
                    false,
                },
                {
                    static_cast<ComponentProperty::KeyType>(AnimatedModelPropertyKeys::ShowAsHoldoutInVirtual),
                    "showAsHoldoutInVirtual",
                    false,
                },
            },
        },
        // [2] VideoPlayer
        ComponentSchema {
            static_cast<ComponentSchema::TypeIdType>(ComponentType::VideoPlayer),
            "VideoPlayer",
            csp::common::Array<ComponentProperty> {
                {
                    static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::Name_DEPRECATED),
                    "name",
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::VideoAssetId),
                    "videoAssetId",
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::VideoAssetURL),
                    "videoAssetURL",
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::AssetCollectionId),
                    "assetCollectionId",
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::Position),
                    "position",
                    csp::common::Vector3 { 0, 0, 0 },
                },
                {
                    static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::Rotation),
                    "rotation",
                    csp::common::Vector4 { 0, 0, 0, 1 },
                },
                {
                    static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::Scale),
                    "scale",
                    csp::common::Vector3 { 1, 1, 1 },
                },
                {
                    static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::IsStateShared),
                    "isStateShared",
                    false,
                },
                {
                    static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::IsAutoPlay),
                    "isAutoPlay",
                    false,
                },
                {
                    static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::IsLoopPlayback),
                    "isLoopPlayback",
                    false,
                },
                {
                    static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::IsAutoResize),
                    "isAutoResize",
                    false,
                },
                {
                    static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::PlaybackState),
                    "playbackState",
                    static_cast<int64_t>(0),
                },
                {
                    static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::CurrentPlayheadPosition),
                    "currentPlayheadPosition",
                    0.0f,
                },
                {
                    static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::TimeSincePlay),
                    "timeSincePlay",
                    0.0f,
                },
                {
                    static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::AttenuationRadius),
                    "attenuationRadius",
                    10.f,
                },
                {
                    static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::VideoPlayerSourceType),
                    "videoPlayerSourceType",
                    static_cast<int64_t>(VideoPlayerSourceType::AssetSource),
                },
                {
                    static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::StereoVideoType),
                    "stereoVideoType",
                    static_cast<int64_t>(StereoVideoType::None),
                },
                {
                    static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::IsStereoFlipped),
                    "isStereoFlipped",
                    false,
                },
                {
                    static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::IsVisible),
                    "isVisible",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::IsARVisible),
                    "isARVisible",
                    true,
                },
                {
                    static_cast<uint16_t>(VideoPlayerPropertyKeys::MeshComponentId),
                    {}, // not exposed to scripting
                    static_cast<int64_t>(0),
                },
                {
                    static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::IsEnabled),
                    "isEnabled",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::IsVirtualVisible),
                    "isVirtualVisible",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::Volume),
                    {}, // not exposed to scripting via schema: we can't express value ranges (min, max) in schemas yet, so manually bind
                    1.f,
                },
                {
                    static_cast<ComponentProperty::KeyType>(VideoPlayerPropertyKeys::AudioType),
                    "audioType",
                    static_cast<int64_t>(AudioType::Spatial),
                },
            },
        },
        // [3] ExternalLink
        ComponentSchema {
            static_cast<ComponentSchema::TypeIdType>(ComponentType::ExternalLink),
            "ExternalLink",
            csp::common::Array<ComponentProperty> {
                {
                    static_cast<ComponentProperty::KeyType>(ExternalLinkPropertyKeys::Name_DEPRECATED),
                    "name",
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(ExternalLinkPropertyKeys::LinkUrl),
                    "linkUrl",
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(ExternalLinkPropertyKeys::Position),
                    "position",
                    csp::common::Vector3 { 0, 0, 0 },
                },
                {
                    static_cast<ComponentProperty::KeyType>(ExternalLinkPropertyKeys::Rotation),
                    "rotation",
                    csp::common::Vector4 { 0, 0, 0, 1 },
                },
                {
                    static_cast<ComponentProperty::KeyType>(ExternalLinkPropertyKeys::Scale),
                    "scale",
                    csp::common::Vector3 { 1, 1, 1 },
                },
                {
                    static_cast<ComponentProperty::KeyType>(ExternalLinkPropertyKeys::DisplayText),
                    "displayText",
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(ExternalLinkPropertyKeys::IsEnabled),
                    "isEnabled",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(ExternalLinkPropertyKeys::IsVisible),
                    "isVisible",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(ExternalLinkPropertyKeys::IsARVisible),
                    "isARVisible",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(ExternalLinkPropertyKeys::IsVirtualVisible),
                    "isVirtualVisible",
                    true,
                },
            },
        },
        // [4] Avatar
        ComponentSchema {
            static_cast<ComponentSchema::TypeIdType>(ComponentType::AvatarData),
            "Avatar",
            csp::common::Array<ComponentProperty> {
                {
                    static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::AvatarId),
                    "avatarId",
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::UserId),
                    "userId",
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::State),
                    "state",
                    static_cast<int64_t>(AvatarState::Idle),
                },
                {
                    static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::AvatarMeshIndex_DEPRECATED),
                    {}, // not exposed to scripting
                    static_cast<int64_t>(-1),
                },
                {
                    static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::AgoraUserId),
                    "agoraUserId",
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::CustomAvatarUrl_DEPRECATED),
                    {}, // not exposed to scripting
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::IsHandIKEnabled),
                    "isHandIKEnabled",
                    false,
                },
                {
                    static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::TargetHandIKTargetLocation),
                    "targetHandIKTargetLocation",
                    csp::common::Vector3 { 0.0f, 0.0f, 0.0f },
                },
                {
                    static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::HandRotation),
                    "handRotation",
                    csp::common::Vector4 { 0.0f, 0.0f, 0.0f, 1.0f },
                },
                {
                    static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::HeadRotation),
                    "headRotation",
                    csp::common::Vector4 { 0.0f, 0.0f, 0.0f, 1.0f },
                },
                {
                    static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::WalkRunBlendPercentage),
                    "walkRunBlendPercentage",
                    0.0f,
                },
                {
                    static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::TorsoTwistAlpha),
                    "torsoTwistAlpha",
                    0.0f,
                },
                {
                    static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::AvatarPlayMode),
                    "avatarPlayMode",
                    static_cast<int64_t>(AvatarPlayMode::Default),
                },
                {
                    static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::MovementDirection),
                    "movementDirection",
                    csp::common::Vector3 { 0.0f, 0.0f, 0.0f },
                },
                {
                    static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::LocomotionModel),
                    "locomotionModel",
                    static_cast<int64_t>(LocomotionModel::Grounded),
                },
                {
                    static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::IsVisible),
                    "isVisible",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::IsARVisible),
                    "isARVisible",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::IsVirtualVisible),
                    "isVirtualVisible",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(AvatarComponentPropertyKeys::AvatarUrl),
                    "avatarUrl",
                    "",
                },
            },
        },
        // [5] Light
        ComponentSchema {
            static_cast<ComponentSchema::TypeIdType>(ComponentType::Light),
            "Light",
            csp::common::Array<ComponentProperty> {
                {
                    static_cast<ComponentProperty::KeyType>(LightPropertyKeys::LightType),
                    "lightType",
                    static_cast<int64_t>(LightType::Point),
                },
                {
                    static_cast<ComponentProperty::KeyType>(LightPropertyKeys::Color),
                    "color",
                    csp::common::Vector3 { 255, 255, 255 },
                },
                {
                    static_cast<ComponentProperty::KeyType>(LightPropertyKeys::Intensity),
                    "Intensity", // Note: exposed as PascalCase for backwards compatibility (casing was wrong when this property was originally
                    // exposed)
                    5000.0f,
                },
                {
                    static_cast<ComponentProperty::KeyType>(LightPropertyKeys::Range),
                    "range",
                    1000.0f,
                },
                {
                    static_cast<ComponentProperty::KeyType>(LightPropertyKeys::InnerConeAngle),
                    "innerConeAngle",
                    0.0f,
                },
                {
                    static_cast<ComponentProperty::KeyType>(LightPropertyKeys::OuterConeAngle),
                    "outerConeAngle",
                    0.78539816339f, // Pi / 4
                },
                {
                    static_cast<ComponentProperty::KeyType>(LightPropertyKeys::Position),
                    "position",
                    csp::common::Vector3 { 0, 0, 0 },
                },
                {
                    static_cast<ComponentProperty::KeyType>(LightPropertyKeys::Rotation),
                    "rotation",
                    csp::common::Vector4 { 0, 0, 0, 1 },
                },
                {
                    static_cast<ComponentProperty::KeyType>(LightPropertyKeys::IsVisible),
                    "isVisible",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(LightPropertyKeys::LightCookieAssetId),
                    "cookieAssetId",
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(LightPropertyKeys::LightCookieAssetCollectionId),
                    "cookieAssetCollectionId",
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(LightPropertyKeys::LightCookieType),
                    "lightCookieType",
                    static_cast<int64_t>(LightCookieType::NoCookie),
                },
                {
                    static_cast<ComponentProperty::KeyType>(LightPropertyKeys::IsARVisible),
                    "isARVisible",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(LightPropertyKeys::ThirdPartyComponentRef),
                    {}, // not exposed to scripting
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(LightPropertyKeys::LightShadowType),
                    {}, // not exposed to scripting
                    static_cast<int64_t>(LightShadowType::None),
                },
                {
                    static_cast<ComponentProperty::KeyType>(LightPropertyKeys::IsVirtualVisible),
                    "isVirtualVisible",
                    true,
                },
            },
        },
        // [6] Button
        ComponentSchema {
            static_cast<ComponentSchema::TypeIdType>(ComponentType::Button),
            "Button",
            csp::common::Array<ComponentProperty> {
                {
                    static_cast<ComponentProperty::KeyType>(ButtonPropertyKeys::LabelText),
                    "labelText",
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(ButtonPropertyKeys::IconAssetId),
                    "iconAssetId",
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(ButtonPropertyKeys::AssetCollectionId),
                    "assetCollectionId",
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(ButtonPropertyKeys::Position),
                    "position",
                    csp::common::Vector3 { 0, 0, 0 },
                },
                {
                    static_cast<ComponentProperty::KeyType>(ButtonPropertyKeys::Rotation),
                    "rotation",
                    csp::common::Vector4 { 0, 0, 0, 1 },
                },
                {
                    static_cast<ComponentProperty::KeyType>(ButtonPropertyKeys::Scale),
                    "scale",
                    csp::common::Vector3 { 1, 1, 1 },
                },
                {
                    static_cast<ComponentProperty::KeyType>(ButtonPropertyKeys::IsVisible),
                    "isVisible",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(ButtonPropertyKeys::IsEnabled),
                    "isEnabled",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(ButtonPropertyKeys::IsARVisible),
                    "isARVisible",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(ButtonPropertyKeys::IsVirtualVisible),
                    "isVirtualVisible",
                    true,
                },
            },
        },
        // [7] Image
        ComponentSchema {
            static_cast<ComponentSchema::TypeIdType>(ComponentType::Image),
            "Image",
            csp::common::Array<ComponentProperty> {
                {
                    static_cast<ComponentProperty::KeyType>(ImagePropertyKeys::Name_DEPRECATED),
                    "name",
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(ImagePropertyKeys::ImageAssetId),
                    "imageAssetId",
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(ImagePropertyKeys::AssetCollectionId),
                    "assetCollectionId",
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(ImagePropertyKeys::Position),
                    "position",
                    csp::common::Vector3::Zero(),
                },
                {
                    static_cast<ComponentProperty::KeyType>(ImagePropertyKeys::Rotation),
                    "rotation",
                    csp::common::Vector4 { 0, 0, 0, 1 },
                },
                {
                    static_cast<ComponentProperty::KeyType>(ImagePropertyKeys::Scale),
                    "scale",
                    csp::common::Vector3::One(),
                },
                {
                    static_cast<ComponentProperty::KeyType>(ImagePropertyKeys::IsVisible),
                    "isVisible",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(ImagePropertyKeys::BillboardMode),
                    "billboardMode",
                    static_cast<int64_t>(BillboardMode::Off),
                },
                {
                    static_cast<ComponentProperty::KeyType>(ImagePropertyKeys::DisplayMode),
                    "displayMode",
                    static_cast<int64_t>(DisplayMode::DoubleSided),
                },
                {
                    static_cast<ComponentProperty::KeyType>(ImagePropertyKeys::IsARVisible),
                    "isARVisible",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(ImagePropertyKeys::IsEmissive),
                    "isEmissive",
                    false,
                },
                {
                    static_cast<ComponentProperty::KeyType>(ImagePropertyKeys::IsVirtualVisible),
                    "isVirtualVisible",
                    true,
                },
            },
        },
        // [8] Script
        ComponentSchema {
            static_cast<ComponentSchema::TypeIdType>(ComponentType::ScriptData),
            {}, // not exposed to scripting
            csp::common::Array<ComponentProperty> {
                {
                    static_cast<ComponentProperty::KeyType>(ScriptComponentPropertyKeys::ScriptSource),
                    {}, // not exposed to scripting
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(ScriptComponentPropertyKeys::OwnerId),
                    {}, // not exposed to scripting
                    static_cast<int64_t>(0),
                },
                {
                    static_cast<ComponentProperty::KeyType>(ScriptComponentPropertyKeys::ScriptScope),
                    {}, // not exposed to scripting
                    static_cast<int64_t>(ScriptScope::Owner),
                },
            },
        },
        // [9] Custom
        ComponentSchema {
            static_cast<ComponentSchema::TypeIdType>(ComponentType::Custom),
            "Custom",
            csp::common::Array<ComponentProperty> {
                {
                    static_cast<ComponentProperty::KeyType>(CustomComponentPropertyKeys::ApplicationOrigin),
                    "applicationOrigin",
                    "",
                },
            },
        },
        // [10] Conversation
        ComponentSchema {
            static_cast<ComponentSchema::TypeIdType>(ComponentType::Conversation),
            "Conversation",
            csp::common::Array<ComponentProperty> {
                {
                    static_cast<ComponentProperty::KeyType>(ConversationPropertyKeys::ConversationId),
                    {}, // not exposed to scripting
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(ConversationPropertyKeys::IsActive),
                    "isActive",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(ConversationPropertyKeys::IsVisible),
                    "isVisible",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(ConversationPropertyKeys::Position),
                    "position",
                    csp::common::Vector3 { 0, 0, 0 },
                },
                {
                    static_cast<ComponentProperty::KeyType>(ConversationPropertyKeys::Rotation),
                    "rotation",
                    csp::common::Vector4 { 0, 0, 0, 1 },
                },
                {
                    static_cast<ComponentProperty::KeyType>(ConversationPropertyKeys::Title),
                    "title",
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(ConversationPropertyKeys::Resolved),
                    "resolved",
                    false,
                },
                {
                    static_cast<ComponentProperty::KeyType>(ConversationPropertyKeys::ConversationCameraPosition),
                    "conversationCameraPosition",
                    csp::common::Vector3 { 0, 0, 0 },
                },
                {
                    static_cast<ComponentProperty::KeyType>(ConversationPropertyKeys::ConversationCameraRotation),
                    "conversationCameraRotation",
                    csp::common::Vector4 { 0, 0, 0, 1 },
                },
            },
        },
        // [11] Portal
        ComponentSchema {
            static_cast<ComponentSchema::TypeIdType>(ComponentType::Portal),
            "Portal",
            csp::common::Array<ComponentProperty> {
                {
                    static_cast<ComponentProperty::KeyType>(PortalPropertyKeys::IsVisible),
                    {}, // not exposed to scripting
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(PortalPropertyKeys::IsActive),
                    {}, // not exposed to scripting
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(PortalPropertyKeys::SpaceId),
                    "spaceId",
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(PortalPropertyKeys::IsARVisible),
                    {}, // not exposed to scripting
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(PortalPropertyKeys::IsEnabled),
                    "isEnabled",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(PortalPropertyKeys::Position),
                    "position",
                    csp::common::Vector3 { 0, 0, 0 },
                },
                {
                    static_cast<ComponentProperty::KeyType>(PortalPropertyKeys::Radius),
                    "radius",
                    1.5f,
                },
            },
        },
        // [12] Audio
        ComponentSchema {
            static_cast<ComponentSchema::TypeIdType>(ComponentType::Audio),
            "Audio",
            csp::common::Array<ComponentProperty> {
                {
                    static_cast<ComponentProperty::KeyType>(AudioPropertyKeys::Position),
                    "position",
                    csp::common::Vector3 { 0, 0, 0 },
                },
                {
                    static_cast<ComponentProperty::KeyType>(AudioPropertyKeys::PlaybackState),
                    "playbackState",
                    static_cast<int64_t>(AudioPlaybackState::Reset),
                },
                {
                    static_cast<ComponentProperty::KeyType>(AudioPropertyKeys::AudioType),
                    "audioType",
                    static_cast<int64_t>(AudioType::Global),
                },
                {
                    static_cast<ComponentProperty::KeyType>(AudioPropertyKeys::AudioAssetId),
                    "audioAssetId",
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(AudioPropertyKeys::AssetCollectionId),
                    "assetCollectionId",
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(AudioPropertyKeys::AttenuationRadius),
                    "attenuationRadius",
                    10.f,
                },
                {
                    static_cast<ComponentProperty::KeyType>(AudioPropertyKeys::IsLoopPlayback),
                    "isLoopPlayback",
                    false,
                },
                {
                    static_cast<ComponentProperty::KeyType>(AudioPropertyKeys::TimeSincePlay),
                    "timeSincePlay",
                    0.f,
                },
                {
                    static_cast<ComponentProperty::KeyType>(AudioPropertyKeys::Volume),
                    {}, // not exposed to scripting via schema: we can't express value ranges (min, max) in schemas yet, so manually bind
                    1.f,
                },
                {
                    static_cast<ComponentProperty::KeyType>(AudioPropertyKeys::IsEnabled),
                    "isEnabled",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(AudioPropertyKeys::ThirdPartyComponentRef),
                    {}, // not exposed to scripting
                    "",
                },
            },
        },
        // [13] Spline
        ComponentSchema {
            static_cast<ComponentSchema::TypeIdType>(ComponentType::Spline),
            "Spline",
            csp::common::Array<ComponentProperty> {
                {
                    static_cast<ComponentProperty::KeyType>(SplinePropertyKeys::Waypoints),
                    {}, // not exposed to scripting via an auto-generated property (has a legacy manual getter function)
                    0.f,
                },
            },
        },
        // [14] Collision
        ComponentSchema {
            static_cast<ComponentSchema::TypeIdType>(ComponentType::Collision),
            {}, // not exposed to scripting
            csp::common::Array<ComponentProperty> {
                {
                    static_cast<ComponentProperty::KeyType>(CollisionPropertyKeys::Position),
                    {}, // not exposed to scripting
                    csp::common::Vector3 { 0, 0, 0 },
                },
                {
                    static_cast<ComponentProperty::KeyType>(CollisionPropertyKeys::Rotation),
                    {}, // not exposed to scripting
                    csp::common::Vector4 { 0, 0, 0, 1 },
                },
                {
                    static_cast<ComponentProperty::KeyType>(CollisionPropertyKeys::Scale),
                    {}, // not exposed to scripting
                    csp::common::Vector3 { 1, 1, 1 },
                },
                {
                    static_cast<ComponentProperty::KeyType>(CollisionPropertyKeys::CollisionShape),
                    {}, // not exposed to scripting
                    static_cast<int64_t>(CollisionShape::Box),
                },
                {
                    static_cast<ComponentProperty::KeyType>(CollisionPropertyKeys::CollisionMode),
                    {}, // not exposed to scripting
                    static_cast<int64_t>(CollisionMode::Collision),
                },
                {
                    static_cast<ComponentProperty::KeyType>(CollisionPropertyKeys::CollisionAssetId),
                    {}, // not exposed to scripting
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(CollisionPropertyKeys::AssetCollectionId),
                    {}, // not exposed to scripting
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(CollisionPropertyKeys::ThirdPartyComponentRef),
                    {}, // not exposed to scripting
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(CollisionPropertyKeys::IsEnabled),
                    {}, // not exposed to scripting
                    true,
                },
            },
        },
        // [15] Reflection
        ComponentSchema {
            static_cast<ComponentSchema::TypeIdType>(ComponentType::Reflection),
            {}, // not exposed to scripting
            csp::common::Array<ComponentProperty> {
                {
                    static_cast<ComponentProperty::KeyType>(ReflectionPropertyKeys::Name_DEPRECATED),
                    {}, // not exposed to scripting
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(ReflectionPropertyKeys::ReflectionAssetId),
                    {}, // not exposed to scripting
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(ReflectionPropertyKeys::AssetCollectionId),
                    {}, // not exposed to scripting
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(ReflectionPropertyKeys::Position),
                    {}, // not exposed to scripting
                    csp::common::Vector3::Zero(),
                },
                {
                    static_cast<ComponentProperty::KeyType>(ReflectionPropertyKeys::Scale),
                    {}, // not exposed to scripting
                    csp::common::Vector3::One(),
                },
                {
                    static_cast<ComponentProperty::KeyType>(ReflectionPropertyKeys::ReflectionShape),
                    {}, // not exposed to scripting
                    static_cast<int64_t>(ReflectionShape::UnitBox),
                },
                {
                    static_cast<ComponentProperty::KeyType>(ReflectionPropertyKeys::ThirdPartyComponentRef),
                    {}, // not exposed to scripting
                    "",
                },
            },
        },
        // [16] Fog
        ComponentSchema {
            static_cast<ComponentSchema::TypeIdType>(ComponentType::Fog),
            "Fog",
            csp::common::Array<ComponentProperty> {
                {
                    static_cast<ComponentProperty::KeyType>(FogPropertyKeys::FogMode),
                    "fogMode",
                    static_cast<int64_t>(FogMode::Exponential),
                },
                {
                    static_cast<ComponentProperty::KeyType>(FogPropertyKeys::Position),
                    "position",
                    csp::common::Vector3 { 0, 0, 0 },
                },
                {
                    static_cast<ComponentProperty::KeyType>(FogPropertyKeys::Rotation),
                    "rotation",
                    csp::common::Vector4 { 0, 0, 0, 1 },
                },
                {
                    static_cast<ComponentProperty::KeyType>(FogPropertyKeys::Scale),
                    "scale",
                    csp::common::Vector3 { 1, 1, 1 },
                },
                {
                    static_cast<ComponentProperty::KeyType>(FogPropertyKeys::StartDistance),
                    "startDistance",
                    0.f,
                },
                {
                    static_cast<ComponentProperty::KeyType>(FogPropertyKeys::EndDistance),
                    "endDistance",
                    0.f,
                },
                {
                    static_cast<ComponentProperty::KeyType>(FogPropertyKeys::Color),
                    "color",
                    csp::common::Vector3 { 0.8f, 0.9f, 1.0f },
                },
                {
                    static_cast<ComponentProperty::KeyType>(FogPropertyKeys::Density),
                    "density",
                    0.4f,
                },
                {
                    static_cast<ComponentProperty::KeyType>(FogPropertyKeys::HeightFalloff),
                    "heightFalloff",
                    0.2f,
                },
                {
                    static_cast<ComponentProperty::KeyType>(FogPropertyKeys::MaxOpacity),
                    "maxOpacity",
                    1.f,
                },
                {
                    static_cast<ComponentProperty::KeyType>(FogPropertyKeys::IsVolumetric),
                    "isVolumetric",
                    false,
                },
                {
                    static_cast<ComponentProperty::KeyType>(FogPropertyKeys::IsVisible),
                    "isVisible",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(FogPropertyKeys::IsARVisible),
                    "isARVisible",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(FogPropertyKeys::ThirdPartyComponentRef),
                    {}, // not exposed to scripting
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(FogPropertyKeys::IsVirtualVisible),
                    "isVirtualVisible",
                    true,
                },
            },
        },
        // [17] ECommerce
        ComponentSchema {
            static_cast<ComponentSchema::TypeIdType>(ComponentType::ECommerce),
            "ECommerce",
            csp::common::Array<ComponentProperty> {
                {
                    static_cast<ComponentProperty::KeyType>(ECommercePropertyKeys::Position),
                    "position",
                    csp::common::Vector3 { 0, 0, 0 },
                },
                {
                    static_cast<ComponentProperty::KeyType>(ECommercePropertyKeys::ProductId),
                    "productId",
                    "",
                },
            },
        },
        // [18] FiducialMarker
        ComponentSchema {
            static_cast<ComponentSchema::TypeIdType>(ComponentType::FiducialMarker),
            "FiducialMarker",
            csp::common::Array<ComponentProperty> {
                {
                    static_cast<ComponentProperty::KeyType>(FiducialMarkerPropertyKeys::Name_DEPRECATED),
                    "name",
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(FiducialMarkerPropertyKeys::MarkerAssetId),
                    "markerAssetId",
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(FiducialMarkerPropertyKeys::AssetCollectionId),
                    "assetCollectionId",
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(FiducialMarkerPropertyKeys::Position),
                    "position",
                    csp::common::Vector3::Zero(),
                },
                {
                    static_cast<ComponentProperty::KeyType>(FiducialMarkerPropertyKeys::Rotation),
                    "rotation",
                    csp::common::Vector4 { 0, 0, 0, 1 },
                },
                {
                    static_cast<ComponentProperty::KeyType>(FiducialMarkerPropertyKeys::Scale),
                    "scale",
                    csp::common::Vector3::One(),
                },
                {
                    static_cast<ComponentProperty::KeyType>(FiducialMarkerPropertyKeys::IsVisible),
                    "isVisible",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(FiducialMarkerPropertyKeys::IsARVisible),
                    "isARVisible",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(FiducialMarkerPropertyKeys::IsVirtualVisible),
                    "isVirtualVisible",
                    true,
                },
            },
        },
        // [19] GaussianSplat
        ComponentSchema {
            static_cast<ComponentSchema::TypeIdType>(ComponentType::GaussianSplat),
            "GaussianSplat",
            csp::common::Array<ComponentProperty> {
                {
                    static_cast<ComponentProperty::KeyType>(GaussianSplatPropertyKeys::ExternalResourceAssetId),
                    "externalResourceAssetId",
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(GaussianSplatPropertyKeys::ExternalResourceAssetCollectionId),
                    "externalResourceAssetCollectionId",
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(GaussianSplatPropertyKeys::Position),
                    "position",
                    csp::common::Vector3::Zero(),
                },
                {
                    static_cast<ComponentProperty::KeyType>(GaussianSplatPropertyKeys::Rotation),
                    "rotation",
                    csp::common::Vector4::Identity(),
                },
                {
                    static_cast<ComponentProperty::KeyType>(GaussianSplatPropertyKeys::Scale),
                    "scale",
                    csp::common::Vector3::One(),
                },
                {
                    static_cast<ComponentProperty::KeyType>(GaussianSplatPropertyKeys::IsVisible),
                    "isVisible",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(GaussianSplatPropertyKeys::IsARVisible),
                    "isARVisible",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(GaussianSplatPropertyKeys::IsShadowCaster_DEPRECATED),
                    {}, // not exposed to scripting: this is a deprecated property that was never exposed to scripting, so no need to start now.
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(GaussianSplatPropertyKeys::Tint),
                    "tint",
                    csp::common::Vector3::One(),
                },
                {
                    static_cast<ComponentProperty::KeyType>(GaussianSplatPropertyKeys::IsVirtualVisible),
                    "isVirtualVisible",
                    true,
                },
            },
        },
        // [20] Text
        ComponentSchema {
            static_cast<ComponentSchema::TypeIdType>(ComponentType::Text),
            "Text",
            csp::common::Array<ComponentProperty> {
                {
                    static_cast<ComponentProperty::KeyType>(TextPropertyKeys::Position),
                    "position",
                    csp::common::Vector3::Zero(),
                },
                {
                    static_cast<ComponentProperty::KeyType>(TextPropertyKeys::Rotation),
                    "rotation",
                    csp::common::Vector4 { 0, 0, 0, 1 },
                },
                {
                    static_cast<ComponentProperty::KeyType>(TextPropertyKeys::Scale),
                    "scale",
                    csp::common::Vector3::One(),
                },
                {
                    static_cast<ComponentProperty::KeyType>(TextPropertyKeys::Text),
                    "text",
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(TextPropertyKeys::TextColor),
                    "textColor",
                    csp::common::Vector3(1.0f, 1.0f, 1.0f),
                },
                {
                    static_cast<ComponentProperty::KeyType>(TextPropertyKeys::BackgroundColor),
                    "backgroundColor",
                    csp::common::Vector3::Zero(),
                },
                {
                    static_cast<ComponentProperty::KeyType>(TextPropertyKeys::IsBackgroundVisible),
                    "isBackgroundVisible",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(TextPropertyKeys::Width),
                    "width",
                    1.0f,
                },
                {
                    static_cast<ComponentProperty::KeyType>(TextPropertyKeys::Height),
                    "height",
                    1.0f,
                },
                {
                    static_cast<ComponentProperty::KeyType>(TextPropertyKeys::BillboardMode),
                    "billboardMode",
                    static_cast<int64_t>(BillboardMode::Off),
                },
                {
                    static_cast<ComponentProperty::KeyType>(TextPropertyKeys::IsVisible),
                    "isVisible",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(TextPropertyKeys::IsARVisible),
                    "isARVisible",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(TextPropertyKeys::IsVirtualVisible),
                    "isVirtualVisible",
                    true,
                },
            },
        },
        // [21] Hotspot
        ComponentSchema {
            static_cast<ComponentSchema::TypeIdType>(ComponentType::Hotspot),
            "Hotspot",
            csp::common::Array<ComponentProperty> {
                {
                    static_cast<ComponentProperty::KeyType>(HotspotPropertyKeys::Position),
                    "position",
                    csp::common::Vector3::Zero(),
                },
                {
                    static_cast<ComponentProperty::KeyType>(HotspotPropertyKeys::Rotation),
                    "rotation",
                    csp::common::Vector4 { 0, 0, 0, 1 },
                },
                {
                    static_cast<ComponentProperty::KeyType>(HotspotPropertyKeys::Name_DEPRECATED),
                    {}, // not exposed to scripting
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(HotspotPropertyKeys::IsTeleportPoint),
                    "isTeleportPoint",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(HotspotPropertyKeys::IsSpawnPoint),
                    "isSpawnPoint",
                    false,
                },
                {
                    static_cast<ComponentProperty::KeyType>(HotspotPropertyKeys::IsVisible),
                    "isVisible",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(HotspotPropertyKeys::IsARVisible),
                    "isARVisible",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(HotspotPropertyKeys::IsVirtualVisible),
                    "isVirtualVisible",
                    true,
                },
            },
        },
        // [22] CinematicCamera
        ComponentSchema {
            static_cast<ComponentSchema::TypeIdType>(ComponentType::CinematicCamera),
            "CinematicCamera",
            csp::common::Array<ComponentProperty> {
                {
                    static_cast<ComponentProperty::KeyType>(CinematicCameraPropertyKeys::Position),
                    "position",
                    csp::common::Vector3::Zero(),
                },
                {
                    static_cast<ComponentProperty::KeyType>(CinematicCameraPropertyKeys::Rotation),
                    "rotation",
                    csp::common::Vector4::Identity(),
                },
                {
                    static_cast<ComponentProperty::KeyType>(CinematicCameraPropertyKeys::IsEnabled),
                    "isEnabled",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(CinematicCameraPropertyKeys::FocalLength),
                    "focalLength",
                    0.035f,
                },
                // 16:9
                {
                    static_cast<ComponentProperty::KeyType>(CinematicCameraPropertyKeys::AspectRatio),
                    "aspectRatio",
                    1.778f,
                },
                {
                    static_cast<ComponentProperty::KeyType>(CinematicCameraPropertyKeys::SensorSize),
                    "sensorSize",
                    csp::common::Vector2 { 0.036f, 0.024f },
                },
                {
                    static_cast<ComponentProperty::KeyType>(CinematicCameraPropertyKeys::NearClip),
                    "nearClip",
                    0.1f,
                },
                {
                    static_cast<ComponentProperty::KeyType>(CinematicCameraPropertyKeys::FarClip),
                    "farClip",
                    20000.0f,
                },
                {
                    static_cast<ComponentProperty::KeyType>(CinematicCameraPropertyKeys::Iso),
                    "iso",
                    400.0f,
                },
                // 60 FPS
                {
                    static_cast<ComponentProperty::KeyType>(CinematicCameraPropertyKeys::ShutterSpeed),
                    "shutterSpeed",
                    0.0167f,
                },
                {
                    static_cast<ComponentProperty::KeyType>(CinematicCameraPropertyKeys::Aperture),
                    "aperture",
                    4.0f,
                },
                {
                    static_cast<ComponentProperty::KeyType>(CinematicCameraPropertyKeys::FocusDistance),
                    "focusDistance",
                    5.0f,
                },
                {
                    static_cast<ComponentProperty::KeyType>(CinematicCameraPropertyKeys::DepthOfFieldEnabled),
                    "depthOfFieldEnabled",
                    false,
                },
                {
                    static_cast<ComponentProperty::KeyType>(CinematicCameraPropertyKeys::IsViewerCamera),
                    "isViewerCamera",
                    false,
                },
                {
                    static_cast<ComponentProperty::KeyType>(CinematicCameraPropertyKeys::ThirdPartyComponentRef),
                    {}, // not exposed to scripting
                    "",
                },
            },
        },
        // [23] ScreenSharing
        ComponentSchema {
            static_cast<ComponentSchema::TypeIdType>(ComponentType::ScreenSharing),
            "ScreenSharing",
            csp::common::Array<ComponentProperty> {
                {
                    static_cast<ComponentProperty::KeyType>(ScreenSharingPropertyKeys::Position),
                    "position",
                    csp::common::Vector3::Zero(),
                },
                {
                    static_cast<ComponentProperty::KeyType>(ScreenSharingPropertyKeys::Rotation),
                    "rotation",
                    csp::common::Vector4::Identity(),
                },
                {
                    static_cast<ComponentProperty::KeyType>(ScreenSharingPropertyKeys::Scale),
                    "scale",
                    csp::common::Vector3::One(),
                },
                {
                    static_cast<ComponentProperty::KeyType>(ScreenSharingPropertyKeys::IsVisible),
                    "isVisible",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(ScreenSharingPropertyKeys::IsARVisible),
                    "isARVisible",
                    true,
                },
                {
                    static_cast<ComponentProperty::KeyType>(ScreenSharingPropertyKeys::IsShadowCaster),
                    "isShadowCaster",
                    false,
                },
                {
                    static_cast<ComponentProperty::KeyType>(ScreenSharingPropertyKeys::UserId),
                    "userId",
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(ScreenSharingPropertyKeys::DefaultImageCollectionId),
                    "defaultImageCollectionId",
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(ScreenSharingPropertyKeys::DefaultImageAssetId),
                    "defaultImageAssetId",
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(ScreenSharingPropertyKeys::AttenuationRadius),
                    "attenuationRadius",
                    10.f,
                },
                {
                    static_cast<ComponentProperty::KeyType>(ScreenSharingPropertyKeys::IsVirtualVisible),
                    "isVirtualVisible",
                    true,
                },
            },
        },
        // [24] AIChatbot
        ComponentSchema {
            static_cast<ComponentSchema::TypeIdType>(ComponentType::AIChatbot),
            "AIChatbot",
            csp::common::Array<ComponentProperty> {
                {
                    static_cast<ComponentProperty::KeyType>(AIChatbotPropertyKeys::Position),
                    "position",
                    csp::common::Vector3::Zero(),
                },
                {
                    static_cast<ComponentProperty::KeyType>(AIChatbotPropertyKeys::Voice),
                    "voice",
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(AIChatbotPropertyKeys::GuardrailAssetCollectionId),
                    "guardrailAssetCollectionId",
                    "",
                },
                {
                    static_cast<ComponentProperty::KeyType>(AIChatbotPropertyKeys::VisualState),
                    "visualState",
                    static_cast<int64_t>(0),
                },
            },
        },
    };

} // namespace

const csp::common::Array<ComponentSchema>& GetAllComponentSchemas() { return AllSchemas; }

const ComponentSchema& GetStaticModelSchema() { return AllSchemas[0]; }
const ComponentSchema& GetAnimatedModelSchema() { return AllSchemas[1]; }
const ComponentSchema& GetVideoPlayerSchema() { return AllSchemas[2]; }
const ComponentSchema& GetExternalLinkSchema() { return AllSchemas[3]; }
const ComponentSchema& GetAvatarSchema() { return AllSchemas[4]; }
const ComponentSchema& GetLightSchema() { return AllSchemas[5]; }
const ComponentSchema& GetButtonSchema() { return AllSchemas[6]; }
const ComponentSchema& GetImageSchema() { return AllSchemas[7]; }
const ComponentSchema& GetScriptSchema() { return AllSchemas[8]; }
const ComponentSchema& GetCustomSchema() { return AllSchemas[9]; }
const ComponentSchema& GetConversationSchema() { return AllSchemas[10]; }
const ComponentSchema& GetPortalSchema() { return AllSchemas[11]; }
const ComponentSchema& GetAudioSchema() { return AllSchemas[12]; }
const ComponentSchema& GetSplineSchema() { return AllSchemas[13]; }
const ComponentSchema& GetCollisionSchema() { return AllSchemas[14]; }
const ComponentSchema& GetReflectionSchema() { return AllSchemas[15]; }
const ComponentSchema& GetFogSchema() { return AllSchemas[16]; }
const ComponentSchema& GetECommerceSchema() { return AllSchemas[17]; }
const ComponentSchema& GetFiducialMarkerSchema() { return AllSchemas[18]; }
const ComponentSchema& GetGaussianSplatSchema() { return AllSchemas[19]; }
const ComponentSchema& GetTextSchema() { return AllSchemas[20]; }
const ComponentSchema& GetHotspotSchema() { return AllSchemas[21]; }
const ComponentSchema& GetCinematicCameraSchema() { return AllSchemas[22]; }
const ComponentSchema& GetScreenSharingSchema() { return AllSchemas[23]; }
const ComponentSchema& GetAIChatbotSchema() { return AllSchemas[24]; }

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

    AddSchema(GetStaticModelSchema());
    AddSchema(GetAnimatedModelSchema());
    AddSchema(GetVideoPlayerSchema());
    AddSchema(GetExternalLinkSchema());
    AddSchema(GetAvatarSchema());
    AddSchema(GetLightSchema());
    AddSchema(GetButtonSchema());
    AddSchema(GetImageSchema());
    AddSchema(GetScriptSchema());
    AddSchema(GetCustomSchema());
    AddSchema(GetConversationSchema());
    AddSchema(GetPortalSchema());
    AddSchema(GetAudioSchema());
    AddSchema(GetSplineSchema());
    AddSchema(GetCollisionSchema());
    AddSchema(GetReflectionSchema());
    AddSchema(GetFogSchema());
    AddSchema(GetECommerceSchema());
    AddSchema(GetFiducialMarkerSchema());
    AddSchema(GetGaussianSplatSchema());
    AddSchema(GetTextSchema());
    AddSchema(GetHotspotSchema());
    AddSchema(GetCinematicCameraSchema());
    AddSchema(GetScreenSharingSchema());
    AddSchema(GetAIChatbotSchema());

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
