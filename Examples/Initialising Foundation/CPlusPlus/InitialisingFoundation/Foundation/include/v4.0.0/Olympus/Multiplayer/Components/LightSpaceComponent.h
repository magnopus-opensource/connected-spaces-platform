#pragma once
#include "Olympus/Common/String.h"
#include "Olympus/Multiplayer/ComponentBase.h"
#include "Olympus/Multiplayer/Components/Interfaces/IVisibleComponent.h"
#include "Olympus/Multiplayer/SpaceTransform.h"
#include "Olympus/OlympusCommon.h"

namespace oly_multiplayer
{

enum class LightType
{
    Directional = 0,
    Point,
    Spot,
    Num
};

enum class LightCookieType
{
    ImageCookie = 0,
    VideoCookie,
    NoCookie
};

enum class LightPropertyKeys
{
    Name = 0,
    LightType,
    Color,
    Intensity,
    Range,
    InnerConeAngle,
    OuterConeAngle,
    Position,
    Rotation,
    IsVisible,
    LightCookieAssetId,
    LightCookieAssetCollectionId,
    LightCookieType,
    IsARVisible,
    Num
};

/// @ingroup LightSpaceComponent
/// @brief Data representation of an LightSpaceComponent.
class OLY_API LightSpaceComponent : public ComponentBase, public IVisibleComponent
{
public:
    LightSpaceComponent(SpaceEntity* Parent);

    LightType GetLightType() const;
    void SetLightType(LightType Value);
    const oly_common::Vector3& GetColor() const;
    void SetColor(const oly_common::Vector3& Value);
    float GetIntensity() const;
    void SetIntensity(float Value);
    float GetRange() const;
    void SetRange(float Value);
    float GetInnerConeAngle() const;
    void SetInnerConeAngle(float Value);
    float GetOuterConeAngle() const;
    void SetOuterConeAngle(float Value);
    const oly_common::Vector3& GetPosition() const;
    void SetPosition(const oly_common::Vector3& Value);
    const oly_common::Vector4& GetRotation() const;
    void SetRotation(const oly_common::Vector4& Value);
    const oly_common::String& GetLightCookieAssetId() const;
    void SetLightCookieAssetId(const oly_common::String& Value);
    const oly_common::String& GetLightCookieAssetCollectionId() const;
    void SetLightCookieAssetCollectionId(const oly_common::String& Value);
    LightCookieType GetLightCookieType() const;
    void SetLightCookieType(LightCookieType Value);
    /* IVisibleComponent */

    bool GetIsVisible() const override;
    void SetIsVisible(bool InValue) override;

    bool GetIsARVisible() const override;
    void SetIsARVisible(bool InValue) override;
};

} // namespace oly_multiplayer
