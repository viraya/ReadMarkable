// Single-file compilation unit for stb_image.
// Only PNG decoding is needed (JPEG is handled by Qt's decoder which works fine).
// Disable everything except PNG to minimize binary size and attack surface.
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_STDIO          // we feed in-memory buffers, no file I/O
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#include "stb_image.h"
