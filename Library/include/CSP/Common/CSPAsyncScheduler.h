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

// Default scheduler override implementation. Just mirrors `async::inline_scheduler` so we don't
// forget to set it when calling async chains, as forgetting won't manifest on desktop tests, but
// will cause errors on WASM. : https://github.com/Amanieu/asyncplusplus/wiki/Schedulers

// This file must be included before any async++ usage in order to get the right default scheduler
// As we set LIBASYNC_CUSTOM_DEFAULT_SCHEDULER in our build though, it should error if you forget.
// Generally, just include this rather than "async++.h" directly.

#pragma once

#include "CSP/CSPCommon.h"
CSP_START_IGNORE

// Forward declare custom scheduler
class CSPAsyncScheduler;

// Declare async::default_scheduler with my_scheduler
namespace async
{
inline CSPAsyncScheduler& default_scheduler();
}

// Technically redundant as we set this in the build, but it's safer to do it both ways as we'll
// definately get an error if we havn't set a default scheduler, regardless of include ordering.
#ifndef LIBASYNC_CUSTOM_DEFAULT_SCHEDULER
#define LIBASYNC_CUSTOM_DEFAULT_SCHEDULER
#endif

// Include async++.h.
// This should be the first time that async++ is included in the translation unit. For safety, just always include this file rather
// then async++.h directly.
#include <async++.h>

// Define the actual async scheduler, after the declaration such that we can avoid including "async++" before declaring the default scheduler override
class CSPAsyncScheduler
{
public:
    static void schedule(async::task_run_handle t) { t.run(); }
};

// Implementation of default_scheduler (override from async++)
CSPAsyncScheduler& async::default_scheduler()
{
    static CSPAsyncScheduler s;
    return s;
}

CSP_END_IGNORE
