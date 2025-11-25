/*
 * Copyright 2025 Magnopus LLC

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
    File to store any functions for use in the wrapper generator.
*/

#pragma once

#include "EmscriptenBindings/CallbackQueue.h"

namespace csp
{

// Generic callback function which will either call the callback directly, or,
// if we're on wasm, push to the main thread if we're not on the main thread.
template <typename... T> static void CallCallback(void (*Callback)(T...), T... Args)
{
#ifdef CSP_WASM
    csp::Emscripten_CallbackOnThread(Callback, Args...);
#else
    Callback(Args...);
#endif
}
}
