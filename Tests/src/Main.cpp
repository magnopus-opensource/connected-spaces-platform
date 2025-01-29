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
#include "InternalTests/XMLTestResultWriter.h"
#include "PublicAPITests/UserSystemTestHelpers.h"
#include "TestHelpers.h"

int main(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv);

#ifdef CSP_WASM
    auto& Listeners = testing::UnitTest::GetInstance()->listeners();

    // Prevent googletest from writing to stdout to ensure the output xml isn't corrupted
    Listeners.Release(Listeners.default_result_printer());

    // Default xml writer hangs on wasm, so we need to add a custom one that writes to stdout
    auto* Listener = new TestListener();
    Listeners.Append(Listener);
#endif

#if RUN_ALL_UNIT_TESTS || RUN_ORGANIZATIONSYSTEM_TESTS
    LoadTestAccountCredentials(); // Needed as long as we cannot create superuser credentials on the fly
#endif

    int res = RUN_ALL_TESTS();

#ifdef CSP_WASM
    emscripten_force_exit(res);
#endif

    return res;
}