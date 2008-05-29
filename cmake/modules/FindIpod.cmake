# - Try to find the libgpod library
# Once done this will define
#
#  IPOD_FOUND - system has libgpod
#  IPOD_INCLUDE_DIR - the libgpod include directory
#  IPOD_LIBRARIES - Link these to use libgpod
#  IPOD_DEFINITIONS - Compiler switches required for using libgpod
#

if (IPOD_INCLUDE_DIR AND IPOD_LIBRARIES)

  # in cache already
  SET(IPOD_FOUND TRUE)

else (IPOD_INCLUDE_DIR AND IPOD_LIBRARIES)
  if(NOT WIN32)
    # use pkg-config to get the directories and then use these values
    # in the FIND_PATH() and FIND_LIBRARY() calls
    INCLUDE(UsePkgConfig)
  
    PKGCONFIG(libgpod-1.0 _IPODIncDir _IPODLinkDir _IPODLinkFlags _IPODCflags)
  
    set(IPOD_DEFINITIONS ${_IPODCflags})
  endif(NOT WIN32)

  FIND_PATH(IPOD_INCLUDE_DIR gpod-1.0/gpod/itdb.h
    ${_IPODIncDir}
  )
  
  FIND_LIBRARY(IPOD_LIBRARIES NAMES gpod
    PATHS
    ${_IPODLinkDir}
  )
  
  if (IPOD_INCLUDE_DIR AND IPOD_LIBRARIES)
     set(IPOD_FOUND TRUE)
  endif (IPOD_INCLUDE_DIR AND IPOD_LIBRARIES)
  
  if (IPOD_FOUND)
    if (NOT Ipod_FIND_QUIETLY)
      message(STATUS "Found IPOD: ${IPOD_LIBRARIES}")
    endif (NOT Ipod_FIND_QUIETLY)
  else (IPOD_FOUND)
    if (Ipod_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find IPOD")
    endif (Ipod_FIND_REQUIRED)
  endif (IPOD_FOUND)
  
  MARK_AS_ADVANCED(IPOD_INCLUDE_DIR IPOD_LIBRARIES)
  
endif (IPOD_INCLUDE_DIR AND IPOD_LIBRARIES)
