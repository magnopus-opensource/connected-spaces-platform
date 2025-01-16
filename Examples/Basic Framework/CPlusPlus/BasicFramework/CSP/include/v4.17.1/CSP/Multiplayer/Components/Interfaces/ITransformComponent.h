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
/// @file IVisibleComponent.h
/// @brief Visibility control for components.

#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Multiplayer/Components/Interfaces/IPositionComponent.h"
#include "CSP/Multiplayer/Components/Interfaces/IRotationComponent.h"
#include "CSP/Multiplayer/Components/Interfaces/IScaleComponent.h"
#include "CSP/Multiplayer/SpaceTransform.h"

namespace csp::multiplayer
{

/// @brief Controls the 3D position (in world space), rotation, and scale of the component.
CSP_INTERFACE class CSP_API ITransformComponent : public IPositionComponent, public IRotationComponent, public IScaleComponent
{
public:
    /// @brief Gets the transform of this component in world space.
    /// @return SpaceTransform : The 3D transform as an object containing position, rotation, and scale.
    virtual SpaceTransform GetTransform() const = 0;

    /// @brief Sets the transform of this component in world space to the specified value.
    /// @param InValue SpaceTransform : The new value expressed as a SpaceTransform.
    virtual void SetTransform(const SpaceTransform& InValue) = 0;

protected:
    virtual ~ITransformComponent() = default;
};

} // namespace csp::multiplayer
