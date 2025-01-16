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
#include "CSP/Memory/DllAllocator.h"

#include "Memory/Memory.h"

namespace csp::memory
{

void* DllAlloc(size_t Size, size_t Alignment) { return CSP_ALLOC_ALIGN(Size, (std::align_val_t)Alignment); }

void* DllRealloc(void* Ptr, size_t NewSize, size_t Alignment) { return CSP_REALLOC_ALIGN(Ptr, NewSize, (std::align_val_t)Alignment); }

void DllFree(void* Ptr) { CSP_FREE(Ptr); }

} // namespace csp::memory
