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

#include "CSP/Common/Hash.h"

#include "CSP/Common/ReplicatedValue.h"
#include "CSP/Common/Settings.h"
#include "CSP/Common/Vector.h"

#include <string_view>

size_t std::hash<csp::common::Vector2>::operator()(const csp::common::Vector2& v) const noexcept
{
    size_t h1 = std::hash<float> {}(v.X);
    size_t h2 = std::hash<float> {}(v.Y);
    return h1 ^ (h2 << 1);
}

size_t std::hash<csp::common::Vector3>::operator()(const csp::common::Vector3& v) const noexcept
{
    size_t h1 = std::hash<float> {}(v.X);
    size_t h2 = std::hash<float> {}(v.Y);
    size_t h3 = std::hash<float> {}(v.Z);
    return h1 ^ (h2 << 1) ^ (h3 << 2);
}

size_t std::hash<csp::common::Vector4>::operator()(const csp::common::Vector4& v) const noexcept
{
    size_t h1 = std::hash<float> {}(v.X);
    size_t h2 = std::hash<float> {}(v.Y);
    size_t h3 = std::hash<float> {}(v.Z);
    size_t h4 = std::hash<float> {}(v.W);
    return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
}

size_t std::hash<csp::common::String>::operator()(const csp::common::String& s) const noexcept { return std::hash<std::string_view> {}(s.c_str()); }

size_t std::hash<csp::common::ReplicatedValue>::operator()(const csp::common::ReplicatedValue& v) const noexcept
{
    size_t TypeHash = std::hash<int> {}(static_cast<int>(v.GetReplicatedValueType()));
    size_t ValueHash = std::visit([](const auto& val) -> size_t { return std::hash<std::decay_t<decltype(val)>> {}(val); }, v.GetValue());

    return TypeHash ^ (ValueHash << 1);
}

size_t std::hash<csp::common::ApplicationSettings>::operator()(const csp::common::ApplicationSettings& s) const noexcept
{
    size_t h1 = std::hash<csp::common::String> {}(s.ApplicationName);
    size_t h2 = std::hash<csp::common::String> {}(s.Context);
    size_t h3 = std::hash<bool> {}(s.AllowAnonymous);
    size_t h4 = std::hash<csp::common::Map<csp::common::String, csp::common::String>> {}(s.Settings);
    return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
}

size_t std::hash<csp::common::SettingsCollection>::operator()(const csp::common::SettingsCollection& s) const noexcept
{
    size_t h1 = std::hash<csp::common::String> {}(s.UserId);
    size_t h2 = std::hash<csp::common::String> {}(s.Context);
    size_t h3 = std::hash<csp::common::Map<csp::common::String, csp::common::String>> {}(s.Settings);
    return h1 ^ (h2 << 1) ^ (h3 << 2);
}
