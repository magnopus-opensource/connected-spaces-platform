#pragma once

#include "Olympus/Memory/DllAllocator.h"
#include "Olympus/OlympusCommon.h"

#include <functional>
#include <utility>

OLY_NO_EXPORT

namespace
{

template <typename T> void DefaultDestructor(T* Pointer)
{
    Pointer->~T();
    oly_memory::DllFree(Pointer);
}

} // namespace

namespace oly_common
{

template <typename T> class OLY_API Optional
{
public:
    Optional()
        : Value(nullptr)
    {
        ValueDestructor = DefaultDestructor<T>;
    }

    Optional(T* InValue, std::function<void(T*)> InValueDestructor = DefaultDestructor<T>)
    {
        Value = InValue;
        ValueDestructor = InValueDestructor;
    }

    Optional(std::nullptr_t InValue)
    {
        Value = nullptr;
        ValueDestructor = DefaultDestructor<T>;
    }

    template <typename U> Optional(const U* InValue)
    {
        static_assert(std::is_constructible<T, U*>::value, "Inner type not constructible from argument type!");
        Value = (T*)oly_memory::DllAlloc(sizeof(T));
        new (Value) T(InValue);

        ValueDestructor = DefaultDestructor<T>;
    }

    template <typename U> Optional(const U& InValue)
    {
        static_assert(std::is_constructible<T, U>::value, "Inner type not constructible from argument type!");
        Value = (T*)oly_memory::DllAlloc(sizeof(T));
        new (Value) T(InValue);

        ValueDestructor = DefaultDestructor<T>;
    }

    Optional(const T& InValue)
    {
        Value = (T*)oly_memory::DllAlloc(sizeof(T));
        new (Value) T(InValue);

        ValueDestructor = DefaultDestructor<T>;
    }

    Optional(const T&& InValue)
    {
        Value = (T*)oly_memory::DllAlloc(sizeof(T));
        new (Value) T(InValue);

        ValueDestructor = DefaultDestructor<T>;
    }

    Optional(const Optional<T>& Other)
    {
        if (Other.HasValue())
        {
            Value = (T*)oly_memory::DllAlloc(sizeof(T));
            new (Value) T(*(Other.Value));
        }
        else
        {
            Value = nullptr;
        }

        ValueDestructor = DefaultDestructor<T>;
    }

    Optional(const Optional<T>&& Other)
    {
        if (Other.HasValue())
        {
            Value = (T*)oly_memory::DllAlloc(sizeof(T));
            new (Value) T(std::move(*(Other.Value)));
        }
        else
        {
            Value = nullptr;
        }

        ValueDestructor = DefaultDestructor<T>;
    }

    ~Optional()
    {
        if (Value)
        {
            ValueDestructor(Value);
        }
    }

    bool HasValue() const { return Value != nullptr; }

    T* operator->() const { return Value; }

    T& operator*() const { return *Value; }

    Optional<T>& operator=(const T& Other)
    {
        if (Value)
        {
            ValueDestructor(Value);
        }

        Value = (T*)oly_memory::DllAlloc(sizeof(T));
        new (Value) T(Other);

        return *this;
    }

private:
    T* Value;

    std::function<void(T*)> ValueDestructor;
};

} // namespace oly_common
