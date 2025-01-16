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

#define CSP_MEMORY_TRACKING_ENABLED 0

/// Override global new
///
/// Define if we want to override all calls to new/delete even in external libraries
#define CSP_MEMORY_OVERRIDE_GLOBAL_NEW 0

#include "Memory/MemoryManager.h"

namespace csp::memory
{

inline void* Allocate(size_t size);
inline void* Allocate(size_t size, std::align_val_t alignment);
inline void* Allocate(size_t size, std::align_val_t alignment, csp::memory::Allocator* Allocator);

inline void* Reallocate(void* Ptr, size_t size);
inline void* Reallocate(void* Ptr, size_t size, std::align_val_t alignment);
inline void* Reallocate(void* Ptr, size_t size, std::align_val_t alignment, csp::memory::Allocator* Allocator);

inline void Deallocate(void* Ptr);
inline void Deallocate(void* Ptr, size_t size);
inline void Deallocate(void* Ptr, size_t size, csp::memory::Allocator* Allocator);
inline void Deallocate(void* Ptr, csp::memory::Allocator* Allocator);

template <typename T> inline void Delete(T* Ptr);
template <typename T> inline void Delete(const T* Ptr);
template <typename T> inline void Delete(T* Ptr, csp::memory::Allocator* Allocator);
template <typename T> inline void Delete(const T* Ptr, csp::memory::Allocator* Allocator);
template <typename T, typename std::enable_if_t<std::is_integral_v<T>>* = nullptr> inline void DeleteArray(const T* Ptr);
template <typename T, typename std::enable_if_t<!std::is_integral_v<T>>* = nullptr> inline void DeleteArray(const T* Ptr);

inline void* Allocate(size_t size) { return MemoryManager::GetDefaultAllocator().Allocate(size); }

inline void* Allocate(size_t size, std::align_val_t alignment) { return MemoryManager::GetDefaultAllocator().Allocate(size, alignment); }

inline void* Allocate(size_t size, std::align_val_t alignment, csp::memory::Allocator* Allocator) { return Allocator->Allocate(size, alignment); }

inline void* Reallocate(void* Ptr, size_t size) { return MemoryManager::GetDefaultAllocator().Reallocate(Ptr, size); }

inline void* Reallocate(void* Ptr, size_t size, std::align_val_t alignment)
{
    return MemoryManager::GetDefaultAllocator().Reallocate(Ptr, size, alignment);
}

inline void* Reallocate(void* Ptr, size_t size, std::align_val_t alignment, csp::memory::Allocator* Allocator)
{
    return Allocator->Reallocate(Ptr, size, alignment);
}

inline void Deallocate(void* Ptr) { csp::memory::MemoryManager::GetDefaultAllocator().Deallocate(Ptr); }

inline void Deallocate(void* Ptr, size_t Size) { csp::memory::MemoryManager::GetDefaultAllocator().Deallocate(Ptr, Size); }

inline void Deallocate(void* Ptr, csp::memory::Allocator* Allocator) { Allocator->Deallocate(Ptr); }

inline void Deallocate(void* Ptr, size_t Size, csp::memory::Allocator* Allocator) { Allocator->Deallocate(Ptr, Size); }

template <typename T> inline void Delete(T* Ptr)
{
    if (Ptr != nullptr)
    {
        Ptr->~T();
        Deallocate(Ptr, sizeof(T));
    }
}

template <typename T> inline void Delete(const T* Ptr)
{
    if (Ptr != nullptr)
    {
        T* NonConstPtr = (T*)Ptr;
        NonConstPtr->~T();
        Deallocate(NonConstPtr, sizeof(T));
    }
}

template <typename T> inline void Delete(T* Ptr, csp::memory::Allocator* Allocator)
{
    if (Ptr != nullptr)
    {
        Ptr->~T();
        Deallocate(Ptr, sizeof(T), Allocator);
    }
}

template <typename T> inline void Delete(const T* Ptr, csp::memory::Allocator* Allocator)
{
    if (Ptr != nullptr)
    {
        T* NonConstPtr = (T*)Ptr;
        NonConstPtr->~T();
        Deallocate(NonConstPtr, sizeof(T), Allocator);
    }
}

template <typename T, typename std::enable_if_t<std::is_integral_v<T>>*> inline void DeleteArray(const T* Ptr)
{
    if (Ptr != nullptr)
    {
        auto& Allocator = csp::memory::MemoryManager::GetDefaultAllocator();
        auto RealPointer = (char*)Ptr - sizeof(size_t);
        auto BufferSize = *(size_t*)RealPointer;
        Deallocate(RealPointer, BufferSize, &Allocator);
    }
}

template <typename T, typename std::enable_if_t<!std::is_integral_v<T>>*> inline void DeleteArray(const T* Ptr)
{
    if (Ptr != nullptr)
    {
        auto& Allocator = csp::memory::MemoryManager::GetDefaultAllocator();
        auto RealPointer = (char*)Ptr - sizeof(size_t);
        auto BufferSize = *(size_t*)RealPointer;
        auto Count = BufferSize / sizeof(T);

        for (int i = 0; i < Count; ++i)
        {
            (Ptr + i)->~T();
        }

        Deallocate(RealPointer, BufferSize, &Allocator);
    }
}

inline Allocator* DefaultAllocator() { return &MemoryManager::GetDefaultAllocator(); }

} // namespace csp::memory

#if CSP_MEMORY_TRACKING_ENABLED

// These are variations of the macros and overrides below that pass __FILE__ and __LINE__ to allow for allocation and leak tracking

