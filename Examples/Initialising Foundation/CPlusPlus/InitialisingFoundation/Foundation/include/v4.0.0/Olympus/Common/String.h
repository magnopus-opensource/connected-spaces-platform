#pragma once

#include "Olympus/Common/List.h"
#include "Olympus/OlympusCommon.h"

OLY_NO_EXPORT

#define OLY_TEXT(txt) oly_common::String(txt)

namespace oly_common
{

/// @brief Custom string class that we can use safely across a DLL boundary.
class OLY_API String
{
public:
    String();
    ~String();

    explicit String(char const* const Text, size_t Length);
    explicit String(size_t Length);

    String(const char* Text);
    String(String const& Other);

    String& operator=(const String& Rhs);

    String& operator=(char const* const Text);

    String& swap(String& Other);

    operator char const*() const { return c_str(); }

    char const* c_str() const { return Get(); }

    // TODO: Possibly switch this to returning an array of StringView
    oly_common::List<oly_common::String> Split(char Delimiter) const;

    bool operator==(const String& Other) const;
    bool operator==(const char* Other) const;
    bool operator!=(const String& Other) const;
    bool operator!=(const char* Other) const;
    bool operator<(const String& Other) const;

    void Append(const String& Other);
    void Append(const char* Other);

    friend String operator+(String Lhs, const String& Rhs)
    {
        Lhs.Append(Rhs);

        return Lhs;
    }

    friend String operator+(String Lhs, const char* Rhs)
    {
        Lhs.Append(Rhs);

        return Lhs;
    }

    String& operator+=(const String& Other);
    String& operator+=(const char* Other);

    size_t Length() const;
    size_t AllocatedMemorySize() const;
    bool IsEmpty() const;

    /// @brief Returns a copy of this string with all leading and trailing whitespace removed.
    /// @returns oly_common::String A copy of this string with leading and trailing whitespace removed.
    String Trim();

    static String Join(const oly_common::List<String>& Parts);
    static String Join(const std::initializer_list<String>& Parts);
    static String Join(char Separator, const oly_common::List<String>& Parts);
    static String Join(char Separator, const std::initializer_list<String>& Parts);

private:
    const char* Get() const;

private:
    class Impl;
    Impl* ImplPtr;
};

} // namespace oly_common
