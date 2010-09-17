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
  if(NOT WIN32)
    # use pkg-config to get the directories and then use these values
    # in the FIND_PATH() and FIND_LIBRARY() calls
    INCLUDE(FindPkgConfig)
  
    pkg_check_modules(_NJB libnjb)
  
    set(NJB_DEFINITIONS ${_NJB_CFLAGS})
  endif(NOT WIN32)

  FIND_PATH(NJB_INCLUDE_DIR libnjb.h
    ${_NJB_INCLUDE_DIRS}
  )
  
  FIND_LIBRARY(NJB_LIBRARIES NAMES njb
    PATHS
    ${_NJB_LIBRARY_DIRS}
  )
 

  include(FindPackageHandleStandardArgs)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(Njb DEFAULT_MSG NJB_INCLUDE_DIR NJB_LIBRARIES )
  
  MARK_AS_ADVANCED(NJB_INCLUDE_DIR NJB_LIBRARIES)
  
endif (NJB_INCLUDE_DIR AND NJB_LIBRARIES)
