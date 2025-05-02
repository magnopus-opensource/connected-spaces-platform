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

#include <new>

namespace csp::memory
{

/// Allocator base class
///
/// Base class for memory allocators intended to be overriden
/// to provide different allocator models
///
class Allocator
{
public:
    Allocator() { }

    virtual ~Allocator() = default;

    virtual void* Allocate(size_t Bytes) = 0;
    virtual void* Allocate(size_t Bytes, std::align_val_t Alignment) = 0;
    virtual void* Reallocate(void* Ptr, size_t Bytes) = 0;
    virtual void* Reallocate(void* Ptr, size_t Bytes, std::align_val_t Alignment) = 0;
    virtual void Deallocate(void* Ptr) = 0;
    virtual void Deallocate(void* Ptr, size_t Bytes) = 0;

    virtual size_t GetAllocatedBytes() const = 0;
};

} // namespace csp::memory
