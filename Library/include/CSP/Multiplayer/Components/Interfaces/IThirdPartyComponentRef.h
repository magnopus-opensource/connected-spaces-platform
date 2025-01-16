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
/// @file IThirdPartyComponentRef.h
/// @brief Stores third party component reference.

#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Common/String.h"

namespace csp::multiplayer
{

/// @brief Controls access to a third party component reference.
/// To enable developers to author CSP powered applications using
/// the native features of their platform of choice, we need to be able
/// to map their platform component definition to the CSP component.
CSP_INTERFACE class CSP_API IThirdPartyComponentRef
{
public:
    /// @brief Returns the third party component reference.
    /// @return The third party component reference.
    virtual const csp::common::String& GetThirdPartyComponentRef() const = 0;

    /// @brief Sets the third party component reference.
    /// @param InValue The third party component reference.
    virtual void SetThirdPartyComponentRef(const csp::common::String& InValue) = 0;

protected:
    virtual ~IThirdPartyComponentRef() = default;
};

} // namespace csp::multiplayer
