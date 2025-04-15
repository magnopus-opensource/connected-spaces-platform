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
#include "Memory/Memory.h"

#if CSP_MEMORY_OVERRIDE_GLOBAL_NEW
void* operator new(size_t size) { return csp::memory::DefaultAllocator()->Allocate(size); }

void* operator new(size_t size, std::align_val_t alignment) { return csp::memory::DefaultAllocator()->Allocate(size, alignment); }

void* operator new[](size_t size) { return csp::memory::DefaultAllocator()->Allocate(size); }

void* operator new[](size_t size, std::align_val_t alignment) { return csp::memory::DefaultAllocator()->Allocate(size, alignment); }
#endif

void* operator new(std::size_t Size, csp::memory::Allocator* Allocator) { return Allocator->Allocate(Size); }

void* operator new(std::size_t Size, std::align_val_t Alignment, csp::memory::Allocator* Allocator) { return Allocator->Allocate(Size, Alignment); }

void* operator new[](std::size_t Size, csp::memory::Allocator* Allocator)
{
    // We need to store the size of the allocated array so that we only call destructors on valid elements.
    // The size of the buffer passed back to us by malloc or mimalloc might be larger than the requested size.
    auto buffer = Allocator->Allocate(Size + sizeof(size_t));
    *(size_t*)buffer = Size;

    return (char*)buffer + sizeof(size_t);
}

void* operator new[](std::size_t Size, std::align_val_t Alignment, csp::memory::Allocator* Allocator)
{
    auto buffer = Allocator->Allocate(Size + sizeof(size_t), Alignment);
    *(size_t*)buffer = Size;

    return (char*)buffer + sizeof(size_t);
}

#if CSP_MEMORY_OVERRIDE_GLOBAL_NEW
void operator delete(void* Ptr) { csp::memory::Deallocate(Ptr); }

void operator delete(void* Ptr, size_t size) { csp::memory::Deallocate(Ptr, size); }
void operator delete[](void* Ptr) { csp::memory::Deallocate(Ptr); }

void operator delete[](void* Ptr, size_t size) { csp::memory::Deallocate(Ptr, size); }
#endif

void operator delete(void* Ptr, csp::memory::Allocator* Allocator) { csp::memory::Deallocate(Ptr, Allocator); }

void operator delete(void* Ptr, std::align_val_t /*Alignment*/, csp::memory::Allocator* Allocator) { csp::memory::Deallocate(Ptr, Allocator); }

void operator delete[](void* Ptr, csp::memory::Allocator* Allocator) { csp::memory::Deallocate(Ptr, Allocator); }

void operator delete[](void* Ptr, std::align_val_t /*Alignment*/, csp::memory::Allocator* Allocator) { csp::memory::Deallocate(Ptr, Allocator); }

// Overrides for EASTL Debug builds when using the standard allocator
// Note that these needs to call global new/malloc, since they will be deleted with delete[] in standard EASTL allocator
void* operator new[](size_t size, const char* /*pName*/, int /*flags*/, unsigned /*debugFlags*/, const char* /*file*/, int /*line*/)
{
    return std::malloc(size);
}

void* operator new[](size_t size, size_t /*alignment*/, size_t /*alignmentOffset*/, const char* /*pName*/, int /*flags*/, unsigned /*debugFlags*/,
    const char* /*file*/, int /*line*/)
{
    return std::malloc(size);
}
