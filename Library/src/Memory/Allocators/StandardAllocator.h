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

#ifndef EA_PLATFORM_PTR_SIZE
#if defined(__WORDSIZE) // Defined by some variations of GCC.
#define EA_PLATFORM_PTR_SIZE ((__WORDSIZE) / 8)
#elif defined(_WIN64) || defined(__LP64__) || defined(_LP64) || defined(_M_IA64) || defined(__ia64__) || defined(__arch64__) || defined(__aarch64__) \
    || defined(__mips64__) || defined(__64BIT__) || defined(__Ptr_Is_64)
#define EA_PLATFORM_PTR_SIZE 8
#elif defined(__CC_ARM) && (__sizeof_ptr == 8)
#define EA_PLATFORM_PTR_SIZE 8
#else
#define EA_PLATFORM_PTR_SIZE 4
#endif
#endif

#include "Memory/Allocator.h"
#include "Memory/LockTraits.h"

#include <assert.h>
#include <atomic>

constexpr std::align_val_t CSP_ALLOCATOR_MIN_ALIGNMENT = std::align_val_t(16);

#ifndef USE_STD_MALLOC
#define USE_STD_MALLOC 0 // 1 for std::malloc or 0 for new MiMalloc lockless allocator
#endif

#if USE_STD_MALLOC

#define STD_ALLOCATOR_MALLOC std::malloc
#define STD_ALLOCATOR_FREE std::free
#define STD_ALLOCATOR_REALLOC std::realloc
#define STD_ALLOCATOR_TRAIT MutexLockTrait

#else

#include "mimalloc.h"

// Use Mimalloc (https://github.com/microsoft/mimalloc). Fast lock-free allocator from Microsoft
#define STD_ALLOCATOR_MALLOC mi_malloc
#define STD_ALLOCATOR_FREE mi_free
#define STD_ALLOCATOR_REALLOC mi_realloc
#define STD_ALLOCATOR_TRAIT NoLockTrait

#endif

namespace csp::memory
{

/// StandardAllocator class
///
/// Simple default allocator type that just uses malloc and free
///
///
template <typename TLockTrait = STD_ALLOCATOR_TRAIT> class StandardAllocator : public Allocator
{
public:
    StandardAllocator();
    virtual ~StandardAllocator();

    void* Allocate(size_t Bytes) override;
    void* Allocate(size_t Bytes, std::align_val_t Alignment) override;
    void* Reallocate(void* Ptr, size_t Bytes) override;
    void* Reallocate(void* Ptr, size_t Bytes, std::align_val_t Alignment) override;
    void Deallocate(void* Ptr) override;
    void Deallocate(void* Ptr, size_t Bytes) override;

    const size_t GetAllocatedBytes() const override;

private:
    std::atomic<size_t> AllocatedBytes;
    TLockTrait AllocMutex;
};

template <typename TLockTrait>
StandardAllocator<TLockTrait>::StandardAllocator()
    : AllocatedBytes(0)
{
}

template <typename TLockTrait> StandardAllocator<TLockTrait>::~StandardAllocator() { }

template <typename TLockTrait> void* StandardAllocator<TLockTrait>::Allocate(size_t n) { return Allocate(n, CSP_ALLOCATOR_MIN_ALIGNMENT); }

template <typename TLockTrait> void* StandardAllocator<TLockTrait>::Allocate(size_t n, std::align_val_t alignment)
{
    AllocatedBytes += n;

    AllocMutex.Lock();

#ifdef CSP_WASM
    void* p = STD_ALLOCATOR_MALLOC(n);
#else
    // This is taken from EASTL\include\EASTL\allocator.h

    size_t adjustedAlignment = (size_t(alignment) > EA_PLATFORM_PTR_SIZE) ? size_t(alignment) : EA_PLATFORM_PTR_SIZE;

    void* p = STD_ALLOCATOR_MALLOC(n + adjustedAlignment + EA_PLATFORM_PTR_SIZE);
    void* pPlusPointerSize = (void*)((uintptr_t)p + EA_PLATFORM_PTR_SIZE);
    void* pAligned = (void*)(((uintptr_t)pPlusPointerSize + adjustedAlignment - 1) & ~(adjustedAlignment - 1));

    void** pStoredPtr = (void**)pAligned - 1;
    assert(pStoredPtr >= p);
    *(pStoredPtr) = p;

    assert(((size_t)pAligned & ~(size_t(alignment) - 1)) == (size_t)pAligned);
#endif

    AllocMutex.Unlock();

#ifdef CSP_WASM
    return p;
#else
    return pAligned;
#endif
}

template <typename TLockTrait> void* StandardAllocator<TLockTrait>::Reallocate(void* p, size_t n)
{
    return Reallocate(p, n, CSP_ALLOCATOR_MIN_ALIGNMENT);
}

template <typename TLockTrait> void* StandardAllocator<TLockTrait>::Reallocate(void* p, size_t n, std::align_val_t alignment)
{
    // TODO increment with the difference between the old allocation and the new one
    // AllocatedBytes += n;

    AllocMutex.Lock();

#ifdef CSP_WASM
    void* pNew = STD_ALLOCATOR_REALLOC(p, n);
#else
    size_t adjustedAlignment = (size_t(alignment) > EA_PLATFORM_PTR_SIZE) ? size_t(alignment) : EA_PLATFORM_PTR_SIZE;

    void* pOriginalAllocation = *((void**)p - 1);
    void* pNew = STD_ALLOCATOR_REALLOC(pOriginalAllocation, n + adjustedAlignment + EA_PLATFORM_PTR_SIZE);
    void* pPlusPointerSize = (void*)((uintptr_t)pNew + EA_PLATFORM_PTR_SIZE);
    void* pAligned = (void*)(((uintptr_t)pPlusPointerSize + adjustedAlignment - 1) & ~(adjustedAlignment - 1));

    void** pStoredPtr = (void**)pAligned - 1;
    assert(pStoredPtr >= pNew);
    *(pStoredPtr) = pNew;

    assert(((size_t)pAligned & ~(size_t(alignment) - 1)) == (size_t)pAligned);
#endif

    AllocMutex.Unlock();

#ifdef CSP_WASM
    return pNew;
#else
    return pAligned;
#endif
}

template <typename TLockTrait> void StandardAllocator<TLockTrait>::Deallocate(void* p, size_t n)
{
    AllocatedBytes -= n;
    Deallocate(p);
}

template <typename TLockTrait> void StandardAllocator<TLockTrait>::Deallocate(void* p)
{
    if (p != nullptr)
    {
        AllocMutex.Lock();

#ifdef CSP_WASM
        STD_ALLOCATOR_FREE(p);
#else
        void* pOriginalAllocation = *((void**)p - 1);
        STD_ALLOCATOR_FREE(pOriginalAllocation);
#endif

        AllocMutex.Unlock();
    }
}

template <typename TLockTrait> const size_t StandardAllocator<TLockTrait>::GetAllocatedBytes() const { return AllocatedBytes; }

} // namespace csp::memory
