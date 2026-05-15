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
    NumberFormatterPtr(char* inPtr, std::size_t inOffset)
        : m_beg(inPtr)
        , m_cur(inPtr)
        , m_end(inPtr + inOffset)
    {
    }

    char*& operator++() // prefix
    {
        return ++m_cur;
    }

    char* operator++(int) // postfix
    {
        char* tmp = m_cur++;
        return tmp;
    }

    char*& operator--() // prefix
    {
        return --m_cur;
    }

    char* operator--(int) // postfix
    {
        char* tmp = m_cur--;
        return tmp;
    }

    char*& operator+=(const int incr) { return m_cur += incr; }

    char*& operator-=(const int decr) { return m_cur -= decr; }

    operator char*() const { return m_cur; }

    std::size_t span() const { return m_end - m_beg; }

private:
    const char* m_beg;
    char* m_cur;
    const char* m_end;
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

    static std::string Format(int value);
    /// Formats an integer value in decimal notation.

    static std::string Format(int value, int width);
    /// Formats an integer value in decimal notation,
    /// right justified in a field having at least
    /// the specified width.

    static std::string Format0(int value, int width);
    /// Formats an integer value in decimal notation,
    /// right justified and zero-padded in a field
    /// having at least the specified width.

    static std::string FormatHex(int value, bool prefix = false);
    /// Formats an int value in hexadecimal notation.
    /// If prefix is true, "0x" prefix is prepended to the
    /// resulting string.
    /// The value is treated as unsigned.

    static std::string FormatHex(int value, int width, bool prefix = false);
    /// Formats a int value in hexadecimal notation,
    /// right justified and zero-padded in
    /// a field having at least the specified width.
    /// If prefix is true, "0x" prefix is prepended to the
    /// resulting string.
    /// The value is treated as unsigned.

    static std::string Format(unsigned value);
    /// Formats an unsigned int value in decimal notation.

    static std::string Format(unsigned value, int width);
    /// Formats an unsigned long int in decimal notation,
    /// right justified in a field having at least the
    /// specified width.

    static std::string Format0(unsigned int value, int width);
    /// Formats an unsigned int value in decimal notation,
    /// right justified and zero-padded in a field having at
    /// least the specified width.

    static std::string FormatHex(unsigned value, bool prefix = false);
    /// Formats an unsigned int value in hexadecimal notation.
    /// If prefix is true, "0x" prefix is prepended to the
    /// resulting string.

    static std::string FormatHex(unsigned value, int width, bool prefix = false);
    /// Formats a int value in hexadecimal notation,
    /// right justified and zero-padded in
    /// a field having at least the specified width.
    /// If prefix is true, "0x" prefix is prepended to the
    /// resulting string.

    static std::string Format(long value);
    /// Formats a long value in decimal notation.

    static std::string Format(long value, int width);
    /// Formats a long value in decimal notation,
    /// right justified in a field having at least the
    /// specified width.

    static std::string Format0(long value, int width);
    /// Formats a long value in decimal notation,
    /// right justified and zero-padded in a field
    /// having at least the specified width.

    static std::string FormatHex(long value, bool prefix = false);
    /// Formats an unsigned long value in hexadecimal notation.
    /// If prefix is true, "0x" prefix is prepended to the
    /// resulting string.
    /// The value is treated as unsigned.

    static std::string FormatHex(long value, int width, bool prefix = false);
    /// Formats an unsigned long value in hexadecimal notation,
    /// right justified and zero-padded in a field having at least the
    /// specified width.
    /// If prefix is true, "0x" prefix is prepended to the
    /// resulting string.
    /// The value is treated as unsigned.

    static std::string Format(unsigned long value);
    /// Formats an unsigned long value in decimal notation.

    static std::string Format(unsigned long value, int width);
    /// Formats an unsigned long value in decimal notation,
    /// right justified in a field having at least the specified
    /// width.

    static std::string Format0(unsigned long value, int width);
    /// Formats an unsigned long value in decimal notation,
    /// right justified and zero-padded
    /// in a field having at least the specified width.

    static std::string FormatHex(unsigned long value, bool prefix = false);
    /// Formats an unsigned long value in hexadecimal notation.
    /// If prefix is true, "0x" prefix is prepended to the
    /// resulting string.

    static std::string FormatHex(unsigned long value, int width, bool prefix = false);
    /// Formats an unsigned long value in hexadecimal notation,
    /// right justified and zero-padded in a field having at least the
    /// specified width.
    /// If prefix is true, "0x" prefix is prepended to the
    /// resulting string.

