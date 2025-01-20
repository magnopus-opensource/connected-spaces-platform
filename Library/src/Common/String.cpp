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

#include "Memory/Memory.h"

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
    ~Impl() { CSP_DELETE_ARRAY(Text); }

    explicit Impl(char const* const InText)
        : Text(nullptr)
        , Length(0)
    {
        const char* _InText = InText;

        if (InText == nullptr)
        {
            _InText = "";
        }

        const size_t Len = strlen(_InText);

        char* NewText = CSP_NEW char[Len + 1];

        if (Len > 0)
        {
            memcpy((void*)NewText, _InText, Len * sizeof(char));
        }

        NewText[Len] = 0;

        Text = NewText;
        Length = Len;
    }

    Impl(char const* const InText, size_t Len)
        : Text(nullptr)
        , Length(0)
    {
        const char* _InText = InText;

        if (InText == nullptr || Len == 0)
        {
            _InText = "";
            Len = 0;
        }

        char* NewText = CSP_NEW char[Len + 1];

        if (Len > 0)
        {
            memcpy((void*)NewText, _InText, Len * sizeof(char));
        }

        NewText[Len] = 0;

        Text = NewText;
        Length = Len;
    }

    explicit Impl(size_t Len)
        : Text(nullptr)
        , Length(0)
    {
        char* NewText = CSP_NEW char[Len + 1];

#if DEBUG
        if (Len > 0)
        {
            memset((void*)NewText, 0, sizeof(NewText));
        }
#endif

        NewText[Len] = 0;
        Text = NewText;
        Length = Len;
    }

    Impl* Clone() const { return CSP_NEW Impl(Text, Length); }

    inline void Append(const char* Other, size_t OtherLength)
    {
        if (Other == nullptr || OtherLength == 0)
        {
            return;
        }

        auto NewLength = Length + OtherLength;
        auto NewText = CSP_NEW char[NewLength + 1];
        memcpy(NewText, Text, Length);
        memcpy(NewText + Length, Other, OtherLength);
        NewText[NewLength] = '\0';

        CSP_DELETE_ARRAY(Text);
        Text = NewText;
        Length = NewLength;
    }

    void Append(const Impl& Other) { Append(Other.Text, Other.Length); }

    void Append(const char* Other)
    {
        if (Other == nullptr)
        {
            return;
        }

        auto OtherLength = strlen(Other);
        Append(Other, OtherLength);
    }

    List<String> Split(char Separator)
    {
        List<String> Parts;

        // NOTE: Don't use strtok here because it ignores empty entries!
        auto Index = strchr(Text, Separator);

        if (Index == nullptr)
        {
            Parts.Append(Text);

            return Parts;
        }

        auto Start = Text;

        for (;;)
        {
            Parts.Append(String(Start, Index - Start));
            Start = Index + 1;
            Index = strchr(Start, Separator);

            // Also look for null-terminator
            if (Index == nullptr)
            {
                Index = strchr(Start, '\0');
                Parts.Append(String(Start, Index - Start));
                break;
            }
        }

        return Parts;
    }

    char* Text;
    size_t Length;
};

String::String()
    : ImplPtr(CSP_NEW Impl(""))
{
}

String::String(char const* const Text, size_t Length)
    : ImplPtr(CSP_NEW Impl(Text, Length))
{
}

String::String(size_t Length)
    : ImplPtr(CSP_NEW Impl(Length))
{
}

String::String(const char* Text)
    : ImplPtr(CSP_NEW Impl(Text))
{
}

String::String(String const& Other)
    : ImplPtr(Other.ImplPtr->Clone())
{
}

String::String(String&& Other)
{
    ImplPtr = Other.ImplPtr;
    Other.ImplPtr = nullptr;
}

List<String> String::Split(char Separator) const { return ImplPtr->Split(Separator); }

String& String::swap(String& Other)
{
    std::swap(ImplPtr, Other.ImplPtr);
    return *this;
}

String& String::operator=(const String& Rhs)
{
    if (ImplPtr != nullptr)
    {
        CSP_DELETE(ImplPtr);
    }

    ImplPtr = Rhs.ImplPtr->Clone();
    return *this;
}

String& String::operator=(String&& Rhs)
{
    if (ImplPtr != nullptr)
    {
        CSP_DELETE(ImplPtr);
    }

    ImplPtr = Rhs.ImplPtr;
    Rhs.ImplPtr = nullptr;
    return *this;
}

String& String::operator=(char const* const Text)
{
    if (ImplPtr != nullptr)
    {
        CSP_DELETE(ImplPtr);
    }

    ImplPtr = CSP_NEW Impl(Text);
    return *this;
}

const char* String::Get() const { return ImplPtr->Text; }

size_t String::Length() const { return ImplPtr->Length; }

