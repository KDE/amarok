# - Find libavcodec and libavformat
# Find the libavcodec and libavformat include directories
# and libraries
#
# This module defines
#   FFMPEG_FOUND            - true if all of the following
#                            are located and set properly
#   LIBAVCODEC_INCLUDE_DIR  - libavcodec include directory
#   LIBAVCODEC_LIBRARY      - libavcodec library
#   LIBAVFORMAT_INCLUDE_DIR - libavformat include directory
#   LIBAVFORMAT_LIBRARY     - libavformat library

find_package( PkgConfig )
if( PKG_CONFIG_FOUND )
    # If pkg-config finds libavcodec, this will set:
    #   PC_LIBAVCODEC_FOUND (to TRUE)
    #   PC_LIBAVCODEC_INCLUDEDIR
    #   PC_LIBAVCODEC_INCLUDE_DIRS
    #   PC_LIBAVCODEC_LIBDIR
    #   PC_LIBAVCODEC_LIBRARY_DIRS
    # These variables are then used as hints to find_path()
    # and find_library()
    pkg_search_module( PC_LIBAVCODEC libavcodec )

    # Same as above, but searching for libavformat instead
    pkg_search_module( PC_LIBAVFORMAT libavformat )
endif( PKG_CONFIG_FOUND )

###############
## libavcodec #
###############

find_path( LIBAVCODEC_INCLUDE_DIR libavcodec/avcodec.h
    HINTS
        # Hints provided by pkg-config
        ${PC_LIBAVCODEC_INCLUDEDIR}
        ${PC_LIBAVCODEC_INCLUDEDIR}/*
        ${PC_LIBAVCODEC_INCLUDE_DIRS}
    PATHS
        # Standard include directories
        /usr/include/
        ~/usr/include/
        /opt/local/include/
        /usr/local/include/
        /opt/kde4/include/
        ${KDE4_INCLUDE_DIR}/
        # Search all subdirs of the above
        /usr/include/*
        ~/usr/include/*
        /opt/local/include/*
        /usr/local/include/*
        /opt/kde4/include/*
        ${KDE4_INCLUDE_DIR}/*
    PATH_SUFFIXES
        # Subdirectory hints
        libavcodec
        ffmpeg
        ffmpeg/libavcodec
)

find_library( LIBAVCODEC_LIBRARY avcodec
    HINTS
        # Hints provided by pkg-config
        ${PC_LIBAVCODEC_LIBDIR}
        ${PC_LIBAVCODEC_LIBRARY_DIRS}
    PATHS
        ~/usr/lib/
        /opt/local/lib/
        /usr/lib/
        /usr/lib64/
        /usr/local/lib/
        /opt/kde4/lib/
        ${KDE4_LIB_DIR}
)

################
## libavformat #
################

find_path( LIBAVFORMAT_INCLUDE_DIR libavformat/avformat.h
    HINTS
        # Hints provided by pkg-config
        ${PC_LIBAVFORMAT_INCLUDEDIR}
        ${PC_LIBAVFORMAT_INCLUDEDIR}/*
        ${PC_LIBAVFORMAT_INCLUDE_DIRS}
    PATHS
        # Standard include directories
        /usr/include/
        ~/usr/include/
        /opt/local/include/
        /usr/local/include/
        /opt/kde4/include/
        ${KDE4_INCLUDE_DIR}/
        # Search all subdirs of the above
        /usr/include/*
        ~/usr/include/*
        /opt/local/include/*
        /usr/local/include/*
        /opt/kde4/include/*
        ${KDE4_INCLUDE_DIR}/*
    PATH_SUFFIXES
        # Subdirectory hints
        libavformat
        ffmpeg
        ffmpeg/libavformat
)

find_library( LIBAVFORMAT_LIBRARY avformat
    HINTS
        # Hints provided by pkg-config
        ${PC_LIBAVFORMAT_LIBDIR}
        ${PC_LIBAVFORMAT_LIBRARY_DIRS}
    PATHS
        ~/usr/lib/
        /opt/local/lib/
        /usr/lib/
        /usr/lib64/
        /usr/local/lib/
        /opt/kde4/lib/
        ${KDE4_LIB_DIR}
)

include( FindPackageHandleStandardArgs )
# Sets FFMPEG_FOUND to true if all of the following are set:
#   LIBAVCODEC_INCLUDE_DIR
#   LIBAVCODEC_LIBRARY
#   LIBAVFORMAT_INCLUDE_DIR
#   LIBAVFORMAT_LIBRARY
find_package_handle_standard_args( FFmpeg DEFAULT_MSG
    LIBAVCODEC_INCLUDE_DIR
    LIBAVCODEC_LIBRARY
    LIBAVFORMAT_INCLUDE_DIR
    LIBAVFORMAT_LIBRARY
)
if( FFMPEG_FOUND )
    message( STATUS "\tlibavcodec: ${LIBAVCODEC_INCLUDE_DIR}, ${LIBAVCODEC_LIBRARY}" )
    message( STATUS "\tlibavformat: ${LIBAVFORMAT_INCLUDE_DIR}, ${LIBAVFORMAT_LIBRARY}" )
endif( FFMPEG_FOUND )

mark_as_advanced( LIBAVCODEC_LIBRARY LIBAVFORMAT_LIBRARY )
