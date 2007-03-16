include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckSymbolExists)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckPrototypeExists)
include(CheckTypeSize)
include(MacroBoolTo01)

# The FindKDE4.cmake module sets _KDE4_PLATFORM_DEFINITIONS with
# definitions like _GNU_SOURCE that are needed on each platform.
set(CMAKE_REQUIRED_DEFINITIONS ${_KDE4_PLATFORM_DEFINITIONS})

#check for libz using the cmake supplied FindZLIB.cmake
macro_bool_to_01(ZLIB_FOUND HAVE_LIBZ)
macro_bool_to_01(JPEG_FOUND HAVE_LIBJPEG)
macro_bool_to_01(PNG_FOUND HAVE_LIBPNG)
macro_bool_to_01(CARBON_FOUND HAVE_CARBON)
macro_bool_to_01(NJB_FOUND HAVE_LIBNJB)
macro_bool_to_01(IFP_FOUND HAVE_IFP)
macro_bool_to_01(LIBVISUAL_FOUND HAVE_LIBVISUAL)
macro_bool_to_01(MTP_FOUND HAVE_MTP)

#now check for dlfcn.h using the cmake supplied CHECK_include_FILE() macro
# If definitions like -D_GNU_SOURCE are needed for these checks they
# should be added to _KDE4_PLATFORM_DEFINITIONS when it is originally
# defined outside this file.  Here we include these definitions in
# CMAKE_REQUIRED_DEFINITIONS so they will be included in the build of
# checks below.
set(CMAKE_REQUIRED_DEFINITIONS ${_KDE4_PLATFORM_DEFINITIONS})
if (WIN32)
   set(CMAKE_REQUIRED_LIBRARIES ${KDEWIN32_LIBRARIES} )
   set(CMAKE_REQUIRED_INCLUDES  ${KDEWIN32_INCLUDES} )
endif (WIN32)
