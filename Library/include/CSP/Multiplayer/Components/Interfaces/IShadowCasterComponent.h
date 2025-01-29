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
/// @file IShadowCasterComponent.h
/// @brief Stores third party component reference.

#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Common/String.h"

namespace csp::multiplayer
{

/// @brief Controls whether the Model cast shadows or not
CSP_INTERFACE class CSP_API IShadowCasterComponent
{
public:
    /// @brief Checks if the mesh casts shadows.
    /// @return True if the mesh casts shadows.
    virtual bool GetIsShadowCaster() const = 0;

    /// @brief Sets if the mesh casts shadows.
    /// @param InValue True to set the mesh to casts shadows.
    virtual void SetIsShadowCaster(bool InValue) = 0;

protected:
    virtual ~IShadowCasterComponent() = default;
};

} // namespace csp::multiplayer
