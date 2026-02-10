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
#include "Multiplayer/ComponentBaseKeys.h"
#include "Multiplayer/Script/ComponentBinding/ComponentExtensionsScriptInterface.h"
#include "Debug/Logging.h"

namespace csp::multiplayer
{

static const csp::common::ReplicatedValue InvalidProperty = csp::common::ReplicatedValue();

// Component properties are expressed in a few ways:
// 1. By the component itself, following a pattern of 0...n explicit property declarations, where each property key is a monotonically increasing integer from 0.
// 2. By ComponentBase, which registers a name property, with a key of COMPONENT_KEY_NAME (64511).
// 3. And here, by this extension mechanism, which allows for properties to be added to a component without modifying the component's class.
// 
// In order to do this, the extension mechanism needs to compute keys at runtime based on string identifiers, without generating collisions with the properties expressed by (1) and (2).
// 
// So we use a hashing function to map string keys to uint32_t keys, reserve a range of keys at the start of the uint32_t space to avoid collisions with current and future explicit component properties (1),
// and include logic to avoid collisions with 'forbidden' keys.
uint32_t HashPropertyKey(const csp::common::String& PropertyKey, size_t ReservedRange, const std::vector<uint32_t>& ForbiddenKeys)
{
    // Core Hash: FNV-1a (32-bit, cross-platform)
    uint32_t Hash = 0x811C9DC5;
    const uint32_t Prime = 0x01000193;
    const unsigned char* CharPtr = reinterpret_cast<const unsigned char*>(PropertyKey.c_str());
    while (*CharPtr)
    {
        Hash ^= static_cast<uint32_t>(*CharPtr++);
        Hash *= Prime;
    }

    // Prepare Exclusions
    uint32_t ReservedRange32 = (ReservedRange >= 0xfffffffd) ? 0xfffffffc : static_cast<uint32_t>(ReservedRange);

    // Remove any forbidden keys that would fall within the reserved range, as those are already being excluded by the reserved range logic.
    std::vector<uint32_t> FilteredForbiddenKeys;
    for (uint32_t ForbiddenKey : ForbiddenKeys)
    {
        if (ForbiddenKey > ReservedRange32)
        {
            FilteredForbiddenKeys.emplace_back(ForbiddenKey);
        }
    }

    // Calculate number of available hashes after exclusions...
    // Total 32-bit values: 2^32 (4294967296) - aka 0x100000000ull
    // Offset range to skip: [0...ReservedRange32] so size is (ReservedRange32 + 1)
    // Extra constants to skip: FilteredForbiddenKeys.size()
    uint64_t NumExcluded = static_cast<uint64_t>(ReservedRange32) + 1 + FilteredForbiddenKeys.size();
    uint64_t NumAvailableHashes = 0x100000000ull - NumExcluded; // max 32bit int - excluded count

    // Map our 32-bit hash into [0, NumAvailableHashes - 1] - Lemire method
    uint32_t Result = static_cast<uint32_t>((static_cast<uint64_t>(Hash) * NumAvailableHashes) >> 32);
    Result += (ReservedRange32 + 1); // Offset hash to avoid collisions with reserved range.

    // Now we handle the forbidden keys. We need to skip over any forbidden keys that are less than or equal to our result.
    std::sort(FilteredForbiddenKeys.begin(), FilteredForbiddenKeys.end());
    // Because ForbiddenKeys is sorted, we can increment 'result' for every forbidden value that is less than or equal to it.
    for (uint32_t ForbiddenKey : FilteredForbiddenKeys)
    {
        if (Result >= ForbiddenKey)
        {
            Result++;
        }
        else
        {
            break;
        }
    }

    return Result;
}

ComponentExtensions::ComponentExtensions()
    : ExtendedComponent(nullptr)
    , ReservedPropertyRange(0)
    , ScriptInterface(nullptr)
{
}

ComponentExtensions::ComponentExtensions(ComponentBase* InComponentToExtend)
    : ExtendedComponent(InComponentToExtend)
    , ReservedPropertyRange(0)
    , ScriptInterface(nullptr)
{
    if (ExtendedComponent != nullptr)
    {
        ReservedPropertyRange = csp::multiplayer::MAX_RESERVED_COMPONENT_PROPERTY_COUNT;
        
        // No property key can be used if it is already in use by the component.
        const csp::common::Map<uint32_t, csp::common::ReplicatedValue>* ComponentProperties = ExtendedComponent->GetProperties();
        for (const auto& [Key, PropertyValue] : *ComponentProperties)
        {
            ForbiddenKeys.emplace_back(Key);
        }

        ScriptInterface = new ComponentExtensionsScriptInterface(this);
    }
    else
    {
        CSP_LOG_MSG(csp::common::LogLevel::Error, "A component extension was initialized with a null ExtendedComponent. This indicates a logical error during component initialization.");
    }
}

ComponentExtensions::ComponentExtensions(const ComponentExtensions& Other)
    : ExtendedComponent(Other.ExtendedComponent)
    , ReservedPropertyRange(Other.ReservedPropertyRange)
    , ScriptInterface(nullptr)
{
    *this = Other;
}

ComponentExtensions& ComponentExtensions::operator=(const ComponentExtensions& Other)
{
    ExtendedComponent = Other.ExtendedComponent;
    ReservedPropertyRange = Other.ReservedPropertyRange;
    ForbiddenKeys = Other.ForbiddenKeys;

    // Since script interface memory is freed on destructor, we need to make a new one here, to avoid multiple instances
    // pointing to the same script interface memory.
    if (Other.ScriptInterface != nullptr)
    {
        ScriptInterface = new ComponentExtensionsScriptInterface(this);
    }

    return *this;
}

ComponentExtensions::~ComponentExtensions()
{
    delete ScriptInterface;
}

const csp::common::ReplicatedValue& ComponentExtensions::GetProperty(const csp::common::String& Key) const
{
    if (ExtendedComponent != nullptr)
    {
        const uint32_t HashedKey = HashPropertyKey(Key, ReservedPropertyRange, ForbiddenKeys);
        return ExtendedComponent->GetProperty(HashedKey);
    }
    else
    {
        CSP_LOG_MSG(csp::common::LogLevel::Error,
            "Attempted to get a property from a component extension that has a null ExtendedComponent. This indicates a logical error during "
            "component initialization.");
    }

    return InvalidProperty;
}

void ComponentExtensions::SetProperty(const csp::common::String& Key, const csp::common::ReplicatedValue& Value)
{
    if (ExtendedComponent != nullptr)
    {
        const uint32_t HashedKey = HashPropertyKey(Key, ReservedPropertyRange, ForbiddenKeys);
        ExtendedComponent->SetProperty(HashedKey, Value);
    }
    else
    {
        CSP_LOG_MSG(csp::common::LogLevel::Error,
            "Attempted to set a property from a component extension that has a null ExtendedComponent. This indicates a logical error during "
            "component initialization.");
    }
}

bool ComponentExtensions::HasProperty(const csp::common::String& Key) const
{
    return ExtendedComponent != nullptr && GetProperty(Key) != InvalidProperty;
}

ComponentBase* ComponentExtensions::GetExtendedComponent() const
{
    return ExtendedComponent;
}

ComponentExtensionsScriptInterface* ComponentExtensions::GetScriptInterface() const
{
    return ScriptInterface;
}

} // namespace csp::multiplayer 
