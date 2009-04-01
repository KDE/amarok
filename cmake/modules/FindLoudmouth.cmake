# - Try to find the loudmouth library
# Once done this will define
#
#  LOUDMOUTH_FOUND - system has libgpod
#  LOUDMOUTH_INCLUDE_DIRS - the libgpod include directory
#  LOUDMOUTH_LIBRARIES - Link these to use libgpod
#  LOUDMOUTH_DEFINITIONS - Compiler switches required for using libgpod
#

if (LOUDMOUTH_INCLUDE_DIRS AND LOUDMOUTH_LIBRARIES)

  # in cache already
  SET(LOUDMOUTH_FOUND TRUE)

else (LOUDMOUTH_INCLUDE_DIRS AND LOUDMOUTH_LIBRARIES)
  if(NOT WIN32)
    # use pkg-config to get the directories and then use these values
    # in the FIND_PATH() and FIND_LIBRARY() calls
    find_package(PkgConfig)
    PKG_SEARCH_MODULE(LOUDMOUTH loudmouth-1.0)
  
  else(NOT WIN32)

    FIND_PATH(LOUDMOUTH_INCLUDE_DIRS loudmouth/loudmouth.h /usr/include/loudmouth-1.0
      ${_LOUDMOUTHIncDir}
    )
  
    FIND_LIBRARY(LOUDMOUTH_LIBRARIES NAMES loudmouth-1
      PATHS
      ${_LOUDMOUTHLinkDir}
    )

  endif(NOT WIN32)

  if (LOUDMOUTH_INCLUDE_DIRS AND LOUDMOUTH_LIBRARIES)
    SET(LOUDMOUTH_FOUND TRUE)
  else (LOUDMOUTH_INCLUDE_DIRS AND LOUDMOUTH_LIBRARIES)
    SET(LOUDMOUTH_FOUND_FALSE)
  endif (LOUDMOUTH_INCLUDE_DIRS AND LOUDMOUTH_LIBRARIES)

  include(FindPackageHandleStandardArgs)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(Loudmouth DEFAULT_MSG LOUDMOUTH_INCLUDE_DIRS LOUDMOUTH_LIBRARIES )
 
  MARK_AS_ADVANCED(LOUDMOUTH_INCLUDE_DIRS LOUDMOUTH_LIBRARIES)
  
endif (LOUDMOUTH_INCLUDE_DIRS AND LOUDMOUTH_LIBRARIES)
