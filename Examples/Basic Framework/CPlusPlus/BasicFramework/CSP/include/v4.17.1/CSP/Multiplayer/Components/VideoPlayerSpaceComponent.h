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
/// @file VideoPlayerSpaceComponent.h
/// @brief Definitions and support for video player components.

#pragma once
#include "CSP/CSPCommon.h"
#include "CSP/Common/Array.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/ComponentBase.h"
#include "CSP/Multiplayer/Components/Interfaces/IEnableableComponent.h"
#include "CSP/Multiplayer/Components/Interfaces/IVisibleComponent.h"

namespace csp::multiplayer
{

/// @brief Enumerates the possible playback states for a video player.
enum class VideoPlayerPlaybackState
{
    Reset = 0,
    Pause,
    Play,
    Num
};

/// @brief Enumerates the actions that can be performed on a video player.
enum class VideoPlayerActions
{
    VideoBegin,
    VideoEnd,
    Num
};

/// @brief Enumerates the type of video sources the video player supports.
enum class VideoPlayerSourceType
{
    /// A video hosted online on a specific URL address.
    URLSource = 0,
    /// A video asset file that can be loaded and played at runtime.
    AssetSource,
    WowzaStreamSource,
    Num
};

/// @brief Enumerates the list of properties that can be replicated for a video player component.
enum class VideoPlayerPropertyKeys
{
    Name = 0,
    VideoAssetId,
    VideoAssetURL,
    AssetCollectionId,
    Position,
    Rotation,
    Scale,
    IsStateShared,
    IsAutoPlay,
    IsLoopPlayback,
    IsAutoResize,
    AttenuationRadius,
    PlaybackState,
    CurrentPlayheadPosition,
    TimeSincePlay,
    VideoPlayerSourceType,
    IsVisible,
    IsARVisible,
    MeshComponentId,
    IsEnabled,
    Num
};

/// @ingroup VideoPlayerSpaceComponent
/// @brief Data representation of an VideoPlayerSpaceComponent.
class CSP_API VideoPlayerSpaceComponent : public ComponentBase, public IVisibleComponent, public IEnableableComponent
{
public:
    /// @brief Constructs the video player component, and associates it with the specified Parent space entity.
    /// @param Parent The Space entity that owns this component.
    VideoPlayerSpaceComponent(SpaceEntity* Parent);

    /// @brief Gets the name of this video player.
    /// @return The name of this video player.
    const csp::common::String& GetName() const;

    /// @brief Sets the name of this video player.
    /// @param Value The name of this video player.
    void SetName(const csp::common::String& Value);

    /// @brief Gets the ID of the video asset associated with this video player.
    /// @return The ID of the video asset assicuated with this video player.
    const csp::common::String& GetVideoAssetId() const;

    /// @brief Sets the ID of the video asset associated with this video player.
    /// @param Value The ID of the video asset assicuated with this video player.
    void SetVideoAssetId(const csp::common::String& Value);

    /// @brief Gets the URL of the video asset associated with this video player.
    /// @return The URL of the video asset associated with this video player.
    const csp::common::String& GetVideoAssetURL() const;

    /// @brief Sets the URL of the video asset associated with this video player.
    /// @param Value The URL of the video asset associated with this video player.
    void SetVideoAssetURL(const csp::common::String& Value);

    /// @brief Gets the ID of the asset collection associated with this component.
    /// @note To retrieve this component's video asset, both the Asset ID and the Asset Collection ID are required.
    /// @return The ID of the asset collection associated with this component.
    const csp::common::String& GetAssetCollectionId() const;

    /// @brief Sets the ID of the asset collection associated with this component.
    /// @note To retrieve this component's video asset, both the Asset ID and the Asset Collection ID are required.
    /// @param Value The ID of the asset collection associated with this component.
    void SetAssetCollectionId(const csp::common::String& Value);

    /// @brief Gets the position of the origin of this component in world space.
    /// @note The coordinate system used follows the glTF 2.0 specification, in meters.
    ///       - Right handed coordinate system
    ///       - +Y is UP
    ///       - +X is left (facing forward)
    ///       - +Z is forward
    /// @return The 3D position as vector (left, up, forward) in meters.
    const csp::common::Vector3& GetPosition() const;

    /// @brief Sets the position of the origin of this component in world space.
    /// @note The coordinate system used follows the glTF 2.0 specification, in meters.
    ///       - Right handed coordinate system
    ///       - +Y is UP
    ///       - +X is left (facing forward)
    ///       - +Z is forward
    void SetPosition(const csp::common::Vector3& Value);

    /// @brief Gets a quaternion representing the rotation of the origin of this component, expressed in radians.
    /// @note The coordinate system respects the following conventions:
    ///       - Right handed coordinate system
    ///       - Positive rotation is counterclockwise
    ///       - The geographic North is along the positive Z axis (+Z) at an orientation of 0 degrees.
    ///       - North: +Z
    ///       - East: -X
    ///       - South: -Z
    ///       - West: +X
    const csp::common::Vector4& GetRotation() const;

