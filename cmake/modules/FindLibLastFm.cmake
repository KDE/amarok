# - Find LibLastFM
# Find the liblastfm includes and the liblastfm libraries
# This module defines
# LIBLASTFM_FOUND, whether liblastfm was found. If it was, it further sets:
# LIBLASTFM_INCLUDE_DIR, root lastfm include dir
# LIBLASTFM_LIBRARY, the path to liblastfm
# LIBLASTFM_VERSION, version of found liblastfm as a string, e.g "0.3"


find_path(LIBLASTFM_INCLUDE_DIR NAMES global.h
HINTS
~/usr/include
/opt/local/include
/usr/include
/usr/local/include
PATH_SUFFIXES lastfm6
)

find_library( LIBLASTFM_LIBRARY NAMES lastfm6
    PATHS
    ~/usr/lib
/opt/local/lib
/usr/lib
/usr/lib64
/usr/local/lib
/usr/local/lib64
)


if(LIBLASTFM_INCLUDE_DIR AND LIBLASTFM_LIBRARY)
   set(LIBLASTFM_FOUND TRUE)
else()
   set(LIBLASTFM_FOUND FALSE)
endif()

if(LIBLASTFM_FOUND)
   set(regex "#define LASTFM_VERSION_STRING \"(.*)\"")
   file(STRINGS "${LIBLASTFM_INCLUDE_DIR}/global.h" LIBLASTFM_VERSION REGEX ${regex})
   if(${LIBLASTFM_VERSION} MATCHES ${regex})
      set(LIBLASTFM_VERSION ${CMAKE_MATCH_1})
      message(STATUS "Found liblastfm: ${LIBLASTFM_INCLUDE_DIR}, ${LIBLASTFM_LIBRARY}, version ${LIBLASTFM_VERSION}")
   else()
      message(WARNING "Found liblastfm: ${LIBLASTFM_INCLUDE_DIR} - but failed to parse version")
      set(LIBLASTFM_FOUND FALSE)
      unset(LIBLASTFM_INCLUDE_DIR)
      unset(LIBLASTFM_LIBRARY)
   endif()
   unset(regex)
endif()

mark_as_advanced(LIBLASTFM_INCLUDE_DIR LIBLASTFM_LIBRARY)
