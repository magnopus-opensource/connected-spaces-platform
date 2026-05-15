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

const char GeneratedTestAccountEmailFormat[] = "testnopus.pokemon+%s@magnopus.com";
const char GeneratedTestAccountPassword[] = "3R{d2}3C<x[J7=jU";

// The implementation for all this is in UserSystemTests.cpp, if you can believe it.

void LogIn(csp::systems::UserSystem* userSystem, csp::common::String& outUserId, const csp::common::String& email,
    const csp::common::String& password, bool createMultiplayerConnection = true, bool ageVerified = true,
    const csp::systems::TokenOptions& tokenOptions = csp::systems::TokenOptions(),
    csp::systems::EResultCode expectedResultCode = csp::systems::EResultCode::Success,
    csp::systems::ERequestFailureReason expectedResultFailureCode = csp::systems::ERequestFailureReason::None);

void LogInAsGuest(csp::systems::UserSystem* userSystem, csp::common::String& outUserId, bool createMultiplayerConnection = true,
    const csp::systems::TokenOptions& tokenOptions = csp::systems::TokenOptions(),
    csp::systems::EResultCode expectedResult = csp::systems::EResultCode::Success);

void LogInAsGuestWithDeferredProfileCreation(csp::systems::UserSystem* userSystem, csp::common::String& outUserId,
    csp::systems::EResultCode expectedResult = csp::systems::EResultCode::Success);

void LogInAsNewTestUser(csp::systems::UserSystem* userSystem, csp::common::String& outUserId, bool createMultiplayerConnection = true,
    bool ageVerified = true, csp::systems::TokenOptions tokenOptions = csp::systems::TokenOptions(),
    csp::systems::EResultCode expectedResultCode = csp::systems::EResultCode::Success,
    csp::systems::ERequestFailureReason expectedResultFailureCode = csp::systems::ERequestFailureReason::None);

void LogInAsAdminUser(csp::systems::UserSystem* userSystem, csp::common::String& outUserId, bool createMultiplayerConnection = true,
    bool ageVerified = true, csp::systems::TokenOptions tokenOptions = csp::systems::TokenOptions(),
    csp::systems::EResultCode expectedResultCode = csp::systems::EResultCode::Success,
    csp::systems::ERequestFailureReason expectedResultFailureCode = csp::systems::ERequestFailureReason::None);

void LogOut(csp::systems::UserSystem* userSystem, csp::systems::EResultCode expectedResultCode = csp::systems::EResultCode::Success);

csp::systems::Profile CreateTestUser();

csp::systems::Profile GetFullProfileByUserId(csp::systems::UserSystem* userSystem, const csp::common::String& userId);