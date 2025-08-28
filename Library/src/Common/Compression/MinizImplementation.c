/*
We have adopted the Miniz library (https://github.com/richgel999/miniz/tree/master) to handle compression and decompression of strings in the GZIP
format. Miniz has adopted the "stb-style" library pattern for packaging library code in a single header file for project integration. This means the
implementation is not parsed and compiled each time the header is included, but rather encapsulated in a big ifdef/endif block which is activated at a
single project include point. In addition to the miniz.c and .h files (in 'Thirdparty/miniz'), this file builds the implementation for the C-based
miniz library and ensures C linkage. It must be compiled as a C file (/TC complile flag) and must not use precompiled headers.
*/

#ifdef __cplusplus
extern "C"
{
#endif

#define MINIZ_IMPLEMENTATION
#include "miniz.c"

#ifdef __cplusplus
}
#endif
