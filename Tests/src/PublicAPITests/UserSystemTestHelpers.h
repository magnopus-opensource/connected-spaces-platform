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

#include "CSP/Common/String.h"
#include "CSP/Systems/Users/UserSystem.h"

#include <optional>


extern csp::common::String DefaultLoginEmail;
extern csp::common::String DefaultLoginPassword;
extern csp::common::String AlternativeLoginEmail;
extern csp::common::String AlternativeLoginPassword;

const char GeneratedTestAccountEmailFormat[] = "testnopus.pokemon+%s@magnopus.com";
const char GeneratedTestAccountPassword[]	 = "3R{d2}3C<x[J7=jU";


void LoadTestAccountCredentials();

void LogOut(csp::systems::UserSystem* UserSystem, csp::services::EResultCode ExpectedResultCode = csp::services::EResultCode::Success);

/// <summary>
/// Attempts to log in with the provided details (or default account details if none provided).
/// Queues a call to `LogOut` to be executed after the test exits, unless otherwise specified.
/// </summary>
csp::common::String LogIn(csp::systems::UserSystem* UserSystem,
						  csp::common::Optional<csp::common::String> Email					   = nullptr,
						  csp::common::Optional<csp::common::String> Password				   = nullptr,
						  csp::common::Optional<csp::services::EResultCode> ExpectedResultCode = nullptr,
						  bool ShouldPushCleanupFunction									   = true);

/// <summary>
/// Attempts to log in as a guest user.
/// Queues a call to `LogOut` to be executed after the test exits, unless otherwise specified.
/// </summary>
csp::common::String LogInAsGuest(csp::systems::UserSystem* UserSystem,
								 csp::common::Optional<csp::services::EResultCode> ExpectedResultCode = nullptr,
								 bool ShouldPushCleanupFunction										  = true);

csp::systems::Profile GetFullProfileByUserId(csp::systems::UserSystem* UserSystem, const csp::common::String& UserId);