# - Try to find the libgpod library
# Once done this will define
#
#  IPOD_FOUND - system has libgpod
#  IPOD_INCLUDE_DIRS - the libgpod include directory
#  IPOD_LIBRARIES - Link these to use libgpod
#  IPOD_CFLAGS - Compiler switches required for using libgpod
#  IPOD_VERSION - Version number of libgpod
#

if (IPOD_INCLUDE_DIRS AND IPOD_LIBRARIES)

  # in cache already
  set(IPOD_FOUND TRUE)

else ()
  if(NOT WIN32)
    # use pkg-config to get the directories and then use these values
    # in the FIND_PATH() and FIND_LIBRARY() calls
    find_package(PkgConfig)
    pkg_search_module(IPOD libgpod-1.0)
  else()
    find_path(IPOD_INCLUDE_DIRS
    NAMES
    gpod/itdb.h
    PATH_SUFFIXES gpod-1.0
    )

    find_library(IPOD_LIBRARIES NAMES
    gpod libgpod gpod-4 libgpod-4
    )
    if(IPOD_INCLUDE_DIRS AND IPOD_LIBRARIES)
       set(IPOD_FOUND ON)
    endif()
  endif()
  if (IPOD_FOUND)
     if (NOT IPOD_FIND_QUIETLY)
        message(STATUS "Found libgpod-1 ${IPOD_VERSION}")
     endif ()
  else ()
     if (IPOD_FIND_REQUIRED)
        message(FATAL_ERROR "Could NOT find libgpod-1, check FindPkgConfig output above!")
     endif ()
  endif ()

  mark_as_advanced(IPOD_INCLUDE_DIRS)

endif ()
