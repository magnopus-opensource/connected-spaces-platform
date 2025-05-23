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

#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Common/Vector.h"

namespace csp::multiplayer
{

/// @brief Controls the 3D scale of the component.
CSP_INTERFACE class CSP_API IScaleComponent
{
public:
    /// @brief Gets the scale of the origin of this component in world space.
    /// @note The coordinate system used follows the glTF 2.0 specification.
    ///       - Right handed coordinate system
    ///       - +Y is UP
    ///       - +X is left (facing forward)
    ///       - +Z is forward
    /// @return csp::common::Vector3 : The 3D scale as a vector (left, up, forward).
    virtual const csp::common::Vector3& GetScale() const = 0;

    /// @brief Sets the scale of the origin of this component in world space to the specified value.
    /// @note The coordinate system used follows the glTF 2.0 specification.
    ///       - Right handed coordinate system
    ///       - +Y is UP
    ///       - +X is left (facing forward)
    ///       - +Z is forward
    /// @param InValue csp::common::Vector3 : The new value expressed as a vector (left, up, forward).
    virtual void SetScale(const csp::common::Vector3& InValue) = 0;

protected:
    virtual ~IScaleComponent() = default;
};

} // namespace csp::multiplayer
