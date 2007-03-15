# - Try to find the libnjb library
# Once done this will define
#
#  NJB_FOUND - system has libnjb
#  NJB_INCLUDE_DIR - the libnjb include directory
#  NJB_LIBRARIES - Link these to use libnjb
#  NJB_DEFINITIONS - Compiler switches required for using libnjb
#

if (NJB_INCLUDE_DIR AND NJB_LIBRARIES)

  # in cache already
  SET(NJB_FOUND TRUE)

else (NJB_INCLUDE_DIR AND NJB_LIBRARIES)

  # use pkg-config to get the directories and then use these values
  # in the FIND_PATH() and FIND_LIBRARY() calls
  INCLUDE(UsePkgConfig)
  
  PKGCONFIG(libnjb _NJBIncDir _NJBLinkDir _NJBLinkFlags _NJBCflags)
  
  set(NJB_DEFINITIONS ${_NJBCflags})
 
  FIND_PATH(NJB_INCLUDE_DIR libnjb.h
    ${_NJBIncDir}
    /usr/include
    /usr/local/include
  )
  
  FIND_LIBRARY(NJB_LIBRARIES NAMES njb
    PATHS
    ${_NJBLinkDir}
    /usr/lib
    /usr/local/lib
  )
  
  if (NJB_INCLUDE_DIR AND NJB_LIBRARIES)
     set(NJB_FOUND TRUE)
  endif (NJB_INCLUDE_DIR AND NJB_LIBRARIES)
  
  if (NJB_FOUND)
    if (NOT Njb_FIND_QUIETLY)
      message(STATUS "Found NJB: ${NJB_LIBRARIES}")
    endif (NOT Njb_FIND_QUIETLY)
  else (NJB_FOUND)
    if (Njb_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find NJB")
    endif (Njb_FIND_REQUIRED)
  endif (NJB_FOUND)
  
  MARK_AS_ADVANCED(NJB_INCLUDE_DIR NJB_LIBRARIES)
  
endif (NJB_INCLUDE_DIR AND NJB_LIBRARIES)
