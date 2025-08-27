#include "CompressionUtils.h"

#include <iostream>
#include <stdexcept>

// The Miniz library is a high-performance, single-source-file C library for lossless data compression.
// Miniz implements the zlib (RFC 1950) and DEFLATE (RFC 1951) standards.
#include "miniz.h"

namespace csp::common::CompressionUtils
{
// Helper function to convert a miniz status code (defined in the library as an anonymous C enum) to a descriptive string.
const char* MzStatusToString(int status)
{
    switch (status)
    {
    case MZ_OK:
        return "MZ_OK (0)";
    case MZ_STREAM_END:
        return "MZ_STREAM_END (1)";
    case MZ_NEED_DICT:
        return "MZ_NEED_DICT (2)";
    case MZ_ERRNO:
        return "MZ_ERRNO (-1)";
    case MZ_STREAM_ERROR:
        return "MZ_STREAM_ERROR (-2)";
    case MZ_DATA_ERROR:
        return "MZ_DATA_ERROR (-3)";
    case MZ_MEM_ERROR:
        return "MZ_MEM_ERROR (-4)";
    case MZ_BUF_ERROR:
        return "MZ_BUF_ERROR (-5)";
    case MZ_VERSION_ERROR:
        return "MZ_VERSION_ERROR (-6)";
    case MZ_PARAM_ERROR:
        return "MZ_PARAM_ERROR (-1000)";
    default:
        return "UNKNOWN_ERROR";
    }
}

// The compiler was unable to resolve the Miniz mz_crc32 function and so we are defining a type alias
// for a function pointer to store it.
typedef mz_ulong (*mz_crc32_func_ptr)(mz_ulong, const unsigned char*, size_t);

// Helper function to write a 32-bit value to a vector in little-endian format.
// The GZIP specification requires that its footer, which contains a CRC-32 checksum of the
// original data and it's size, be stored in little-endian byte order.
inline void Append32BitLittleEndian(std::vector<unsigned char>& Vector, mz_uint32 Value)
{
    Vector.push_back(static_cast<unsigned char>(Value & 0xFF));
    Vector.push_back(static_cast<unsigned char>((Value >> 8) & 0xFF));
    Vector.push_back(static_cast<unsigned char>((Value >> 16) & 0xFF));
    Vector.push_back(static_cast<unsigned char>((Value >> 24) & 0xFF));
}

std::vector<unsigned char> CompressStringAsGzip(const csp::common::String& Data)
{
    // GZIP file structure: (10 bytes) + DEFLATE Payload + GZIP Footer (8 bytes)

    std::vector<unsigned char> Result;

    // 1. GZIP Header
    // Construct the standard 10-byte GZIP header as defined by the RFC 1952 for GZIP format specification.
    // Every GZIP file or stream must begin with this header.
    const unsigned char GzipHeader[] = {
        0x1f, 0x8b, // Magic number identifies the file as being in GZIP format
        0x08, // Compression method (DEFLATE algorithm)
        0x00, // Flags
        0x00, 0x00, 0x00, 0x00, // Modification time (unused)
        0x00, // Extra flags
        0x03 // Operating system (Unix)
    };

    Result.insert(Result.end(), GzipHeader, GzipHeader + sizeof(GzipHeader));

    // 2. DEFLATE payload
    mz_stream Stream = {};

    // Memory level (0-9). A higher value generally results in slightly better compression and faster speed
    // at the cost of using more RAM during the compression process. 9 is a safe default.
    int MemoryLevel = 9;

    // Initialize miniz compression using the DEFLATE algorithm to set up a mz_stream object to hold the compression process.
    // MZ_DEFAULT_WINDOW_BITS defines the size of the sliding window buffer that the DEFLATE algorithm uses to find repeating byte sequences.
    // This creates a a 32KB window which is the standard size. A negative window_bits value tells miniz to produce a raw DEFLATE stream
    // with no zlib header/footer. This is what we requre to embed in the GZIP wrapper.
    int CompressStatus = mz_deflateInit2(&Stream, MZ_DEFAULT_COMPRESSION, MZ_DEFLATED, -MZ_DEFAULT_WINDOW_BITS, MemoryLevel, MZ_DEFAULT_STRATEGY);

    if (CompressStatus != MZ_OK)
    {
        throw std::runtime_error("mz_deflateInit2 failed with status " + std::to_string(CompressStatus));
    }

    // Set the input data for compression.
    Stream.next_in = reinterpret_cast<const unsigned char*>(Data.c_str());
    Stream.avail_in = static_cast<unsigned int>(Data.Length());

    // Declares a fixed-size array of 16,384 bytes (16 KB) on the stack.
    unsigned char OutBuffer[16384];

    // Takes the raw input data and uses the initialized miniz stream to process it in chunks.
    do
    {
        Stream.next_out = OutBuffer;
        Stream.avail_out = sizeof(OutBuffer);
        CompressStatus = mz_deflate(&Stream, MZ_FINISH);

        if (CompressStatus != MZ_STREAM_END && CompressStatus != MZ_OK)
        {
            mz_deflateEnd(&Stream);

            std::string ErrorMessage = "mz_deflate failed with status ";
            ErrorMessage += MzStatusToString(CompressStatus); // eg "MZ_STREAM_ERROR (-2)"
            throw std::runtime_error(ErrorMessage);
        }

        size_t compressed_chunk_size = sizeof(OutBuffer) - Stream.avail_out;
        Result.insert(Result.end(), OutBuffer, OutBuffer + compressed_chunk_size);

    } while (CompressStatus == MZ_OK);

    // Release internal memory allocated by mz_deflateInit2.
    mz_deflateEnd(&Stream);

    // 3. GZIP Footer
    // Append the 8-byte GZIP footer to the compressed data stream. The first 4 bytes are the CRC-32 checksum.
    // The second 4 bytes are the original data's size. Both are written in the correct little endian byte order.

    // Function ptr to hold the memory address of the Miniz CRC32 function.
    mz_crc32_func_ptr CRCFunc = mz_crc32;

    // Calculate CRC-32 checksum on the original uncompressed data.
    mz_uint32 CRC32Checksum = CRCFunc(0xFFFFFFFF, reinterpret_cast<const unsigned char*>(Data.c_str()), Data.Length());
    Append32BitLittleEndian(Result, CRC32Checksum);

    // Append the original size of the data to the footer.
    Append32BitLittleEndian(Result, static_cast<mz_uint32>(Data.Length()));

    return Result;
}

csp::common::String DecompressGzipAsString(const std::vector<unsigned char>& CompressedData)
{
    // Must have at least a header and footer.
    if (CompressedData.size() < 18)
    {
        throw std::runtime_error("Invalid GZIP data: too short");
    }

    // 1. Validate GZIP Header
    if (CompressedData[0] != 0x1f || CompressedData[1] != 0x8b)
    {
        throw std::runtime_error("Invalid GZIP data: incorrect magic number");
    }
    if (CompressedData[2] != 0x08)
    {
        throw std::runtime_error("Invalid GZIP data: unsupported compression method");
    }

    // 2. Decompress DEFLATE payload
    mz_stream Stream = {};
    // A negative window_bits value tells miniz to expect a raw DEFLATE stream.
    int DecompressStatus = mz_inflateInit2(&Stream, -MZ_DEFAULT_WINDOW_BITS);
    if (DecompressStatus != MZ_OK)
    {
        std::string ErrorMessage = "mz_inflateInit2 failed with status ";
        ErrorMessage += MzStatusToString(DecompressStatus); // eg "MZ_STREAM_ERROR (-2)"
        throw std::runtime_error(ErrorMessage);
    }

    // The payload is between the 10-byte header and 8-byte footer.
    Stream.next_in = CompressedData.data() + 10;
    Stream.avail_in = static_cast<unsigned int>(CompressedData.size() - 18);

    csp::common::String DecompressedData;
    unsigned char OutBuffer[16384];
    do
    {
        Stream.next_out = OutBuffer;
        Stream.avail_out = sizeof(OutBuffer);
        DecompressStatus = mz_inflate(&Stream, MZ_NO_FLUSH);

        if (DecompressStatus != MZ_STREAM_END && DecompressStatus != MZ_OK && DecompressStatus != MZ_BUF_ERROR)
        {
            mz_inflateEnd(&Stream);
            std::string ErrorMessage = "mz_inflate failed with status ";
            ErrorMessage += MzStatusToString(DecompressStatus); // eg "MZ_STREAM_ERROR (-2)"
            throw std::runtime_error(ErrorMessage);
        }

        DecompressedData.Append(reinterpret_cast<char*>(OutBuffer));

    } while (DecompressStatus == MZ_OK);

    mz_inflateEnd(&Stream);

    // 3. --- VALIDATE GZIP FOOTER ---
    // Extract CRC-32 and size from the footer (last 8 bytes).
    const unsigned char* pFooter = CompressedData.data() + CompressedData.size() - 8;
    mz_uint32 FooterCRC32 = pFooter[0] | (pFooter[1] << 8) | (pFooter[2] << 16) | (pFooter[3] << 24);
    mz_uint32 FooterSize = pFooter[4] | (pFooter[5] << 8) | (pFooter[6] << 16) | (pFooter[7] << 24);

    // Calculate CRC-32 of the actual decompressed data.
    mz_crc32_func_ptr CRCFunc = mz_crc32;
    mz_uint32 ActualCRC32Checksum = CRCFunc(0xFFFFFFFF, reinterpret_cast<const unsigned char*>(DecompressedData.c_str()), DecompressedData.Length());

    if (FooterCRC32 != ActualCRC32Checksum)
    {
        throw std::runtime_error("GZIP checksum mismatch!");
    }
    if (FooterSize != DecompressedData.Length())
    {
        throw std::runtime_error("GZIP size mismatch!");
    }

    return DecompressedData;
}

}
