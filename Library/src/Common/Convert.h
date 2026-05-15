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
template <typename T> std::vector<T> Convert(const Array<T>& in)
{
    std::vector<T> out;
    out.reserve(in.Size());

    for (size_t idx = 0; idx < in.Size(); ++idx)
    {
        out.push_back({ in[idx] });
    }

    return out;
}

/// @brief Converts std::vector to csp::common::Array.
/// @param In const std::vector<T>&
/// return Array<T>
template <typename T> Array<T> Convert(const std::vector<T>& in)
{
    Array<T> out(in.size());

    for (size_t idx = 0; idx < in.size(); ++idx)
    {
        out[idx] = in[idx];
    }

    return out;
}
/// @brief Converts std::Map to csp::common::Map.
/// @param In const std::map<T1, T2>&
/// return Map<T1,T2>
template <typename T1, typename T2> Map<T1, T2> Convert(const std::map<T1, T2>& in)
{
    Map<T1, T2> out;

    for (auto const& pair : in)
    {
        out[pair.first] = pair.second;
    }

    return out;
}
/// @brief Converts csp::common::Map to std::map.
/// @param In const Map<T1, T2>&
/// @return std::map<T1, T2>
template <typename T1, typename T2> std::map<T1, T2> Convert(const Map<T1, T2>& in)
{
    std::map<T1, T2> out;

    auto* keys = in.Keys();

    for (size_t idx = 0; idx < keys->Size(); ++idx)
    {
        auto key = (*keys)[idx];
        auto value = in[key];
        out.insert(std::pair<T1, T2>(key, value));
    }

    return out;
}

/// @brief Converts std::optional to csp::common::Optional.
/// @param In const std::optional<T>
/// @return Optional<T>&
template <typename T> Optional<T> Convert(const std::optional<T>& in)
{
    Optional<T> out;

    if (in.has_value())
    {
        out = *in;
    }

    return out;
}

/// @brief Converts csp::common::Optional to std::optional.
/// @param In const Optional<T>&
/// @return std::optional<T>
template <typename T> std::optional<T> Convert(const Optional<T>& in)
{
    std::optional<T> out;

    if (in.HasValue())
    {
        out = *in;
    }

    return out;
}

/// @brief Converts csp::common::Optional<csp::common::Array> to std::optional<std::vector>.
/// @param In const Optional<Array<T>>&
/// @return std::optional<std::vector<T>>
template <typename T> std::optional<std::vector<T>> Convert(const Optional<Array<T>>& in)
{
    std::optional<std::vector<T>> out;

    if (in.HasValue())
    {
        out = Convert(*in);
    }

    return out;
}

/// @brief Converts csp::common::Optional<csp::map> to std::optional<std::map>.
/// @param In const Optional<Map<T1, T2>>&
/// @return std::optional<std::map<T1, T2>>
template <typename T1, typename T2> std::optional<std::map<T1, T2>> Convert(const Optional<Map<T1, T2>>& in)
{
    std::optional<std::map<T1, T2>> out;

    if (in.HasValue())
    {
        out = Convert(*in);
    }

    return out;
}

} // namespace csp::common
