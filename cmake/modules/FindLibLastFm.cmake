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
   /opt/kde4/include
   ${KDE4_INCLUDE_DIR}
   PATH_SUFFIXES lastfm
)

find_library( LIBLASTFM_LIBRARY NAMES lastfm
    PATHS
    ~/usr/lib
   /opt/local/lib
   /usr/lib
   /usr/lib64
   /usr/local/lib
   /opt/kde4/lib
   ${KDE4_LIB_DIR}
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
