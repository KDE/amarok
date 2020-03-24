# - Try to find MySQL library
# Find the MySQL includes and client library
# This module defines
#  MYSQL_INCLUDE_DIR, where to find mysql.h
#  MYSQL_LIBRARIES, the libraries needed to use MySQL.
#  MySQL_FOUND, If false, do not try to use MySQL.

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

if(NOT MYSQL_LIBRARIES)
    find_library(MYSQL_LIBRARIES NAMES mysqlclient
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

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(MySQL
    REQUIRED_VARS MYSQL_LIBRARIES MYSQL_INCLUDE_DIR
    VERSION_VAR MySQL_VERSION_STRING
)

mark_as_advanced(MYSQL_INCLUDE_DIR MYSQL_LIBRARIES)

set_package_properties(MySQL PROPERTIES
    DESCRIPTION "MySQL Client Library (libmysqlclient)"
    URL "https://www.mysql.com"
)
