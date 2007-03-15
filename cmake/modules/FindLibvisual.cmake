# - Try to find the libvisual library
# Once done this will define
#
#  LIBVISUAL_FOUND - system has libvisual
#  LIBVISUAL_INCLUDE_DIR - the libvisual include directory
#  LIBVISUAL_LIBRARIES - Link these to use libvisual
#  LIBVISUAL_DEFINITIONS - Compiler switches required for using libvisual
#

if (LIBVISUAL_INCLUDE_DIR AND LIBVISUAL_LIBRARIES)

  # in cache already
  SET(LIBVISUAL_FOUND TRUE)

else (LIBVISUAL_INCLUDE_DIR AND LIBVISUAL_LIBRARIES)

  # use pkg-config to get the directories and then use these values
  # in the FIND_PATH() and FIND_LIBRARY() calls
  INCLUDE(UsePkgConfig)
  
  PKGCONFIG(libvisual _LIBVISUALIncDir _LIBVISUALLinkDir _LIBVISUALLinkFlags _LIBVISUALCflags)
  
  set(LIBVISUAL_DEFINITIONS ${_LIBVISUALCflags})
 
  FIND_PATH(NJB_INCLUDE_DIR libvisual.h
    ${_LIBVISUALIncDir}
    /usr/include
    /usr/local/include
  )
  
  FIND_LIBRARY(LIBVISUAL_LIBRARIES NAMES visual
    PATHS
    ${_LIBVISUALLinkDir}
    /usr/lib
    /usr/local/lib
  )
  
  if (LIBVISUAL_INCLUDE_DIR AND LIBVISUAL_LIBRARIES)
     set(LIBVISUAL_FOUND TRUE)
  endif (LIBVISUAL_INCLUDE_DIR AND LIBVISUAL_LIBRARIES)
  
  if (LIBVISUAL_FOUND)
    if (NOT Libvisual_FIND_QUIETLY)
      message(STATUS "Found LIBVISUAL: ${LIBVISUAL_LIBRARIES}")
    endif (NOT Libvisual_FIND_QUIETLY)
  else (LIBVISUAL_FOUND)
    if (Libvisual_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find LIBVISUAL")
    endif (Libvisual_FIND_REQUIRED)
  endif (LIBVISUAL_FOUND)
  
  MARK_AS_ADVANCED(LIBVISUAL_INCLUDE_DIR LIBVISUAL_LIBRARIES)
  
endif (LIBVISUAL_INCLUDE_DIR AND LIBVISUAL_LIBRARIES)
