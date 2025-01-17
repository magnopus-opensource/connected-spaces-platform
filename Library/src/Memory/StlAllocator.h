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

#include "Memory/Allocator.h"
#include "Memory/Memory.h"

namespace csp::memory
{

template <typename T> class StlAllocator
{
public:
    typedef T value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

public:
    template <typename U> struct rebind
    {
        typedef StlAllocator<U> other;
    };

public:
    inline StlAllocator()
        : Allocator { nullptr }
    {
    }

    inline StlAllocator(csp::memory::Allocator* InAllocator)
        : Allocator { InAllocator }
    {
    }

    inline ~StlAllocator() = default;
    inline StlAllocator(StlAllocator const&) = default;

    template <typename U>
    inline StlAllocator(StlAllocator<U> const& other)
        : Allocator { other.Allocator }
    {
    }

    // address
    inline pointer address(reference r) { return &r; }

    inline const_pointer address(const_reference r) { return &r; }

    // memory allocation
    inline pointer allocate(size_type cnt)
    {
        if (Allocator)
        {
            return static_cast<T*>(Allocator->Allocate(sizeof(T) * cnt));
        }

        return static_cast<T*>(csp::memory::Allocate(sizeof(T) * cnt));
    }

    inline void deallocate(pointer p, size_type cnt)
    {
        if (Allocator)
        {
            Allocator->Deallocate(p, sizeof(T) * cnt);
        }
        else
        {
            csp::memory::Deallocate(p, sizeof(T) * cnt);
        }
    }

    // size
    inline size_type max_size() const { return std::numeric_limits<size_type>::max() / sizeof(T); }

    // construction/destruction
    inline void construct(pointer p, const T& t) { new (p) T(t); }

    inline void destroy(pointer p) { p->~T(); }

    inline bool operator==(StlAllocator const& a) { return this == &a; }
    inline bool operator!=(StlAllocator const& a) { return !operator==(a); }

private:
    // Allocator used for all allocations/deallocations.
    csp::memory::Allocator* Allocator;

    // Allow all related types to access our private allocator.
    template <typename> friend class StlAllocator;
};

} // namespace csp::memory
