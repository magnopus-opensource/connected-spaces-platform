#pragma once

#include "Olympus/Multiplayer/ComponentBase.h"
#include "Olympus/Multiplayer/Components/Interfaces/IEnableableComponent.h"

namespace oly_multiplayer
{

enum class AudioPlaybackState
{
    Reset = 0,
    Pause,
    Play,
    Num
};

enum class AudioType
{
    Global = 0,
    Spatial,
    Num
};

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
    Num
};

// @ingroup AudioSpaceComponent
/// @brief Data representation of an AudioSpaceComponent.
class OLY_API AudioSpaceComponent : public ComponentBase, public IEnableableComponent
{
public:
    AudioSpaceComponent(SpaceEntity* Parent);

    const oly_common::Vector3& GetPosition() const;
    void SetPosition(const oly_common::Vector3& Value);
    AudioPlaybackState GetPlaybackState() const;
    void SetPlaybackState(AudioPlaybackState Value);
    AudioType GetAudioType() const;
    void SetAudioType(AudioType Value);
    const oly_common::String& GetAudioAssetId() const;
    void SetAudioAssetId(const oly_common::String& Value);
    const oly_common::String& GetAssetCollectionId() const;
    void SetAssetCollectionId(const oly_common::String& Value);
    float GetAttenuationRadius() const;
    void SetAttenuationRadius(float Value);
    bool GetIsLoopPlayback() const;
    void SetIsLoopPlayback(bool Value);
    float GetTimeSincePlay() const;
    void SetTimeSincePlay(float Value);
    float GetVolume() const;
    void SetVolume(float Value);

    // Inherited via IEnableableComponent
    virtual bool GetIsEnabled() const override;
    virtual void SetIsEnabled(bool InValue) override;
};

} // namespace oly_multiplayer
