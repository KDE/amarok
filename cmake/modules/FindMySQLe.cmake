# - Try to find MySQL Embedded library
# Find the MySQL embedded library
# This module defines
#  MYSQLE_LIBRARIES, the libraries needed to use MySQL Embedded.
#  MySQLe_FOUND, If false, do not try to use MySQL Embedded.

# Copyright (c) 2006-2018, Jarosław Staniek <staniek@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(CheckCXXSourceCompiles)
include(CMakePushCheckState)

if(NOT MySQL_FOUND)
    find_package(MySQL)

    if( NOT MySQL_FOUND )
        find_package(MariaDB REQUIRED)
    endif()
endif()

if(MySQL_FOUND OR MariaDB_FOUND)

# First try to get information from mysql_config which might be a shell script
# or an executable. Unfortunately not every distro has pkgconfig files for
# MySQL/MariaDB.
find_program(MYSQLCONFIG_EXECUTABLE
    NAMES mysql_config mysql_config5
    HINTS ${BIN_INSTALL_DIR}
)

if(MYSQLCONFIG_EXECUTABLE)
    execute_process(
        COMMAND ${MYSQLCONFIG_EXECUTABLE} --libmysqld-libs
        RESULT_VARIABLE MC_return_embedded
        OUTPUT_VARIABLE MC_MYSQLE_LIBRARIES
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if ("${MC_return_embedded}" STREQUAL "0")
        set(MYSQLE_LIBRARIES ${MC_MYSQLE_LIBRARIES})
    endif()
endif()

# Try searching manually via find_path/find_library, possibly with hints
# from pkg-config
find_package(PkgConfig)
pkg_check_modules(PC_MYSQL QUIET mysql mariadb)

if(NOT MYSQLE_LIBRARIES)
# mysql-config removed --libmysql-libs, but amarok needs libmysqld other
# than libmysqlclient to run mysql embedded server.
    find_library(MYSQLE_LIBRARIES NAMES mysqld libmysqld
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
    set(MySQLe_VERSION_STRING ${PC_MYSQL_VERSION})
endif()

if(MYSQLE_LIBRARIES)
    # libmysqld on FreeBSD apparently doesn't properly report what libraries
    # it likes to link with, libmysqlclient does though.
    #if(${CMAKE_HOST_SYSTEM_NAME} MATCHES "FreeBSD")
    #    string(REGEX REPLACE "-lmysqlclient" "-lmysqld" _mysql_libs ${MYSQL_LIBRARIES})
    #    string(STRIP ${_mysql_libs} _mysql_libs)
    #    set(MYSQLE_LIBRARIES ${_mysql_libs})
    #endif()
    cmake_push_check_state()
    set(CMAKE_REQUIRED_INCLUDES ${MYSQL_INCLUDE_DIR})
    set(CMAKE_REQUIRED_LIBRARIES ${MYSQLE_LIBRARIES})
    check_cxx_source_compiles( "#include <mysql.h>\nint main() { int i = MYSQL_OPT_USE_EMBEDDED_CONNECTION; }" HAVE_MYSQL_OPT_EMBEDDED_CONNECTION )
    cmake_pop_check_state()
endif()

endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(MySQLe
    REQUIRED_VARS MYSQLE_LIBRARIES HAVE_MYSQL_OPT_EMBEDDED_CONNECTION
    VERSION_VAR MySQLe_VERSION_STRING
)

mark_as_advanced(MYSQLE_LIBRARIES HAVE_MYSQL_OPT_EMBEDDED_CONNECTION)

set_package_properties(MySQLe PROPERTIES
    DESCRIPTION "MySQL Embedded Library (libmysqld)"
    URL "https://www.mysql.com"
)
