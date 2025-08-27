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

#include "TestHelpers.h"
#include "gtest/gtest.h"

#include <string>

#include "CSP/Common/Systems/Log/LogSystem.h"
#include "Common/Compression/CompressionUtils.h"
#include "Debug/Logging.h"

static const csp::common::String TestString = "The quick brown fox jumps over the lazy dog."
                                              "The quick brown fox jumps over the lazy dog."
                                              "The quick brown fox jumps over the lazy dog."
                                              "The quick brown fox jumps over the lazy dog."
                                              "The quick brown fox jumps over the lazy dog.";

CSP_INTERNAL_TEST(CSPEngine, GzipTests, GzipCompressDecompressTest)
{
    // Compress the string
    std::vector<unsigned char> Compressed = csp::common::CompressionUtils::CompressStringAsGzip(TestString);

    // Decompress the data
    csp::common::String Decompressed = csp::common::CompressionUtils::DecompressGzipAsString(Compressed);

    // Verify the result
    int CompressedSize = static_cast<int>(Compressed.size()) * sizeof(unsigned char);
    size_t DecompressedSize = Decompressed.Length();
    EXPECT_TRUE(TestString == Decompressed);
    EXPECT_TRUE(CompressedSize < DecompressedSize);
}

CSP_INTERNAL_TEST(CSPEngine, GzipTests, GzipValidHeaderTest)
{
    std::vector<unsigned char> Compressed = csp::common::CompressionUtils::CompressStringAsGzip(TestString);

    // The expected 10-byte header.
    const unsigned char expected_header[] = {
        0x1f, 0x8b, // Magic number identifies the file as being in GZIP format
        0x08, // Compression method (DEFLATE algorithm)
        0x00, // Flags
        0x00, 0x00, 0x00, 0x00, // Modification time (unused)
        0x00, // Extra flags
        0x03 // Operating system (Unix)
    };

    // Checks that the compressed data starts with a valid GZIP header.
    EXPECT_EQ(Compressed[0], expected_header[0]); // Magic Number 1
    EXPECT_EQ(Compressed[1], expected_header[1]); // Magic Number 2
    EXPECT_EQ(Compressed[2], expected_header[2]); // Compression Method
    EXPECT_EQ(Compressed[3], expected_header[3]); // Flags
    EXPECT_EQ(Compressed[4], expected_header[4]); // MTIME 1
    EXPECT_EQ(Compressed[5], expected_header[5]); // MTIME 2
    EXPECT_EQ(Compressed[6], expected_header[6]); // MTIME 3
    EXPECT_EQ(Compressed[7], expected_header[7]); // MTIME 4
    EXPECT_EQ(Compressed[8], expected_header[8]); // Extra Flags
    EXPECT_EQ(Compressed[9], expected_header[9]); // OS
}

// TODO: Test each of the exceptions.
