#pragma once
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

/// @file ECommerceSpaceComponent.h
/// @brief Definitions and support for ECommerce components.

#pragma once

#include "CSP/Multiplayer/ComponentBase.h"
#include "CSP/Multiplayer/Components/Interfaces/IPositionComponent.h"

namespace csp::multiplayer
{

/// @brief Enumerates the list of properties that can be replicated for an ECommerce component.
enum class ECommercePropertyKeys
{
    Position = 0,
    ProductId,
    Num
};

// @ingroup ECommerceSpaceComponent
/// @brief Can be used alongside CSP's Stripe integration to add e-commerce to your space.
///
/// This component is used to represent physical objects that can be purchased as virtual items in the environment.
class CSP_API ECommerceSpaceComponent : public ComponentBase, public IPositionComponent
{
public:
    /// @brief Constructs the ECommerce space component, and associates it with the specified Parent space entity.
    /// @param Parent The Space entity that owns this component.
    ECommerceSpaceComponent(SpaceEntity* Parent);

    /// \addtogroup ITransformComponent
    /// @{
    /// @copydoc IPositionComponent::GetPosition()
    const csp::common::Vector3& GetPosition() const override;
    /// @copydoc IPositionComponent::SetPosition()
    void SetPosition(const csp::common::Vector3& InValue) override;
    /// @}

    /// @brief Gets the product ID associated with the ECommerce component.
    /// @return The product ID associated with the ECommerce component.
    csp::common::String GetProductId() const;

    /// @brief Sets the new product ID for the ECommerce component.
    /// @param Value The product ID to set for the ECommerce component.
    void SetProductId(csp::common::String Value);
};

} // namespace csp::multiplayer
