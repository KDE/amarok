# - Try to find the loudmouth library
# Once done this will define
#
#  LOUDMOUTH_FOUND - system has libgpod
#  LOUDMOUTH_INCLUDE_DIR - the libgpod include directory
#  LOUDMOUTH_LIBRARIES - Link these to use libgpod
#  LOUDMOUTH_DEFINITIONS - Compiler switches required for using libgpod
#

if (LOUDMOUTH_INCLUDE_DIR AND LOUDMOUTH_LIBRARIES)

  # in cache already
  SET(LOUDMOUTH_FOUND TRUE)

else (LOUDMOUTH_INCLUDE_DIR AND LOUDMOUTH_LIBRARIES)
  if(NOT WIN32)
    # use pkg-config to get the directories and then use these values
    # in the FIND_PATH() and FIND_LIBRARY() calls
    INCLUDE(UsePkgConfig)
  
    PKGCONFIG(loudmouth-1.0 _LOUDMOUTHIncDir _LOUDMOUTHLinkDir _LOUDMOUTHLinkFlags _LOUDMOUTHCflags)
  
    set(LOUDMOUTH_DEFINITIONS ${_LOUDMOUTHCflags})
  endif(NOT WIN32)

  FIND_PATH(LOUDMOUTH_INCLUDE_DIR loudmouth/loudmouth.h /usr/include/loudmouth-1.0
    ${_LOUDMOUTHIncDir}
  )
  
  FIND_LIBRARY(LOUDMOUTH_LIBRARIES NAMES loudmouth-1
    PATHS
    ${_LOUDMOUTHLinkDir}
  )

  if (LOUDMOUTH_INCLUDE_DIR AND LOUDMOUTH_LIBRARIES)
    SET(LOUDMOUTH_FOUND TRUE)
  else (LOUDMOUTH_INCLUDE_DIR AND LOUDMOUTH_LIBRARIES)
    SET(LOUDMOUTH_FOUND_FALSE)
  endif (LOUDMOUTH_INCLUDE_DIR AND LOUDMOUTH_LIBRARIES)

  include(FindPackageHandleStandardArgs)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(Loudmouth DEFAULT_MSG LOUDMOUTH_INCLUDE_DIR LOUDMOUTH_LIBRARIES )
 
  MARK_AS_ADVANCED(LOUDMOUTH_INCLUDE_DIR LOUDMOUTH_LIBRARIES)
  
endif (LOUDMOUTH_INCLUDE_DIR AND LOUDMOUTH_LIBRARIES)
