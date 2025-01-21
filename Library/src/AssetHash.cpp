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
#include "CSP/AssetHash.h"

#include <memory>

#ifdef _MSC_VER
#include <stdlib.h>
#define bswap_32(x) _byteswap_ulong(x)
#elif defined(__APPLE__)
// Mac OS X / Darwin features
#include <libkern/OSByteOrder.h>
#define bswap_32(x) OSSwapInt32(x)
#elif defined(__sun) || defined(sun)
#include <sys/byteorder.h>
#define bswap_32(x) BSWAP_32(x)
#elif defined(__FreeBSD__)
#include <machine/endian.h>
#define bswap_32(x) __bswap32_var(x)
#elif defined(__OpenBSD__)
#include <sys/types.h>
#define bswap_32(x) swap32(x)
#elif defined(__NetBSD__)
#include <machine/bswap.h>
#include <sys/types.h>
#if defined(__BSWAP_RENAME) && !defined(__bswap_32)
#define bswap_32(x) bswap32(x)
#endif
#else
#define bswap_32(x) (((x) >> 24) + (((x) >> 8) & 0xff00) + (((x) << 8) & 0xff0000) + ((x) << 24))
#endif

#undef PERMUTE3
#define PERMUTE3(a, b, c)                                                                                                                            \
    do                                                                                                                                               \
    {                                                                                                                                                \
        SwapValues(a, b);                                                                                                                            \
        SwapValues(a, c);                                                                                                                            \
    } while (0)

namespace
{

// Magic numbers for 32-bit hashing.  Copied from Murmur3.
constexpr uint32_t c1 = 0xcc9e2d51;
constexpr uint32_t c2 = 0x1b873593;

template <typename T> void SwapValues(T& a, T& b)
{
    T c = a;
    a = b;
    b = c;
}

uint32_t UNALIGNED_LOAD32(const char* p)
{
    uint32_t result;
    std::memcpy(&result, p, sizeof(result));
    return result;
}

// A 32-bit to 32-bit integer hash copied from Murmur3.
uint32_t fmix(uint32_t h)
{
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

uint32_t Fetch32(const char* p) { return UNALIGNED_LOAD32(p); }

uint32_t Rotate32(uint32_t val, int shift)
{
    // Avoid shifting by 32: doing so yields an undefined result.
    return shift == 0 ? val : ((val >> shift) | (val << (32 - shift)));
}

uint32_t Mur(uint32_t a, uint32_t h)
{
    // Helper from Murmur3 for combining two 32-bit values.
    a *= c1;
    a = Rotate32(a, 17);
    a *= c2;
    h ^= a;
    h = Rotate32(h, 19);
    return h * 5 + 0xe6546b64;
}

uint32_t Hash32Len0to4(const char* s, uint32_t len)
{
    uint32_t b = 0;
    uint32_t c = 9;

    for (uint32_t i = 0; i < len; i++)
    {
        signed char v = s[i];
        b = b * c1 + v;
        c ^= b;
    }

    return fmix(Mur(b, Mur(len, c)));
}

uint32_t Hash32Len5to12(const char* s, uint32_t len)
{
    uint32_t a = len, b = len * 5, c = 9, d = b;
    a += Fetch32(s);
    b += Fetch32(s + len - 4);
    c += Fetch32(s + ((len >> 1) & 4));
    return fmix(Mur(c, Mur(b, Mur(a, d))));
}

uint32_t Hash32Len13to24(const char* s, uint32_t len)
{
    uint32_t a = Fetch32(s - 4 + (len >> 1));
    uint32_t b = Fetch32(s + 4);
    uint32_t c = Fetch32(s + len - 8);
    uint32_t d = Fetch32(s + (len >> 1));
    uint32_t e = Fetch32(s);
    uint32_t f = Fetch32(s + len - 4);
    uint32_t h = len;

    return fmix(Mur(f, Mur(e, Mur(d, Mur(c, Mur(b, Mur(a, h)))))));
}

} // namespace

namespace csp
{

uint32_t GenerateAssetHash(const csp::common::String& AssetId)
{
    const char* s = AssetId.c_str();

    // The length of an ID will always be short enough that this is a safe cast.
    uint32_t len = static_cast<uint32_t>(AssetId.Length());

    if (len <= 24)
    {
        return (len <= 12 ? (len <= 4 ? Hash32Len0to4(s, len) : Hash32Len5to12(s, len)) : Hash32Len13to24(s, len)) >> 1;
    }

    // len > 24
    uint32_t h = len, g = c1 * len, f = g;
    uint32_t a0 = Rotate32(Fetch32(s + len - 4) * c1, 17) * c2;
    uint32_t a1 = Rotate32(Fetch32(s + len - 8) * c1, 17) * c2;
    uint32_t a2 = Rotate32(Fetch32(s + len - 16) * c1, 17) * c2;
    uint32_t a3 = Rotate32(Fetch32(s + len - 12) * c1, 17) * c2;
    uint32_t a4 = Rotate32(Fetch32(s + len - 20) * c1, 17) * c2;
    h ^= a0;
    h = Rotate32(h, 19);
    h = h * 5 + 0xe6546b64;
    h ^= a2;
    h = Rotate32(h, 19);
    h = h * 5 + 0xe6546b64;
    g ^= a1;
    g = Rotate32(g, 19);
    g = g * 5 + 0xe6546b64;
    g ^= a3;
    g = Rotate32(g, 19);
    g = g * 5 + 0xe6546b64;
    f += a4;
    f = Rotate32(f, 19);
    f = f * 5 + 0xe6546b64;
    uint32_t iters = (len - 1) / 20;

    do
    {
        uint32_t _a0 = Rotate32(Fetch32(s) * c1, 17) * c2;
        uint32_t _a1 = Fetch32(s + 4);
        uint32_t _a2 = Rotate32(Fetch32(s + 8) * c1, 17) * c2;
        uint32_t _a3 = Rotate32(Fetch32(s + 12) * c1, 17) * c2;
        uint32_t _a4 = Fetch32(s + 16);
        h ^= _a0;
        h = Rotate32(h, 18);
        h = h * 5 + 0xe6546b64;
        f += _a1;
        f = Rotate32(f, 19);
        f = f * c1;
        g += _a2;
        g = Rotate32(g, 18);
        g = g * 5 + 0xe6546b64;
        h ^= _a3 + _a1;
        h = Rotate32(h, 19);
        h = h * 5 + 0xe6546b64;
        g ^= _a4;
        g = bswap_32(g) * 5;
        h += _a4 * 5;
        h = bswap_32(h);
        f += _a0;
        PERMUTE3(f, h, g);
        s += 20;
    } while (--iters != 0);

    g = Rotate32(g, 11) * c1;
    g = Rotate32(g, 17) * c1;
    f = Rotate32(f, 11) * c1;
    f = Rotate32(f, 17) * c1;
    h = Rotate32(h + g, 19);
    h = h * 5 + 0xe6546b64;
    h = Rotate32(h, 17) * c1;
    h = Rotate32(h + f, 19);
    h = h * 5 + 0xe6546b64;
    h = Rotate32(h, 17) * c1;

    return h >> 1;
}

} // namespace csp
