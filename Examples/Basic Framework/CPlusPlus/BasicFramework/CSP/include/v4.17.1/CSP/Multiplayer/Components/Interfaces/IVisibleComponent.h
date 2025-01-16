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

namespace csp::multiplayer
{

/// @brief Controls the visibility of the component when in default mode or in AR mode.
CSP_INTERFACE class CSP_API IVisibleComponent
{
public:
    /// @brief Checks if the component is visible when in default mode.
    /// @return True if the component is visible, false otherwise.
    virtual bool GetIsVisible() const = 0;

    /// @brief Sets if the component is visible when in default mode.
    /// @param InValue True if the component is visible, false otherwise.
    virtual void SetIsVisible(bool InValue) = 0;

    /// @brief Checks if the component is visible when in AR mode.
    /// @return True if the component is visible when in AR mode, false otherwise.
    virtual bool GetIsARVisible() const = 0;

    /// @brief Sets if the component is visible in AR mode.
    /// @param InValue True if the component is visible in AR mode, false otherwise.
    virtual void SetIsARVisible(bool InValue) = 0;

protected:
    virtual ~IVisibleComponent() = default;
};

} // namespace csp::multiplayer
