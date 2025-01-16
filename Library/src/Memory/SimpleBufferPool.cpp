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
#include "SimpleBufferPool.h"

#include "Memory/Memory.h"

namespace csp::memory
{

SimpleBufferPool::SimpleBufferPool(size_t BufferSize, size_t InitialPoolSize)
    : BufferSize(BufferSize)
{
    Buffers.reserve(InitialPoolSize);

    for (int i = 0; i < InitialPoolSize; ++i)
    {
        Buffers[i] = CSP_NEW unsigned char[BufferSize];
    }
}

SimpleBufferPool::~SimpleBufferPool()
{
    for (auto Buffer : Buffers)
    {
        CSP_DELETE_ARRAY(Buffer);
    }

    Buffers.clear();
}

unsigned char* SimpleBufferPool::Rent()
{
    unsigned char* Buffer = nullptr;

    LockMutex.lock();
    {
        if (Buffers.size() > 0)
        {
            Buffer = Buffers.back();
            Buffers.pop_back();
        }
    }
    LockMutex.unlock();

    if (Buffer == nullptr)
    {
        return CSP_NEW unsigned char[BufferSize];
    }

    return Buffer;
}

void SimpleBufferPool::Return(unsigned char* Buffer)
{
    LockMutex.lock();
    {
        Buffers.push_back(Buffer);
    }
    LockMutex.unlock();
}

} // namespace csp::memory
