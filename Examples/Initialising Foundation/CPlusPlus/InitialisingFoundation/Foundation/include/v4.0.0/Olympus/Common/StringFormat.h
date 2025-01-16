#pragma once

#include "Olympus/Common/String.h"
#include "Olympus/Memory/DllAllocator.h"

#include <functional>
#include <memory>
#include <stdio.h>

OLY_NO_EXPORT

namespace oly_common
{

// Based on...
// https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf

template <typename... Args> static String StringFormat(const String& Format, Args... args)
{
    int SizeS = std::snprintf(nullptr, 0, Format.c_str(), args...) + 1; // Extra space for '\0'
    if (SizeS <= 0)
    {
        return String();
    }
    auto Size = static_cast<size_t>(SizeS);

    std::unique_ptr<char[], oly_memory::DllDeleter<char>> Buf((char*)oly_memory::DllAlloc(Size));

    std::snprintf(Buf.get(), Size, Format.c_str(), args...);
    return String(Buf.get(), Size - 1); // We don't want the '\0' inside
};

} // namespace oly_common
