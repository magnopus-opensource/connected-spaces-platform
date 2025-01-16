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
/// @file IEnableableComponent.h
/// @brief Controls for enabling or disabling components.

#pragma once

#include "CSP/CSPCommon.h"

namespace csp::multiplayer
{

/// @brief Controls whether a component is enabled or disabled.
CSP_INTERFACE class CSP_API IEnableableComponent
{
public:
    /// @brief Checks if the component is enabled.
    /// @return True if the component is enabled, false otherwise.
    virtual bool GetIsEnabled() const = 0;

    /// @brief Sets if the component is enabled.
    /// @param InValue True to set the component to enabled, false otherwise.
    virtual void SetIsEnabled(bool InValue) = 0;

protected:
    virtual ~IEnableableComponent() = default;
};

} // namespace csp::multiplayer
