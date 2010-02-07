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
  SET(IPOD_FOUND TRUE)

else (IPOD_INCLUDE_DIRS AND IPOD_LIBRARIES)
  if(NOT WIN32)
    # use pkg-config to get the directories and then use these values
    # in the FIND_PATH() and FIND_LIBRARY() calls
    find_package(PkgConfig)
    PKG_SEARCH_MODULE(IPOD libgpod-1.0)
  else(NOT WIN32)
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
    endif(IPOD_INCLUDE_DIRS AND IPOD_LIBRARIES)
  endif(NOT WIN32)
  IF (IPOD_FOUND)
     IF (NOT IPOD_FIND_QUIETLY)
        MESSAGE(STATUS "Found libgpod-1 ${IPOD_VERSION}")
     ENDIF (NOT IPOD_FIND_QUIETLY)
  ELSE (IPOD_FOUND)
     IF (IPOD_FIND_REQUIRED)
        MESSAGE(FATAL_ERROR "Could NOT find libgpod-1, check FindPkgConfig output above!")
     ENDIF (IPOD_FIND_REQUIRED)
  ENDIF (IPOD_FOUND)

  MARK_AS_ADVANCED(IPOD_INCLUDE_DIRS)

endif (IPOD_INCLUDE_DIRS AND IPOD_LIBRARIES)
