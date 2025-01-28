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

#include "MemoryManager.h"

#include "CSP/CSPCommon.h"
#include "Memory.h"

namespace csp::memory
{

#if defined(CSP_WINDOWS)
// (initializers put in library initialization area) Not at all sure why this is here, hopefully we can delete the custom memory management because no
// one really understands it.
#pragma warning(disable : 4073)
#pragma init_seg(lib)
#else
__attribute__((init_priority(101)))
#endif

MemoryManager::MultiThreadStandardAllocator MemoryManager::OlyDefaultAllocator;

void MemoryManager::Initialise() { }

void MemoryManager::Shutdown() { }

csp::memory::Allocator& MemoryManager::GetDefaultAllocator() { return OlyDefaultAllocator; }

} // namespace csp::memory
