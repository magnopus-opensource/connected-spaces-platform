#pragma once

#include "Olympus/Common/String.h"
#include "Olympus/Multiplayer/ComponentBase.h"
#include "Olympus/Multiplayer/Components/Interfaces/IEnableableComponent.h"
#include "Olympus/Multiplayer/Components/Interfaces/IVisibleComponent.h"
#include "Olympus/Multiplayer/SpaceTransform.h"
#include "Olympus/OlympusCommon.h"

namespace oly_multiplayer
{

enum class ButtonPropertyKeys
{
    Name = 0,
    LabelText,
    IconAssetId,
    AssetCollectionId,
    Position,
    Rotation,
    Scale,
    IsVisible,
    IsEnabled,
    IsARVisible,
    Num
};

/// @ingroup ButtonSpaceComponent
/// @brief Data representation of an ButtonSpaceComponent.
class OLY_API ButtonSpaceComponent : public ComponentBase, public IEnableableComponent, public IVisibleComponent
{
public:
    ButtonSpaceComponent(SpaceEntity* Parent);

    const oly_common::String& GetLabelText() const;
    void SetLabelText(const oly_common::String& Value);
    const oly_common::String& GetIconAssetId() const;
    void SetIconAssetId(const oly_common::String& Value);
    const oly_common::String& GetAssetCollectionId() const;
    void SetAssetCollectionId(const oly_common::String& Value);
    const oly_common::Vector3& GetPosition() const;
    void SetPosition(const oly_common::Vector3& Value);
    const oly_common::Vector4& GetRotation() const;
    void SetRotation(const oly_common::Vector4& Value);
    const oly_common::Vector3& GetScale() const;
    void SetScale(const oly_common::Vector3& Value);

    /* IClickableComponent */

    bool GetIsEnabled() const override;
    void SetIsEnabled(bool InValue) override;

    /* IVisibleComponent */

    bool GetIsVisible() const override;
    void SetIsVisible(bool InValue) override;

    bool GetIsARVisible() const override;
    void SetIsARVisible(bool InValue) override;
};

} // namespace oly_multiplayer
