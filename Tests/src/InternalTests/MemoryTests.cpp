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
#ifndef SKIP_INTERNAL_TESTS

#include "CSP/CSPFoundation.h"
#include "Memory/Memory.h"
#include "Memory/MemoryHelpers.h"
#include "Memory/StlAllocator.h"
#include "TestHelpers.h"

#include "gtest/gtest.h"
#include <list>

using namespace csp::memory;

typedef csp::memory::StandardAllocator<csp::memory::MutexLockTrait> MemoryAllocator;
typedef csp::memory::StlAllocator<int> ListAllocator;
typedef std::list<int, ListAllocator> StlList;

CSP_INTERNAL_TEST(CSPEngine, MemoryTests, AllocationTest)
{
    struct TestObject
    {
        int32_t IntMember;
    };

    TestObject* Obj = CSP_NEW TestObject();
    EXPECT_TRUE(Obj != nullptr);
    CSP_DELETE(Obj);
}

CSP_INTERNAL_TEST(CSPEngine, MemoryTests, NewCustomAllocatorTest)
{
    MemoryAllocator MyAllocator;

    struct TestObject
    {
        int32_t IntMember;
    };

    EXPECT_TRUE(MyAllocator.GetAllocatedBytes() == 0);

    TestObject* Obj = CSP_NEW_P(&MyAllocator) TestObject();
    EXPECT_TRUE(Obj != nullptr);
    EXPECT_TRUE(MyAllocator.GetAllocatedBytes() == sizeof(TestObject));

    CSP_DELETE_P(Obj, &MyAllocator);
    EXPECT_TRUE(MyAllocator.GetAllocatedBytes() == 0);
}

CSP_INTERNAL_TEST(CSPEngine, MemoryTests, StlCustomAllocatorTest)
{
    MemoryAllocator MyAllocator;
    ListAllocator AllocatorWrapper(&MyAllocator);

    EXPECT_TRUE(MyAllocator.GetAllocatedBytes() == 0);

    StlList MyList(AllocatorWrapper);

    size_t InitialAllocatedBytes = MyAllocator.GetAllocatedBytes();
    EXPECT_TRUE(InitialAllocatedBytes >= 0);

    for (int i = 0; i < 10; ++i)
        MyList.push_back(i);

    EXPECT_TRUE(MyAllocator.GetAllocatedBytes() > InitialAllocatedBytes);

    MyList.clear();

    EXPECT_TRUE(MyAllocator.GetAllocatedBytes() == InitialAllocatedBytes);
}

CSP_INTERNAL_TEST(CSPEngine, MemoryTests, ReallocationTest)
{
    auto* Buffer = CSP_ALLOC(16);
    *(uint64_t*)Buffer = 0x0123456789ABCDEF;

    Buffer = CSP_REALLOC(Buffer, 32);
    EXPECT_TRUE(*(uint64_t*)Buffer == 0x0123456789ABCDEF);

    Buffer = CSP_REALLOC(Buffer, 128 * 1024);
    EXPECT_TRUE(*(uint64_t*)Buffer == 0x0123456789ABCDEF);
}

#endif