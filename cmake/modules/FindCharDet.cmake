# - Try to find the chardet library
# Once done this will define
#
#  CHARDET_FOUND - system has chardet
#  CHARDET_INCLUDE_DIRS - the chardet include directory
#  CHARDET_LIBRARIES - Link these to use chardet
#  CHARDET_CFLAGS - Compiler switches required for using chardet
#  CHARDET_VERSION - Version number of chardet
#

if (CHARDET_INCLUDE_DIRS AND CHARDET_LIBRARIES)

  # in cache already
  SET(CHARDET_FOUND TRUE)

else (CHARDET_INCLUDE_DIRS AND CHARDET_LIBRARIES)
  if(NOT WIN32)
    # use pkg-config to get the directories and then use these values
    # in the FIND_PATH() and FIND_LIBRARY() calls
    find_package(PkgConfig)
    PKG_SEARCH_MODULE(CHARDET chardet)

  endif(NOT WIN32)
  IF (CHARDET_FOUND)
     IF (NOT CHARDET_FIND_QUIETLY)
        MESSAGE(STATUS "Found chardet ${CHARDET_VERSION}")
     ENDIF (NOT CHARDET_FIND_QUIETLY)
  ELSE (CHARDET_FOUND)
     IF (CHARDET_FIND_REQUIRED)
        MESSAGE(FATAL_ERROR "Could NOT find chardet, check FindPkgConfig output above!")
     ENDIF (CHARDET_FIND_REQUIRED)
  ENDIF (CHARDET_FOUND)

  MARK_AS_ADVANCED(CHARDET_INCLUDE_DIRS)

endif (CHARDET_INCLUDE_DIRS AND CHARDET_LIBRARIES)
