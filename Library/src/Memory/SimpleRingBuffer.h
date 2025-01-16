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

#include <cstddef>

namespace csp::memory
{

/// @brief A very simple fixed-size ring buffer.
/// Does not resize when full. Instead, throws an assert error.
class SimpleRingBuffer
{
public:
    SimpleRingBuffer(size_t BufferSize);
    ~SimpleRingBuffer();

    inline bool IsDataAvailable() { return AvailableDataLength > 0; }

    inline const size_t GetAvailableDataLength() { return AvailableDataLength; }

    size_t Read(void* OutBuffer, size_t Length);
    void Write(const void* InBuffer, size_t Length);
    void Skip(size_t ByteCount);
    void Rewind(size_t ByteCount);

private:
    unsigned char* Buffer;
    size_t BufferSize;
    size_t ReadPosition;
    size_t WritePosition;
    size_t AvailableDataLength;
};

} // namespace csp::memory
