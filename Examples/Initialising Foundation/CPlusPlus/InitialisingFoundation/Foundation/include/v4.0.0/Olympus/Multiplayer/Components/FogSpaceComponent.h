#pragma once

#include "Olympus/Multiplayer/ComponentBase.h"
#include "Olympus/Multiplayer/Components/Interfaces/IVisibleComponent.h"
#include "Olympus/OlympusCommon.h"

namespace oly_multiplayer
{

enum class FogPropertyKeys
{
    FogMode = 0,
    Position,
    Rotation,
    Scale,
    StartDistance,
    EndDistance,
    Color,
    Density,
    HeightFalloff,
    MaxOpacity,
    IsVolumetric,
    IsVisible,
    IsARVisible,
    Num
};

enum class FogMode
{
    Linear = 0,
    Exponential,
    Exponential2
};

/// @ingroup FogSpaceComponent
/// @brief Data representation of an FogSpaceComponent.
class OLY_API FogSpaceComponent : public ComponentBase, public IVisibleComponent
{
public:
    FogSpaceComponent(SpaceEntity* Parent);

    FogMode GetFogMode() const;
    void SetFogMode(FogMode Value);

    const oly_common::Vector3& GetPosition() const;
    void SetPosition(const oly_common::Vector3& Value);
    const oly_common::Vector4& GetRotation() const;
    void SetRotation(const oly_common::Vector4& Value);
    const oly_common::Vector3& GetScale() const;
    void SetScale(const oly_common::Vector3& Value);

    /// @brief Get start sistance
    /// Note: Distance from camera that the fog will start.
    /// Note: 0 = this property has no effect.
    /// @return Current start distance
    float GetStartDistance() const;

    /// @brief Set Current start distance
    /// Note: Distance from camera that the fog will start.
    /// Note: 0 = this property has no effect.
    /// @param Value float : Current start distance
    void SetStartDistance(float Value);

    /// @brief Get Current end distance
    /// Note: objects passed this distance will not be affected by fog.
    /// Note: 0 = this property has no effect.
    /// @return Current end distance
    float GetEndDistance() const;

    /// @brief Set Current end distance
    /// Note: objects passed this distance will not be affected by fog.
    /// Note: 0 = this property has no effect.
    /// @param Value float : Current end distance
    void SetEndDistance(float Value);

    /// @brief Get fog color
    /// @return Current fog color
    const oly_common::Vector3& GetColor() const;

    /// @brief Set Current fog color
    /// @param Value float : Current fog color
    void SetColor(const oly_common::Vector3& Value);

    /// @brief Get Global density factor
    /// Note: Global density factor
    /// @return Current Global density factor
    float GetDensity() const;

    /// @brief Set Global density factor
    /// Note: Global density factor
    /// @param Value float : Global density factor
    void SetDensity(float Value);

    /// @brief Get Height density factor
    /// Note: Height density factor, controls how the density increases and height decreases. Smaller values make the visible transition larger.
    /// @return Current Height density factor
    float GetHeightFalloff() const;

    /// @brief Set Height density factor
    /// Note: Height density factor, controls how the density increases and height decreases. Smaller values make the visible transition larger.
    /// @param Value float : Height density factor
    void SetHeightFalloff(float Value);

    /// @brief Get Maximum opacity of the Fog.
    /// Maximum opacity of the Fog.
    /// Note: 1 = fog becomes fully opaque at a distance and replaces the scene colour completely.
    /// Note: 0 = fog colour will have no impact.
    /// @return Current Maximum opacity of the Fog
    float GetMaxOpacity() const;

    /// @brief Set Maximum opacity of the Fog.
    /// @param Value float : Maximum opacity of the Fog.
    void SetMaxOpacity(float Value);

    /// @brief Get Is Fog Volumetric.
    /// @param Value float : Fog Volumetric Flag
    bool GetIsVolumetric() const;

    /// @brief Set Is Fog Volumetric
    /// @param Value float : Is Fog Volumetric Flag
    void SetIsVolumetric(bool Value);

    /* IVisibleComponent */

    bool GetIsVisible() const override;
    void SetIsVisible(bool InValue) override;

    bool GetIsARVisible() const override;
    void SetIsARVisible(bool InValue) override;
};

} // namespace oly_multiplayer
