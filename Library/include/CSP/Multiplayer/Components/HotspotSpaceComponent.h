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

/// @file HotspotSpaceComponent.h
/// @brief Definitions and support for Hotspot components.

#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/ComponentBase.h"
#include "CSP/Multiplayer/Components/Interfaces/IPositionComponent.h"
#include "CSP/Multiplayer/Components/Interfaces/IRotationComponent.h"
#include "CSP/Multiplayer/Components/Interfaces/IVisibleComponent.h"
#include "CSP/Systems/SystemsResult.h"

namespace csp::multiplayer
{

/// @brief Enumerates the list of properties that can be replicated for an Hotspot space component.
enum class HotspotPropertyKeys
{
    Position,
    Rotation,
    Name_DEPRECATED,
    IsTeleportPoint,
    IsSpawnPoint,
    IsVisible,
    IsARVisible,
    Num
};

/// @ingroup HotspotSpaceComponent
/// @brief Data representation of an HotspotSpaceComponent.
class CSP_API HotspotSpaceComponent : public ComponentBase, public IPositionComponent, public IRotationComponent, public IVisibleComponent
{
public:
    /// @brief Constructs the Hotspot space component, and associates it with the specified Parent space entity.
    /// @param Parent The Space entity that owns this component.
    HotspotSpaceComponent(SpaceEntity* Parent);

    /// @brief Gets the Name of the Hotspot.
    /// @return The Name of the Hotspot.
    [[deprecated("Deprecated in favour of ComponentBase::GetComponentName()")]] const csp::common::String& GetName() const;

    /// @brief Sets the Name of this Hotspot.
    /// @param Value The Name of this Hotspot.
    [[deprecated("Deprecated in favour of ComponentBase::SetComponentName()")]] void SetName(const csp::common::String& Value);

    /// @brief Gets the IsTeleportPoint of this Hotspot.
    bool GetIsTeleportPoint() const;

    /// @brief Sets this Hotspot to be a teleport point.
    /// @param InValue The teleport point state flag value.
    void SetIsTeleportPoint(bool InValue);

    /// @brief Gets the IsSpawnPoint of this Hotspot.
    bool GetIsSpawnPoint() const;

    /// @brief Sets this Hotspot to be a spawn point.
    /// @param InValue The spawn point state flag value.
    void SetIsSpawnPoint(bool InValue);

    /// @brief Gets a unique identifier for this component in the hierarchy.
    /// @note This does not give a complete hierarchy path, only the entityId of the parent for the component.
    /// @return A string composed of 'parentId:componentId'.
    csp::common::String GetUniqueComponentId() const;

    /// \addtogroup IPositionComponent
    /// @{
    /// @copydoc IPositionComponent::GetPosition()
    const csp::common::Vector3& GetPosition() const override;
    /// @copydoc IPositionComponent::SetPosition()
    void SetPosition(const csp::common::Vector3& InValue) override;
    /// @}

    /// \addtogroup IRotationComponent
    /// @{
    /// @copydoc IRotationComponent::GetRotation()
    const csp::common::Vector4& GetRotation() const override;
    /// @copydoc IRotationComponent::SetRotation()
    void SetRotation(const csp::common::Vector4& InValue) override;
    /// @}

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

private:
    void OnLocalDelete() override;
};

} // namespace csp::multiplayer
