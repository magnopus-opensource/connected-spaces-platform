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

#include "CSP/Common/String.h"
#include <string>
#include <vector>

/// Utility functions for compressing/decompressing a String in the GZIP data format.
/// GZIP is a lossless compression method that includes a cyclic redundancy check (CRC32) for detecting corruption.
/// GZIP is designed to be independent of CPU, OS and character set for interchange, and uses the DEFLATE compression method.
namespace csp::common::CompressionUtils
{

/// @brief Compresses a string into a vector of bytes using the GZIP format.
/// @param Data csp::common::String : The input string to compress.
/// @return std::vector<unsigned char> containing the compressed GZIP data.
/// @throw std::runtime_error on compression failure.
std::vector<unsigned char> CompressStringAsGzip(const csp::common::String& Data);

/// @brief Decompresses a vector of GZIP-formatted bytes back into a String.
/// @param CompressedData std::vector<unsigned char> : The input vector of compressed GZIP bytes.
/// @return csp::common::String containing the original decompressed data.
/// @throw std::runtime_error on decompression or validation failure.
csp::common::String DecompressGzipAsString(const std::vector<unsigned char>& CompressedData);

}
