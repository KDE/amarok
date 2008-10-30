# - Find LibLastFM
# Find the liblastfm includes and the liblastfm libraries
# This module defines
# LIBLASTFM_INCLUDE_DIR, root lastfm include dir
# LIBLASTFM_LIBRARIES, the liblastfm libs, e.g. libcore libradio libscrobble libws libtypes
# LIBLASTFM_FOUND, whether liblastfm was found


find_path(LIBLASTFM_INCLUDE_DIR Scrobbler.h 
   ~/usr/include/lastfm
   /opt/local/include/lastfm
   /usr/include/lastfm
   /usr/local/include/lastfm
   /opt/kde4/include/lastfm
)

find_library( LIBLASTFM_LIBRARY_CORE NAMES core
    PATHS
    ~/usr/lib
   /opt/local/lib
   /usr/lib
   /usr/lib64
   /usr/local/lib
   /opt/kde4/lib
)

find_library( LIBLASTFM_LIBRARY_WS NAMES ws
    PATHS
    ~/usr/lib
   /opt/local/lib
   /usr/lib
   /usr/lib64
   /usr/local/lib
   /opt/kde4/lib
)

find_library( LIBLASTFM_LIBRARY_RADIO NAMES radio
    PATHS
    ~/usr/lib
   /opt/local/lib
   /usr/lib
   /usr/lib64
   /usr/local/lib
   /opt/kde4/lib
)
find_library( LIBLASTFM_LIBRARY_TYPES NAMES types
    PATHS
    ~/usr/lib
   /opt/local/lib
   /usr/lib
   /usr/lib64
   /usr/local/lib
   /opt/kde4/lib
)
find_library( LIBLASTFM_LIBRARY_SCROBBLE NAMES scrobble
    PATHS
    ~/usr/lib
   /opt/local/lib
   /usr/lib
   /usr/lib64
   /usr/local/lib
   /opt/kde4/lib
)

if(LIBLASTFM_INCLUDE_DIR AND LIBLASTFM_LIBRARY_CORE AND LIBLASTFM_LIBRARY_WS AND LIBLASTFM_LIBRARY_TYPES AND LIBLASTFM_LIBRARY_RADIO AND LIBLASTFM_LIBRARY_SCROBBLE)
   set(LIBLASTFM_FOUND TRUE)
   message(STATUS "Found liblastfm: ${LIBLASTFM_INCLUDE_DIR}, ${LIBLASTFM_LIBRARY_SCROBBLE}")
else(LIBLASTFM_INCLUDE_DIR AND LIBLASTFM_LIBRARY_CORE AND LIBLASTFM_LIBRARY_WS AND LIBLASTFM_LIBRARY_TYPES AND LIBLASTFM_LIBRARY_RADIO AND LIBLASTFM_LIBRARY_SCROBBLE)
   set(LIBLASTFM_FOUND FALSE) 
   message(STATUS "liblastfm not found.")
endif(LIBLASTFM_INCLUDE_DIR AND LIBLASTFM_LIBRARY_CORE AND LIBLASTFM_LIBRARY_WS AND LIBLASTFM_LIBRARY_TYPES AND LIBLASTFM_LIBRARY_RADIO AND LIBLASTFM_LIBRARY_SCROBBLE)

mark_as_advanced(LIBLASTFM_INCLUDE_DIR LIBLASTFM_LIBRARY_CORE LIBLASTFM_LIBRARY_WS LIBLASTFM_LIBRARY_TYPES LIBLASTFM_LIBRARY_RADIO LIBLASTFM_LIBRARY_SCROBBLE)
# - Find LibLastFM
# Find the liblastfm includes and the liblastfm libraries
# This module defines
# LIBLASTFM_INCLUDE_DIR, root lastfm include dir
# LIBLASTFM_LIBRARIES, the liblastfm libs, e.g. libcore libradio libscrobble libws libtypes
# LIBLASTFM_FOUND, whether liblastfm was found


find_path(LIBLASTFM_INCLUDE_DIR Scrobbler.h 
   ~/usr/include/lastfm
   /opt/local/include/lastfm
   /usr/include/lastfm
   /usr/local/include/lastfm
   /opt/kde4/include/lastfm
)

find_library( LIBLASTFM_LIBRARY_CORE NAMES core
    PATHS
    ~/usr/lib
   /opt/local/lib
   /usr/lib
   /usr/lib64
   /usr/local/lib
   /opt/kde4/lib
)

find_library( LIBLASTFM_LIBRARY_WS NAMES ws
    PATHS
    ~/usr/lib
   /opt/local/lib
   /usr/lib
   /usr/lib64
   /usr/local/lib
   /opt/kde4/lib
)

find_library( LIBLASTFM_LIBRARY_RADIO NAMES radio
    PATHS
    ~/usr/lib
   /opt/local/lib
   /usr/lib
   /usr/lib64
   /usr/local/lib
   /opt/kde4/lib
)
find_library( LIBLASTFM_LIBRARY_TYPES NAMES types
    PATHS
    ~/usr/lib
   /opt/local/lib
   /usr/lib
   /usr/lib64
   /usr/local/lib
   /opt/kde4/lib
)
find_library( LIBLASTFM_LIBRARY_SCROBBLE NAMES scrobble
    PATHS
    ~/usr/lib
   /opt/local/lib
   /usr/lib
   /usr/lib64
   /usr/local/lib
   /opt/kde4/lib
)

if(LIBLASTFM_INCLUDE_DIR AND LIBLASTFM_LIBRARY_CORE AND LIBLASTFM_LIBRARY_WS AND LIBLASTFM_LIBRARY_TYPES AND LIBLASTFM_LIBRARY_RADIO AND LIBLASTFM_LIBRARY_SCROBBLE)
   set(LIBLASTFM_FOUND TRUE)
   message(STATUS "Found liblastfm: ${LIBLASTFM_INCLUDE_DIR}, ${LIBLASTFM_LIBRARY_SCROBBLE}")
else(LIBLASTFM_INCLUDE_DIR AND LIBLASTFM_LIBRARY_CORE AND LIBLASTFM_LIBRARY_WS AND LIBLASTFM_LIBRARY_TYPES AND LIBLASTFM_LIBRARY_RADIO AND LIBLASTFM_LIBRARY_SCROBBLE)
   set(LIBLASTFM_FOUND FALSE) 
   message(STATUS "liblastfm not found.")
endif(LIBLASTFM_INCLUDE_DIR AND LIBLASTFM_LIBRARY_CORE AND LIBLASTFM_LIBRARY_WS AND LIBLASTFM_LIBRARY_TYPES AND LIBLASTFM_LIBRARY_RADIO AND LIBLASTFM_LIBRARY_SCROBBLE)

mark_as_advanced(LIBLASTFM_INCLUDE_DIR LIBLASTFM_LIBRARY_CORE LIBLASTFM_LIBRARY_WS LIBLASTFM_LIBRARY_TYPES LIBLASTFM_LIBRARY_RADIO LIBLASTFM_LIBRARY_SCROBBLE)
