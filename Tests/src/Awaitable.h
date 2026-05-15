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

#include <atomic>
#include <chrono>
#include <functional>
#include <stdexcept>
#include <thread>
#include <tuple>

using namespace std::chrono_literals;

#define STRIP(__Type_) std::remove_const_t<std::remove_reference_t<__Type_>>

constexpr std::chrono::duration<int> DefaultTimeout = 40s;

class timeout_exception : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

template <typename... CallbackArgs> class Awaitable
{
public:
    typedef std::function<void(CallbackArgs...)> CallbackType;
    typedef std::tuple<typename std::remove_reference_t<CallbackArgs>...> ResultType;

    template <typename ClassType> Awaitable(void (ClassType::*function)(CallbackType), ClassType* context)
    {
        m_callback = [this](CallbackArgs... args)
        {
            m_result = new ResultType(args...);
            m_completed = true;
        };

        this->m_function = std::bind(function, context, std::placeholders::_1);
    }

    template <typename ClassType, typename T0> Awaitable(void (ClassType::*function)(T0, CallbackType), ClassType* context, const STRIP(T0) & arg0)
    {
        m_callback = [this](CallbackArgs... args)
        {
            m_result = new ResultType(args...);
            m_completed = true;
        };

        this->m_function = std::bind(function, context, arg0, std::placeholders::_1);
    }

    template <typename ClassType, typename T0, typename T1>
    Awaitable(void (ClassType::*function)(T0, T1, CallbackType), ClassType* context, const STRIP(T0) & arg0, const STRIP(T1) & arg1)
    {
        m_callback = [this](CallbackArgs... args)
        {
            m_result = new ResultType(args...);
            m_completed = true;
        };

        this->m_function = std::bind(function, context, arg0, arg1, std::placeholders::_1);
    }

    template <typename ClassType, typename T0, typename T1, typename T2, typename DT,
        typename std::enable_if_t<!std::is_same_v<STRIP(T2), STRIP(DT)> && std::is_base_of_v<STRIP(T2), STRIP(DT)>>* = nullptr>
    Awaitable(
        void (ClassType::*function)(T0, T1, T2, CallbackType), ClassType* context, const STRIP(T0) & arg0, const STRIP(T1) & arg1, const DT& arg2)
    {
        m_callback = [this](CallbackArgs... args)
        {
            m_result = new ResultType(args...);
            m_completed = true;
        };

        this->m_function = std::bind(function, context, arg0, arg1, arg2, std::placeholders::_1);
    }

    template <typename ClassType, typename T0, typename T1, typename T2, typename DT,
        typename std::enable_if_t<std::is_same_v<STRIP(T2), STRIP(DT)> || std::is_constructible_v<STRIP(T2), STRIP(DT)>>* = nullptr>
    Awaitable(
        void (ClassType::*function)(T0, T1, T2, CallbackType), ClassType* context, const STRIP(T0) & arg0, const STRIP(T1) & arg1, const DT& arg2)
    {
        m_callback = [this](CallbackArgs... args)
        {
            m_result = new ResultType(args...);
            m_completed = true;
        };

        this->m_function = std::bind(function, context, arg0, arg1, arg2, std::placeholders::_1);
    }

    template <typename ClassType, typename T0, typename T1, typename T2, typename T3>
    Awaitable(void (ClassType::*function)(T0, T1, T2, T3, CallbackType), ClassType* context, const STRIP(T0) & arg0, const STRIP(T1) & arg1,
        const STRIP(T2) & arg2, const STRIP(T3) & arg3)
    {
        m_callback = [this](CallbackArgs... args)
        {
            m_result = new ResultType(args...);
            m_completed = true;
        };

        this->m_function = std::bind(function, context, arg0, arg1, arg2, arg3, std::placeholders::_1);
    }

    template <typename ClassType, typename T0, typename T1, typename T2, typename T3, typename T4>
    Awaitable(void (ClassType::*function)(T0, T1, T2, T3, T4, CallbackType), ClassType* context, const STRIP(T0) & arg0, const STRIP(T1) & arg1,
        const STRIP(T2) & arg2, const STRIP(T3) & arg3, const STRIP(T4) & arg4)
    {
        m_callback = [this](CallbackArgs... args)
        {
            m_result = new ResultType(args...);
            m_completed = true;
        };

        this->m_function = std::bind(function, context, arg0, arg1, arg2, arg3, arg4, std::placeholders::_1);
    }

    template <typename ClassType, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
    Awaitable(void (ClassType::*function)(T0, T1, T2, T3, T4, T5, CallbackType), ClassType* context, const STRIP(T0) & arg0, const STRIP(T1) & arg1,
        const STRIP(T2) & arg2, const STRIP(T3) & arg3, const STRIP(T4) & arg4, const STRIP(T5) & arg5)
    {
        m_callback = [this](CallbackArgs... args)
        {
            m_result = new ResultType(args...);
            m_completed = true;
        };

        this->m_function = std::bind(function, context, arg0, arg1, arg2, arg3, arg4, arg5, std::placeholders::_1);
    }

    template <typename ClassType, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
    Awaitable(void (ClassType::*function)(T0, T1, T2, T3, T4, T5, T6, CallbackType), ClassType* context, const STRIP(T0) & arg0,
        const STRIP(T1) & arg1, const STRIP(T2) & arg2, const STRIP(T3) & arg3, const STRIP(T4) & arg4, const STRIP(T5) & arg5,
        const STRIP(T6) & arg6)
    {
        m_callback = [this](CallbackArgs... args)
        {
            m_result = new ResultType(args...);
            m_completed = true;
        };

        this->m_function = std::bind(function, context, arg0, arg1, arg2, arg3, arg4, arg5, arg6, std::placeholders::_1);
    }

    template <typename ClassType, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
    Awaitable(void (ClassType::*function)(T0, T1, T2, T3, T4, T5, T6, T7, CallbackType), ClassType* context, const STRIP(T0) & arg0,
        const STRIP(T1) & arg1, const STRIP(T2) & arg2, const STRIP(T3) & arg3, const STRIP(T4) & arg4, const STRIP(T5) & arg5,
        const STRIP(T6) & arg6, const STRIP(T7) & arg7)
    {
        m_callback = [this](CallbackArgs... args)
        {
            m_result = new ResultType(args...);
            m_completed = true;
        };

        this->m_function = std::bind(function, context, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, std::placeholders::_1);
    }

    template <typename ClassType, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
    Awaitable(void (ClassType::*function)(T0, T1, T2, T3, T4, T5, T6, T7, T8, CallbackType), ClassType* context, const STRIP(T0) & arg0,
        const STRIP(T1) & arg1, const STRIP(T2) & arg2, const STRIP(T3) & arg3, const STRIP(T4) & arg4, const STRIP(T5) & arg5,
        const STRIP(T6) & arg6, const STRIP(T7) & arg7, const STRIP(T8) & arg8)
    {
        m_callback = [this](CallbackArgs... args)
        {
            m_result = new ResultType(args...);
            m_completed = true;
        };

        this->m_function = std::bind(function, context, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, std::placeholders::_1);
    }

    template <typename ClassType, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8,
        typename T9>
    Awaitable(void (ClassType::*function)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, CallbackType), ClassType* context, const STRIP(T0) & arg0,
        const STRIP(T1) & arg1, const STRIP(T2) & arg2, const STRIP(T3) & arg3, const STRIP(T4) & arg4, const STRIP(T5) & arg5,
        const STRIP(T6) & arg6, const STRIP(T7) & arg7, const STRIP(T8) & arg8, const STRIP(T9) & arg9)
    {
        m_callback = [this](CallbackArgs... args)
        {
            m_result = new ResultType(args...);
            m_completed = true;
        };

        this->m_function = std::bind(function, context, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, std::placeholders::_1);
    }

    ~Awaitable()
    {
        if (m_result)
        {
            delete m_result;
        }
    }

    /// <summary>
    /// Wait for the asynchronous function to complete and passes a default timeout.
    /// <para>
    ///		Uses `DefaultTimeout` for the timeout.
    /// </para>
    /// </summary>
    /// <param name="Predicate">An optional function used to determine if the asynchronous function has completed.</param>
    /// <returns>The arguments passed to the asynchronous callback upon completion.</returns>
    ResultType Await(std::function<bool(CallbackArgs...)> predicate = nullptr) { return Await(DefaultTimeout, predicate); }

    /// <summary>Wait for the asynchronous function to complete with a given timeout.</summary>
    /// <param name="Timeout">How long to wait before timing out.</param>
    /// <param name="Predicate">An optional function used to determine if the asynchronous function has completed.</param>
    /// <returns>The arguments passed to the asynchronous callback upon completion.</returns>
    ResultType Await(std::chrono::duration<int> timeout, std::function<bool(CallbackArgs...)> predicate = nullptr)
    {
#ifdef CSP_WASM
        // Spawn thread on wasm to prevent wait loop from blocking the async operation
        std::thread TestThread(
            [&]()
            {
#endif
                if (predicate != nullptr)
                {
                    m_function(
                        [this, predicate](CallbackArgs... args)
                        {
                            if (predicate(args...))
                            {
                                m_callback(args...);
                            }
                        });
                }
                else
                {
                    m_function(m_callback);
                }

#ifdef CSP_WASM
            });
#endif

        auto startTime = std::chrono::system_clock::now();
        auto endTime = startTime + timeout;

        while (!m_completed)
        {
            if (std::chrono::system_clock::now() >= endTime)
            {
                throw timeout_exception("Await(): wait exceeded specified timeout");
            }

            std::this_thread::sleep_for(1ms);
        }

#ifdef CSP_WASM
        TestThread.join();
#endif

        return *m_result;
    }

private:
    std::function<void(CallbackType)> m_function;
    CallbackType m_callback;
    ResultType* m_result = nullptr;
    std::atomic_bool m_completed = false;
};

// Helper macro for awaiting an async function
#define AWAIT(__instance_, __function_, ...) Awaitable(&std::remove_pointer_t<decltype(__instance_)>::__function_, __instance_, ##__VA_ARGS__).Await()
#define AWAIT_PRE(__instance_, __function_, __completion_predicate__, ...)                                                                           \
    Awaitable(&std::remove_pointer_t<decltype(__instance_)>::__function_, __instance_, ##__VA_ARGS__).Await(__completion_predicate__)

#undef STRIP