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
#include <vector>

namespace csp::memory
{

/// @brief Extremely simple (read: "naïve") pool for fixed-size buffers.
/// Uses a std::mutex for thread-safe borrowing and returning.
class SimpleBufferPool
{
public:
    SimpleBufferPool(size_t BufferSize, size_t InitialPoolSize = 5);
    ~SimpleBufferPool();

    unsigned char* Rent();
    void Return(unsigned char* Buffer);

private:
    size_t BufferSize;
    std::vector<unsigned char*> Buffers;
    std::mutex LockMutex;
};

} // namespace csp::memory
