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
#include "SimpleRingBuffer.h"

#include "Memory/Memory.h"

#include <algorithm>

namespace csp::memory
{

SimpleRingBuffer::SimpleRingBuffer(size_t BufferSize)
    : BufferSize(BufferSize)
    , Buffer(CSP_NEW unsigned char[BufferSize])
    , ReadPosition(0)
    , WritePosition(0)
    , AvailableDataLength(0)
{
}

SimpleRingBuffer::~SimpleRingBuffer() { CSP_DELETE_ARRAY(Buffer); }

size_t SimpleRingBuffer::Read(void* OutBuffer, size_t Length)
{
    if (AvailableDataLength == 0)
    {
        return 0;
    }

    size_t Available = std::min(AvailableDataLength, Length);

    if (ReadPosition + Length <= BufferSize)
    {
        std::memcpy(OutBuffer, Buffer + ReadPosition, Available);
        ReadPosition += Available;

        if (ReadPosition == BufferSize)
        {
            ReadPosition = 0;
        }
    }
    else
    {
        size_t First = BufferSize - ReadPosition;
        size_t Second = Length - First;

        std::memcpy(OutBuffer, Buffer + ReadPosition, First);
        std::memcpy((unsigned char*)OutBuffer + First, Buffer, Second);

        ReadPosition = Second;
    }

    AvailableDataLength -= Available;

    return Available;
}

void SimpleRingBuffer::Write(const void* InBuffer, size_t Length)
{
    assert(AvailableDataLength + Length <= BufferSize && "Buffer full!");

    if (WritePosition + Length <= BufferSize)
    {
        std::memcpy(Buffer + WritePosition, InBuffer, Length);
        WritePosition += Length;

        if (WritePosition == BufferSize)
        {
            WritePosition = 0;
        }
    }
    else
    {
        size_t First = BufferSize - WritePosition;
        size_t Second = Length - First;

        std::memcpy(Buffer + WritePosition, InBuffer, First);
        std::memcpy(Buffer, (const unsigned char*)InBuffer + First, Second);

        WritePosition = Second;
    }

    AvailableDataLength += Length;
}

void SimpleRingBuffer::Skip(size_t ByteCount)
{
    AvailableDataLength -= ByteCount;

    if (ReadPosition + ByteCount <= BufferSize)
    {
        ReadPosition += ByteCount;
    }
    else
    {
        size_t End = ReadPosition + ByteCount;
        ReadPosition = End - BufferSize;
    }
}

void SimpleRingBuffer::Rewind(size_t ByteCount)
{
    AvailableDataLength += ByteCount;

    if (ReadPosition < ByteCount)
    {
        ReadPosition = BufferSize - (ByteCount - ReadPosition);
    }
    else
    {
        ReadPosition -= ByteCount;
    }
}

} // namespace csp::memory
