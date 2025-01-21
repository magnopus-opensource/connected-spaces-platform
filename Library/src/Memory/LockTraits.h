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

#include <mutex>

namespace csp::memory
{

class NoLockTrait
{
public:
    inline void Lock() { }
    inline void Unlock() { }
};

class MutexLockTrait
{
public:
    inline void Lock() { Mutex.lock(); }
    inline void Unlock() { Mutex.unlock(); }

private:
    std::mutex Mutex;
};

/** Example usage
   template <typename TLockTrait = MutexLockTrait>
   class CustomAllocator : public Allocator
   {
   public:

                void* Allocate(size_t Bytes)
                {
                                void* Ptr;
                                Mutex.Lock();
                                // Allocation here
                                Mutex.Unlock();
                                return Ptr;
                }

                void Deallocate(void* Ptr)
                {
                                Mutex.Lock();
                                // De-allocation here
                                Mutex.Unlock();
                }

   private:
                TLockTrait Mutex;
   };

   using SingleThreadAllocator = CustomAllocator<NoLockTrait>;
   using MultiThreadAllocator = CustomAllocator<MutexLockTrait>;
 */

} // namespace csp::memory
