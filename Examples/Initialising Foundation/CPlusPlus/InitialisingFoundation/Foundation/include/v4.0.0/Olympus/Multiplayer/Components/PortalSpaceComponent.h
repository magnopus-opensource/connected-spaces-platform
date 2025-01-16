#pragma once

#include "Olympus/Multiplayer/ComponentBase.h"
#include "Olympus/Multiplayer/Components/Interfaces/IEnableableComponent.h"
#include "Olympus/Systems/Assets/Asset.h"

namespace oly_multiplayer
{

// IsVisible, IsARVisible and IsActive are no longer exposed but retained for backwards compatibility
enum class PortalPropertyKeys
{
    SpaceId = 0,
    IsVisible,
    IsActive,
    IsARVisible,
    IsEnabled,
    Position,
    Radius,
    Num
};

/**
 * @ingroup PortalSpaceComponent
 * @brief Data representation of a PortalSpaceComponent.
 *
 * To ensure the connection to the new space is successful, clients should use the following steps:
 *
 * 1. Store the new space Id by calling PortalSpaceComponent::GetSpaceId()
 * 2. Disconnect by calling MultiplayerConnection::Disconnect()
 * 3. Create a new MultiplayerConnection instance using the space Id from step 1
 * 4. Follow the standard procedure to re-connect to a space
 */

class OLY_API PortalSpaceComponent : public ComponentBase, public IEnableableComponent
{
public:
    PortalSpaceComponent(SpaceEntity* Parent);

    const oly_common::String& GetSpaceId() const;
    void SetSpaceId(const oly_common::String& Value);

    const oly_common::Vector3& GetPosition() const;
    void SetPosition(const oly_common::Vector3& Value);

    float GetRadius() const;
    void SetRadius(float Value);

    /* IEnableableComponent */
    bool GetIsEnabled() const override;
    void SetIsEnabled(bool InValue) override;

    OLY_ASYNC_RESULT void GetSpaceThumbnail(oly_systems::UriResultCallback Callback) const;
};

} // namespace oly_multiplayer
