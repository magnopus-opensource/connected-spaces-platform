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

#include "CSP/Common/Array.h"
#include "Convert.h"

#include <algorithm>
#include <functional>

namespace csp::common
{

/// @brief Finds a given value in an array.
/// @param Array const csp::common::Array<T>& : Array to search
/// @param Value const T& : Value to search for
/// @return int : Index of first found element or -1 if not found
template <typename T> int Find(const Array<T>& Array, const T& Value)
{
    for (size_t i = 0; i < Array.Size(); ++i)
    {
        if (Array[i] == Value)
        {
            return i;
        }
    }

    return -1;
}

/// @brief Finds a value in the array by using the provided callback.
/// Callback structure should be: bool(const T&).
/// If the callback returns true for a value, this signals that this value meets the find criteria.
/// @param Array const csp::common::Array<T>& : Array to search
/// @param Callback const F& : Function used to signal whether am array element matches search criteria
/// @return int : Index of first found element or -1 if not found
template <typename T, typename F> int FindIf(const Array<T>& Array, const F& Callback)
{
    for (size_t i = 0; i < Array.Size(); ++i)
    {
        if (Callback(Array[i]))
        {
            return static_cast<int>(i);
        }
    }

    return -1;
}

/// @brief Sorts array using the provided callback.
/// Callback structure should be: bool(const T&, const T&).
/// If the callback returns true for a pair of values, this singnals that value 1 should come before value 2.
/// @param Array csp::common::Array<T>& : Array to sort
/// @param Callback const F& : Function used to singal whether value 1 should come before value 2
template <typename T, typename F> void Sort(Array<T>& Array, const F& Callback)
{
    std::vector<T> Vec = Convert(Array);
    std::sort(Vec.begin(), Vec.end(), Callback);

    Array = Convert(Vec);
}
} // namespace csp::common
