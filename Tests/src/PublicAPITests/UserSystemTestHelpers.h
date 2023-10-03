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



extern csp::common::String DefaultLoginEmail;
extern csp::common::String DefaultLoginPassword;
extern csp::common::String AlternativeLoginEmail;
extern csp::common::String AlternativeLoginPassword;

const char GeneratedTestAccountEmailFormat[] = "testnopus.pokemon+%s@magnopus.com";
const char GeneratedTestAccountPassword[]	 = "3R{d2}3C<x[J7=jU";


void LoadTestAccountCredentials();

void LogIn(csp::systems::UserSystem* UserSystem,
		   csp::common::String& OutUserId,
		   const csp::common::String& Email									 = DefaultLoginEmail,
		   const csp::common::String& Password								 = DefaultLoginPassword,
		   bool AgeVerified													 = true,
		   csp::services::EResultCode ExpectedResultCode					 = csp::services::EResultCode::Success,
		   csp::services::EResultBaseFailureReason ExpectedResultFailureCode = csp::services::EResultBaseFailureReason::None);

void LogInAsGuest(csp::systems::UserSystem* UserSystem,
				  csp::common::String& OutUserId,
				  csp::services::EResultCode ExpectedResult = csp::services::EResultCode::Success);
void LogOut(csp::systems::UserSystem* UserSystem, csp::services::EResultCode ExpectedResultCode = csp::services::EResultCode::Success);

csp::systems::Profile GetFullProfileByUserId(csp::systems::UserSystem* UserSystem, const csp::common::String& UserId);