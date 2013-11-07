# Copyright (c) 2013, Vedant Agarwala <vedant.kota@gmail.com>
#
# Redistribution and use is allowed according to the terms of the GNU license.

# Find LibChromaprint
# Find the libchromaprint includes and the libchromaprint libraries
# This module defines
# LIBCHROMAPRINT_FOUND, whether libchromaprint was found. If it was, it further sets:
# LIBCHROMAPRINT_INCLUDE_DIR, root libchromaprint include dir
# LIBCHROMAPRINT_LIBRARY, the path to libchromaprint
# TODO: LIBCHROMAPRINT_VERSION, version of found libchromaprint as a string, e.g "0.3"
# TODO: also, edit the Amarok/CMakeLists.txt to add the MACRO_ENSURE_VERSION for chromaprint


find_path( LIBCHROMAPRINT_INCLUDE_DIR NAMES chromaprint.h
   HINTS
   ~/usr/include
   /opt/local/include
   /usr/include
   /usr/local/include
   /opt/kde4/include
   ${KDE4_INCLUDE_DIR}
   PATH_SUFFIXES chromaprint
)

find_library( LIBCHROMAPRINT_LIBRARY NAMES chromaprint
    PATHS
    ~/usr/lib
   /opt/local/lib
   /usr/lib
   /usr/lib64
   /usr/local/lib
   /opt/kde4/lib
   ${KDE4_LIB_DIR}
)


if(LIBCHROMAPRINT_INCLUDE_DIR AND LIBCHROMAPRINT_LIBRARY)
   set(LIBCHROMAPRINT_FOUND TRUE)
else(LIBCHROMAPRINT_INCLUDE_DIR AND LIBCHROMAPRINT_LIBRARY)
   set(LIBCHROMAPRINT_FOUND FALSE)
endif(LIBCHROMAPRINT_INCLUDE_DIR AND LIBCHROMAPRINT_LIBRARY)

# TODO: add code to find out the installed version

mark_as_advanced(LIBCHROMAPRINT_INCLUDE_DIR LIBCHROMAPRINT_LIBRARY)
