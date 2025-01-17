#pragma once

#include "Olympus/Memory/DllAllocator.h"
#include "Olympus/OlympusCommon.h"

#include <assert.h>
#include <cstring>
#include <initializer_list>

namespace oly_common
{

OLY_START_IGNORE
template <typename T> class Array;
OLY_END_IGNORE

const auto LIST_DEFAULT_SIZE = 4;

inline size_t next_pow2(size_t val)
{
    --val;
    val |= val >> 1;
    val |= val >> 2;
    val |= val >> 4;
    val |= val >> 8;
    val |= val >> 16;
    ++val;

    return val;
}

/// @brief Simple DLL-safe resizable collection of objects.
///
/// Simple list type a la std::vector used to pass a collection of objects across the DLL boundary.
/// This class is implemented using an array and, as such, removing items is not cheap as it requires
/// us to move all items after it down one space.
///
/// @tparam T Object type to store in the list
template <typename T> class OLY_API List
{
public:
    List()
        : CurrentSize(0)
        , MaximumSize(0)
        , ObjectArray(nullptr)
    {
        AllocList(LIST_DEFAULT_SIZE);
    }

    List(size_t MinimumSize)
        : CurrentSize(0)
        , MaximumSize(0)
        , ObjectArray(nullptr)
    {
        auto Size = next_pow2(MinimumSize);
        AllocList(Size);
    }

    List(const List<T>& Other)
        : CurrentSize(0)
        , MaximumSize(0)
        , ObjectArray(nullptr)
    {
        if (Other.CurrentSize == 0)
        {
            AllocList(LIST_DEFAULT_SIZE);

            return;
        }

        AllocList(Other.MaximumSize);
        CurrentSize = Other.CurrentSize;

        for (size_t i = 0; i < CurrentSize; ++i)
        {
            ObjectArray[i] = Other.ObjectArray[i];
        }
    }

    OLY_START_IGNORE
    explicit List(const oly_common::Array<T>& Array);
    OLY_END_IGNORE

    OLY_NO_EXPORT List(std::initializer_list<T> List)
        : CurrentSize(0)
        , MaximumSize(0)
        , ObjectArray(nullptr)
    {
        if (List.size() == 0)
        {
            AllocList(LIST_DEFAULT_SIZE);

            return;
        }

        auto Size = next_pow2(List.size());
        AllocList(Size);
        CurrentSize = List.size();

        for (size_t i = 0; i < CurrentSize; ++i)
        {
            ObjectArray[i] = *(List.begin() + i);
        }
    }

    ~List() { FreeList(); }

    OLY_NO_EXPORT T* Data() { return CurrentSize > 0 ? &ObjectArray[0] : nullptr; }

    OLY_NO_EXPORT const T* Data() const { return CurrentSize > 0 ? &ObjectArray[0] : nullptr; }

    List<T>& operator=(const List<T>& Other)
    {
        if (this == &Other)
        {
            return *this;
        }

        CurrentSize = Other.CurrentSize;
        MaximumSize = 0;
        ObjectArray = nullptr;

        if (CurrentSize == 0)
        {
            AllocList(LIST_DEFAULT_SIZE);

            return *this;
        }

        AllocList(Other.MaximumSize);

        for (size_t i = 0; i < CurrentSize; i++)
        {
            ObjectArray[i] = Other.ObjectArray[i];
        }

        return *this;
    }

    T& operator[](size_t Index)
    {
        assert(Index < CurrentSize);

        return ObjectArray[Index];
    }

    const T& operator[](size_t Index) const
    {
        assert(Index < CurrentSize);

        return ObjectArray[Index];
    }

    void Append(const T& Item)
    {
        if (CurrentSize == MaximumSize)
        {
            auto Size = next_pow2(MaximumSize + 1);
            ReallocList(Size);
        }

        ObjectArray[CurrentSize++] = Item;
    }

    void Insert(size_t Index, const T& Item)
    {
        if (CurrentSize == MaximumSize)
        {
            auto Size = next_pow2(MaximumSize + 1);
            ReallocList(Size);
        }

        auto After = CurrentSize - Index;
        std::memmove(ObjectArray + (Index + 1), ObjectArray + Index, sizeof(T) * After);
        ++CurrentSize;

        ObjectArray[Index] = Item;
    }

    void Remove(size_t Index)
    {
        assert(Index < CurrentSize);

        T* ObjectPtr = &ObjectArray[Index];
        ObjectPtr->~T();

        --CurrentSize;
        auto Remaining = CurrentSize - Index;
        std::memmove(ObjectArray + Index, ObjectArray + (Index + 1), sizeof(T) * Remaining);
    }

    void RemoveItem(const T& Item)
    {
        for (auto i = CurrentSize - 1; i >= 0; --i)
        {
            if (ObjectArray[i] == Item)
            {
                Remove(i);

                return;
            }
        }
    }

    const size_t Size() const { return CurrentSize; }

    void Clear()
    {
        FreeList();
        AllocList(LIST_DEFAULT_SIZE);
    }

    bool Contains(const T& Item) const
    {
        for (size_t i = 0; i < CurrentSize; ++i)
        {
            if (ObjectArray[i] == Item)
            {
                return true;
            }
        }

        return false;
    }

private:
    void AllocList(const size_t Size)
    {
        ObjectArray = (T*)oly_memory::DllAlloc(sizeof(T) * Size);
        MaximumSize = Size;
    }

    void ReallocList(const size_t Size)
    {
        ObjectArray = (T*)oly_memory::DllRealloc(ObjectArray, sizeof(T) * Size);
        MaximumSize = Size;
    }

    void FreeList()
    {
        if (ObjectArray == nullptr)
        {
            return;
        }

        for (size_t i = 0; i < CurrentSize; ++i)
        {
            T* ObjectPtr = &ObjectArray[i];
            ObjectPtr->~T();
        }

        oly_memory::DllFree(ObjectArray);
        ObjectArray = nullptr;
        CurrentSize = 0;
        MaximumSize = 0;
    }

    size_t CurrentSize;
    size_t MaximumSize;
    T* ObjectArray;
};

} // namespace oly_common
