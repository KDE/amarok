# - Try to find MySQL / MySQL Embedded library
# Find the MySQL includes and client library
# This module defines
#  MYSQL_INCLUDE_DIR, where to find mysql.h
#  MYSQL_LIBRARIES, the libraries needed to use MySQL.
#  MYSQL_LIB_DIR, path to the MYSQL_LIBRARIES
#  MYSQL_EMBEDDED_LIBRARIES, the libraries needed to use MySQL Embedded.
#  MYSQL_EMBEDDED_LIB_DIR, path to the MYSQL_EMBEDDED_LIBRARIES
#  MySQL_FOUND, If false, do not try to use MySQL.
#  MySQL_Embedded_FOUND, If false, do not try to use MySQL Embedded.
#  MYSQL_USING_MARIADB, If true MariaDB has been found and will be used as a replacement for MySQL

# Copyright (c) 2006-2018, Jarosław Staniek <staniek@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(CheckCXXSourceCompiles)
include(MacroPushRequiredVars)
set_package_properties(MySQL PROPERTIES
    DESCRIPTION "MySQL Client Library (libmysqlclient)" URL "http://www.mysql.com")

find_program(MYSQLCONFIG_EXECUTABLE NAMES mysql_config mysql_config5 HINTS ${BIN_INSTALL_DIR})

if(WIN32)
   set(ProgramFilesX86 "ProgramFiles(x86)")
   find_path(MYSQL_INCLUDE_DIR mysql.h
      PATHS
      $ENV{MYSQL_INCLUDE_DIR}
      $ENV{MYSQL_DIR}/include/mysql
      $ENV{ProgramW6432}/MySQL/*/include/mysql
      $ENV{ProgramFiles}/MySQL/*/include/mysql
      $ENV{${ProgramFilesX}}/MySQL/*/include/mysql
      $ENV{SystemDrive}/MySQL/*/include/mysql
      $ENV{ProgramW6432}/*/include/mysql
      $ENV{ProgramFiles}/*/include/mysql # MariaDB
      $ENV{${ProgramFilesX}}/*/include/mysql # MariaDB
   )
else()
   # use pkg-config to get the directories and then use these values
   # in the FIND_PATH() and FIND_LIBRARY() calls
   find_package(PkgConfig)
   pkg_check_modules(PC_MYSQL QUIET mysql mariadb)
   if(PC_MYSQL_VERSION)
       set(MySQL_VERSION_STRING ${PC_MYSQL_VERSION})
   endif()

   find_path(MYSQL_INCLUDE_DIR mysql.h
      PATHS
      $ENV{MYSQL_INCLUDE_DIR}
      $ENV{MYSQL_DIR}/include
      ${PC_MYSQL_INCLUDEDIR}
      ${PC_MYSQL_INCLUDE_DIRS}
      /usr/local/mysql/include
      /opt/mysql/mysql/include
      PATH_SUFFIXES
      mysql
   )
endif()

set(MYSQL_USING_MARIADB FALSE)
if(WIN32)
   string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_TOLOWER)

   # path suffix for debug/release mode
   # binary_dist: mysql binary distribution
   # build_dist: custom build
   if(CMAKE_BUILD_TYPE_TOLOWER MATCHES "debug")
      set(binary_dist debug)
      set(build_dist Debug)
   else()
      add_definitions(-DDBUG_OFF)
      set(binary_dist opt)
      set(build_dist Release)
   endif()

   set(MYSQL_LIB_PATHS
      $ENV{MYSQL_DIR}/lib/${binary_dist}
      $ENV{MYSQL_DIR}/libmysql/${build_dist}
      $ENV{MYSQL_DIR}/client/${build_dist}
      $ENV{ProgramW6432}/MySQL/*/lib/${binary_dist}
      $ENV{ProgramFiles}/MySQL/*/lib/${binary_dist}
      $ENV{${ProgramFilesX}}/MySQL/*/lib/${binary_dist}
      $ENV{SystemDrive}/MySQL/*/lib/${binary_dist}
      $ENV{ProgramW6432}/*/lib # MariaDB
      $ENV{ProgramFiles}/*/lib # MariaDB
      $ENV{${ProgramFilesX}}/*/lib # MariaDB
   )
   find_library(_LIBMYSQL_LIBRARY NAMES libmysql
      PATHS ${MYSQL_LIB_PATHS}
   )
   find_library(_MYSQLCLIENT_LIBRARY NAMES mysqlclient
      PATHS ${MYSQL_LIB_PATHS}
   )
   if(_LIBMYSQL_LIBRARY AND _MYSQLCLIENT_LIBRARY)
      set(MYSQL_LIBRARIES ${_LIBMYSQL_LIBRARY} ${_MYSQLCLIENT_LIBRARY})
   else()
      find_library(_LIBMYSQL_LIBRARY NAMES libmariadb
         PATHS ${MYSQL_LIB_PATHS}
      )
      find_library(_MYSQLCLIENT_LIBRARY NAMES mariadbclient
         PATHS ${MYSQL_LIB_PATHS}
      )
      if(_LIBMYSQL_LIBRARY AND _MYSQLCLIENT_LIBRARY)
         # once we find one MariaDB component, always search for MariaDB, not MySQL components
         set(MYSQL_USING_MARIADB TRUE)
         set(MYSQL_LIBRARIES ${_LIBMYSQL_LIBRARY} ${_MYSQLCLIENT_LIBRARY})
      endif()
   endif()
