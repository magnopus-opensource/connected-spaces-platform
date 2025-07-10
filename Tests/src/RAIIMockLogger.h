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
#pragma once

#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include <gmock/gmock.h>

/* We need to unset the mock logger before CSP shuts down,
 * because you get interdependent memory errors in the "Foundation shutdown"
 * log if you don't. (Another reason we don't want to be starting/stopping
 * ALL of CSP in these tests really.)
 */
struct RAIIMockLogger
{
    RAIIMockLogger() { csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(MockLogCallback.AsStdFunction()); }
    ~RAIIMockLogger() { csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr); }
    ::testing::MockFunction<void(const csp::common::String&)> MockLogCallback;
};