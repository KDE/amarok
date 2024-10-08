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
  set(LOUDMOUTH_FOUND TRUE)

else ()
  if(NOT WIN32)
    # use pkg-config to get the directories and then use these values
    # in the FIND_PATH() and FIND_LIBRARY() calls
    find_package(PkgConfig)
    pkg_search_module(LOUDMOUTH loudmouth-1.0)

  else()

    find_path(LOUDMOUTH_INCLUDE_DIRS loudmouth/loudmouth.h /usr/include/loudmouth-1.0
      ${_LOUDMOUTHIncDir}
    )

    find_library(LOUDMOUTH_LIBRARIES NAMES loudmouth-1
      PATHS
      ${_LOUDMOUTHLinkDir}
    )

  endif()

  if (LOUDMOUTH_INCLUDE_DIRS AND LOUDMOUTH_LIBRARIES)
    set(LOUDMOUTH_FOUND TRUE)
  else ()
    set(LOUDMOUTH_FOUND_FALSE)
  endif ()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Loudmouth DEFAULT_MSG LOUDMOUTH_INCLUDE_DIRS LOUDMOUTH_LIBRARIES )
 
  mark_as_advanced(LOUDMOUTH_INCLUDE_DIRS LOUDMOUTH_LIBRARIES)
  
endif ()