else() # !win32
   find_library(_MYSQLCLIENT_LIBRARY NAMES mysqlclient
      PATHS
      $ENV{MYSQL_DIR}/libmysql_r/.libs
      $ENV{MYSQL_DIR}/lib
      $ENV{MYSQL_DIR}/lib/mysql
      ${PC_MYSQL_LIBDIR}
      ${PC_MYSQL_LIBRARY_DIRS}
      PATH_SUFFIXES
      mysql
   )
   set(MYSQL_LIBRARIES ${_MYSQLCLIENT_LIBRARY})
   # TODO: set MYSQL_USING_MARIADB if MariaDB found
endif()

if(_LIBMYSQL_LIBRARY)
   get_filename_component(MYSQL_LIB_DIR ${_LIBMYSQL_LIBRARY} PATH)
   unset(_LIBMYSQL_LIBRARY)
endif()
if(_MYSQLCLIENT_LIBRARY)
    if(NOT MYSQL_LIB_DIR)
        get_filename_component(MYSQL_LIB_DIR ${_MYSQLCLIENT_LIBRARY} PATH)
    endif()
    unset(_MYSQLCLIENT_LIBRARY)
endif()

find_library(MYSQL_EMBEDDED_LIBRARIES NAMES mysqld
   PATHS
   $ENV{MYSQL_DIR}/libmysql_r/.libs
   $ENV{MYSQL_DIR}/lib
   $ENV{MYSQL_DIR}/lib/mysql
   ${PC_MYSQL_LIBDIR}
   ${PC_MYSQL_LIBRARY_DIRS}
   PATH_SUFFIXES
   mysql
)

if(MYSQL_EMBEDDED_LIBRARIES)
   get_filename_component(MYSQL_EMBEDDED_LIB_DIR ${MYSQL_EMBEDDED_LIBRARIES} PATH)

    macro_push_required_vars()
    set( CMAKE_REQUIRED_INCLUDES ${MYSQL_INCLUDE_DIR} )
    set( CMAKE_REQUIRED_LIBRARIES ${MYSQL_EMBEDDED_LIBRARIES} )
    check_cxx_source_compiles( "#include <mysql.h>\nint main() { int i = MYSQL_OPT_USE_EMBEDDED_CONNECTION; }" HAVE_MYSQL_OPT_EMBEDDED_CONNECTION )
    macro_pop_required_vars()

    exec_program(${MYSQLCONFIG_EXECUTABLE}
        ARGS --libmysqld-libs
        RETURN_VALUE _return_VALUE
        OUTPUT_VARIABLE MYSQL_EMBEDDED_LIBSTEMP
    )
    # Apparently the --libmysqld-libs option doesn't exist everywhere
    if(NOT _return_VALUE)
        set(MYSQL_EMBEDDED_LIBRARIES ${MYSQL_EMBEDDED_LIBSTEMP})
    endif()
endif()

# Did we find anything?
include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(MySQL
    REQUIRED_VARS MYSQL_LIBRARIES MYSQL_INCLUDE_DIR MYSQL_LIB_DIR
    VERSION_VAR MySQL_VERSION_STRING)
if(MYSQL_EMBEDDED_LIBRARIES AND MYSQL_EMBEDDED_LIB_DIR AND HAVE_MYSQL_OPT_EMBEDDED_CONNECTION)
    find_package_handle_standard_args(MySQL_Embedded
                                  REQUIRED_VARS MYSQL_EMBEDDED_LIBRARIES MYSQL_INCLUDE_DIR
                                                MYSQL_EMBEDDED_LIB_DIR
                                                HAVE_MYSQL_OPT_EMBEDDED_CONNECTION)
endif()

mark_as_advanced(MYSQL_INCLUDE_DIR MYSQL_LIBRARIES MYSQL_LIB_DIR
                 MYSQL_EMBEDDED_LIBRARIES MYSQL_EMBEDDED_LIB_DIR HAVE_MYSQL_OPT_EMBEDDED_CONNECTION
                 MYSQL_USING_MARIADB)

if(NOT MYSQL_FIND_QUIETLY)
   if(MYSQL_USING_MARIADB)
      message(STATUS "Found MariaDB, using as replacement for MySQL")
   endif()
endif()
