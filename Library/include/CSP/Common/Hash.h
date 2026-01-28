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

/*
 * Hash functions for CSP types in the common namespace
 * Defined centrally, partly because it's a nice organization, but also partly due to wrapper gen constraints.
 * Something having one of these is one of the best markers as to whether it's a "value-type" or not, albeit
 * that term is nebulous. (Less so in other language runtimes though!)
 *
 * Try not to forget about these, they're very handy to have, and not having one means reference based languages
 * like C# have to rely on reference equality which is really quite annoying, and leads to worse performance
 * inside hashing containers.
 */

#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Common/Array.h"
#include "CSP/Common/List.h"
#include "CSP/Common/Map.h"
#include "CSP/Common/String.h"

#include <functional>

namespace csp::common
{
class Vector2;
class Vector3;
class Vector4;
class ReplicatedValue;
class ApplicationSettings;
class SettingsCollection;
}

CSP_START_IGNORE
namespace std
{

template <> struct CSP_API hash<csp::common::Vector2>
{
    size_t operator()(const csp::common::Vector2& v) const noexcept;
};

template <> struct CSP_API hash<csp::common::Vector3>
{
    size_t operator()(const csp::common::Vector3& v) const noexcept;
};

template <> struct CSP_API hash<csp::common::Vector4>
{
    size_t operator()(const csp::common::Vector4& v) const noexcept;
};

template <> struct CSP_API hash<csp::common::String>
{
    size_t operator()(const csp::common::String& s) const noexcept;
};

// These hashes aren't ideal. Even without getting super fancy you could vary the shift by element
// to reduce collisions a lot, but then you'd need to handle wraparound.
// Remember shifting by more than 64 (on x64 systems) is undefined behavior.
template <typename T> struct hash<csp::common::Array<T>>
{
    size_t operator()(const csp::common::Array<T>& a) const noexcept
    {
        size_t h = 0;
        for (size_t i = 0; i < a.Size(); ++i)
        {
            h ^= std::hash<T> {}(a[i]) << 1;
        }
        return h;
    }
};

template <typename T> struct hash<csp::common::List<T>>
{
    size_t operator()(const csp::common::List<T>& l) const noexcept
    {
        size_t h = 0;
        for (size_t i = 0; i < l.Size(); ++i)
        {
            h ^= std::hash<T> {}(l[i]) << 1;
        }
        return h;
    }
};

template <typename TKey, typename TValue> struct hash<csp::common::Map<TKey, TValue>>
{
    size_t operator()(const csp::common::Map<TKey, TValue>& m) const noexcept
    {
        size_t h = 0;
        for (const auto& pair : m)
        {
            h ^= std::hash<TKey> {}(pair.first) ^ (std::hash<TValue> {}(pair.second) << 1);
        }
        return h;
    }
};

template <> struct CSP_API hash<csp::common::ReplicatedValue>
{
    size_t operator()(const csp::common::ReplicatedValue& v) const noexcept;
};

template <> struct CSP_API hash<csp::common::ApplicationSettings>
{
    size_t operator()(const csp::common::ApplicationSettings& s) const noexcept;
};

template <> struct CSP_API hash<csp::common::SettingsCollection>
{
    size_t operator()(const csp::common::SettingsCollection& s) const noexcept;
};

} // namespace std
CSP_END_IGNORE
