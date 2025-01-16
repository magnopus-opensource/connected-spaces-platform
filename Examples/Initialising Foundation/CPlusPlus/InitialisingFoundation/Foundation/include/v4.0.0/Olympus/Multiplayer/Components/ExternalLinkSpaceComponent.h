#pragma once
#include "Olympus/Common/String.h"
#include "Olympus/Multiplayer/ComponentBase.h"
#include "Olympus/Multiplayer/Components/Interfaces/IEnableableComponent.h"
#include "Olympus/Multiplayer/Components/Interfaces/IVisibleComponent.h"
#include "Olympus/Multiplayer/SpaceTransform.h"
#include "Olympus/OlympusCommon.h"

namespace oly_multiplayer
{

/// @brief The type of actions that can be performed by an external link component.
enum class ExternalLinkActions
{
    LinkClicked,
    Num
};

/// @brief The list of properties defined within an external link component.
enum class ExternalLinkPropertyKeys
{
    Name = 0,
    LinkUrl,
    Position,
    Rotation,
    Scale,
    DisplayText,
    IsEnabled,
    IsVisible,
    IsARVisible,
    Num
};

/// @ingroup ExternalLinkSpaceComponent
/// @brief Data representation of an ExternalLinkSpaceComponent.
class OLY_API ExternalLinkSpaceComponent : public ComponentBase, public IEnableableComponent, public IVisibleComponent
{
public:
    /// @brief Creates an external link component that can be added to an existing space entity.
    /// @param Parent - The space entity to which this new component will belong to.
    ExternalLinkSpaceComponent(SpaceEntity* Parent);

    /// @brief Gets the name of this external link component.
    const oly_common::String& GetName() const;
    /// @brief Sets the name for this external link component.
    /// @param Value - The new name to assign to this external link componenent.
    void SetName(const oly_common::String& Value);

    /// @brief Gets the URL address to which this external link component redirects the user on trigger.
    const oly_common::String& GetLinkUrl() const;
    /// @brief Sets the URL address to which this external link component redirects the user to on trigger.
    /// @param Value - The URL to be assigned to this external link component.
    void SetLinkUrl(const oly_common::String& Value);

    /// @brief Gets the 3D position in world coordinates where the origin of this component is located.
    const oly_common::Vector3& GetPosition() const;
    /// @brief Sets the 3D position in world coordinates where the origin of this component will be located.
    /// @param Value - The new 3D position assigned to the origin of this component.
    void SetPosition(const oly_common::Vector3& Value);

    /// @brief Gets the quaternion of the rotation of the origin of this component.
    const oly_common::Vector4& GetRotation() const;
    /// @brief Sets the quaternion of the rotation of the origin of this component.
    /// @param Value - The new rotation assigned to the origin of this component.
    void SetRotation(const oly_common::Vector4& Value);

    /// @brief Gets the 3D scale of this component.
    const oly_common::Vector3& GetScale() const;
    /// @brief Sets the 3D scale of this component.
    /// @param Value - The new 3D scale assigned to this component.
    void SetScale(const oly_common::Vector3& Value);

    /// @brief Gets the text that will be displayed by the component as hyperlink to the URL it redirects to.
    const oly_common::String& GetDisplayText() const;
    /// @brief Sets the text that will be displayed by the component as hyperlink to the URL it redirects to.
    /// @param Value - The new text to be displayed as hyperlink.
    void SetDisplayText(const oly_common::String& Value);

    /* IEnableableComponent */

    bool GetIsEnabled() const override;
    void SetIsEnabled(bool InValue) override;

    /* IVisibleComponent */

    virtual bool GetIsVisible() const;
    virtual void SetIsVisible(bool InValue);

    virtual bool GetIsARVisible() const;
    virtual void SetIsARVisible(bool InValue);
};

} // namespace oly_multiplayer
