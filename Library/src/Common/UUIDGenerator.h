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

/// @brief Generates a uuid
/// @return std::string
inline std::string GenerateUUID()
{
    // Generate a random UUID by combining 2 64-bit unsigned integers
    uint8_t Uuid[16];

    // Use this as rand() only offers 15 bits of randomness
    std::mt19937_64 Rand;
    auto CurrentTime = std::chrono::high_resolution_clock::now();
    auto CurrentNanoseconds = std::chrono::time_point_cast<std::chrono::nanoseconds>(CurrentTime);
    Rand.seed(CurrentNanoseconds.time_since_epoch().count());
    *reinterpret_cast<uint64_t*>(&Uuid[0]) = Rand();
    // Re-seed for extra randomness
    CurrentTime = std::chrono::high_resolution_clock::now();
    CurrentNanoseconds = std::chrono::time_point_cast<std::chrono::nanoseconds>(CurrentTime);
    Rand.seed(CurrentNanoseconds.time_since_epoch().count());
    [[maybe_unused]] auto SkippedVal = Rand(); // Skip one pseudo-random number
    *reinterpret_cast<uint64_t*>(&Uuid[8]) = Rand();

    // Convert to hex string
    std::ostringstream UUIDStringStream;
    UUIDStringStream << std::hex << std::uppercase << std::setfill('0');

    for (const auto uuid : Uuid)
    {
        UUIDStringStream << std::setw(2) << static_cast<int>(uuid);
    }

    return UUIDStringStream.str();
}
}; // namespace csp
