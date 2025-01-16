#pragma once
#include "Olympus/Common/Array.h"
#include "Olympus/Common/String.h"
#include "Olympus/Multiplayer/ComponentBase.h"
#include "Olympus/Multiplayer/Components/Interfaces/IVisibleComponent.h"
#include "Olympus/OlympusCommon.h"

namespace oly_multiplayer
{

enum class VideoPlayerPlaybackState
{
    Reset = 0,
    Pause,
    Play,
    Num
};

enum class VideoPlayerActions
{
    VideoBegin,
    VideoEnd,
    Num
};

enum class VideoPlayerSourceType
{
    URLSource = 0,
    AssetSource,
    WowzaStreamSource,
    Num
};

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
    Num
};

/// @ingroup VideoPlayerSpaceComponent
/// @brief Data representation of an VideoPlayerSpaceComponent.
class OLY_API VideoPlayerSpaceComponent : public ComponentBase, public IVisibleComponent
{
public:
    VideoPlayerSpaceComponent(SpaceEntity* Parent);

    const oly_common::String& GetName() const;
    void SetName(const oly_common::String& Value);
    const oly_common::String& GetVideoAssetId() const;
    void SetVideoAssetId(const oly_common::String& Value);
    const oly_common::String& GetVideoAssetURL() const;
    void SetVideoAssetURL(const oly_common::String& Value);
    const oly_common::String& GetAssetCollectionId() const;
    void SetAssetCollectionId(const oly_common::String& Value);
    const oly_common::Vector3& GetPosition() const;
    void SetPosition(const oly_common::Vector3& Value);
    const oly_common::Vector4& GetRotation() const;
    void SetRotation(const oly_common::Vector4& Value);
    const oly_common::Vector3& GetScale() const;
    void SetScale(const oly_common::Vector3& Value);
    bool GetIsStateShared() const;
    void SetIsStateShared(bool Value);
    bool GetIsAutoPlay() const;
    void SetIsAutoPlay(bool Value);
    bool GetIsLoopPlayback() const;
    void SetIsLoopPlayback(bool Value);
    bool GetIsAutoResize() const;
    void SetIsAutoResize(bool Value);
    float GetAttenuationRadius() const;
    void SetAttenuationRadius(float Value);
    VideoPlayerPlaybackState GetPlaybackState() const;
    void SetPlaybackState(VideoPlayerPlaybackState Value);
    float GetCurrentPlayheadPosition() const;
    void SetCurrentPlayheadPosition(float Value);
    float GetTimeSincePlay() const;
    void SetTimeSincePlay(float Value);
    VideoPlayerSourceType GetVideoPlayerSourceType() const;
    void SetVideoPlayerSourceType(VideoPlayerSourceType);

    /*
     * @brief Gets the Id of the mesh component that the video should be rendered to
     */
    uint16_t GetMeshComponentId() const;

    /*
     * @brief Sets the Id of the mesh component that the video should be rendered to
     */
    void SetMeshComponentId(uint16_t Id);

    /* IVisibleComponent */

    bool GetIsVisible() const override;
    void SetIsVisible(bool InValue) override;

    bool GetIsARVisible() const override;
    void SetIsARVisible(bool InValue) override;
};

} // namespace oly_multiplayer
