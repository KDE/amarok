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
  set(NJB_FOUND TRUE)

else ()
  if(NOT WIN32)
    # use pkg-config to get the directories and then use these values
    # in the FIND_PATH() and FIND_LIBRARY() calls
    include(FindPkgConfig)
  
    pkg_check_modules(_NJB libnjb)
  
    set(NJB_DEFINITIONS ${_NJB_CFLAGS})
  endif()

  find_path(NJB_INCLUDE_DIR libnjb.h
    ${_NJB_INCLUDE_DIRS}
  )
  
  find_library(NJB_LIBRARIES NAMES njb
    PATHS
    ${_NJB_LIBRARY_DIRS}
  )
 

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Njb DEFAULT_MSG NJB_INCLUDE_DIR NJB_LIBRARIES )
  
  mark_as_advanced(NJB_INCLUDE_DIR NJB_LIBRARIES)
  
endif ()
