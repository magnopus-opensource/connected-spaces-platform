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
#include "CSP/Common/Map.h"
#include "CSP/Common/String.h"
#include "CSP/Common/Vector.h"

#include "CSP/Multiplayer/ReplicatedValue.h"

#include <variant>

namespace csp::multiplayer
{

class ReplicatedValueImpl;

using ReplicatedValueImplType = std::variant<bool, float, int64_t, csp::common::String, csp::common::Vector2, csp::common::Vector3,
    csp::common::Vector4, csp::common::Map<csp::common::String, ReplicatedValue>>;

class ReplicatedValueImpl
{
public:
    ReplicatedValueImpl() = default;
    CSP_NO_EXPORT template <class T> ReplicatedValueImpl(T InValue);

    /// @param Other ReplicatedValue : Other replicated value to set this one to.
    ReplicatedValueImpl(const ReplicatedValueImpl& Other);

    // TODO: rule of 5

    CSP_NO_EXPORT template <class T> void Set(T InValue);
    CSP_NO_EXPORT template <class T> const T& Get() const;

    /// @brief Assignment operator overload.
    /// @param Other ReplicatedValue : Other replicated value to set this one to.
    ReplicatedValueImpl& operator=(const ReplicatedValueImpl& Other);

    /// @brief Equality operator overload.
    /// @param ReplicatedValueImpl : Other value to compare to.
    bool operator==(const ReplicatedValueImpl& Other) const;

    /// @brief Non equality operator overload.
    /// @param ReplicatedValueImpl : Other value to compare to.
    bool operator!=(const ReplicatedValueImpl& Other) const;

    /// @brief Less than operator overload.
    /// @param ReplicatedValueImpl : Other value to compare to.
    bool operator<(const ReplicatedValueImpl& Other) const;

    /// @brief Greater than operator overload.
    /// @param ReplicatedValueImpl : Other value to compare to.
    bool operator>(const ReplicatedValueImpl& Other) const;

private:
    ReplicatedValueImplType Value;
};

CSP_NO_EXPORT template <class T> inline ReplicatedValueImpl::ReplicatedValueImpl(T InValue) { Value = InValue; }

CSP_NO_EXPORT template <class T> inline void ReplicatedValueImpl::Set(T InValue) { Value = InValue; }

CSP_NO_EXPORT template <class T> inline const T& ReplicatedValueImpl::Get() const { return std::get<T>(Value); }
}
