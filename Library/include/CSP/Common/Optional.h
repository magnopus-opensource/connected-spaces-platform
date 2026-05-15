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

#include <functional>
#include <utility>

CSP_NO_EXPORT

/** @cond DO_NOT_DOCUMENT */
namespace
{

template <typename T> void DefaultDestructor(T* pointer) { delete pointer; }

} // namespace
/** @endcond */

namespace csp::common
{

/// @brief Simple DLL-safe container holding an optional object.
/// Simple optional type used to pass optional object across the DLL boundary.
///
/// @tparam T : Object type to store in the optional
template <typename T> class CSP_API Optional
{
public:
    /// @brief Constructs an optional with a null value.
    Optional()
        : m_value(nullptr)
    {
        m_valueDestructor = DefaultDestructor<T>;
    }

    /// @brief Constructs an optional with a given pointer.
    /// @param InValue T* : Pointer to construct optional with
    /// @param InValueDestructor std::function<void(T*)> : Optional deleter to be called on destructer
    Optional(T* inValue, std::function<void(T*)> inValueDestructor = DefaultDestructor<T>)
    {
        m_value = inValue;
        m_valueDestructor = inValueDestructor;
    }

    /// @brief Constructs an optional with a null value.
    /// @param InValue std::nullptr_t : nullptr value
    Optional(std::nullptr_t /*InValue*/)
    {
        m_value = nullptr;
        m_valueDestructor = DefaultDestructor<T>;
    }

    /// @brief Constructs an optional by copying given value of a different type.
    /// @param InValue const U& : Reference to construct optional with
    template <typename U> Optional(const U& inValue)
    {
        static_assert(std::is_constructible<T, U>::value, "Inner type not constructible from argument type!");
        m_value = new T(inValue);

        m_valueDestructor = DefaultDestructor<T>;
    }

    /// @brief Constructs an optional with a given reference.
    /// @param InValue const T& : Reference to construct optional with
    Optional(const T& inValue)
    {
        m_value = new T(inValue);

        m_valueDestructor = DefaultDestructor<T>;
    }

    /// @brief Constructs an optional with a given rvalue reference.
    /// @param InValue T&& : Rvalue reference to construct optional with
    Optional(T&& inValue)
    {
        m_value = new T(std::move(inValue));
        m_valueDestructor = DefaultDestructor<T>;
    }

    /// @brief Copy constructor.
    /// @param Other const Optional<T>&
    Optional(const Optional<T>& other)
    {
        if (other.HasValue())
        {
            m_value = new T(*other.m_value);
        }
        else
        {
            m_value = nullptr;
        }

        m_valueDestructor = DefaultDestructor<T>;
    }

    /// @brief Move constructor.
    /// @param Other Optional<T>&&
    Optional(Optional<T>&& other)
    {
        if (other.HasValue())
        {
            m_value = new T(std::move(*other.m_value)); // move the object, not the pointer
        }
        else
        {
            m_value = nullptr;
        }

        m_valueDestructor = DefaultDestructor<T>;
    }

    /// @brief Destructor.
    ~Optional()
    {
        if (m_value)
        {
            m_valueDestructor(m_value);
        }
    }

    /// @brief Checks of the options contains a value (non null).
    /// @return bool
    bool HasValue() const { return m_value != nullptr; }

    /// @brief Accesses internal value by pointer.
    /// @return T*
    T* operator->() const { return m_value; }

    /// @brief Accesses internal value by reference.
    /// @return T&
    T& operator*() const { return *m_value; }

    /// @brief Assigns a value to the optional.
    /// @param Other const T& : Reference to assign to optional
    /// @return Optional<T>&
    Optional<T>& operator=(const T& inValue)
    {
        if (m_value)
        {
            m_valueDestructor(m_value);
        }

        m_value = new T(inValue);

        return *this;
    }

    /// @brief Copy assignment
    /// @param Other const T& : Reference to assign to optional
    /// @return Optional<T>&
    Optional<T>& operator=(const Optional<T>& other)
    {
        if (m_value)
        {
            m_valueDestructor(m_value);
        }

        if (other.HasValue())
        {
            m_value = new T(*other.m_value);
        }
        else
        {
            m_value = nullptr;
        }

        return *this;
    }

    /// @brief Move assignment
    /// @param Other const T& : Reference to assign to optional
    /// @return Optional<T>&
    Optional<T>& operator=(Optional<T>&& other)
    {
        if (m_value)
        {
            m_valueDestructor(m_value);
        }

        m_value = other.m_value;
        other.m_value = nullptr;

        return *this;
    }

private:
    T* m_value;

    std::function<void(T*)> m_valueDestructor;
};

} // namespace csp::common
