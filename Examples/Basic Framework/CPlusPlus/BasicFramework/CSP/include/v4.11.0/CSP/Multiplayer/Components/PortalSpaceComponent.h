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
/// @file PortalSpaceComponent.h
/// @brief Definitions and support for portals.

#pragma once

#include "CSP/Multiplayer/ComponentBase.h"
#include "CSP/Multiplayer/Components/Interfaces/IEnableableComponent.h"
#include "CSP/Systems/Assets/Asset.h"

namespace csp::multiplayer
{

/// @brief Enumerates the list of properties that can be replicated for a portal component.
/// @note IsVisible, IsARVisible and IsActive are no longer exposed but retained for backwards compatibility.
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

class CSP_API PortalSpaceComponent : public ComponentBase, public IEnableableComponent
{
public:
	/// @brief Constructs the portal space component, and associates it with the specified Parent space entity.
	/// @param Parent The Space entity that owns this component.
	PortalSpaceComponent(SpaceEntity* Parent);

	/// @brief Retrieves the space ID that this portal points to.
	/// @note When the user uses the portal, it should be able to leave the current space and enter the one
	///       identified by this function.
	/// @return The ID of the space the portal component leads the player to.
	const csp::common::String& GetSpaceId() const;

	/// @brief Sets the space ID that this portal points to.
	/// @note When the user uses the portal, it should be able to leave the current space and enter the one
	///       identified by this function.
	/// @param Value The ID of the space the portal component leads the player to.
	void SetSpaceId(const csp::common::String& Value);

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

	/// @brief Gets the radius of this portal.
	/// @return The radius of this portal.
	float GetRadius() const;

	/// @brief Sets the radius of this portal.
	/// @param Value The radius of this portal.
	void SetRadius(float Value);

	/// \addtogroup IEnableableComponent
	/// @{
	/// @copydoc IEnableableComponent::GetIsEnabled()
	bool GetIsEnabled() const override;
	/// @copydoc IEnableableComponent::SetIsEnabled()
	void SetIsEnabled(bool InValue) override;
	/// @}

	/// @brief Retrieves the space thumbnail information associated with the space.
	/// If the space does not have a thumbnail associated with it the result callback will be successful, the
	/// HTTP res code will be ResponseNotFound and the Uri field inside the UriResult will be empty.
	/// @param Callback UriResultCallback : callback when asynchronous task finishes
	CSP_ASYNC_RESULT void GetSpaceThumbnail(csp::systems::UriResultCallback Callback) const;
};

} // namespace csp::multiplayer
