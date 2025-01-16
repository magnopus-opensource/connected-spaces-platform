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

    template <typename ClassType> Awaitable(void (ClassType::*Function)(CallbackType), ClassType* Context)
    {
        Callback = [this](CallbackArgs... _Args)
        {
            Result = new ResultType(_Args...);
            Completed = true;
        };

        this->Function = std::bind(Function, Context, std::placeholders::_1);
    }

    template <typename ClassType, typename T0> Awaitable(void (ClassType::*Function)(T0, CallbackType), ClassType* Context, const STRIP(T0) & Arg0)
    {
        Callback = [this](CallbackArgs... _Args)
        {
            Result = new ResultType(_Args...);
            Completed = true;
        };

        this->Function = std::bind(Function, Context, Arg0, std::placeholders::_1);
    }

    template <typename ClassType, typename T0, typename T1>
    Awaitable(void (ClassType::*Function)(T0, T1, CallbackType), ClassType* Context, const STRIP(T0) & Arg0, const STRIP(T1) & Arg1)
    {
        Callback = [this](CallbackArgs... _Args)
        {
            Result = new ResultType(_Args...);
            Completed = true;
        };

        this->Function = std::bind(Function, Context, Arg0, Arg1, std::placeholders::_1);
    }

    template <typename ClassType, typename T0, typename T1, typename T2, typename DT,
        typename std::enable_if_t<!std::is_same_v<STRIP(T2), STRIP(DT)> && std::is_base_of_v<STRIP(T2), STRIP(DT)>>* = nullptr>
    Awaitable(
        void (ClassType::*Function)(T0, T1, T2, CallbackType), ClassType* Context, const STRIP(T0) & Arg0, const STRIP(T1) & Arg1, const DT& Arg2)
    {
        Callback = [this](CallbackArgs... _Args)
        {
            Result = new ResultType(_Args...);
            Completed = true;
        };

        this->Function = std::bind(Function, Context, Arg0, Arg1, Arg2, std::placeholders::_1);
    }

    template <typename ClassType, typename T0, typename T1, typename T2, typename DT,
        typename std::enable_if_t<std::is_same_v<STRIP(T2), STRIP(DT)> || std::is_constructible_v<STRIP(T2), STRIP(DT)>>* = nullptr>
    Awaitable(
        void (ClassType::*Function)(T0, T1, T2, CallbackType), ClassType* Context, const STRIP(T0) & Arg0, const STRIP(T1) & Arg1, const DT& Arg2)
    {
        Callback = [this](CallbackArgs... _Args)
        {
            Result = new ResultType(_Args...);
            Completed = true;
        };

        this->Function = std::bind(Function, Context, Arg0, Arg1, Arg2, std::placeholders::_1);
    }

    template <typename ClassType, typename T0, typename T1, typename T2, typename T3>
    Awaitable(void (ClassType::*Function)(T0, T1, T2, T3, CallbackType), ClassType* Context, const STRIP(T0) & Arg0, const STRIP(T1) & Arg1,
        const STRIP(T2) & Arg2, const STRIP(T3) & Arg3)
    {
        Callback = [this](CallbackArgs... _Args)
        {
            Result = new ResultType(_Args...);
            Completed = true;
        };

        this->Function = std::bind(Function, Context, Arg0, Arg1, Arg2, Arg3, std::placeholders::_1);
    }

    template <typename ClassType, typename T0, typename T1, typename T2, typename T3, typename T4>
    Awaitable(void (ClassType::*Function)(T0, T1, T2, T3, T4, CallbackType), ClassType* Context, const STRIP(T0) & Arg0, const STRIP(T1) & Arg1,
        const STRIP(T2) & Arg2, const STRIP(T3) & Arg3, const STRIP(T4) & Arg4)
    {
        Callback = [this](CallbackArgs... _Args)
        {
            Result = new ResultType(_Args...);
            Completed = true;
        };

        this->Function = std::bind(Function, Context, Arg0, Arg1, Arg2, Arg3, Arg4, std::placeholders::_1);
    }

    template <typename ClassType, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
    Awaitable(void (ClassType::*Function)(T0, T1, T2, T3, T4, T5, CallbackType), ClassType* Context, const STRIP(T0) & Arg0, const STRIP(T1) & Arg1,
        const STRIP(T2) & Arg2, const STRIP(T3) & Arg3, const STRIP(T4) & Arg4, const STRIP(T5) & Arg5)
    {
        Callback = [this](CallbackArgs... _Args)
        {
            Result = new ResultType(_Args...);
            Completed = true;
        };

        this->Function = std::bind(Function, Context, Arg0, Arg1, Arg2, Arg3, Arg4, Arg5, std::placeholders::_1);
    }

    template <typename ClassType, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
    Awaitable(void (ClassType::*Function)(T0, T1, T2, T3, T4, T5, T6, CallbackType), ClassType* Context, const STRIP(T0) & Arg0,
        const STRIP(T1) & Arg1, const STRIP(T2) & Arg2, const STRIP(T3) & Arg3, const STRIP(T4) & Arg4, const STRIP(T5) & Arg5,
        const STRIP(T6) & Arg6)
    {
        Callback = [this](CallbackArgs... _Args)
        {
            Result = new ResultType(_Args...);
            Completed = true;
        };

        this->Function = std::bind(Function, Context, Arg0, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, std::placeholders::_1);
    }

    template <typename ClassType, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
    Awaitable(void (ClassType::*Function)(T0, T1, T2, T3, T4, T5, T6, T7, CallbackType), ClassType* Context, const STRIP(T0) & Arg0,
        const STRIP(T1) & Arg1, const STRIP(T2) & Arg2, const STRIP(T3) & Arg3, const STRIP(T4) & Arg4, const STRIP(T5) & Arg5,
        const STRIP(T6) & Arg6, const STRIP(T7) & Arg7)
    {
        Callback = [this](CallbackArgs... _Args)
        {
            Result = new ResultType(_Args...);
            Completed = true;
        };

        this->Function = std::bind(Function, Context, Arg0, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, std::placeholders::_1);
    }

    template <typename ClassType, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
    Awaitable(void (ClassType::*Function)(T0, T1, T2, T3, T4, T5, T6, T7, T8, CallbackType), ClassType* Context, const STRIP(T0) & Arg0,
        const STRIP(T1) & Arg1, const STRIP(T2) & Arg2, const STRIP(T3) & Arg3, const STRIP(T4) & Arg4, const STRIP(T5) & Arg5,
        const STRIP(T6) & Arg6, const STRIP(T7) & Arg7, const STRIP(T8) & Arg8)
    {
        Callback = [this](CallbackArgs... _Args)
        {
            Result = new ResultType(_Args...);
            Completed = true;
        };

        this->Function = std::bind(Function, Context, Arg0, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8, std::placeholders::_1);
    }

    template <typename ClassType, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8,
        typename T9>
    Awaitable(void (ClassType::*Function)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, CallbackType), ClassType* Context, const STRIP(T0) & Arg0,
        const STRIP(T1) & Arg1, const STRIP(T2) & Arg2, const STRIP(T3) & Arg3, const STRIP(T4) & Arg4, const STRIP(T5) & Arg5,
        const STRIP(T6) & Arg6, const STRIP(T7) & Arg7, const STRIP(T8) & Arg8, const STRIP(T9) & Arg9)
    {
        Callback = [this](CallbackArgs... _Args)
        {
            Result = new ResultType(_Args...);
            Completed = true;
        };

        this->Function = std::bind(Function, Context, Arg0, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8, Arg9, std::placeholders::_1);
    }

    ~Awaitable()
    {
        if (Result)
        {
            delete Result;
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
    ResultType Await(std::function<bool(CallbackArgs...)> Predicate = nullptr) { return Await(DefaultTimeout, Predicate); }

    /// <summary>Wait for the asynchronous function to complete with a given timeout.</summary>
    /// <param name="Timeout">How long to wait before timing out.</param>
    /// <param name="Predicate">An optional function used to determine if the asynchronous function has completed.</param>
    /// <returns>The arguments passed to the asynchronous callback upon completion.</returns>
    ResultType Await(std::chrono::duration<int> Timeout, std::function<bool(CallbackArgs...)> Predicate = nullptr)
    {
#ifdef CSP_WASM
        // Spawn thread on wasm to prevent wait loop from blocking the async operation
        std::thread TestThread(
            [&]()
            {
#endif
                if (Predicate != nullptr)
                {
                    Function(
                        [this, Predicate](CallbackArgs... _Args)
                        {
                            if (Predicate(_Args...))
                            {
                                Callback(_Args...);
                            }
                        });
                }
                else
                {
                    Function(Callback);
                }

#ifdef CSP_WASM
            });
#endif

        auto StartTime = std::chrono::system_clock::now();
        auto EndTime = StartTime + Timeout;

        while (!Completed)
        {
            if (std::chrono::system_clock::now() >= EndTime)
            {
                throw timeout_exception("Await(): wait exceeded specified timeout");
            }

            std::this_thread::sleep_for(1ms);
        }

#ifdef CSP_WASM
        TestThread.join();
#endif

        return *Result;
    }

private:
    std::function<void(CallbackType)> Function;
    CallbackType Callback;
    ResultType* Result = nullptr;
    std::atomic_bool Completed = false;
};

// Helper macro for awaiting an async function
#define AWAIT(__instance_, __function_, ...) Awaitable(&std::remove_pointer_t<decltype(__instance_)>::__function_, __instance_, ##__VA_ARGS__).Await()
#define AWAIT_PRE(__instance_, __function_, __completion_predicate__, ...)                                                                           \
    Awaitable(&std::remove_pointer_t<decltype(__instance_)>::__function_, __instance_, ##__VA_ARGS__).Await(__completion_predicate__)

#undef STRIP