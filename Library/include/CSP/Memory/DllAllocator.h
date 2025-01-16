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

#include "CSP/CSPCommon.h"

#include <new>
#include <stddef.h>

CSP_NO_EXPORT

namespace csp::memory
{

/// @brief Dll safe allocation of memory.
/// @param Size size_t : Size in bytes to allocate
/// @param Alignment size_t : Alignment of data in memory. Defaults to 16
/// @return void* : Pointer to the start of the allocated memory
CSP_API void* DllAlloc(size_t Size, size_t Alignment = size_t(16));

/// @brief Dll safe re-allocation of memory.
/// @param Size size_t : Size in bytes to allocate
/// @param Alignment size_t : Alignment of data in memory. Defaults to 16
/// @return void* : Pointer to the start of the allocated memory
CSP_API void* DllRealloc(void* Ptr, size_t NewSize, size_t Alignment = size_t(16));

/// @brief Dll safe freeing of memory.
/// @param Ptr void* : Data to free
CSP_API void DllFree(void* Ptr);

/// @brief Struct wrapper for csp::memory::DllFree.
template <typename T> struct DllDeleter
{
    constexpr void operator()(T* Ptr) { csp::memory::DllFree((void*)Ptr); }
};

} // namespace csp::memory
