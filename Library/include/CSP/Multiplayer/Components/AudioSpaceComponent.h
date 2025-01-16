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

/// @brief Specifies the type of audio source for an audio component.
enum class AudioType
{
    /// A global audio type keeps the volume of the audio independent from the player position.
    Global = 0,
    /// A spatial audio takes the player position into account to attenuate or amplify the volume.
    Spatial,
    Num
};

/// @brief Enumerates the list of properties that can be replicated for an audio component.
enum class AudioPropertyKeys
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
class CSP_API AudioSpaceComponent : public ComponentBase, public IEnableableComponent, public IPositionComponent, public IThirdPartyComponentRef
{
public:
    /// @brief Constructs the audio space component, and associates it with the specified Parent space entity.
    /// @param Parent The Space entity that owns this component.
    AudioSpaceComponent(SpaceEntity* Parent);

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

    /// @brief Gets the type of the audio of this audio component.
    /// @return The type of the audio of this audio component.
    AudioType GetAudioType() const;

    /// @brief Sets the type of the audio of this audio component.
    /// @param Value Type of the audio of this audio component.
    void SetAudioType(AudioType Value);

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

    /// @brief Gets the attenuation for the audio when a spatial audio type.
    ///        The radius is the minimum distance between the origin of this audio component and
    ///        the position of the player, from within which the player can start hearing
    ///        the spatial audio in range.
    ///        The radius is expressed in meters.
    /// @return The minimum radius in meters from the origin of the audio component to hear the spatial audio.
    float GetAttenuationRadius() const;

    /// @brief Sets the attenuation for the audio when a spatial audio type.
    ///        The radius is the minimum distance between the origin of this audio component and
    ///        the position of the player, from within which the player can start hearing
    ///        the spatial audio in range.
    ///        The radius is expressed in meters.
    /// @param Value The minimum radius in meters from the origin of the audio component to hear the spatial audio.
    void SetAttenuationRadius(float Value);

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

    /// @brief Gets the volume of the audio in a ratio between 0 and 1.
    ///        Volume 1 represents the full volume of the audio clip of this component.
    /// @return The volume of the audio, in a ratio between 0 and 1.
    float GetVolume() const;

    /// @brief Sets the volume of the audio in a ratio between 0 and 1.
    ///        Volume 1 represents the full volume of the audio clip of this component.
    /// @param Value The volume of the audio, in a ratio between 0 and 1.
    void SetVolume(float Value);

    /// \addtogroup IEnableableComponent
    /// @{
    /// @copydoc IEnableableComponent::GetIsEnabled()
    virtual bool GetIsEnabled() const override;
    /// @copydoc IEnableableComponent::SetIsEnabled()
    virtual void SetIsEnabled(bool InValue) override;
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
