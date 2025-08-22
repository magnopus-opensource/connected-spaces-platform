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
/// @file IVisibilityBehaviourComponent.h
/// @brief Holdout visibility control for components.

#pragma once

#include "CSP/CSPCommon.h"

namespace csp::multiplayer
{

/// @brief Controls the holdout visibility behaviour of the component when in default mode, AR mode or Virtual mode.
/// A holdout is an object that is rendered as a “mask”, cutting it out from the final image.
/// It still participates in depth testing, allowing objects to move behind it, but does not contribute pixels to the final image.
CSP_INTERFACE class CSP_API IVisibilityBehaviourComponent
{
public:
    /// @brief Checks if the component is shown as holdout when in AR mode.
    /// @return True if the component is shown as holdout when in AR mode, false otherwise.
    virtual bool GetShowAsHoldoutInAR() const = 0;

    /// @brief Sets if the component is shown as holdout in AR mode.
    /// @param InValue True if the component is shown as holdout in AR mode, false otherwise.
    virtual void SetShowAsHoldoutInAR(bool InValue) = 0;

    /// @brief Checks if the component is shown as holdout when in VR mode.
    /// @return True if the component is shown as holdout when in VR mode, false otherwise.
    virtual bool GetShowAsHoldoutInVirtual() const = 0;

    /// @brief Sets if the component is shown as holdout in VR mode.
    /// @param InValue True if the component is shown as holdout in VR mode, false otherwise.
    virtual void SetShowAsHoldoutInVirtual(bool InValue) = 0;

protected:
    virtual ~IVisibilityBehaviourComponent() = default;
};

} // namespace csp::multiplayer
