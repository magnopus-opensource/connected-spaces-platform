/*
 * Copyright 2025 Magnopus LLC

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
/// @file IRenderBehaviourComponent.h
/// @brief Holdout visibility control for components.

#pragma once

#include "CSP/CSPCommon.h"

namespace csp::multiplayer
{

/// @brief Controls rendering behavior properties, such as a holdout state related to display modes, for a component.
CSP_INTERFACE class CSP_API IRenderBehaviourComponent
{
public:
    /// @defgroup HoldoutConcept Holdouts and Data Access
    /// A holdout is an object that is rendered as a mask, cutting it out from the final image.
    /// It still participates in depth testing, allowing objects to move behind it, but does not contribute pixels to the final image.

    /// @brief Checks if the component is shown as holdout when in AR mode.
    /// @return True if the component is shown as holdout when in AR mode, false otherwise.
    ///
    /// @see HoldoutConcept
    virtual bool GetShowAsHoldoutInAR() const = 0;

    /// @brief Sets if the component is shown as holdout in AR mode.
    /// @param InValue True if the component is shown as holdout in AR mode, false otherwise.
    ///
    /// @see HoldoutConcept
    virtual void SetShowAsHoldoutInAR(bool InValue) = 0;

    /// @brief Checks if the component is shown as holdout when in VR and Desktop mode.
    /// @return True if the component is shown as holdout when in VR and Desktop mode, false otherwise.
    ///
    /// @see HoldoutConcept
    virtual bool GetShowAsHoldoutInVR() const = 0;

    /// @brief Sets if the component is shown as holdout in VR and Desktop mode.
    /// @param InValue True if the component is shown as holdout in VR and Desktop mode, false otherwise.
    ///
    /// @see HoldoutConcept
    virtual void SetShowAsHoldoutInVR(bool InValue) = 0;

protected:
    virtual ~IRenderBehaviourComponent() = default;
};

} // namespace csp::multiplayer
