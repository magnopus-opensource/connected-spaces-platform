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

#include "Allocator.h"
#include "Allocators/StandardAllocator.h"

namespace csp::memory
{

/// Manager class for memory handling
///
/// Provides access to vaious allocators and heaps that are optimised
/// for different situations
///
class MemoryManager
{
public:
    static void Initialise();
    static void Shutdown();

    static csp::memory::Allocator& GetDefaultAllocator();

    // To do :- More Allocator types here. E.g. optimised for Small, Medium, Large allocations.
    // Stack/Frame allocators.  Scratch Memory etc

private:
    using MultiThreadStandardAllocator = csp::memory::StandardAllocator<MutexLockTrait>;

    static MultiThreadStandardAllocator OlyDefaultAllocator;
};

} // namespace csp::memory
