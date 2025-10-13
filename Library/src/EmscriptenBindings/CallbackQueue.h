/*
    Callback queue designed to push callbacks onto the main thread using the emscripten api.
    This works because emscriptens threads are just web workers, which allows us to push messages to different workers.

    This is implemented to work around the limitation of callbacks needing to fire on the same thread they were created on.
*/

#ifdef CSP_WASM

#include <emscripten.h>
#include <emscripten/proxying.h>
#include <emscripten/threading.h>
#include <pthread.h>
#include <tuple>

namespace csp
{
static em_proxying_queue* ProxyQueue = em_proxying_queue_create();
static pthread_t MainThread = emscripten_main_runtime_thread_id();

// Structure to hold the callback and arguments.
// With the current implementation, the first argument will always be the callback context.
template <typename... T> struct CallbackData
{
    void (*Callback)(T...);
    std::tuple<T...> Args;
};

// This function is called internally from emscripten_proxy_sync on the main thread.
template <typename... T> static void Emscripten_CallbackWrapper(void* InData)
{
    auto* Data = static_cast<CallbackData<T...>*>(InData);
    std::apply(Data->Callback, Data->Args);
}

// Function that will push the callback to the main thread if we are not on it.
template <typename... T> static void Emscripten_CallbackOnThread(void (*Callback)(T...), T... Args)
{
    bool OnMainThread = pthread_equal(pthread_self(), emscripten_main_runtime_thread_id());
    if (OnMainThread)
    {
        // We're on the main thread already, just call normally
        Callback(Args...);
    }
    else
    {
        // Pack our data and send to the emscripten queue.
        CallbackData<T...> Data { Callback, std::make_tuple(Args...) };
        emscripten_proxy_sync(ProxyQueue, MainThread, Emscripten_CallbackWrapper<T...>, static_cast<void*>(&Data));
    }
}
}
#endif
