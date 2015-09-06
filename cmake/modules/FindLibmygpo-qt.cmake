
# - Find libmygpo-qt
# Find the libmygpo-qt includes and the libmygpo-qt libraries
# This module defines
# LIBMYGPO_QT_INCLUDE_DIR, root mygpo-qt include dir
# LIBMYGPO_QT_LIBRARY, the path to libmygpo-qt
# LIBMYGPO_QT_FOUND, whether libmygpo-qt was found


find_path(LIBMYGPO_QT_INCLUDE_DIR NAMES ApiRequest.h
   HINTS
   ~/usr/include
   /opt/local/include
   /usr/include
   /usr/local/include
   /opt/kde4/include
  ~/kde/include
  PATH_SUFFIXES mygpo-qt
)

find_library( LIBMYGPO_QT_LIBRARY NAMES mygpo-qt5
    PATHS
    ~/usr/lib
    ~/usr/lib64
   /opt/local/lib
   /opt/local/lib64
   /usr/lib
   /usr/lib64
   /usr/local/lib
   /usr/local/lib64
   /opt/kde4/lib
   /opt/kde4/lib64
    ~/kde/lib
    ~/kde/lib64
)


if(LIBMYGPO_QT_INCLUDE_DIR AND LIBMYGPO_QT_LIBRARY)
   set(LIBMYGPO_QT_FOUND TRUE)
   message(STATUS "Found libmygpo-qt: ${LIBMYGPO_QT_INCLUDE_DIR}, ${LIBMYGPO_QT_LIBRARY}")
else()
   set(LIBMYGPO_QT_FOUND FALSE)
   if (LIBMYGPO_QT_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find required package libmygpo-qt")
   endif()
endif()

mark_as_advanced(LIBMYGPO_QT_INCLUDE_DIR LIBMYGPO_QT_LIBRARY)