size_t String::AllocatedMemorySize() const { return ImplPtr->Length + 1; }

bool String::IsEmpty() const { return ImplPtr->Length == 0; }

bool String::operator==(const String& Other) const
{
    if (ImplPtr->Length == 0 && Other.Length() == 0)
    {
        return true;
    }

    if (ImplPtr->Length == 0 || Other.Length() == 0)
    {
        return false;
    }

    return strcmp(Get(), Other.Get()) == 0;
}

bool String::operator==(const char* Other) const
{
    auto OtherLength = strlen(Other);

    if (ImplPtr->Length == 0 && OtherLength == 0)
    {
        return true;
    }

    if (ImplPtr->Length == 0 || OtherLength == 0)
    {
        return false;
    }

    return strcmp(Get(), Other) == 0;
}

bool String::operator!=(const String& Other) const { return !(*this == Other); }

bool String::operator!=(const char* Other) const { return !(*this == Other); }

bool String::operator<(const String& Other) const { return strcmp(Get(), Other.Get()) < 0; }

String::~String()
{
    if (ImplPtr != nullptr)
    {
        CSP_DELETE(ImplPtr);
    }
}

void String::Append(const String& Other) { ImplPtr->Append(*Other.ImplPtr); }

void String::Append(const char* Other) { ImplPtr->Append(Other); }

String& String::operator+=(const String& Other)
{
    Append(Other);

    return *this;
}

String& String::operator+=(const char* Other)
{
    Append(Other);

    return *this;
}

String String::Trim() const
{
    static char Whitespace[] = { ' ', '\r', '\n', '\t' };

    auto Length = ImplPtr->Length;
    auto Text = ImplPtr->Text;

    // Trim leading whitespace
    while (Length > 0)
    {
        auto IsWhitespace = std::find(std::begin(Whitespace), std::end(Whitespace), Text[0]) != std::end(Whitespace);

        if (!IsWhitespace)
            break;

        ++Text;
        --Length;
    }

    // Trim trailing whitespace
    while (Length > 0)
    {
        auto IsWhitespace = std::find(std::begin(Whitespace), std::end(Whitespace), Text[Length - 1]) != std::end(Whitespace);

        if (!IsWhitespace)
            break;

        --Length;
    }

    return String(Text, Length);
}

String String::ToLower() const
{
    String Copy = *this;
    auto Length = Copy.ImplPtr->Length;
    auto Text = Copy.ImplPtr->Text;

    for (int i = 0; i < Length; ++i)
    {
        Text[i] = static_cast<char>(std::tolower(Text[i]));
    }

    return Copy;
}

String String::Join(const List<String>& Parts, Optional<char> Separator)
{
    if (Parts.Size() == 0)
    {
        return String();
    }

    size_t Length = 0;

    for (int i = 0; i < Parts.Size(); ++i)
    {
        Length += Parts[i].Length();
    }

    if (Length == 0)
    {
        return String();
    }

    if (Separator.HasValue())
    {
        Length += Parts.Size() - 1;
    }

    auto Buffer = CSP_NEW char[Length + 1]();
    size_t Pos = 0;

    for (size_t i = 0; i < Parts.Size(); ++i)
    {
        auto PartLength = Parts[i].Length();

        if (PartLength == 0)
        {
            continue;
        }

        memcpy(Buffer + Pos, Parts[i].c_str(), PartLength);
        Pos += PartLength;

        if (Separator.HasValue())
        {
            Buffer[Pos++] = *Separator;
        }
    }

    Buffer[Length] = '\0';

    String JoinedString(Buffer);

    CSP_DELETE_ARRAY(Buffer);

    return JoinedString;
}

String String::Join(const std::initializer_list<String>& Parts, Optional<char> Separator)
{
    if (Parts.size() == 0)
    {
        return String();
    }

    size_t Length = 0;

    for (int i = 0; i < Parts.size(); ++i)
    {
        Length += (Parts.begin() + i)->Length();
    }

    if (Length == 0)
    {
        return String();
    }

    if (Separator.HasValue())
    {
        Length += Parts.size() - 1;
    }

    auto Buffer = CSP_NEW char[Length + 1]();
    size_t Pos = 0;

    for (size_t i = 0; i < Parts.size(); ++i)
    {
        auto PartLength = (Parts.begin() + i)->Length();

        if (PartLength == 0)
        {
            continue;
        }

        memcpy(Buffer + Pos, (Parts.begin() + i)->c_str(), PartLength);
        Pos += PartLength;

        if (Separator.HasValue())
        {
            Buffer[Pos++] = *Separator;
        }
    }

    Buffer[Length] = '\0';

    String JoinedString(Buffer);

    CSP_DELETE_ARRAY(Buffer);

    return JoinedString;
}

} // namespace csp::common
