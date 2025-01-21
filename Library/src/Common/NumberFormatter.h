//
// Copyright (c) 2004-2008, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// Connected Spaces Platform note: This code originates from Poco, so the BSL license requires this license header be present
// SPDX-License-Identifier:	BSL-1.0
//

#pragma once

namespace csp
{

/// Utility char pointer wrapper class.
/// Class ensures increment/decrement remain within boundaries.
class NumberFormatterPtr
{
public:
    NumberFormatterPtr(char* InPtr, std::size_t InOffset)
        : Beg(InPtr)
        , Cur(InPtr)
        , End(InPtr + InOffset)
    {
    }

    char*& operator++() // prefix
    {
        return ++Cur;
    }

    char* operator++(int) // postfix
    {
        char* Tmp = Cur++;
        return Tmp;
    }

    char*& operator--() // prefix
    {
        return --Cur;
    }

    char* operator--(int) // postfix
    {
        char* tmp = Cur--;
        return tmp;
    }

    char*& operator+=(const int Incr) { return Cur += Incr; }

    char*& operator-=(const int Decr) { return Cur -= Decr; }

    operator char*() const { return Cur; }

    std::size_t span() const { return End - Beg; }

private:
    const char* Beg;
    char* Cur;
    const char* End;
};

class NumberFormatter
/// The NumberFormatter class provides static methods
/// for formatting numeric values into strings.
///
/// There are two kind of static member functions:
///	* Format* functions return a std::string containing
///	  the formatted value.
{
public:
    static constexpr unsigned NF_MAX_INT_STRING_LEN = 32; // increase for 64-bit binary formatting support
    static constexpr unsigned NF_MAX_FLT_STRING_LEN = 780;
    static constexpr unsigned CSP_MAX_INT_STRING_LEN = 65;

    static std::string Format(int Value);
    /// Formats an integer value in decimal notation.

    static std::string Format(int Value, int Width);
    /// Formats an integer value in decimal notation,
    /// right justified in a field having at least
    /// the specified width.

    static std::string Format0(int Value, int Width);
    /// Formats an integer value in decimal notation,
    /// right justified and zero-padded in a field
    /// having at least the specified width.

    static std::string FormatHex(int Value, bool Prefix = false);
    /// Formats an int value in hexadecimal notation.
    /// If prefix is true, "0x" prefix is prepended to the
    /// resulting string.
    /// The value is treated as unsigned.

    static std::string FormatHex(int Value, int Width, bool Prefix = false);
    /// Formats a int value in hexadecimal notation,
    /// right justified and zero-padded in
    /// a field having at least the specified width.
    /// If prefix is true, "0x" prefix is prepended to the
    /// resulting string.
    /// The value is treated as unsigned.

    static std::string Format(unsigned Value);
    /// Formats an unsigned int value in decimal notation.

    static std::string Format(unsigned Value, int Width);
    /// Formats an unsigned long int in decimal notation,
    /// right justified in a field having at least the
    /// specified width.

    static std::string Format0(unsigned int Value, int Width);
    /// Formats an unsigned int value in decimal notation,
    /// right justified and zero-padded in a field having at
    /// least the specified width.

    static std::string FormatHex(unsigned Value, bool Prefix = false);
    /// Formats an unsigned int value in hexadecimal notation.
    /// If prefix is true, "0x" prefix is prepended to the
    /// resulting string.

    static std::string FormatHex(unsigned Value, int Width, bool Prefix = false);
    /// Formats a int value in hexadecimal notation,
    /// right justified and zero-padded in
    /// a field having at least the specified width.
    /// If prefix is true, "0x" prefix is prepended to the
    /// resulting string.

    static std::string Format(long Value);
    /// Formats a long value in decimal notation.

    static std::string Format(long Value, int Width);
    /// Formats a long value in decimal notation,
    /// right justified in a field having at least the
    /// specified width.

    static std::string Format0(long Value, int Width);
    /// Formats a long value in decimal notation,
    /// right justified and zero-padded in a field
    /// having at least the specified width.

    static std::string FormatHex(long Value, bool Prefix = false);
    /// Formats an unsigned long value in hexadecimal notation.
    /// If prefix is true, "0x" prefix is prepended to the
    /// resulting string.
    /// The value is treated as unsigned.

    static std::string FormatHex(long Value, int Width, bool Prefix = false);
    /// Formats an unsigned long value in hexadecimal notation,
    /// right justified and zero-padded in a field having at least the
    /// specified width.
    /// If prefix is true, "0x" prefix is prepended to the
    /// resulting string.
    /// The value is treated as unsigned.

    static std::string Format(unsigned long Value);
    /// Formats an unsigned long value in decimal notation.

    static std::string Format(unsigned long Value, int Width);
    /// Formats an unsigned long value in decimal notation,
    /// right justified in a field having at least the specified
    /// width.

    static std::string Format0(unsigned long Value, int Width);
    /// Formats an unsigned long value in decimal notation,
    /// right justified and zero-padded
    /// in a field having at least the specified width.

    static std::string FormatHex(unsigned long Value, bool Prefix = false);
    /// Formats an unsigned long value in hexadecimal notation.
    /// If prefix is true, "0x" prefix is prepended to the
    /// resulting string.

    static std::string FormatHex(unsigned long Value, int Width, bool Prefix = false);
    /// Formats an unsigned long value in hexadecimal notation,
    /// right justified and zero-padded in a field having at least the
    /// specified width.
    /// If prefix is true, "0x" prefix is prepended to the
    /// resulting string.

private:
    template <typename T>
    static bool IntToStr(
        T Value, unsigned short Base, char* Result, std::size_t& Size, bool Prefix = false, int Width = -1, char Fill = ' ', char ThSep = 0)
    /// Converts integer to string. Numeric bases from binary to hexadecimal are supported.
    /// If width is non-zero, it pads the return value with fill character to the specified width.
    /// When padding is zero character ('0'), it is prepended to the number itself; all other
    /// paddings are prepended to the formatted Result with minus sign or base prefix included
    /// If prefix is true and base is octal or hexadecimal, respective prefix ('0' for octal,
    /// "0x" for hexadecimal) is prepended. For all other bases, prefix argument is ignored.
    /// Formatted string has at least [width] total length.
    {
        if (Base < 2 || Base > 0x10)
        {
            *Result = '\0';
            return false;
        }

        NumberFormatterPtr Ptr(Result, Size);
        int ThCount = 0;
        T TmpVal;
        do
        {
            TmpVal = Value;
            Value /= Base;
            *Ptr++ = "FEDCBA9876543210123456789ABCDEF"[15 + (TmpVal - Value * Base)];
            if (ThSep && (Base == 10) && (++ThCount == 3))
            {
                *Ptr++ = ThSep;
                ThCount = 0;
            }
        } while (Value);

        if ('0' == Fill)
        {
            if (TmpVal < 0)
                --Width;
            if (Prefix && Base == 010)
                --Width;
            if (Prefix && Base == 0x10)
                Width -= 2;
            while ((Ptr - Result) < Width)
                *Ptr++ = Fill;
        }

        if (Prefix && Base == 010)
            *Ptr++ = '0';
        else if (Prefix && Base == 0x10)
        {
            *Ptr++ = 'x';
            *Ptr++ = '0';
        }

        if (TmpVal < 0)
            *Ptr++ = '-';

        if ('0' != Fill)
        {
            while ((Ptr - Result) < Width)
                *Ptr++ = Fill;
        }

        Size = Ptr - Result;
        *Ptr-- = '\0';

        char* Ptrr = Result;
        char Tmp;
        while (Ptrr < Ptr)
        {
            Tmp = *Ptr;
            *Ptr-- = *Ptrr;
            *Ptrr++ = Tmp;
        }

        return true;
    }

    template <typename T>
    static bool UIntToStr(
        T Value, unsigned short Base, char* Result, std::size_t& Size, bool Prefix = false, int Width = -1, char Fill = ' ', char ThSep = 0)
    /// Converts unsigned integer to string. Numeric bases from binary to hexadecimal are supported.
    /// If width is non-zero, it pads the return value with fill character to the specified width.
    /// When padding is zero character ('0'), it is prepended to the number itself; all other
    /// paddings are prepended to the formatted result with minus sign or base prefix included
    /// If prefix is true and base is octal or hexadecimal, respective prefix ('0' for octal,
    /// "0x" for hexadecimal) is prepended. For all other bases, prefix argument is ignored.
    /// Formatted string has at least [width] total length.
    {
        if (Base < 2 || Base > 0x10)
        {
            *Result = '\0';
            return false;
        }

        NumberFormatterPtr Ptr(Result, Size);
        int ThCount = 0;
        T TmpVal;
        do
        {
            TmpVal = Value;
            Value /= Base;
            *Ptr++ = "FEDCBA9876543210123456789ABCDEF"[15 + (TmpVal - Value * Base)];
            if (ThSep && (Base == 10) && (++ThCount == 3))
            {
                *Ptr++ = ThSep;
                ThCount = 0;
            }
        } while (Value);

        if ('0' == Fill)
        {
            if (Prefix && Base == 010)
                --Width;
            if (Prefix && Base == 0x10)
                Width -= 2;
            while ((Ptr - Result) < Width)
                *Ptr++ = Fill;
        }

        if (Prefix && Base == 010)
            *Ptr++ = '0';
        else if (Prefix && Base == 0x10)
        {
            *Ptr++ = 'x';
            *Ptr++ = '0';
        }

        if ('0' != Fill)
        {
            while ((Ptr - Result) < Width)
                *Ptr++ = Fill;
        }

        Size = Ptr - Result;
        *Ptr-- = '\0';

        char* Ptrr = Result;
        char Tmp;
        while (Ptrr < Ptr)
        {
            Tmp = *Ptr;
            *Ptr-- = *Ptrr;
            *Ptrr++ = Tmp;
        }

        return true;
    }

    template <typename T>
    static bool IntToStr(T number, unsigned short base, std::string& result, bool prefix = false, int width = -1, char fill = ' ', char thSep = 0)
    /// Converts integer to string; This is a wrapper function, for details see see the
    /// bool IntToStr(T, unsigned short, char*, int, int, char, char) implementation.
    {
        char Res[CSP_MAX_INT_STRING_LEN] = { 0 };
        std::size_t Size = CSP_MAX_INT_STRING_LEN;
        bool Ret = IntToStr(number, base, Res, Size, prefix, width, fill, thSep);
        result.assign(Res, Size);
        return Ret;
    }

    template <typename T>
    static bool UIntToStr(T number, unsigned short base, std::string& result, bool prefix = false, int width = -1, char fill = ' ', char thSep = 0)
    /// Converts unsigned integer to string; This is a wrapper function, for details see see the
    /// bool UIntToStr(T, unsigned short, char*, int, int, char, char) implementation.
    {
        char Res[CSP_MAX_INT_STRING_LEN] = { 0 };
        std::size_t size = CSP_MAX_INT_STRING_LEN;
        bool Ret = UIntToStr(number, base, Res, size, prefix, width, fill, thSep);
        result.assign(Res, size);
        return Ret;
    }
};

inline std::string NumberFormatter::Format(int Value)
{
    std::string Result;
    IntToStr(Value, 10, Result);
    return Result;
}

inline std::string NumberFormatter::Format(int Value, int Width)
{
    std::string Result;
    IntToStr(Value, 10, Result, false, Width, ' ');
    return Result;
}

inline std::string NumberFormatter::Format0(int Value, int Width)
{
    std::string Result;
    IntToStr(Value, 10, Result, false, Width, '0');
    return Result;
}

inline std::string NumberFormatter::FormatHex(int Value, bool Prefix)
{
    std::string Result;
    UIntToStr(static_cast<unsigned int>(Value), 0x10, Result, Prefix);
    return Result;
}

inline std::string NumberFormatter::FormatHex(int Value, int Width, bool Prefix)
{
    std::string Result;
    UIntToStr(static_cast<unsigned int>(Value), 0x10, Result, Prefix, Width, '0');
    return Result;
}

inline std::string NumberFormatter::Format(unsigned Value)
{
    std::string Result;
    UIntToStr(Value, 10, Result);
    return Result;
}

inline std::string NumberFormatter::Format(unsigned Value, int Width)
{
    std::string Result;
    UIntToStr(Value, 10, Result, false, Width, ' ');
    return Result;
}

inline std::string NumberFormatter::Format0(unsigned int Value, int Width)
{
    std::string Result;
    UIntToStr(Value, 10, Result, false, Width, '0');
    return Result;
}

inline std::string NumberFormatter::FormatHex(unsigned Value, bool Prefix)
{
    std::string Result;
    UIntToStr(Value, 0x10, Result, Prefix);
    return Result;
}

inline std::string NumberFormatter::FormatHex(unsigned Value, int Width, bool Prefix)
{
    std::string Result;
    UIntToStr(Value, 0x10, Result, Prefix, Width, '0');
    return Result;
}

inline std::string NumberFormatter::Format(long Value)
{
    std::string Result;
    IntToStr(Value, 10, Result);
    return Result;
}

inline std::string NumberFormatter::Format(long Value, int Width)
{
    std::string Result;
    IntToStr(Value, 10, Result, false, Width, ' ');
    return Result;
}

inline std::string NumberFormatter::Format0(long Value, int Width)
{
    std::string Result;
    IntToStr(Value, 10, Result, false, Width, '0');
    return Result;
}

inline std::string NumberFormatter::FormatHex(long Value, bool Prefix)
{
    std::string Result;
    UIntToStr(static_cast<unsigned long>(Value), 0x10, Result, Prefix);
    return Result;
}

inline std::string NumberFormatter::FormatHex(long Value, int Width, bool Prefix)
{
    std::string Result;
    UIntToStr(static_cast<unsigned long>(Value), 0x10, Result, Prefix, Width, '0');
    return Result;
}

inline std::string NumberFormatter::Format(unsigned long Value)
{
    std::string Result;
    UIntToStr(Value, 10, Result);
    return Result;
}

inline std::string NumberFormatter::Format(unsigned long Value, int Width)
{
    std::string Result;
    UIntToStr(Value, 10, Result, false, Width, ' ');
    return Result;
}

inline std::string NumberFormatter::Format0(unsigned long Value, int Width)
{
    std::string Result;
    UIntToStr(Value, 10, Result, false, Width, '0');
    return Result;
}

inline std::string NumberFormatter::FormatHex(unsigned long Value, bool Prefix)
{
    std::string Result;
    UIntToStr(Value, 0x10, Result, Prefix);
    return Result;
}

inline std::string NumberFormatter::FormatHex(unsigned long Value, int Width, bool Prefix)
{
    std::string Result;
    UIntToStr(Value, 0x10, Result, Prefix, Width, '0');
    return Result;
}

} // namespace csp
