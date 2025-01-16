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

#include <gtest/gtest.h>

/// @brief Custom xml writer for googletest on wasm that writes to stdout.
class TestListener : public testing::EmptyTestEventListener
{
public:
    void OnTestIterationEnd(const testing::UnitTest& UnitTest, int /*iteration*/) override;

    virtual ~TestListener() = default;
};
