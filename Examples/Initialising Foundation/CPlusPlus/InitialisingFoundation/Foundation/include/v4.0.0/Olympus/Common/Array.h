#pragma once

#include "Olympus/Memory/DllAllocator.h"
#include "Olympus/OlympusCommon.h"

#include <assert.h>
#include <cstring>
#include <initializer_list>

namespace oly_common
{

OLY_START_IGNORE
template <typename T> class List;
OLY_END_IGNORE

/// @brief Simple DLL-safe array of objects.
///
/// Simple array type used to pass arrays of objects across the DLL boundary.
///
/// @tparam T Object type to store in the array
template <typename T> class OLY_API Array
{
public:
    Array()
        : ArraySize(0)
        , ObjectArray(nullptr)
    {
    }

    Array(const size_t Size)
        : ArraySize(0)
        , ObjectArray(nullptr)
    {
        if (Size > 0)
        {
            AllocArray(Size);
        }
    }

    OLY_START_IGNORE
    explicit Array(const oly_common::List<T>& List);
    OLY_END_IGNORE

    OLY_NO_EXPORT Array(const T* Buffer, size_t Size)
        : ArraySize(0)
        , ObjectArray(nullptr)
    {
        if (Size > 0)
        {
            AllocArray(Size);
            memcpy(ObjectArray, Buffer, Size * sizeof(T));
        }
    }

    OLY_NO_EXPORT Array(const Array<T>& Other)
        : ArraySize(0)
        , ObjectArray(nullptr)
    {
        ArraySize = Other.ArraySize;

        if (ArraySize > 0)
        {
            AllocArray(ArraySize);

            for (size_t i = 0; i < ArraySize; i++)
            {
                ObjectArray[i] = Other.ObjectArray[i];
            }
        }
    }

    OLY_NO_EXPORT Array(std::initializer_list<T> List)
        : ArraySize(0)
        , ObjectArray(nullptr)
    {
        if (List.size() > 0)
        {
            AllocArray(List.size());

            for (size_t i = 0; i < List.size(); ++i)
            {
                ObjectArray[i] = *(List.begin() + i);
            }
        }
    }

    ~Array() { FreeArray(); }

    OLY_NO_EXPORT T* Data() { return ArraySize > 0 ? &ObjectArray[0] : nullptr; }

    OLY_NO_EXPORT const T* Data() const { return ArraySize > 0 ? &ObjectArray[0] : nullptr; }

    Array<T>& operator=(const Array<T>& Other)
    {
        if (this == &Other)
        {
            return *this;
        }

        ArraySize = Other.ArraySize;
        ObjectArray = nullptr;

        if (ArraySize > 0)
        {
            AllocArray(ArraySize);

            for (size_t i = 0; i < ArraySize; i++)
            {
                ObjectArray[i] = Other.ObjectArray[i];
            }
        }

        return *this;
    }

    T& operator[](const size_t Index)
    {
        assert(Index < ArraySize);

        return ObjectArray[Index];
    }

    const T& operator[](const size_t Index) const
    {
        assert(Index < ArraySize);

        return ObjectArray[Index];
    }

    const size_t Size() const { return ArraySize; }

    bool IsEmpty() const { return (ArraySize == 0); }

private:
    void AllocArray(const size_t Size)
    {
        if (ObjectArray == nullptr)
        {
            ObjectArray = (T*)oly_memory::DllAlloc(sizeof(T) * Size);

            for (size_t i = 0; i < Size; ++i)
            {
                T* ObjectPtr = &ObjectArray[i];
                new (ObjectPtr) T;
            }

            ArraySize = Size;
        }
    }

    void FreeArray()
    {
        if (ObjectArray != nullptr)
        {
            for (size_t i = 0; i < ArraySize; ++i)
            {
                T* ObjectPtr = &ObjectArray[i];
                ObjectPtr->~T();
            }

            oly_memory::DllFree(ObjectArray);
            ObjectArray = nullptr;
        }
    }

    size_t ArraySize;
    T* ObjectArray;
};

} // namespace oly_common