private:
    template <typename T>
    static bool IntToStr(
        T value, unsigned short base, char* result, std::size_t& size, bool prefix = false, int width = -1, char fill = ' ', char thSep = 0)
    /// Converts integer to string. Numeric bases from binary to hexadecimal are supported.
    /// If width is non-zero, it pads the return value with fill character to the specified width.
    /// When padding is zero character ('0'), it is prepended to the number itself; all other
    /// paddings are prepended to the formatted Result with minus sign or base prefix included
    /// If prefix is true and base is octal or hexadecimal, respective prefix ('0' for octal,
    /// "0x" for hexadecimal) is prepended. For all other bases, prefix argument is ignored.
    /// Formatted string has at least [width] total length.
    {
        if (base < 2 || base > 0x10)
        {
            *result = '\0';
            return false;
        }

        NumberFormatterPtr ptr(result, size);
        int thCount = 0;
        T tmpVal;
        do
        {
            tmpVal = value;
            value /= base;
            *ptr++ = "FEDCBA9876543210123456789ABCDEF"[15 + (tmpVal - value * base)];
            if (thSep && (base == 10) && (++thCount == 3))
            {
                *ptr++ = thSep;
                thCount = 0;
            }
        } while (value);

        if ('0' == fill)
        {
            if (tmpVal < 0)
                --width;
            if (prefix && base == 010)
                --width;
            if (prefix && base == 0x10)
                width -= 2;
            while ((ptr - result) < width)
                *ptr++ = fill;
        }

        if (prefix && base == 010)
            *ptr++ = '0';
        else if (prefix && base == 0x10)
        {
            *ptr++ = 'x';
            *ptr++ = '0';
        }

        if (tmpVal < 0)
            *ptr++ = '-';

        if ('0' != fill)
        {
            while ((ptr - result) < width)
                *ptr++ = fill;
        }

        size = ptr - result;
        *ptr-- = '\0';

        char* ptrr = result;
        char tmp;
        while (ptrr < ptr)
        {
            tmp = *ptr;
            *ptr-- = *ptrr;
            *ptrr++ = tmp;
        }

        return true;
    }

    template <typename T>
    static bool UIntToStr(
        T value, unsigned short base, char* result, std::size_t& size, bool prefix = false, int width = -1, char fill = ' ', char thSep = 0)
    /// Converts unsigned integer to string. Numeric bases from binary to hexadecimal are supported.
    /// If width is non-zero, it pads the return value with fill character to the specified width.
    /// When padding is zero character ('0'), it is prepended to the number itself; all other
    /// paddings are prepended to the formatted result with minus sign or base prefix included
    /// If prefix is true and base is octal or hexadecimal, respective prefix ('0' for octal,
    /// "0x" for hexadecimal) is prepended. For all other bases, prefix argument is ignored.
    /// Formatted string has at least [width] total length.
    {
        if (base < 2 || base > 0x10)
        {
            *result = '\0';
            return false;
        }

        NumberFormatterPtr ptr(result, size);
        int thCount = 0;
        T tmpVal;
        do
        {
            tmpVal = value;
            value /= base;
            *ptr++ = "FEDCBA9876543210123456789ABCDEF"[15 + (tmpVal - value * base)];
            if (thSep && (base == 10) && (++thCount == 3))
            {
                *ptr++ = thSep;
                thCount = 0;
            }
        } while (value);

        if ('0' == fill)
        {
            if (prefix && base == 010)
                --width;
            if (prefix && base == 0x10)
                width -= 2;
            while ((ptr - result) < width)
                *ptr++ = fill;
        }

        if (prefix && base == 010)
            *ptr++ = '0';
        else if (prefix && base == 0x10)
        {
            *ptr++ = 'x';
            *ptr++ = '0';
        }

        if ('0' != fill)
        {
            while ((ptr - result) < width)
                *ptr++ = fill;
        }

        size = ptr - result;
        *ptr-- = '\0';

        char* ptrr = result;
        char tmp;
        while (ptrr < ptr)
        {
            tmp = *ptr;
            *ptr-- = *ptrr;
            *ptrr++ = tmp;
        }

        return true;
    }

    template <typename T>
    static bool IntToStr(T number, unsigned short base, std::string& result, bool prefix = false, int width = -1, char fill = ' ', char thSep = 0)
    /// Converts integer to string; This is a wrapper function, for details see see the
    /// bool IntToStr(T, unsigned short, char*, int, int, char, char) implementation.
    {
        char res[CSP_MAX_INT_STRING_LEN] = { 0 };
        std::size_t size = CSP_MAX_INT_STRING_LEN;
        bool ret = IntToStr(number, base, res, size, prefix, width, fill, thSep);
        result.assign(res, size);
        return ret;
    }

    template <typename T>
    static bool UIntToStr(T number, unsigned short base, std::string& result, bool prefix = false, int width = -1, char fill = ' ', char thSep = 0)
    /// Converts unsigned integer to string; This is a wrapper function, for details see see the
    /// bool UIntToStr(T, unsigned short, char*, int, int, char, char) implementation.
    {
        char res[CSP_MAX_INT_STRING_LEN] = { 0 };
        std::size_t size = CSP_MAX_INT_STRING_LEN;
        bool ret = UIntToStr(number, base, res, size, prefix, width, fill, thSep);
        result.assign(res, size);
        return ret;
    }
};

