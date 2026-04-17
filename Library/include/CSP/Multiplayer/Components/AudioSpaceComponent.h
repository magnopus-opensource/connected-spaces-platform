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

/// @file AudioSpaceComponent.h
/// @brief Definitions and support for audio components.

#pragma once

#include "CSP/Multiplayer/ComponentBase.h"
#include "CSP/Multiplayer/Components/Interfaces/IAudioControlComponent.h"
#include "CSP/Multiplayer/Components/Interfaces/IEnableableComponent.h"
#include "CSP/Multiplayer/Components/Interfaces/IPositionComponent.h"
#include "CSP/Multiplayer/Components/Interfaces/IThirdPartyComponentRef.h"

namespace csp::multiplayer
{

/// @brief Enumerates the playback states for an audio clip.
enum class AudioPlaybackState
{
    Reset = 0,
    Pause,
    Play,
    Num
};

/// @brief Enumerates the list of properties that can be replicated for an audio component.
enum class AudioPropertyKeys : uint16_t
{
    Position = 0,
    PlaybackState,
    AudioType,
    AudioAssetId,
    AssetCollectionId,
    AttenuationRadius,
    IsLoopPlayback,
    TimeSincePlay,
    Volume,
    IsEnabled,
    ThirdPartyComponentRef,
    Num
};

// @ingroup AudioSpaceComponent
/// @brief Adds spatial audio to a SpaceEntity.
///
/// This component creates immersive soundscapes by playing audio that reacts to the user's position in the space.
/// Whether it's background music, sound effects, or voiceovers, the AudioSpaceComponent makes sound more engaging by positioning it in 3D space.
class CSP_API AudioSpaceComponent : public ComponentBase, public IAudioControlComponent, public IEnableableComponent, public IPositionComponent, public IThirdPartyComponentRef
{
public:
    /// @brief Constructs the audio space component, and associates it with the specified Parent space entity.
    /// @param Parent The Space entity that owns this component.
    AudioSpaceComponent(csp::common::LogSystem* LogSystem, SpaceEntity* Parent);

    /// \addtogroup IPositionComponent
    /// @{
    /// @copydoc IPositionComponent::GetPosition()
    const csp::common::Vector3& GetPosition() const override;
    /// @copydoc IPositionComponent::SetPosition()
    void SetPosition(const csp::common::Vector3& InValue) override;
    /// @}

    /// @brief Gets the current playback state of the audio of this audio component.
    /// @return The current playback state of the audio of this audio component.
    AudioPlaybackState GetPlaybackState() const;

    /// @brief Sets the new playback state of the audio of this audio component.
    /// @param Value The new playback state of the audio of this audio component.
    void SetPlaybackState(AudioPlaybackState Value);

    /// @brief Gets the asset ID for this audio asset.
    /// @return The ID of this audio asset.
    const csp::common::String& GetAudioAssetId() const;

    /// @brief Sets the asset ID for this audio asset.
    /// @param Value The ID for this audio asset.
    void SetAudioAssetId(const csp::common::String& Value);

    /// @brief Gets the ID of the asset collection associated with this component.
    /// @note To retrieve this component's audio asset, both the Asset ID and the Asset Collection ID are required.
    /// @return The ID of the asset collection associated with this component.
    const csp::common::String& GetAssetCollectionId() const;

    /// @brief Sets the ID of the asset collection associated with this component.
    /// @note To retrieve this component's audio asset, both the Asset ID and the Asset Collection ID are required.
    /// @param Value The ID of the asset collection associated with this component.
    void SetAssetCollectionId(const csp::common::String& Value);

    /// @brief Checks if the audio playback is looping.
    /// @return True if the audio loops (i.e. starts from the beginning when ended), false otherwise.
    bool GetIsLoopPlayback() const;

    /// @brief Sets if the audio playback is looping.
    /// @param Value True if the audio loops (i.e. starts from the beginning when ended), false otherwise.
    void SetIsLoopPlayback(bool Value);

    /// @brief Gets the timestamp recorded from the moment when the audio clip started playing, in Unix timestamp format.
    /// @return The timestamp recorded from the moment when the audio clip started playing, in Unix timestamp format.
    float GetTimeSincePlay() const;

    /// @brief Sets the timestamp recorded from the moment when the audio clip started playing, in Unix timestamp format.
    /// @param Value The timestamp recorded from the moment when the audio clip started playing, in Unix timestamp format.
    void SetTimeSincePlay(float Value);

    /// \addtogroup IAudioControlComponent
    /// @{
    /// @copydoc IAudioControlComponent::GetAudioType()
    AudioType GetAudioType() const override;
    /// @copydoc IAudioControlComponent::SetAudioType()
    void SetAudioType(AudioType Value) override;
    /// @copydoc IAudioControlComponent::GetAttenuationRadius()
    float GetAttenuationRadius() const override;
    /// @copydoc IAudioControlComponent::SetAttenuationRadius()
    void SetAttenuationRadius(float Value) override;
    /// @copydoc IAudioControlComponent::GetVolume()
    float GetVolume() const override;
    /// @copydoc IAudioControlComponent::SetVolume()
    void SetVolume(float Value) override;
    /// @}
 
    /// \addtogroup IEnableableComponent
    /// @{
    /// @copydoc IEnableableComponent::GetIsEnabled()
    bool GetIsEnabled() const override;
    /// @copydoc IEnableableComponent::SetIsEnabled()
    void SetIsEnabled(bool InValue) override;
    /// @}

    /// \addtogroup IThirdPartyComponentRef
    /// @{
    /// @copydoc IThirdPartyComponentRef::GetThirdPartyComponentRef()
    const csp::common::String& GetThirdPartyComponentRef() const override;
    /// @copydoc IThirdPartyComponentRef::SetThirdPartyComponentRef()
    void SetThirdPartyComponentRef(const csp::common::String& InValue) override;
    /// @}
};

} // namespace csp::multiplayer
