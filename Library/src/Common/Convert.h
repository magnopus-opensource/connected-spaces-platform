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

#include "CSP/Common/Array.h"
#include "CSP/Common/Map.h"
#include "CSP/Common/Optional.h"

#include <map>
#include <optional>
#include <vector>

namespace csp::common
{

/// @brief Converts csp::common::Array to std::vector.
/// @param In const Array<T>&
/// return std::vector<T>
template <typename T> std::vector<T> Convert(const Array<T>& In)
{
    std::vector<T> Out;
    Out.reserve(In.Size());

    for (size_t idx = 0; idx < In.Size(); ++idx)
    {
        Out.push_back({ In[idx] });
    }

    return Out;
}

/// @brief Converts std::vector to csp::common::Array.
/// @param In const std::vector<T>&
/// return Array<T>
template <typename T> Array<T> Convert(const std::vector<T>& In)
{
    Array<T> Out(In.size());

    for (size_t idx = 0; idx < In.size(); ++idx)
    {
        Out[idx] = In[idx];
    }

    return Out;
}
/// @brief Converts std::Map to csp::common::Map.
/// @param In const std::map<T1, T2>&
/// return Map<T1,T2>
template <typename T1, typename T2> Map<T1, T2> Convert(const std::map<T1, T2>& In)
{
    Map<T1, T2> Out;

    for (auto const& pair : In)
    {
        Out[pair.first] = pair.second;
    }

    return Out;
}
/// @brief Converts csp::common::Map to std::map.
/// @param In const Map<T1, T2>&
/// @return std::map<T1, T2>
template <typename T1, typename T2> std::map<T1, T2> Convert(const Map<T1, T2>& In)
{
    std::map<T1, T2> Out;

    auto* Keys = In.Keys();

    for (auto idx = 0; idx < Keys->Size(); ++idx)
    {
        auto Key = (*Keys)[idx];
        auto Value = In[Key];
        Out.insert(std::pair<T1, T2>(Key, Value));
    }

    return Out;
}

/// @brief Converts csp::common::Optional to std::optional.
/// @param In const Optional<T>&
/// @return std::optional<T>
template <typename T> std::optional<T> Convert(const Optional<T>& In)
{
    std::optional<T> Out;

    if (In.HasValue())
    {
        Out = *In;
    }

    return Out;
}

/// @brief Converts csp::common::Optional<csp::common::Array> to std::optional<std::vector>.
/// @param In const Optional<Array<T>>&
/// @return std::optional<std::vector<T>>
template <typename T> std::optional<std::vector<T>> Convert(const Optional<Array<T>>& In)
{
    std::optional<std::vector<T>> Out;

    if (In.HasValue())
    {
        Out = Convert(*In);
    }

    return Out;
}

/// @brief Converts csp::common::Optional<csp::map> to std::optional<std::map>.
/// @param In const Optional<Map<T1, T2>>&
/// @return std::optional<std::map<T1, T2>>
template <typename T1, typename T2> std::optional<std::map<T1, T2>> Convert(const Optional<Map<T1, T2>>& In)
{
    std::optional<std::map<T1, T2>> Out;

    if (In.HasValue())
    {
        Out = Convert(*In);
    }

    return Out;
}

} // namespace csp::common