    /// @brief Sets the rotation of the origin of this component according to the specified quaternion "Value", expressed in radians.
    /// @note The coordinate system respects the following conventions:
    ///       - Right handed coordinate system
    ///       - Positive rotation is counterclockwise
    ///       - The geographic North is along the positive Z axis (+Z) at an orientation of 0 degrees.
    ///       - North: +Z
    ///       - East: -X
    ///       - South: -Z
    ///       - West: +X
    /// @param Value The quaternion in radians to use as new rotation of this component.
    void SetRotation(const csp::common::Vector4& Value);

    /// @brief Gets the scale of the origin of this component in world space.
    /// @note The coordinate system used follows the glTF 2.0 specification.
    ///       - Right handed coordinate system
    ///       - +Y is UP
    ///       - +X is left (facing forward)
    ///       - +Z is forward
    /// @return The 3D scale as vector (left, up, forward).
    const csp::common::Vector3& GetScale() const;

    /// @brief Sets the scale of the origin of this component in world space to the specified "Value".
    /// @param Value The new value expressed as vector (left, up, forward).
    /// @note The coordinate system used follows the glTF 2.0 specification.
    ///       - Right handed coordinate system
    ///       - +Y is UP
    ///       - +X is left (facing forward)
    ///       - +Z is forward
    void SetScale(const csp::common::Vector3& Value);

    /// @brief Checks if the playback state of this video player needs to be shared with other users through replication.
    /// @return True if the playback state of the video needs to be shared among all users, false otherwise.
    bool GetIsStateShared() const;

    /// @brief Establishes if the playback state of this video player needs to be shared with other users through replication.
    /// @param Value True if the playback state of the video needs to be shared among all users, false otherwise.
    void SetIsStateShared(bool Value);

    /// @brief Checks if the video plays automatically on load.
    /// @return True if the video auto-plays on load, false otherwise.
    bool GetIsAutoPlay() const;

    /// @brief Establishes if the video plays automatically on load.
    /// @param Value True if the video auto-plays on load, false otherwise.
    void SetIsAutoPlay(bool Value);

    /// @brief Checks if the video loops (i.e. starts over on end).
    /// @return True if the video loops, false otherwise.
    bool GetIsLoopPlayback() const;

    /// @brief Establishes if the video loops (i.e. starts over on end).
    /// @param Value True if the video loops, false otherwise.
    void SetIsLoopPlayback(bool Value);

    /// @brief Checks if the video auto-resizes if its frame has different dimensions.
    /// @return True if the video will resize automatically, false otherwise.
    bool GetIsAutoResize() const;

    /// @brief Establishes if the video auto-resizes if its frame has different dimensions.
    /// @param Value True if the video will resize automatically, false otherwise.
    void SetIsAutoResize(bool Value);

    /// @brief Gets the radius from this component origin within which the audio of this video can be heard by the user.
    /// @note Only when the user position is within this radius the audio of the video should be heard.
    /// @return The radius within which the audio of the video can be heard by the user.
    float GetAttenuationRadius() const;

    /// @brief Sets the radius from this component origin within which the audio of this video can be heard by the user.
    /// @note Only when the user position is within this radius the audio of the video should be heard.
    /// @param Value The radius within which the audio of the video can be heard by the user.
    void SetAttenuationRadius(float Value);

    /// @brief Retrieves the playback state of the video of this component.
    /// @return The playback state of the video.
    VideoPlayerPlaybackState GetPlaybackState() const;

    /// @brief Sets the playback state of the video of this component.
    /// @param Value The playback state of the video.
    void SetPlaybackState(VideoPlayerPlaybackState Value);

    /// @brief Gets the current playhead position of the played video.
    /// @return The current playhead position of the played video.
    float GetCurrentPlayheadPosition() const;

    /// @brief Sets the current playhead position of the played video.
    /// @param Value The current playhead position of the played video.
    void SetCurrentPlayheadPosition(float Value);

    /// @brief Gets the time in Unix timestamp format that identifies the moment when the video started to play.
    /// @return The time in Unix timestamp format when the video started to play.
    float GetTimeSincePlay() const;

    /// @brief Sets the time in Unix timestamp format that identifies the moment when the video started to play.
    /// @param Value The time in Unix timestamp format when the video started to play.
    void SetTimeSincePlay(float Value);

    /// @brief Gets the type of source the video of this component uses.
    /// @return The type of video source used by this component.
    VideoPlayerSourceType GetVideoPlayerSourceType() const;

    /// @brief Sets the type of source the video of this component uses.
    /// @param Value The type of video source used by this component.
    void SetVideoPlayerSourceType(VideoPlayerSourceType Value);

    /// @brief Gets the Id of the mesh component that the video should be rendered to
    uint16_t GetMeshComponentId() const;

    /// @brief Sets the Id of the mesh component that the video should be rendered to
    void SetMeshComponentId(uint16_t Id);

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

    /// \addtogroup IEnableableComponent
    /// @{
    /// @copydoc IEnableableComponent::GetIsEnabled()
    bool GetIsEnabled() const override;
    /// @copydoc IEnableableComponent::SetIsEnabled()
    void SetIsEnabled(bool InValue) override;
    /// @}
};

} // namespace csp::multiplayer
