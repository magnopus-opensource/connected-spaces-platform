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
#include <CSP/Common/Vector.h>

#include <functional>

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

} // namespace std
CSP_END_IGNORE
