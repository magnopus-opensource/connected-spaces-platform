#pragma once
#include "Olympus/Common/String.h"
#include "Olympus/Multiplayer/ComponentBase.h"
#include "Olympus/Multiplayer/Components/Interfaces/IVisibleComponent.h"
#include "Olympus/OlympusCommon.h"

namespace oly_multiplayer
{
enum class ImagePropertyKeys
{
    Name,
    ImageAssetId,
    AssetCollectionId,
    Position,
    Rotation,
    Scale,
    IsVisible,
    BillboardMode,
    DisplayMode,
    IsARVisible,
    IsEmissive,
    Num
};

enum class BillboardMode
{
    Off = 0,
    Billboard,
    YawLockedBillboard
};

enum class DisplayMode
{
    SingleSided = 0,
    DoubleSided,
    DoubleSidedReversed
};

/// @ingroup ImageSpaceComponent
/// @brief Data representation of an ImageSpaceComponent.
class OLY_API ImageSpaceComponent : public ComponentBase, public IVisibleComponent
{
public:
    ImageSpaceComponent(SpaceEntity* Parent);

    const oly_common::String& GetName() const;
    void SetName(const oly_common::String& Value);
    const oly_common::String& GetImageAssetId() const;
    void SetImageAssetId(const oly_common::String& Value);
    const oly_common::String& GetAssetCollectionId() const;
    void SetAssetCollectionId(const oly_common::String& Value);
    const oly_common::Vector3& GetPosition() const;
    void SetPosition(const oly_common::Vector3& Value);
    const oly_common::Vector4& GetRotation() const;
    void SetRotation(const oly_common::Vector4& Value);
    const oly_common::Vector3& GetScale() const;
    void SetScale(const oly_common::Vector3& Value);
    BillboardMode GetBillboardMode() const;
    void SetBillboardMode(BillboardMode billboardMode);
    DisplayMode GetDisplayMode() const;
    void SetDisplayMode(DisplayMode displayMode);
    bool GetIsEmissive() const;
    void SetIsEmissive(bool InValue);

    /* IVisibleComponent */

    bool GetIsVisible() const override;
    void SetIsVisible(bool InValue) override;

    bool GetIsARVisible() const override;
    void SetIsARVisible(bool InValue) override;
};

} // namespace oly_multiplayer
