# - Try to find the libmtp library
# Once done this will define
#
#  MTP_FOUND - system has libmtp
#  MTP_INCLUDE_DIR - the libmtp include directory
#  MTP_LIBRARIES - Link these to use libmtp
#  MTP_DEFINITIONS - Compiler switches required for using libmtp
#

if (MTP_INCLUDE_DIR AND MTP_LIBRARIES AND MTP_VERSION_OKAY)

  # in cache already
  set(MTP_FOUND TRUE)

else ()
  if(NOT WIN32)
    # use pkg-config to get the directories and then use these values
    # in the FIND_PATH() and FIND_LIBRARY() calls
    include(FindPkgConfig)
  
    pkg_check_modules(_MTP libmtp)
  
    set(MTP_DEFINITIONS ${_MTP_CFLAGS})
  endif()
  find_path(MTP_INCLUDE_DIR libmtp.h
    ${_MTP_INCLUDE_DIRS}
  )
  
  find_library(MTP_LIBRARIES NAMES mtp
    PATHS
    ${_MTP_LIBRARY_DIRS}
  )

  exec_program(${PKG_CONFIG_EXECUTABLE} ARGS --atleast-version=1.0.0 libmtp OUTPUT_VARIABLE _pkgconfigDevNull RETURN_VALUE MTP_VERSION_OKAY)
  
  if (MTP_INCLUDE_DIR AND MTP_LIBRARIES AND MTP_VERSION_OKAY STREQUAL "0")
     set(MTP_FOUND TRUE)
  endif ()
  
  if (MTP_FOUND)
    if (NOT Mtp_FIND_QUIETLY)
      message(STATUS "Found MTP: ${MTP_LIBRARIES}")
    endif ()
  else ()
    if (MTP_INCLUDE_DIR AND MTP_LIBRARIES AND NOT MTP_VERSION_OKAY STREQUAL "0")
      message(STATUS "Found MTP but version requirements not met")
    endif ()
    if (Mtp_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find MTP")
    endif ()
  endif ()
  
  mark_as_advanced(MTP_INCLUDE_DIR MTP_LIBRARIES MTP_VERSION_OKAY)
  
endif ()
