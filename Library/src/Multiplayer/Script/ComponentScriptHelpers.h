/*
 * Copyright 2026 Magnopus LLC

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

#include "CSP/Multiplayer/ComponentSchema.h"

#include <algorithm>
#include <type_traits>
#include <variant>

namespace csp::multiplayer
{

template <typename T> struct ScriptTypeMap;

template <typename T> using ScriptTypeT = typename ScriptTypeMap<T>::Type;

template <typename T, typename = void> inline constexpr bool IsScriptableV = false;

template <typename T> inline constexpr bool IsScriptableV<T, std::void_t<ScriptTypeT<T>>> = true;

template <> struct ScriptTypeMap<bool>
{
    using Type = bool;
};

template <> struct ScriptTypeMap<float>
{
    using Type = float;
};

template <> struct ScriptTypeMap<int64_t>
{
    using Type = int64_t;
};

template <> struct ScriptTypeMap<csp::common::String>
{
    using Type = std::string;
};

template <> struct ScriptTypeMap<csp::common::Vector2>
{
    using Type = std::vector<float>;
};

template <> struct ScriptTypeMap<csp::common::Vector3>
{
    using Type = std::vector<float>;
};

template <> struct ScriptTypeMap<csp::common::Vector4>
{
    using Type = std::vector<float>;
};

inline bool IsScriptable(const csp::common::String& Name) { return !Name.IsEmpty(); }

inline bool IsScriptable(const ComponentProperty& Property)
{
    return IsScriptable(Property.Name)
        && std::visit(
            [](const auto& V)
            {
                using T = std::decay_t<decltype(V)>;
                return IsScriptableV<T>;
            },
            Property.DefaultValue.GetValue());
}

inline bool IsScriptable(const ComponentSchema& Schema)
{
    const auto HasScriptableProperties = [&](const auto& Properties)
    { return std::any_of(Properties.begin(), Properties.end(), [](const auto& Property) { return IsScriptable(Property); }); };

    return IsScriptable(Schema.Name) && HasScriptableProperties(Schema.Properties);
}

} // namespace csp::multiplayer
