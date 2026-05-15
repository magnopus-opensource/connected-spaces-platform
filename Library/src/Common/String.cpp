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

#include "CSP/Common/String.h"

#include <algorithm>
#include <cctype>

namespace csp::common
{

/**
                Custom string class that we can use safely across a DLL boundary
 */

/// @brief Internal implementation for DLL safe string class
class String::Impl
{
public:
    ~Impl() { delete[] Text; }

    explicit Impl(char const* const inText)
        : Text(nullptr)
        , Length(0)
    {
        const char* effectiveText = inText;

        if (inText == nullptr)
        {
            effectiveText = "";
        }

        const size_t len = strlen(effectiveText);

        char* newText = new char[len + 1];

        if (len > 0)
        {
            memcpy((void*)newText, effectiveText, len * sizeof(char));
        }

        newText[len] = 0;

        Text = newText;
        Length = len;
    }

    Impl(char const* const inText, size_t len)
        : Text(nullptr)
        , Length(0)
    {
        const char* effectiveText = inText;

        if (inText == nullptr || len == 0)
        {
            effectiveText = "";
            len = 0;
        }

        char* newText = new char[len + 1];

        if (len > 0)
        {
            memcpy((void*)newText, effectiveText, len * sizeof(char));
        }

        newText[len] = 0;

        Text = newText;
        Length = len;
    }

    explicit Impl(size_t len)
        : Text(nullptr)
        , Length(0)
    {
        char* newText = new char[len + 1];

        newText[len] = 0;
        Text = newText;
        Length = len;
    }

    Impl* Clone() const { return new Impl(Text, Length); }

    inline void Append(const char* other, size_t otherLength)
    {
        if (other == nullptr || otherLength == 0)
        {
            return;
        }

        auto newLength = Length + otherLength;
        auto newText = new char[newLength + 1];
        memcpy(newText, Text, Length);
        memcpy(newText + Length, other, otherLength);
        newText[newLength] = '\0';

        delete[] Text;
        Text = newText;
        Length = newLength;
    }

    void Append(const Impl& other) { Append(other.Text, other.Length); }

    void Append(const char* other)
    {
        if (other == nullptr)
        {
            return;
        }

        auto otherLength = strlen(other);
        Append(other, otherLength);
    }

    List<String> Split(char separator)
    {
        List<String> parts;

        // NOTE: Don't use strtok here because it ignores empty entries!
        auto index = strchr(Text, separator);

        if (index == nullptr)
        {
            parts.Append(Text);

            return parts;
        }

        auto start = Text;

        for (;;)
        {
            parts.Append(String(start, index - start));
            start = index + 1;
            index = strchr(start, separator);

            // Also look for null-terminator
            if (index == nullptr)
            {
                index = strchr(start, '\0');
                parts.Append(String(start, index - start));
                break;
            }
        }

        return parts;
    }

