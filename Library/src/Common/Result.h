/*
 * Copyright 2026 Magnopus LLC

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

#include <string>
#include <type_traits>
#include <utility>
#include <variant>

namespace csp::common
{

struct ResultError
{
    std::string Description;
};

/// @brief Either a value of type T, or an Error explaining why there isn't one. A poor substitute
/// for std::expected, which would make this unnecessary, but better than nothing until we can use it.
///
/// The error is just a string, as we only ever log these. std::expected also has monadic operations
/// (transform_error, transform, and_then, or_else etc), which we go without and unwrap by hand
/// instead. A string makes that easier to live with, as adding context at each return is just
/// formatting.
template <typename T> class [[nodiscard]] Result
{
public:
    using Error = ResultError;

    /// @brief The type held on success. std::variant cannot hold void.
    using StoredType = std::conditional_t<std::is_void_v<T>, std::monostate, T>;

    Result(StoredType InValue)
        : Storage(std::move(InValue))
    {
    }

    Result(Error InError)
        : Storage(std::move(InError))
    {
    }

    /// @brief A successful Result. The default argument is for Result<void>, which has no value to carry.
    static Result Ok(StoredType Value = {}) { return std::move(Value); }

    explicit operator bool() const { return std::holds_alternative<StoredType>(Storage); }

    StoredType& operator*() { return *std::get_if<StoredType>(&Storage); }

    const StoredType& operator*() const { return *std::get_if<StoredType>(&Storage); }

    Error& GetError() { return *std::get_if<Error>(&Storage); }

    const Error& GetError() const { return *std::get_if<Error>(&Storage); }

private:
    std::variant<StoredType, Error> Storage;
};

} // namespace csp::common
