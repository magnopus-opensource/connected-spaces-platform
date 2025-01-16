#pragma once

#include "Olympus/Common/Array.h"
#include "Olympus/Memory/DllAllocator.h"
#include "Olympus/OlympusCommon.h"

#include <map>

namespace oly_common
{

template <typename TKey, typename TValue> class OLY_API Map
{
    using MapType = std::map<TKey, TValue>;

    /*class Item
       {

       };*/

    /*class Iterator
       {

       };*/

public:
    Map()
    {
        Container = (MapType*)oly_memory::DllAlloc(sizeof(MapType));
        new (Container) MapType;
    }

    Map(const Map<TKey, TValue>& Other)
    {
        Container = (MapType*)oly_memory::DllAlloc(sizeof(MapType));
        new (Container) MapType(*Other.Container);
    }

    OLY_NO_EXPORT Map(const Map<TKey, TValue>&& Other)
    {
        Container = (MapType*)oly_memory::DllAlloc(sizeof(MapType));
        new (Container) MapType(*Other.Container);
    }

    OLY_NO_EXPORT Map(const std::initializer_list<std::pair<const TKey, const TValue>> Values)
    {
        Container = (MapType*)oly_memory::DllAlloc(sizeof(MapType));
        new (Container) MapType;

        for (const auto& Pair : Values)
        {
            Container->emplace(Pair.first, Pair.second);
        }
    }

    ~Map()
    {
        Container->~MapType();
        oly_memory::DllFree(Container);
    }

    TValue& operator[](const TKey& Key) { return Container->operator[](Key); }

    const TValue& operator[](const TKey& Key) const { return Container->operator[](Key); }

    OLY_NO_EXPORT Map<TKey, TValue>& operator=(const Map<TKey, TValue>& Other)
    {
        if (this == &Other)
        {
            return *this;
        }

        Container->~MapType();
        new (Container) MapType(*Other.Container);

        return *this;
    }

    OLY_NO_EXPORT Map<TKey, TValue>& operator=(Map<TKey, TValue>&& Other)
    {
        if (this == &Other)
        {
            return *this;
        }

        Container->~MapType();
        new (Container) MapType(*Other.Container);

        return *this;
    }

    size_t Size() const { return Container->size(); }

    bool HasKey(const TKey& Key) const { return Container->count(Key) > 0; }

    const oly_common::Array<TKey>* Keys() const
    {
        auto Keys = (oly_common::Array<TKey>*)oly_memory::DllAlloc(sizeof(oly_common::Array<TKey>));
        new (Keys) oly_common::Array<TKey>(Container->size());
        int i = 0;

        for (const auto& Pair : *Container)
        {
            Keys->operator[](i++) = Pair.first;
        }

        return Keys;
    }

    const oly_common::Array<TValue>* Values() const
    {
        auto Values = (oly_common::Array<TValue>*)oly_memory::DllAlloc(sizeof(oly_common::Array<TValue>));
        new (Values) oly_common::Array<TValue>(Container->size());
        int i = 0;

        for (const auto& Pair : *Container)
        {
            Values->operator[](i++) = Pair.second;
        }

        return Values;
    }

    void Remove(const TKey& Key)
    {
        if (HasKey(Key))
        {
            Container->erase(Key);
        }
    }

    void Clear() { Container->clear(); }

private:
    MapType* Container;
};

} // namespace oly_common
