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

#include <chrono>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>

namespace csp
{

inline uint64_t RandInt()
{
    // Use this as rand() only offers 15 bits of randomness
    std::mt19937_64 rand;
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto currentNanoseconds = std::chrono::time_point_cast<std::chrono::nanoseconds>(currentTime);
    rand.seed(currentNanoseconds.time_since_epoch().count());
    return rand();
}

/// @brief Generates a uuid
/// @return std::string
inline std::string GenerateUUID()
{
    // Generate a random UUID by combining 2 64-bit unsigned integers
    uint8_t uuid[16];

    *reinterpret_cast<uint64_t*>(&uuid[0]) = RandInt();
    *reinterpret_cast<uint64_t*>(&uuid[8]) = RandInt();

    // Convert to hex string
    std::ostringstream uuidStringStream;
    uuidStringStream << std::hex << std::uppercase << std::setfill('0');

    for (const auto byte : uuid)
    {
        uuidStringStream << std::setw(2) << static_cast<int>(byte);
    }

    return uuidStringStream.str();
}
}; // namespace csp