#define CSP_ALLOC(size) csp::memory::Allocate(size, std::align_val_t(16), csp::memory::DefaultAllocator(), __FILE__, __LINE__)
#define CSP_ALLOC_ALIGN(size, alignment) csp::memory::Allocate(size, alignment, csp::memory::DefaultAllocator(), __FILE__, __LINE__)
#define CSP_ALLOC_P(allocator, size) csp::memory::Allocate(size, std::align_val_t(16), allocator, __FILE__, __LINE__)
#define CSP_ALLOC_ALIGN_P(allocator, size, alignment) csp::memory::Allocate(size, alignment, allocator, __FILE__, __LINE__)

#define CSP_REALLOC(ptr, size) csp::memory::Reallocate(ptr, size, std::align_val_t(16), csp::memory::DefaultAllocator(), __FILE__, __LINE__)
#define CSP_REALLOC_ALIGN(ptr, size, alignment) csp::memory::Reallocate(ptr, size, alignment, csp::memory::DefaultAllocator(), __FILE__, __LINE__)
#define CSP_REALLOC_P(allocator, size) csp::memory::Reallocate(ptr, size, std::align_val_t(16), allocator, __FILE__, __LINE__)
#define CSP_REALLOC_ALIGN_P(allocator, size, alignment) csp::memory::Reallocate(ptr, size, alignment, allocator, __FILE__, __LINE__)

#define CSP_NEW new (csp::memory::DefaultAllocator(), __FILE__, __LINE__)
#define CSP_NEW_P(allocator) new (allocator, __FILE__, __LINE__)
#define CSP_NEW_ALIGN_P(allocator, alignment) new (alignment, allocator, __FILE__, __LINE__)

#define CSP_FREE(ptr) csp::memory::Deallocate(ptr, __FILE__, __LINE__)
#define CSP_FREE_P(ptr, allocator) csp::memory::Deallocate(ptr, allocator, __FILE__, __LINE__)

#define CSP_DELETE(ptr) csp::memory::Delete(ptr, __FILE__, __LINE__)
#define CSP_DELETE_P(ptr, allocator) csp::memory::Delete(ptr, allocator, __FILE__, __LINE__)
#define CSP_DELETE_ARRAY(ptr) delete[] (ptr, csp::memory::DefaultAllocator(), __FILE__, __LINE__)

#else

#define CSP_ALLOC(size) csp::memory::Allocate(size, std::align_val_t(16), csp::memory::DefaultAllocator())
#define CSP_ALLOC_ALIGN(size, alignment) csp::memory::Allocate(size, alignment, csp::memory::DefaultAllocator())
#define CSP_ALLOC_P(allocator, size) csp::memory::Allocate(size, std::align_val_t(16), allocator)
#define CSP_ALLOC_ALIGN_P(allocator, size, alignment) csp::memory::Allocate(size, alignment, allocator)

#define CSP_REALLOC(ptr, size) csp::memory::Reallocate(ptr, size, std::align_val_t(16), csp::memory::DefaultAllocator())
#define CSP_REALLOC_ALIGN(ptr, size, alignment) csp::memory::Reallocate(ptr, size, alignment, csp::memory::DefaultAllocator())
#define CSP_REALLOC_P(allocator, size) csp::memory::Reallocate(ptr, size, std::align_val_t(16), allocator)
#define CSP_REALLOC_ALIGN_P(allocator, size, alignment) csp::memory::Reallocate(ptr, size, alignment, allocator)

#define CSP_NEW new (csp::memory::DefaultAllocator())
#define CSP_NEW_P(allocator) new (allocator)
#define CSP_NEW_ALIGN_P(allocator, alignment) new (alignment, allocator)

#define CSP_FREE(ptr) csp::memory::Deallocate(ptr)
#define CSP_FREE_P(ptr, allocator) csp::memory::Deallocate(ptr, allocator)

#define CSP_DELETE(ptr) csp::memory::Delete(ptr)
#define CSP_DELETE_P(ptr, allocator) csp::memory::Delete(ptr, allocator)
#define CSP_DELETE_ARRAY(ptr) csp::memory::DeleteArray(ptr)

#endif // #if !CSP_MEMORY_TRACKING_ENABLED

template <class T> struct OlyDeleter
{
    void operator()(T* Ptr) { CSP_DELETE(Ptr); }
};

#if CSP_MEMORY_OVERRIDE_GLOBAL_NEW
void* operator new(size_t size);
void* operator new(size_t size, std::align_val_t alignment);
void* operator new[](size_t size);
void* operator new[](size_t size, std::align_val_t alignment);
#endif

void* operator new(std::size_t size, csp::memory::Allocator* Allocator);
void* operator new(std::size_t size, std::align_val_t Alignment, csp::memory::Allocator* Allocator);
void* operator new[](std::size_t size, csp::memory::Allocator* Allocator);
void* operator new[](std::size_t size, std::align_val_t Alignment, csp::memory::Allocator* Allocator);

#if CSP_MEMORY_OVERRIDE_GLOBAL_NEW
void operator delete(void* Ptr);
void operator delete(void* Ptr, std::align_val_t Alignment);
void operator delete[](void* Ptr);
void operator delete[](void* Ptr, std::align_val_t Alignment);
#endif

void operator delete(void* Ptr, csp::memory::Allocator* Allocator);
void operator delete(void* Ptr, std::align_val_t Alignment, csp::memory::Allocator* Allocator);
void operator delete[](void* Ptr, csp::memory::Allocator* Allocator);
void operator delete[](void* Ptr, std::align_val_t Alignment, csp::memory::Allocator* Allocator);

// For EASTL
void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line);
void* operator new[](
    size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags, const char* file, int line);