inline std::string NumberFormatter::Format(int value)
{
    std::string result;
    IntToStr(value, 10, result);
    return result;
}

inline std::string NumberFormatter::Format(int value, int width)
{
    std::string result;
    IntToStr(value, 10, result, false, width, ' ');
    return result;
}

inline std::string NumberFormatter::Format0(int value, int width)
{
    std::string result;
    IntToStr(value, 10, result, false, width, '0');
    return result;
}

inline std::string NumberFormatter::FormatHex(int value, bool prefix)
{
    std::string result;
    UIntToStr(static_cast<unsigned int>(value), 0x10, result, prefix);
    return result;
}

inline std::string NumberFormatter::FormatHex(int value, int width, bool prefix)
{
    std::string result;
    UIntToStr(static_cast<unsigned int>(value), 0x10, result, prefix, width, '0');
    return result;
}

inline std::string NumberFormatter::Format(unsigned value)
{
    std::string result;
    UIntToStr(value, 10, result);
    return result;
}

inline std::string NumberFormatter::Format(unsigned value, int width)
{
    std::string result;
    UIntToStr(value, 10, result, false, width, ' ');
    return result;
}

inline std::string NumberFormatter::Format0(unsigned int value, int width)
{
    std::string result;
    UIntToStr(value, 10, result, false, width, '0');
    return result;
}

inline std::string NumberFormatter::FormatHex(unsigned value, bool prefix)
{
    std::string result;
    UIntToStr(value, 0x10, result, prefix);
    return result;
}

inline std::string NumberFormatter::FormatHex(unsigned value, int width, bool prefix)
{
    std::string result;
    UIntToStr(value, 0x10, result, prefix, width, '0');
    return result;
}

inline std::string NumberFormatter::Format(long value)
{
    std::string result;
    IntToStr(value, 10, result);
    return result;
}

inline std::string NumberFormatter::Format(long value, int width)
{
    std::string result;
    IntToStr(value, 10, result, false, width, ' ');
    return result;
}

inline std::string NumberFormatter::Format0(long value, int width)
{
    std::string result;
    IntToStr(value, 10, result, false, width, '0');
    return result;
}

inline std::string NumberFormatter::FormatHex(long value, bool prefix)
{
    std::string result;
    UIntToStr(static_cast<unsigned long>(value), 0x10, result, prefix);
    return result;
}

inline std::string NumberFormatter::FormatHex(long value, int width, bool prefix)
{
    std::string result;
    UIntToStr(static_cast<unsigned long>(value), 0x10, result, prefix, width, '0');
    return result;
}

inline std::string NumberFormatter::Format(unsigned long value)
{
    std::string result;
    UIntToStr(value, 10, result);
    return result;
}

inline std::string NumberFormatter::Format(unsigned long value, int width)
{
    std::string result;
    UIntToStr(value, 10, result, false, width, ' ');
    return result;
}

inline std::string NumberFormatter::Format0(unsigned long value, int width)
{
    std::string result;
    UIntToStr(value, 10, result, false, width, '0');
    return result;
}

inline std::string NumberFormatter::FormatHex(unsigned long value, bool prefix)
{
    std::string result;
    UIntToStr(value, 0x10, result, prefix);
    return result;
}

inline std::string NumberFormatter::FormatHex(unsigned long value, int width, bool prefix)
{
    std::string result;
    UIntToStr(value, 0x10, result, prefix, width, '0');
    return result;
}

} // namespace csp
