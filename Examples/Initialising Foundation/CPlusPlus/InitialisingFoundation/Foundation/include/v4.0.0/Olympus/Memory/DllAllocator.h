#pragma once

#include "Olympus/OlympusCommon.h"

#include <new>
#include <stddef.h>

OLY_NO_EXPORT

namespace oly_memory
{

OLY_API void* DllAlloc(size_t Size, size_t Alignment = size_t(16));
OLY_API void* DllRealloc(void* Ptr, size_t NewSize, size_t Alignment = size_t(16));
OLY_API void DllFree(void* Ptr);

template <typename T> struct DllDeleter
{
    constexpr void operator()(T* Ptr) { oly_memory::DllFree((void*)Ptr); }
};

} // namespace oly_memory
