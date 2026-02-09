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
#include "CSP/Multiplayer/ComponentExtensions.h"

namespace csp::multiplayer
{

static const csp::common::ReplicatedValue InvalidProperty = csp::common::ReplicatedValue();

uint32_t HashPropertyKey(const csp::common::String& PropertyKey, size_t ReservedRange)
{
    // FNV-1a 32-bit Hash - produces the same 32 bit result on any architecture.     
    uint32_t Hash = 0x811C9DC5;
    const uint32_t Prime = 0x01000193;

    // Process byte-by-byte to avoid endianness issues
    const unsigned char* CharPtr = reinterpret_cast<const unsigned char*>(PropertyKey.c_str());
    while (*CharPtr)
    {
        Hash ^= static_cast<uint32_t>(*CharPtr++);
        Hash *= Prime;
    }

    // Clamp range to max 32-bit value if necessary.
    uint32_t ReservedRange32 = ReservedRange >= 0xffffffff ? ReservedRange32 = 0xfffffffe : ReservedRange32 = static_cast<uint32_t>(ReservedRange);

    // Linearly map the hash to the range of valid property keys for extensions, which is [n + 1, 0xffffffff].
    // The size of our available space is L = (max - (n + 1) + 1)
    const uint32_t RangeSize = 0xffffffff - ReservedRange32;

    // We use a 64-bit intermediate to map the 32-bit hash into the range [0, L-1] and then offset it by (n + 1).
    // This preserves the distribution quality of the original hash.
    const uint32_t RemappedHash = static_cast<uint32_t>((static_cast<uint64_t>(Hash) * RangeSize) >> 32);

    return RemappedHash + (ReservedRange32 + 1);
}

ComponentExtensions::ComponentExtensions()
    : ExtendedComponent(nullptr)
    , ReservedPropertyRange(0)
{
    
}

ComponentExtensions::ComponentExtensions(ComponentBase* InComponentToExtend)
    : ExtendedComponent(InComponentToExtend)
    , ReservedPropertyRange(0)
{
    if (ExtendedComponent != nullptr)
    {
        ReservedPropertyRange = ExtendedComponent->GetNumProperties();
    }
}

ComponentExtensions::~ComponentExtensions()
{

}

const csp::common::ReplicatedValue& ComponentExtensions::GetProperty(const csp::common::String& Key) const
{
    if (ExtendedComponent != nullptr)
    {
        const uint32_t HashedKey = HashPropertyKey(Key, ReservedPropertyRange);
        return ExtendedComponent->GetProperty(HashedKey);
    }

    return InvalidProperty;
}

void ComponentExtensions::SetProperty(const csp::common::String& Key, const csp::common::ReplicatedValue& Value)
{
    if (ExtendedComponent != nullptr)
    {
        const uint32_t HashedKey = HashPropertyKey(Key, ReservedPropertyRange);
        ExtendedComponent->SetProperty(HashedKey, Value);
    }
}

bool ComponentExtensions::HasProperty(const csp::common::String& Key) const
{
    return ExtendedComponent != nullptr && GetProperty(Key) != InvalidProperty;
}

} // namespace csp::multiplayer 
