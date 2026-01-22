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
#include <CSP/Common/Array.h>
#include <CSP/Common/List.h>
#include <CSP/Common/Map.h>
#include <CSP/Common/ReplicatedValue.h>
#include <CSP/Common/Settings.h>
#include <CSP/Common/String.h>
#include <CSP/Common/Vector.h>

#include <functional>
#include <string_view>

CSP_START_IGNORE
namespace std
{

template <> struct hash<csp::common::Vector2>
{
    size_t operator()(const csp::common::Vector2& v) const noexcept
    {
        size_t h1 = std::hash<float> {}(v.X);
        size_t h2 = std::hash<float> {}(v.Y);
        return h1 ^ (h2 << 1);
    }
};

template <> struct hash<csp::common::Vector3>
{
    size_t operator()(const csp::common::Vector3& v) const noexcept
    {
        size_t h1 = std::hash<float> {}(v.X);
        size_t h2 = std::hash<float> {}(v.Y);
        size_t h3 = std::hash<float> {}(v.Z);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

template <> struct hash<csp::common::Vector4>
{
    size_t operator()(const csp::common::Vector4& v) const noexcept
    {
        size_t h1 = std::hash<float> {}(v.X);
        size_t h2 = std::hash<float> {}(v.Y);
        size_t h3 = std::hash<float> {}(v.Z);
        size_t h4 = std::hash<float> {}(v.W);
        return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
    }
};

template <> struct hash<csp::common::String>
{
    size_t operator()(const csp::common::String& s) const noexcept { return std::hash<std::string_view> {}(s.c_str()); }
};

// These hashes arn't ideal. Even without getting super fancy you could vary the shift by element
// to reduce collisions a lot, but they you'd need to handle wraparound.
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

template <> struct hash<csp::common::ReplicatedValue>
{
    size_t operator()(const csp::common::ReplicatedValue& v) const noexcept
    {
        size_t typeHash = std::hash<int> {}(static_cast<int>(v.GetReplicatedValueType()));
        size_t valueHash = 0;

        // Could make this smaller with std::visit, but that's quite "clever" and I don't like it.
        switch (v.GetReplicatedValueType())
        {
        case csp::common::ReplicatedValueType::Boolean:
            valueHash = std::hash<bool> {}(v.GetBool());
            break;
        case csp::common::ReplicatedValueType::Integer:
            valueHash = std::hash<int64_t> {}(v.GetInt());
            break;
        case csp::common::ReplicatedValueType::Float:
            valueHash = std::hash<float> {}(v.GetFloat());
            break;
        case csp::common::ReplicatedValueType::String:
            valueHash = std::hash<csp::common::String> {}(v.GetString());
            break;
        case csp::common::ReplicatedValueType::Vector2:
            valueHash = std::hash<csp::common::Vector2> {}(v.GetVector2());
            break;
        case csp::common::ReplicatedValueType::Vector3:
            valueHash = std::hash<csp::common::Vector3> {}(v.GetVector3());
            break;
        case csp::common::ReplicatedValueType::Vector4:
            valueHash = std::hash<csp::common::Vector4> {}(v.GetVector4());
            break;
        case csp::common::ReplicatedValueType::StringMap:
            valueHash = std::hash<csp::common::Map<csp::common::String, csp::common::ReplicatedValue>> {}(v.GetStringMap());
            break;
        case csp::common::ReplicatedValueType::InvalidType:
        default:
            break;
        }

        return typeHash ^ (valueHash << 1);
    }
};

template <> struct hash<csp::common::ApplicationSettings>
{
    size_t operator()(const csp::common::ApplicationSettings& s) const noexcept
    {
        size_t h1 = std::hash<csp::common::String> {}(s.ApplicationName);
        size_t h2 = std::hash<csp::common::String> {}(s.Context);
        size_t h3 = std::hash<bool> {}(s.AllowAnonymous);
        size_t h4 = std::hash<csp::common::Map<csp::common::String, csp::common::String>> {}(s.Settings);
        return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
    }
};

template <> struct hash<csp::common::SettingsCollection>
{
    size_t operator()(const csp::common::SettingsCollection& s) const noexcept
    {
        size_t h1 = std::hash<csp::common::String> {}(s.UserId);
        size_t h2 = std::hash<csp::common::String> {}(s.Context);
        size_t h3 = std::hash<csp::common::Map<csp::common::String, csp::common::String>> {}(s.Settings);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

} // namespace std
CSP_END_IGNORE
