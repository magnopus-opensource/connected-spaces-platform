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

/*
    Callback queue designed to push callbacks onto the main thread using the emscripten api.

    Issue -

    Emscripten/Wasm uses pthreads which run inside their own web worker.
    CSP receives a lot of its remote events from signalR, which can come back on a different thread from where they were registered.
    Due to these internal callbacks often invoking a client provided callback that was created on a different thread,
    this causes a 'table index is out of bounds' error due to the thread not being able to access the function pointer.

    Solution -

    To support this, CSP leverages emscriptens proxying api to push callbacks onto the main thread.
    This works by first checking if the callback is on the main thread, if so, will fire immediately.
    Otherwise, the callback arguments will be stored in a buffer and pushed to the main thread using the emscripten_proxy_sync method.

    Limitations -

    This currently only supports callers using the main thread, so if clients want to bind callbacks inside a client worker, this will fail.
    This behaviour can be implemented with this api by storing the thread the function was originally invoked on. However, due to this currently being
   used inside generated code, it would be hard to implement.

    Docs-

    Proxy api: https://emscripten.org/docs/api_reference/proxying.h.html
    Emscripten pthreads: https://emscripten.org/docs/porting/pthreads.html
*/

#pragma once

#ifdef CSP_WASM

#include <emscripten.h>
#include <emscripten/proxying.h>
#include <emscripten/threading.h>
#include <pthread.h>
#include <tuple>
#include <utility>

namespace csp
{
// Setup our proxy queue to which sends callbacks to the main thread
static em_proxying_queue* ProxyQueue = em_proxying_queue_create();
// Get a reference to the main thread to send callbacks to
static pthread_t MainThread = emscripten_main_runtime_thread_id();

// Structure to hold the callback and arguments.
// With the current implementation, the first argument will always be the callback context.
template <typename... T> struct CallbackData
{
    void (*Callback)(void*, T...);
    std::tuple<void*, T...> Args;
};

// Called internally from emscripten_proxy_sync on the main thread.
// This function is passed to the emscripten proxy api from Emscripten_CallbackOnThread to be called on the main thread.
template <typename... T> static void Emscripten_CallbackWrapper(void* InData)
{
    auto* Data = static_cast<CallbackData<T...>*>(InData);
    // Unpacks the tuple and passes values as the Data->Callback function arguments.
    std::apply(Data->Callback, Data->Args);
}

// Function that will push the callback to the main thread if we are not on it.
template <typename... T> static void Emscripten_CallbackOnThread(void (*Callback)(void*, T...), void* Context, T... Args)
{
    bool OnMainThread = pthread_equal(pthread_self(), emscripten_main_runtime_thread_id());
    if (OnMainThread)
    {
        // We're on the main thread already, just call normally
        Callback(Context, std::forward<T>(Args)...);
    }
    else
    {
        // Pack our data and send to the emscripten queue.
        CallbackData<T...> Data { Callback, std::tuple<void*, T...> { Context, std::forward<T>(Args)... } };

        // Passing refs is safe here due to emscripten_proxy_sync guaranteeing the callback finishes before Emscripten_CallbackOnThread returns.
        emscripten_proxy_sync(ProxyQueue, MainThread, Emscripten_CallbackWrapper<T...>, static_cast<void*>(&Data));
    }
}
}
#endif
