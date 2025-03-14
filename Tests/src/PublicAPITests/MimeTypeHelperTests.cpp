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

#include "CSP/Common/MimeTypeHelper.h"
#include "TestHelpers.h"

#include "gtest/gtest.h"

using namespace csp::common;

CSP_PUBLIC_TEST(CSPEngine, MimeTypeHelperTests, BasicTest)
{
    auto Helper = MimeTypeHelper::Get();

    EXPECT_EQ(Helper.GetMimeType("some/file/path.png"), "image/png");
    EXPECT_EQ(Helper.GetMimeType("some/file/path.jpg"), "image/jpeg");
    EXPECT_EQ(Helper.GetMimeType("some/file/path.jpeg"), "image/jpeg");
    EXPECT_EQ(Helper.GetMimeType("some/file/path.gltf"), "model/gltf-json");
    EXPECT_EQ(Helper.GetMimeType("some/file/path.glb"), "model/gltf-binary");
    EXPECT_EQ(Helper.GetMimeType("some/file/path.usdz"), "model/vnd.usdz+zip");
    EXPECT_EQ(Helper.GetMimeType("some/file/path.zip"), "application/zip");
    EXPECT_EQ(Helper.GetMimeType("some/file/path.unknown"), "application/octet-stream");
}

CSP_PUBLIC_TEST(CSPEngine, MimeTypeHelperTests, UppercaseTests)
{
    auto Helper = MimeTypeHelper::Get();
    EXPECT_EQ(Helper.GetMimeType("SOME/FILE/PATH.JPG"), "image/jpeg");
}

CSP_PUBLIC_TEST(CSPEngine, MimeTypeHelperTests, UnknownInputTest)
{
    auto Helper = MimeTypeHelper::Get();
    EXPECT_EQ(Helper.GetMimeType("some/path/to/a/file.unknown"), "application/octet-stream");
}

CSP_PUBLIC_TEST(CSPEngine, MimeTypeHelperTests, EmptyInputTest)
{
    auto Helper = MimeTypeHelper::Get();
    EXPECT_EQ(Helper.GetMimeType(""), "application/octet-stream");
}

CSP_PUBLIC_TEST(CSPEngine, MimeTypeHelperTests, NoExtensionTest)
{
    auto Helper = MimeTypeHelper::Get();
    EXPECT_EQ(Helper.GetMimeType("path_with_no_extension"), "application/octet-stream");
}

CSP_PUBLIC_TEST(CSPEngine, MimeTypeHelperTests, MultiplePeriodsTest)
{
    auto Helper = MimeTypeHelper::Get();
    EXPECT_EQ(Helper.GetMimeType("path.jpg.zip"), "application/zip");
}

CSP_PUBLIC_TEST(CSPEngine, MimeTypeHelperTests, WhitespaceTest)
{
    auto Helper = MimeTypeHelper::Get();
    EXPECT_EQ(Helper.GetMimeType("path.jpg      \n   "), "image/jpeg");
}

CSP_PUBLIC_TEST(CSPEngine, MimeTypeHelperTests, AccessMimeTypesTest) { EXPECT_EQ(MimeTypeHelper::Get().MimeType.IMAGE_JPEG, "image/jpeg"); }

CSP_PUBLIC_TEST(CSPEngine, MimeTypeHelperTests, AccessFileExtensionsTest) { EXPECT_EQ(MimeTypeHelper::Get().FileExtension.JPEG, "jpeg"); }
