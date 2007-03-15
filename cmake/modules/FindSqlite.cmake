# - Try to find the SQLITE library
# Once done this will define
#
#  SQLITE_FOUND - system has sqlite
#  SQLITE_INCLUDE_DIR - the sqlite include directory
#  SQLITE_LIBRARIES - Link these to use sqlite
#  SQLITE_DEFINITIONS - Compiler switches required for using sqlite
#

if (SQLITE_INCLUDE_DIR AND SQLITE_LIBRARIES)

  # in cache already
  SET(SQLITE_FOUND TRUE)

else (SQLITE_INCLUDE_DIR AND SQLITE_LIBRARIES)

  # use pkg-config to get the directories and then use these values
  # in the FIND_PATH() and FIND_LIBRARY() calls
  INCLUDE(UsePkgConfig)
  
  PKGCONFIG(sqlite3 _SQLITEIncDir _SQLITELinkDir _SQLITELinkFlags _SQLITECflags)
  
  set(SQLITE_DEFINITIONS ${_SQLITECflags})
 
  FIND_PATH(SQLITE_INCLUDE_DIR qlite3.h
    ${_SQLITEIncDir}
    /usr/include
    /usr/local/include
  )
  
  FIND_LIBRARY(SQLITE_LIBRARIES NAMES sqlite3
    PATHS
    ${_SQLITELinkDir}
    /usr/lib
    /usr/local/lib
  )
  
  if (SQLITE_INCLUDE_DIR AND SQLITE_LIBRARIES)
     set(SQLITE_FOUND TRUE)
  endif (SQLITE_INCLUDE_DIR AND SQLITE_LIBRARIES)
  
  if (SQLITE_FOUND)
    if (NOT Sqlite_FIND_QUIETLY)
      message(STATUS "Found SQLITE: ${SQLITE_LIBRARIES}")
    endif (NOT Sqlite_FIND_QUIETLY)
  else (SQLITE_FOUND)
    if (Sqlite_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find SQLITE")
    endif (Sqlite_FIND_REQUIRED)
  endif (SQLITE_FOUND)
  
  MARK_AS_ADVANCED(SQLITE_INCLUDE_DIR SQLITE_LIBRARIES)
  
endif (SQLITE_INCLUDE_DIR AND SQLITE_LIBRARIES)
