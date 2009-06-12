# - Find LibLastFM
# Find the liblastfm includes and the liblastfm libraries
# This module defines
# LIBLASTFM_INCLUDE_DIR, root lastfm include dir
# LIBLASTFM_LIBRARY, the path to liblastfm
# LIBLASTFM_FOUND, whether liblastfm was found


find_path(LIBLASTFM_INCLUDE_DIR Scrobbler
   ~/usr/include/lastfm
   /opt/local/include/lastfm
   /usr/include/lastfm
   /usr/local/include/lastfm
   /opt/kde4/include/lastfm
)

find_library( LIBLASTFM_LIBRARY NAMES lastfm
    PATHS
    ~/usr/lib
   /opt/local/lib
   /usr/lib
   /usr/lib64
   /usr/local/lib
   /opt/kde4/lib
)


if(LIBLASTFM_INCLUDE_DIR AND LIBLASTFM_LIBRARY)
   set(LIBLASTFM_FOUND TRUE)
   message(STATUS "Found liblastfm: ${LIBLASTFM_INCLUDE_DIR}, ${LIBLASTFM_LIBRARY}")
else(LIBLASTFM_INCLUDE_DIR AND LIBLASTFM_LIBRARY)
   set(LIBLASTFM_FOUND FALSE) 
   message(STATUS "liblastfm not found.")
endif(LIBLASTFM_INCLUDE_DIR AND LIBLASTFM_LIBRARY)

mark_as_advanced(LIBLASTFM_INCLUDE_DIR LIBLASTFM_LIBRARY)
