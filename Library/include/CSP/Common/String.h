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

#include "CSP/CSPCommon.h"
#include "CSP/Common/List.h"
#include "CSP/Common/Optional.h"

CSP_NO_EXPORT

#define CSP_TEXT(txt) csp::common::String(txt)

namespace csp::common
{

/// @brief Custom string class that we can use safely across a DLL boundary.
class CSP_API String
{
public:
    /// @brief Constructs an empty string.
    String();

    /// @brief Destructor.
    /// Frees string memory.
    ~String();

    /// @brief Constructs a string from a pointer with a given length.
    /// @param Text const char* : Pointer to a string buffer to copy data from
    /// @param Length size_t : Size of buffer
    explicit String(char const* const Text, size_t Length);

    /// @brief Constructs a string with a given length.
    /// Buffer is set to 0 for debug builds.
    /// Buffer isn't guaranteed to be set to a particular value for release builds.
    /// @param Length size_t : Size of buffer
    explicit String(size_t Length);

    /// @brief Constructs a string from a cstring.
    /// In buffer is treated as a cstring and will assume the end of the buffer is the first /0.
    /// @param Text const char* : Pointer to a string buffer to copy data from
    String(const char* Text);

    /// @brief Copy constructor.
    /// @param Other String const&
    String(String const& Other);

    /// @brief Move constructor
    String(String&& Other);

    /// @brief Copy assignment.
    /// @param Rhs const String&
    /// @return String&
    String& operator=(const String& Rhs);

    /// @brief Move assignment.
    /// @param Rhs const String&
    /// @return String&
    String& operator=(String&& Rhs);

    /// @brief Assigns a cstring to the string.
    /// @param Text const char* : Pointer to a string buffer to copy data from
    String& operator=(char const* const Text);

    /// @brief Swaps string data with given string.
    /// @param Other String& : String to swap data with
    String& swap(String& Other);

    /// @brief Auto converts to cstring by returning internal buffer.
    operator char const*() const { return c_str(); }

    /// @brief Returns internal buffer.
    /// @return char const*
    char const* c_str() const { return Get(); }

    // TODO: Possibly switch this to returning an array of StringView

    /// @brief Splits current string by a given delimiter into individual elements.
    /// @param Delimiter char
    /// @return csp::common::List<csp::common::String>
    List<String> Split(char Delimiter) const;

    bool operator==(const String& Other) const;
    bool operator==(const char* Other) const;
    bool operator!=(const String& Other) const;
    bool operator!=(const char* Other) const;
    bool operator<(const String& Other) const;

    /// @brief Appends given string.
    /// This will resize the buffer of the current string.
    /// @param Other const String& : String to append
    void Append(const String& Other);

    /// @brief Appends given cstring.
    /// This will resize the buffer of the current string.
    /// @param Other const char* : Cstring to append
    void Append(const char* Other);

    /// @brief Returns a new string created by appending rhs string to lhs string.
    /// @param Lhs String : String to append to
    /// @param Rhs const String& : String to append
    /// @return String
    friend String operator+(String Lhs, const String& Rhs)
    {
        String Result = Lhs;
        Result.Append(Rhs);

        return Result;
    }

    /// @brief Returns a new string created by appending rhs cstring to lhs string.
    /// @param Lhs String : String to append to
    /// @param Rhs const char* : Cstring to append
    /// @return String
    friend String operator+(String Lhs, const char* Rhs)
    {
        String Result = Lhs;
        Result.Append(Rhs);

        return Result;
    }

    /// @brief Appends given string.
    /// This will resize the buffer of the current string.
    /// @param Other const String& : String to append
    /// @return String&
    String& operator+=(const String& Other);

    /// @brief Appends given cstring.
    /// This will resize the buffer of the current string.
    /// @param Other const char* : Cstring to append
    /// @return String&
    String& operator+=(const char* Other);

    /// @brief Returns the length of the string.
    /// @return size_t
    size_t Length() const;

    /// @brief Returns the length of the string including the terminator.
    /// @return size_t
    size_t AllocatedMemorySize() const;

    /// @brief Checks if the string has any data.
    /// @return bool : Returns true if buffer size > 0
    bool IsEmpty() const;

    /// @brief Returns a copy of this string with all leading and trailing whitespace removed.
    /// @return String : A copy of this string with leading and trailing whitespace removed.
    String Trim() const;

    /// @brief Returns a copy of this string with all characters converted to lower-case.
    /// @return String : A copy of this string with characters converted to lower-case.
    String ToLower() const;

    /// @brief Concatenates all elements in the list with a separator after each element and returns as a string.
    /// @param Parts const csp::common::List<String>& : List to concatenate
    /// @param Separator csp::common::Optional<char> : An optional separator to add after each concatenated element
    /// @return String
    static String Join(const List<String>& Parts, Optional<char> Separator = nullptr);

    /// @brief Concatenates all elements in the initializer_list and returns as a string.
    /// @param Parts const std::initializer_list<String>& : initializer_list to concatenate
    /// @param Separator csp::common::Optional<char> : An optional separator to add after each concatenated element
    /// @return String
    static String Join(const std::initializer_list<String>& Parts, Optional<char> Separator = nullptr);

    ///  @brief Checks if the string contains the given substring.
    ///  @param Substring const String& : Substring to search for.
    ///  @return bool : Returns true if the substring is found. Always returns false if the substring is empty.
    bool Contains(const String& Substring) const;

private:
    /// @brief Returns internal buffer.
    /// @return const char*
    const char* Get() const;

private:
    class Impl;
    Impl* ImplPtr;
};

} // namespace csp::common
