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

#pragma once

#include "CSP/CSPCommon.h"

namespace csp::multiplayer
{

/// @brief Specifies the type of audio source for an audio source component.
enum class AudioType
{
    /// A global audio type keeps the volume of the audio independent from the player position.
    Global = 0,
    /// A spatial audio takes the player position into account to attenuate or amplify the volume.
    Spatial,
    Num
};

/// @brief Controls the audio properties of a component.
CSP_INTERFACE class CSP_API IAudioControlComponent
{
public:
    /// @brief Gets the type of the audio of this audio component.
    /// @return The type of the audio of this audio component.
    virtual AudioType GetAudioType() const = 0;

    /// @brief Sets the type of the audio of this audio component.
    /// @param Value Type of the audio of this audio component.
    virtual void SetAudioType(AudioType Value) = 0;

    /// @brief Gets the attenuation for the audio when a spatial audio type.
    ///        The radius is the minimum distance between the origin of this audio component and
    ///        the position of the player, from within which the player can start hearing
    ///        the spatial audio in range.
    ///        The radius is expressed in meters.
    /// @return The minimum radius in meters from the origin of the audio component to hear the spatial audio.
    virtual float GetAttenuationRadius() const = 0;

    /// @brief Sets the attenuation for the audio when a spatial audio type.
    ///        The radius is the minimum distance between the origin of this audio component and
    ///        the position of the player, from within which the player can start hearing
    ///        the spatial audio in range.
    ///        The radius is expressed in meters.
    /// @param Value The minimum radius in meters from the origin of the audio component to hear the spatial audio.
    virtual void SetAttenuationRadius(float Value) = 0;

    /// @brief Gets the volume of the audio in a ratio between 0 and 1.
    ///        Volume 1 represents the full volume of the audio clip of this component.
    /// @return The volume of the audio, in a ratio between 0 and 1.
    virtual float GetVolume() const = 0;

    /// @brief Sets the volume of the audio in a ratio between 0 and 1.
    ///        Volume 1 represents the full volume of the audio clip of this component.
    /// @param Value The volume of the audio, in a ratio between 0 and 1.
    virtual void SetVolume(float Value) = 0;

protected:
    virtual ~IAudioControlComponent() = default;
};

} // namespace csp::multiplayer
