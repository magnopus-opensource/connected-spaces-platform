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

/// @file AnimatedModelSpaceComponent.h
/// @brief Definitions and support for animated models.

#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/ComponentBase.h"
#include "CSP/Multiplayer/Components/Interfaces/IExternalResourceComponent.h"
#include "CSP/Multiplayer/Components/Interfaces/IShadowCasterComponent.h"
#include "CSP/Multiplayer/Components/Interfaces/IThirdPartyComponentRef.h"
#include "CSP/Multiplayer/Components/Interfaces/ITransformComponent.h"
#include "CSP/Multiplayer/Components/Interfaces/IVisibleComponent.h"
#include "CSP/Multiplayer/SpaceTransform.h"

namespace csp::multiplayer
{

/// @brief Enumerates the actions that can be performed on an animated model component.
enum class AnimatedModelActions
{
    Play,
    Pause,
    Restart,
    Num
};

/// @brief Enumerates the list of properties that can be replicated for an animated model component.
enum class AnimatedModelPropertyKeys
{
    Name_DEPRECATED = 0,
    ExternalResourceAssetId,
    ExternalResourceAssetCollectionId,
    Position,
    Rotation,
    Scale,
    IsLoopPlayback,
    IsPlaying,
    IsVisible,
    RESERVED,
    AnimationIndex,
    IsARVisible,
    ThirdPartyComponentRef,
    IsShadowCaster,
    MaterialOverrides,
    Num
};

/// @ingroup AnimatedModelSpaceComponent
/// @brief Adds animated skeletal meshes to a SpaceEntity.
///
/// These are used for objects that need to move or act within the space, such as characters or animated props.
/// This component makes it possible to play, pause, or loop animations.
class CSP_API AnimatedModelSpaceComponent : public ComponentBase,
                                            public ITransformComponent,
                                            public IVisibleComponent,
                                            public IExternalResourceComponent,
                                            public IThirdPartyComponentRef,
                                            public IShadowCasterComponent
{
public:
    /// @brief Constructs the animated model space component, and associates it with the specified Parent space entity.
    /// @param Parent The Space entity that owns this component.
    AnimatedModelSpaceComponent(SpaceEntity* Parent);

    /* clang-format off */
	[[deprecated("Due to the introduction of LODs it doesn't make sense to set a specific asset anymore")]]
    const csp::common::String& GetExternalResourceAssetId() const override;

	[[deprecated("Due to the introduction of LODs it doesn't make sense to set a specific asset anymore")]]
    void SetExternalResourceAssetId(const csp::common::String& Value) override;
    /* clang-format on */

    /// @brief Gets the ID of the asset collection associated with this component.
    /// @note To retrieve this component's animated asset, both the Asset ID and the Asset Collection ID are required.
    /// @return The ID of the asset collection associated with this component.
    const csp::common::String& GetExternalResourceAssetCollectionId() const override;

    /// @brief Sets the ID of the asset collection associated with this component.
    /// @note To retrieve this component's animated asset, both the Asset ID and the Asset Collection ID are required.
    /// @param Value The ID of the asset collection associated with this component.
    void SetExternalResourceAssetCollectionId(const csp::common::String& Value) override;

    /// \addtogroup ITransformComponent
    /// @{
    /// @copydoc IPositionComponent::GetPosition()
    const csp::common::Vector3& GetPosition() const override;
    /// @copydoc IPositionComponent::SetPosition()
    void SetPosition(const csp::common::Vector3& InValue) override;
    /// @copydoc IRotationComponent::GetRotation()
    const csp::common::Vector4& GetRotation() const override;
    /// @copydoc IRotationComponent::SetRotation()
    void SetRotation(const csp::common::Vector4& InValue) override;
    /// @copydoc IScaleComponent::GetScale()
    const csp::common::Vector3& GetScale() const override;
    /// @copydoc IScaleComponent::SetScale()
    void SetScale(const csp::common::Vector3& InValue) override;
    /// @copydoc ITransformComponent::GetTransform()
    SpaceTransform GetTransform() const override;
    /// @copydoc ITransformComonent::SetTransform()
    void SetTransform(const SpaceTransform& InValue) override;
    /// @}

    /// @brief Checks if the animation of this animated model is in loop.
    /// @return True if the animation of this animated model loops (i.e. restarts from the beginning on end), false otherwise.
    bool GetIsLoopPlayback() const;

    /// @brief Establishes if the animation of this animated model is in loop.
    /// @param Value True if the animation of this animated model will loop (i.e. restarts from the beginning on end), false otherwise.
    void SetIsLoopPlayback(bool Value);

    /// @brief Checks if the animation of this animated model is playing.
    /// @return True if the animation of this animated is playing, false otherwise.
    bool GetIsPlaying() const;

    /// @brief Establishes if the animation of this animated model is playing.
    /// @param Value True if the animation of this animated model is playing, false otherwise.
    void SetIsPlaying(bool Value);

    /// @brief Gets the index of the currently active animation.
    /// @return The index of the currently active animation.
    int64_t GetAnimationIndex() const;

    /// @brief Sets the index of the currently active animation.
    /// @return The index of the currently active animation.
    void SetAnimationIndex(int64_t Value);

    /// @brief Gets the material overrides of this component.
    /// Should be in the format:
    /// Key = Path to the model
    /// Value = The material id
    /// @return The material overrides on this component.
    csp::common::Map<csp::common::String, csp::common::String> GetMaterialOverrides() const;

    /// @brief Adds a new material override to this component.
    /// @param The path to the models material to override.
    /// @param The id of the material to override with.
    void AddMaterialOverride(const csp::common::String& ModelPath, const csp::common::String& MaterialAssetId);

    /// @brief Removes a material override from this component.
    /// @param The path to the models material to override to be removed.
    void RemoveMaterialOverride(const csp::common::String& ModelPath);

    /// \addtogroup IVisibleComponent
    /// @{
    /// @copydoc IVisibleComponent::GetIsVisible()
    bool GetIsVisible() const override;
    /// @copydoc IVisibleComponent::SetIsVisible()
    void SetIsVisible(bool InValue) override;
    /// @copydoc IVisibleComponent::GetIsARVisible()
    bool GetIsARVisible() const override;
    /// @copydoc IVisibleComponent::SetIsARVisible()
    void SetIsARVisible(bool InValue) override;
    /// @}

    /// \addtogroup IThirdPartyComponentRef
    /// @{
    /// @copydoc IThirdPartyComponentRef::GetThirdPartyComponentRef()
    const csp::common::String& GetThirdPartyComponentRef() const override;
    /// @copydoc IThirdPartyComponentRef::SetThirdPartyComponentRef()
    void SetThirdPartyComponentRef(const csp::common::String& InValue) override;
    /// @}

    /// \addtogroup IShadowCasterComponent
    /// @{
    /// @copydoc IShadowCasterComponent::GetIsShadowCaster()
    bool GetIsShadowCaster() const override;
    /// @copydoc IShadowCasterComponent::SetIsShadowCaster()
    void SetIsShadowCaster(bool Value) override;
    /// @}
};

} // namespace csp::multiplayer
