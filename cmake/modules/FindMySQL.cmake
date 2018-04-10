# - Try to find MySQL / MySQL Embedded library
# Find the MySQL includes and client library
# This module defines
#  MYSQL_INCLUDE_DIR, where to find mysql.h
#  MYSQL_LIBRARIES, the libraries needed to use MySQL.
#  MYSQL_EMBEDDED_LIBRARIES, the libraries needed to use MySQL Embedded.
#  MySQL_FOUND, If false, do not try to use MySQL.
#  MySQL_Embedded_FOUND, If false, do not try to use MySQL Embedded.

# Copyright (c) 2006-2018, Jarosław Staniek <staniek@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(CheckCXXSourceCompiles)
include(CMakePushCheckState)

# First try to get information from mysql_config which might be a shell script
# or an executable. Unfortunately not every distro has pkgconfig files for
# MySQL/MariaDB.
find_program(MYSQLCONFIG_EXECUTABLE
    NAMES mysql_config mysql_config5
    HINTS ${BIN_INSTALL_DIR}
)

if(MYSQLCONFIG_EXECUTABLE)
    execute_process(
        COMMAND ${MYSQLCONFIG_EXECUTABLE} --libs
        RESULT_VARIABLE MC_return_libraries
        OUTPUT_VARIABLE MYSQL_LIBRARIES
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
        COMMAND ${MYSQLCONFIG_EXECUTABLE} --libmysqld-libs
        RESULT_VARIABLE MC_return_embedded
        OUTPUT_VARIABLE MC_MYSQL_EMBEDDED_LIBRARIES
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(NOT MC_MYSQL_EMBEDDED_LIBRARIES)
        # At least on OpenSUSE --libmysql-libs doesn't exist, so we just use
        # MYSQL_LIBRARIES for that. We'll see if that's enough when testing
        # below.
        set(MYSQL_EMBEDDED_LIBRARIES ${MYSQL_LIBRARIES})
    else()
        set(MYSQL_EMBEDDED_LIBRARIES ${MC_MYSQL_EMBEDDED_LIBRARIES})
    endif()
endif()

# Try searching manually via find_path/find_library,  possibly with hints
# from pkg-config
find_package(PkgConfig)
pkg_check_modules(PC_MYSQL QUIET mysql mariadb)

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

if(NOT MYSQL_LIBRARIES AND NOT MYSQL_EMBEDDED_LIBRARIES)
    find_library(MYSQ_LIBRARY NAMES mysqlclient
        PATHS
            $ENV{MYSQL_DIR}/libmysql_r/.libs
            $ENV{MYSQL_DIR}/lib
            $ENV{MYSQL_DIR}/lib/mysql
            ${PC_MYSQL_LIBDIR}
            ${PC_MYSQL_LIBRARY_DIRS}
        PATH_SUFFIXES
            mysql
   )

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
endif()

if(PC_MYSQL_VERSION)
    set(MySQL_VERSION_STRING ${PC_MYSQL_VERSION})
endif()

if(MYSQL_EMBEDDED_LIBRARIES)
    # libmysqld on FreeBSD apparently doesn't properly report what libraries
    # it likes to link with, libmysqlclient does though.
    if(${CMAKE_HOST_SYSTEM_NAME} MATCHES "FreeBSD")
        string(REGEX REPLACE "-L.* -lmysqlclient " "" _mysql_libs ${MYSQL_LIBRARIES})
        set(MYSQL_EMBEDDED_LIBRARIES ${MYSQL_EMBEDDED_LIBRARIES} ${_mysql_libs})
    endif()
    cmake_push_check_state()
    set(CMAKE_REQUIRED_INCLUDES ${MYSQL_INCLUDE_DIR})
    set(CMAKE_REQUIRED_LIBRARIES ${MYSQL_EMBEDDED_LIBRARIES})
    check_cxx_source_compiles( "#include <mysql.h>\nint main() { int i = MYSQL_OPT_USE_EMBEDDED_CONNECTION; }" HAVE_MYSQL_OPT_EMBEDDED_CONNECTION )
    cmake_pop_check_state()
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(MySQL
    REQUIRED_VARS MYSQL_LIBRARIES MYSQL_INCLUDE_DIR
    VERSION_VAR MySQL_VERSION_STRING
)

find_package_handle_standard_args(MySQL_Embedded
    REQUIRED_VARS MYSQL_EMBEDDED_LIBRARIES MYSQL_INCLUDE_DIR HAVE_MYSQL_OPT_EMBEDDED_CONNECTION
)

mark_as_advanced(MYSQL_INCLUDE_DIR MYSQL_LIBRARIES
    MYSQL_EMBEDDED_LIBRARIES HAVE_MYSQL_OPT_EMBEDDED_CONNECTION
)
set_package_properties(MySQL PROPERTIES
    DESCRIPTION "MySQL Client Library (libmysqlclient)"
    URL "http://www.mysql.com"
)