    char* Text;
    size_t Length;
};

String::String()
    : m_implPtr(new Impl(""))
{
}

String::String(char const* const text, size_t length)
    : m_implPtr(new Impl(text, length))
{
}

String::String(size_t length)
    : m_implPtr(new Impl(length))
{
}

String::String(const char* text)
    : m_implPtr(new Impl(text))
{
}

String::String(String const& other)
    : m_implPtr(other.m_implPtr->Clone())
{
}

String::String(String&& other)
{
    m_implPtr = other.m_implPtr;
    other.m_implPtr = nullptr;
}

List<String> String::Split(char separator) const { return m_implPtr->Split(separator); }

String& String::swap(String& other)
{
    std::swap(m_implPtr, other.m_implPtr);
    return *this;
}

String& String::operator=(const String& rhs)
{
    if (m_implPtr != nullptr)
    {
        delete (m_implPtr);
    }

    m_implPtr = rhs.m_implPtr->Clone();
    return *this;
}

String& String::operator=(String&& rhs)
{
    if (m_implPtr != nullptr)
    {
        delete (m_implPtr);
    }

    m_implPtr = rhs.m_implPtr;
    rhs.m_implPtr = nullptr;
    return *this;
}

String& String::operator=(char const* const text)
{
    if (m_implPtr != nullptr)
    {
        delete (m_implPtr);
    }

    m_implPtr = new Impl(text);
    return *this;
}

const char* String::Get() const { return m_implPtr->Text; }

size_t String::Length() const { return m_implPtr->Length; }

size_t String::AllocatedMemorySize() const { return m_implPtr->Length + 1; }

bool String::IsEmpty() const { return m_implPtr->Length == 0; }

bool String::operator==(const String& other) const
{
    if (m_implPtr->Length == 0 && other.Length() == 0)
    {
        return true;
    }

    if (m_implPtr->Length == 0 || other.Length() == 0)
    {
        return false;
    }

    return strcmp(Get(), other.Get()) == 0;
}

bool String::operator==(const char* other) const
{
    auto otherLength = strlen(other);

    if (m_implPtr->Length == 0 && otherLength == 0)
    {
        return true;
    }

    if (m_implPtr->Length == 0 || otherLength == 0)
    {
        return false;
    }

    return strcmp(Get(), other) == 0;
}

bool String::operator!=(const String& other) const { return !(*this == other); }

bool String::operator!=(const char* other) const { return !(*this == other); }

bool String::operator<(const String& other) const { return strcmp(Get(), other.Get()) < 0; }

String::~String()
{
    if (m_implPtr != nullptr)
    {
        delete (m_implPtr);
    }
}

void String::Append(const String& other) { m_implPtr->Append(*other.m_implPtr); }

void String::Append(const char* other) { m_implPtr->Append(other); }

String& String::operator+=(const String& other)
{
    Append(other);

    return *this;
}

String& String::operator+=(const char* other)
{
    Append(other);

    return *this;
}

String String::Trim() const
{
    static char whitespace[] = { ' ', '\r', '\n', '\t' };

    auto length = m_implPtr->Length;
    auto text = m_implPtr->Text;

    // Trim leading whitespace
    while (length > 0)
    {
        auto isWhitespace = std::find(std::begin(whitespace), std::end(whitespace), text[0]) != std::end(whitespace);

        if (!isWhitespace)
            break;

        ++text;
        --length;
    }

    // Trim trailing whitespace
    while (length > 0)
    {
        auto isWhitespace = std::find(std::begin(whitespace), std::end(whitespace), text[length - 1]) != std::end(whitespace);

        if (!isWhitespace)
            break;

        --length;
    }

    return String(text, length);
}

String String::ToLower() const
{
    String copy = *this;
    auto length = copy.m_implPtr->Length;
    auto text = copy.m_implPtr->Text;

    for (size_t i = 0; i < length; ++i)
    {
        text[i] = static_cast<char>(std::tolower(text[i]));
    }

    return copy;
}

String String::Join(const List<String>& parts, Optional<char> separator)
{
    if (parts.Size() == 0)
    {
        return String();
    }

    size_t length = 0;

    for (size_t i = 0; i < parts.Size(); ++i)
    {
        length += parts[i].Length();
    }

    if (length == 0)
    {
        return String();
    }

    if (separator.HasValue())
    {
        length += parts.Size() - 1;
    }

    auto buffer = new char[length + 1]();
    size_t pos = 0;

    for (size_t i = 0; i < parts.Size(); ++i)
    {
        auto partLength = parts[i].Length();

        if (partLength == 0)
        {
            continue;
        }

        memcpy(buffer + pos, parts[i].c_str(), partLength);
        pos += partLength;

        if (separator.HasValue())
        {
            buffer[pos++] = *separator;
        }
    }

    buffer[length] = '\0';

    String joinedString(buffer);

    delete[] buffer;

    return joinedString;
}

bool String::Contains(const String& substring) const
{
    if (substring.Length() == 0)
    {
        return false;
    }

    return strstr(Get(), substring.Get()) != nullptr;
}

bool String::StartsWith(const String& prefix) const
{
    if (prefix.Length() == 0 || prefix.Length() > m_implPtr->Length)
    {
        return false;
    }

    return std::memcmp(Get(), prefix.Get(), prefix.Length()) == 0;
}

bool String::EndsWith(const String& postfix) const
{
    if (postfix.Length() == 0 || postfix.Length() > m_implPtr->Length)
    {
        return false;
    }

    return std::memcmp(Get() + (m_implPtr->Length - postfix.Length()), postfix.Get(), postfix.Length()) == 0;
}

String String::SubString(size_t offset, Optional<size_t> length)
{
    if (offset >= m_implPtr->Length)
    {
        return "";
    }

    size_t maxSubStringLength = m_implPtr->Length - offset;

    size_t substringLength = length.HasValue() ? std::min(*length, maxSubStringLength) : maxSubStringLength;

    return String(m_implPtr->Text + offset, substringLength);
}

String String::Join(const std::initializer_list<String>& parts, Optional<char> separator)
{
    if (parts.size() == 0)
    {
        return String();
    }

    size_t length = 0;

    for (size_t i = 0; i < parts.size(); ++i)
    {
        length += (parts.begin() + i)->Length();
    }

    if (length == 0)
    {
        return String();
    }

    if (separator.HasValue())
    {
        length += parts.size() - 1;
    }

    auto buffer = new char[length + 1]();
    size_t pos = 0;

    for (size_t i = 0; i < parts.size(); ++i)
    {
        auto partLength = (parts.begin() + i)->Length();

        if (partLength == 0)
        {
            continue;
        }

        memcpy(buffer + pos, (parts.begin() + i)->c_str(), partLength);
        pos += partLength;

        if (separator.HasValue())
        {
            buffer[pos++] = *separator;
        }
    }

    buffer[length] = '\0';

    String joinedString(buffer);

    delete[] buffer;

    return joinedString;
}

} // namespace csp::common
