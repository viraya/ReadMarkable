cmake_minimum_required(VERSION 3.16)

# =============================================================================
# ReadMarkable third-party libraries
# Target: AArch64 cross-compilation for reMarkable Paper Pro Move (chiappa)
#
# Libraries built:
#   - pugixml 1.15          (vendored, MIT)
#   - QuaZip 1.5+           (submodule, LGPL)
#   - CREngine koreader fork (submodule, GPL2+)
#
# CREngine dependency strategy:
#   FROM SYSROOT: freetype2 (needs FT_HAS_COLOR, FT_Error_String, FT_SFNT_OS2),
#                 libjpeg-turbo (needs JCS_EXT_BGRX), harfbuzz
#   BUNDLED:      zlib, libpng, chmlib, antiword (old APIs  -  still compatible)
#   HEADER-ONLY:  xxhash (via XXH_INLINE_ALL define)
# =============================================================================

# Sysroot absolute path (set by SDK environment-setup script)
set(SYSROOT "$ENV{SDKTARGETSYSROOT}")

# -----------------------------------------------------------------------------
# 1. pugixml 1.15 (vendored)
# -----------------------------------------------------------------------------
add_subdirectory(pugixml)

# -----------------------------------------------------------------------------
# 2. QuaZip 1.5+  -  Qt6, static library
# Note: using a post-1.5 commit that makes Core5Compat optional.
# The chiappa SDK sysroot does NOT have Qt6Core5Compat; QUAZIP_ENABLE_QTEXTCODEC=OFF
# lets QuaZip build cleanly.
# -----------------------------------------------------------------------------
set(QUAZIP_QT_MAJOR_VERSION 6 CACHE STRING "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(QUAZIP_INSTALL OFF CACHE BOOL "" FORCE)
set(QUAZIP_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(QUAZIP_ENABLE_QTEXTCODEC OFF CACHE BOOL "" FORCE)
add_subdirectory(quazip)

# -----------------------------------------------------------------------------
# 3. CREngine (koreader fork)  -  static library for AArch64
#
# We bypass the koreader/crengine top-level CMakeLists.txt (it needs Qt4 and
# builds full GUI applications). We directly add the crengine library
# subdirectory after setting up all needed variables.
# -----------------------------------------------------------------------------

set(CRENGINE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/crengine)

# --- GUI flag: CRGUI_QT selects full feature set (epubfmt, lvdocview, etc.)
set(GUI "CRGUI_QT" CACHE STRING "" FORCE)

# --- Compile-time feature definitions
# Note: crsetup.h unconditionally sets USE_HARFBUZZ=1 for _LINUX  -  we accept
# this and provide sysroot harfbuzz. XXH_INLINE_ALL makes xxhash header-only.
add_compile_definitions(
    LINUX=1
    _LINUX=1
    COLOR_BACKBUFFER=1
    LDOM_USE_OWN_MEM_MAN=1
    USE_FREETYPE=1
    USE_FONTCONFIG=0
    CR_EMULATE_GETTEXT=1
    CR_INTERNAL_PAGE_ORIENTATION=0
    USE_UNRAR=0
    CHM_SUPPORT_ENABLED=1
    ENABLE_ANTIWORD=1
    CR3_ANTIWORD_PATCH=1
    CR3_PATCH=1
    DOC_DATA_COMPRESSION_LEVEL=1
    DOC_BUFFER_SIZE=0x400000
    _LARGEFILE64_SOURCE=1
    _FILE_OFFSET_BITS=64
    LVLONG_FILE_SUPPORT=1
    NDEBUG=1
    XXH_INLINE_ALL=1
)

# --- HarfBuzz from sysroot
set(HARFBUZZ_INCLUDE_DIR "${SYSROOT}/usr/include/harfbuzz")
set(HARFBUZZ_LIBRARIES harfbuzz)

# --- FreeType from sysroot (needs modern APIs: FT_HAS_COLOR, FT_Error_String)
# The bundled thirdparty/freetype is too old for the koreader CREngine fork.
set(FREETYPE_INCLUDE_DIRS "${SYSROOT}/usr/include/freetype2")
set(FREETYPE_LIBRARIES freetype)

# --- libjpeg-turbo from sysroot (needs JCS_EXT_BGRX, libjpeg-turbo extension)
# The bundled thirdparty/libjpeg is plain libjpeg without turbo extensions.
set(JPEG_INCLUDE_DIR "${SYSROOT}/usr/include")
set(JPEG_LIBRARIES jpeg)

# --- zlib (bundled)  -  target: z
add_subdirectory(${CRENGINE_SRC}/thirdparty/zlib
                 ${CMAKE_CURRENT_BINARY_DIR}/zlib)
set(ZLIB_INCLUDE_DIR ${CRENGINE_SRC}/thirdparty/zlib)
set(ZLIB_LIBRARIES z)
set(ZLIB_LIBRARY z)   # for libpng CMake

# --- libpng (bundled)  -  creates target: png16_static
set(PNG_STATIC ON CACHE BOOL "" FORCE)
set(PNG_SHARED OFF CACHE BOOL "" FORCE)
set(PNG_TESTS OFF CACHE BOOL "" FORCE)
set(SKIP_INSTALL_ALL ON CACHE BOOL "" FORCE)
add_subdirectory(${CRENGINE_SRC}/thirdparty/libpng
                 ${CMAKE_CURRENT_BINARY_DIR}/libpng)
set(SKIP_INSTALL_ALL OFF CACHE BOOL "" FORCE)
set(PNG_INCLUDE_DIR ${CRENGINE_SRC}/thirdparty/libpng)
set(PNG_LIBRARIES png16_static)
add_library(png ALIAS png16_static)

# --- chmlib (bundled)  -  target: chmlib
add_subdirectory(${CRENGINE_SRC}/thirdparty/chmlib
                 ${CMAKE_CURRENT_BINARY_DIR}/chmlib)
set(CHM_INCLUDE_DIR ${CRENGINE_SRC}/thirdparty/chmlib/src)
set(CHM_LIBRARIES chmlib)

# --- antiword (bundled)  -  target: antiword
add_subdirectory(${CRENGINE_SRC}/thirdparty/antiword
                 ${CMAKE_CURRENT_BINARY_DIR}/antiword)
set(ANTIWORD_INCLUDE_DIR ${CRENGINE_SRC}/thirdparty/antiword)
set(ANTIWORD_LIBRARIES antiword)

# --- tinydict (bundled)  -  target: tinydict
add_subdirectory(${CRENGINE_SRC}/tinydict
                 ${CMAKE_CURRENT_BINARY_DIR}/tinydict)

# --- Global include directories for CREngine compilation
include_directories(
    ${CRENGINE_SRC}/crengine/include    # lvdocview.h, lvstream.h, etc.
    ${CMAKE_CURRENT_SOURCE_DIR}         # xxhash.h (vendored beside submodule)
    ${HARFBUZZ_INCLUDE_DIR}             # hb.h, hb-ft.h (sysroot)
    ${FREETYPE_INCLUDE_DIRS}            # ft2build.h, freetype/freetype.h (sysroot)
    ${JPEG_INCLUDE_DIR}                 # jpeglib.h (sysroot libjpeg-turbo)
    ${ZLIB_INCLUDE_DIR}                 # zlib.h (bundled, CR3_PATCH guards qconfig.h)
    ${PNG_INCLUDE_DIR}                  # png.h (bundled)
    ${CMAKE_CURRENT_BINARY_DIR}/libpng  # generated pnglibconf.h
    ${CHM_INCLUDE_DIR}                  # chm_lib.h (bundled)
    ${ANTIWORD_INCLUDE_DIR}             # antiword headers (bundled)
    ${CRENGINE_SRC}/tinydict            # tinydict.h
    ${CRENGINE_SRC}/crengine/qimagescale # qimagescale_p.h (CRe::qSmoothScaleImage)
)

# --- Force-include limits.h for CREngine sources that use INT_MAX without including it
# (lvstring.cpp uses INT_MAX but doesn't include <limits.h>)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -include limits.h")

# --- CREngine core library  -  target: crengine
# Build directly (not via add_subdirectory) to exclude crgui.cpp and crskin.cpp.
# Those modules use LVDrawBuf::DrawTo which is inside #if 0 in the koreader fork.
# We only need the sources required for LVDocView (EPUB loading + rendering).
add_library(crengine STATIC
    ${CRENGINE_SRC}/crengine/src/cp_stats.cpp
    ${CRENGINE_SRC}/crengine/src/lvstring.cpp
    ${CRENGINE_SRC}/crengine/src/props.cpp
    ${CRENGINE_SRC}/crengine/src/lstridmap.cpp
    ${CRENGINE_SRC}/crengine/src/rtfimp.cpp
    ${CRENGINE_SRC}/crengine/src/cri18n.cpp
    ${CRENGINE_SRC}/crengine/src/lvmemman.cpp
    ${CRENGINE_SRC}/crengine/src/lvstyles.cpp
    ${CRENGINE_SRC}/crengine/src/crtxtenc.cpp
    ${CRENGINE_SRC}/crengine/src/lvtinydom.cpp
    ${CRENGINE_SRC}/crengine/src/lvstream.cpp
    ${CRENGINE_SRC}/crengine/src/lvxml.cpp
    ${CRENGINE_SRC}/crengine/src/lvstsheet.cpp
    ${CRENGINE_SRC}/crengine/src/txtselector.cpp
    ${CRENGINE_SRC}/crengine/src/crtest.cpp
    # Rendering sources (excludes crgui.cpp + crskin.cpp  -  old APIs not in koreader fork)
    ${CRENGINE_SRC}/crengine/src/lvbmpbuf.cpp
    ${CRENGINE_SRC}/crengine/src/lvfnt.cpp
    ${CRENGINE_SRC}/crengine/src/hyphman.cpp
    ${CRENGINE_SRC}/crengine/src/lvfntman.cpp
    ${CRENGINE_SRC}/crengine/src/lvimg.cpp
    ${CRENGINE_SRC}/crengine/src/lvdrawbuf.cpp
    ${CRENGINE_SRC}/crengine/src/lvdocview.cpp
    ${CRENGINE_SRC}/crengine/src/lvpagesplitter.cpp
    ${CRENGINE_SRC}/crengine/src/lvtextfm.cpp
    ${CRENGINE_SRC}/crengine/src/lvrend.cpp
    ${CRENGINE_SRC}/crengine/src/wolutil.cpp
    ${CRENGINE_SRC}/crengine/src/hist.cpp
    ${CRENGINE_SRC}/crengine/src/chmfmt.cpp
    ${CRENGINE_SRC}/crengine/src/epubfmt.cpp
    ${CRENGINE_SRC}/crengine/src/pdbfmt.cpp
    ${CRENGINE_SRC}/crengine/src/wordfmt.cpp
    ${CRENGINE_SRC}/crengine/src/textlang.cpp
    ${CRENGINE_SRC}/crengine/src/fb3fmt.cpp
    ${CRENGINE_SRC}/crengine/src/lvopc.cpp
    ${CRENGINE_SRC}/crengine/src/odxutil.cpp
    ${CRENGINE_SRC}/crengine/src/docxfmt.cpp
    ${CRENGINE_SRC}/crengine/src/odtfmt.cpp
    # qimagescale: provides CRe::qSmoothScaleImage used by lvdrawbuf.cpp for image scaling
    ${CRENGINE_SRC}/crengine/qimagescale/qimagescale.cpp
    # crconcurrent.cpp EXCLUDED: crlocks.h has #if 0, CRMutex/CRThread types disabled
    # in koreader fork; crconcurrent.cpp references those undefined types
)

# --- Interface target: crengine_with_deps
# Parent CMakeLists.txt links against this to get CREngine + all its deps.
add_library(crengine_with_deps INTERFACE)
target_link_libraries(crengine_with_deps INTERFACE
    crengine
    tinydict
    ${FREETYPE_LIBRARIES}       # sysroot libfreetype.so
    ${HARFBUZZ_LIBRARIES}       # sysroot libharfbuzz.so
    png16_static                # bundled
    ${JPEG_LIBRARIES}           # sysroot libjpeg-turbo
    ${ZLIB_LIBRARIES}           # bundled
    ${CHM_LIBRARIES}            # bundled
    ${ANTIWORD_LIBRARIES}       # bundled
)
target_include_directories(crengine_with_deps INTERFACE
    ${CRENGINE_SRC}/crengine/include
    ${FREETYPE_INCLUDE_DIRS}
    ${HARFBUZZ_INCLUDE_DIR}
)
