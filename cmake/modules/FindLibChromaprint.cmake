# - Find LibChromaprint
# Find the libchromaprint includes and the libchromaprint libraries
# This module defines
# LIBCHROMAPRINT_FOUND, whether libchromaprint was found. If it was, it further sets:
# LIBCHROMAPRINT_INCLUDE_DIR, root libchromaprint include dir
# LIBCHROMAPRINT_LIBRARY, the path to libchromaprint
# LIBCHROMAPRINT_VERSION, version of found libchromaprint as a string, e.g "0.3"


#find_path( LIBCHROMAPRINT_INCLUDE_DIR NAMES chromaprint.h
#   HINTS
#   ~/usr/include
#   /opt/local/include
#   /usr/include
#   /usr/local/include
#   /opt/kde4/include
#   ${KDE4_INCLUDE_DIR}
#   PATH_SUFFIXES chromaprint
#)

#find_library( LIBCHROMAPRINT_LIBRARY NAMES chromaprint
#    PATHS
#    ~/usr/lib
#   /opt/local/lib
#   /usr/lib
#   /usr/lib64
#   /usr/local/lib
#   /opt/kde4/lib
#   ${KDE4_LIB_DIR}
#)


#if(LIBCHROMAPRINT_INCLUDE_DIR AND LIBCHROMAPRINT_LIBRARY)
#   set(LIBCHROMAPRINT_FOUND TRUE)
#else(LIBCHROMAPRINT_INCLUDE_DIR AND LIBCHROMAPRINT_LIBRARY)
#   set(LIBCHROMAPRINT_FOUND FALSE)
#endif(LIBCHROMAPRINT_INCLUDE_DIR AND LIBCHROMAPRINT_LIBRARY)

#if(LIBCHROMAPRINT_FOUND)
#   set(regex "#define CHROMAPRINT_VERSION_STRING \"(.*)\"")
#   file(STRINGS "${LIBCHROMAPRINT_INCLUDE_DIR}/chromaprint.h" LIBCHROMAPRINT_VERSION REGEX ${regex})
#   if(${LIBCHROMAPRINT_VERSION} MATCHES ${regex})
#      set(LIBCHROMAPRINT_VERSION ${CMAKE_MATCH_1})
#      message(STATUS "Found libchromaprint: ${LIBCHROMAPRINT_INCLUDE_DIR}, ${LIBCHROMAPRINT_LIBRARY}, version ${LIBCHROMAPRINT_VERSION}")
#   else(${LIBCHROMAPRINT_VERSION} MATCHES ${regex})
#      message(WARNING "Found libchromaprint: ${LIBCHROMAPRINT_INCLUDE_DIR} - but failed to parse version")
#      set(LIBCHROMAPRINT_FOUND FALSE)
#      unset(LIBCHROMAPRINT_INCLUDE_DIR)
#      unset(LIBCHROMAPRINT_LIBRARY)
#   endif(${LIBCHROMAPRINT_VERSION} MATCHES ${regex})
#   unset(regex)
#endif(LIBCHROMAPRINT_FOUND)

#mark_as_advanced(LIBCHROMAPRINT_INCLUDE_DIR LIBCHROMAPRINT_LIBRARY)
