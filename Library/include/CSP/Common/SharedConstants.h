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
 * Enums common between all the libraries that make up CSP.
 * These tend to merely be types for data exchange between libraries.
 * Don't get too hung up on namespaces
 */

#pragma once
#include "CSP/CSPCommon.h"
#include <cstdint>

namespace csp::common
{
CSP_START_IGNORE
/* The maximum value expressible in a JS `number`. Sort of the upper limit for all values that can flow through CSP. Would be nice to enforce this
   more at the type level */
static constexpr std::uint64_t Precision53Bits = 9007199254740991;

/* Client ID we assign for offline realtime engines. Here because the Systems module needs to know about this to populate EnterSpace result values, as
   well as the offline realtime engine wanting to use it. Is just std::uint53_t::max(), arbitrary, could have been 0 but that has issues with people
   sometimes using 0 to mean null or empty. */
static constexpr std::uint64_t LocalClientID = Precision53Bits;
CSP_END_IGNORE
} // namespace csp::common
