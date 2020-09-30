# - Try to find MariaDB library
# Find the MariaDB includes and client library
# This module defines
#  MYSQL_INCLUDE_DIR, where to find mysql.h
#  MYSQL_LIBRARIES, the libraries needed to use MariaDB.
#  MariaDB_FOUND, If false, do not try to use MariaDB.

# Copyright (c) 2006-2018, Jarosław Staniek <staniek@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(CheckCXXSourceCompiles)
include(CMakePushCheckState)

# First try to get information from mariadb_config which might be a shell script
# or an executable. Unfortunately not every distro has pkgconfig files for
# MariaDB.
find_program(MARIADBCONFIG_EXECUTABLE
    NAMES mariadb_config
    HINTS ${BIN_INSTALL_DIR}
)

if(MARIADBCONFIG_EXECUTABLE)
    execute_process(
        COMMAND ${MARIADBCONFIG_EXECUTABLE} --libs
        RESULT_VARIABLE MC_return_libraries
        OUTPUT_VARIABLE MYSQL_LIBRARIES
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

# Try searching manually via find_path/find_library, possibly with hints
# from pkg-config
find_package(PkgConfig)
pkg_check_modules(PC_MARIADB libmariadb)

find_path(MYSQL_INCLUDE_DIR mysql.h
    PATHS
        $ENV{MYSQL_INCLUDE_DIR}
        $ENV{MYSQL_DIR}/include
        ${PC_MARIADB_INCLUDEDIR}
        ${PC_MARIADB_INCLUDE_DIRS}
        /usr/local/mariadb/include
        /opt/mariadb/mariadb/include
    PATH_SUFFIXES
    mariadb
)

if(PC_MARIADB_VERSION)
    set(MySQL_VERSION_STRING ${PC_MARIADB_VERSION})
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(MariaDB
    REQUIRED_VARS MYSQL_LIBRARIES MYSQL_INCLUDE_DIR
    VERSION_VAR MySQL_VERSION_STRING
)

mark_as_advanced(MYSQL_INCLUDE_DIR MYSQL_LIBRARIES)

set_package_properties(MariaDB PROPERTIES
    DESCRIPTION "MariaDB Client Library (libmariadb)"
    URL "https://mariadb.org/"
)
