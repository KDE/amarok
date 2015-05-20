# - Find MySQL / MySQL Embedded
# Find the MySQL includes and client library
# This module defines
#  MYSQL_INCLUDE_DIR, where to find mysql.h
#  MYSQL_LIBRARIES, the libraries needed to use MySQL.
#  MYSQL_EMBEDDED_LIBRARIES, the libraries needed to use MySQL Embedded.
#  MYSQL_FOUND, If false, do not try to use MySQL.
#  MYSQL_EMBEDDED_FOUND, If false, do not try to use MySQL Embedded.
#  MYSQLAMAROK_FOUND, TRUE if both MYSQL and MYSQL_EMBEDDED have been found, FALSE otherwise.

# Copyright (c) 2006, Jaroslaw Staniek, <js@iidea.pl>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if(NOT WIN32)
    find_program(MYSQLCONFIG_EXECUTABLE NAMES mysql_config mysql_config5 HINTS ${BIN_INSTALL_DIR})
endif(NOT WIN32)

find_path(MYSQL_INCLUDE_DIR mysql.h PATH_SUFFIXES mysql mysql5/mysql)

if(MYSQLCONFIG_EXECUTABLE)
    exec_program(${MYSQLCONFIG_EXECUTABLE} ARGS --cflags RETURN_VALUE _return_VALUE OUTPUT_VARIABLE MYSQL_CFLAGS)
    exec_program(${MYSQLCONFIG_EXECUTABLE} ARGS --libs RETURN_VALUE _return_VALUE OUTPUT_VARIABLE MYSQL_LIBRARIES)
    exec_program(${MYSQLCONFIG_EXECUTABLE} ARGS --libmysqld-libs RETURN_VALUE _return_VALUE OUTPUT_VARIABLE MYSQL_EMBEDDED_LIBSTEMP)

    set(MYSQL_EMBEDDED_CFLAGS ${MYSQL_CFLAGS})

    if(MYSQL_EMBEDDED_LIBSTEMP)
        set( HAVE_MYSQL_EMBEDDED true )
    endif(MYSQL_EMBEDDED_LIBSTEMP)

    find_library(MYSQLD_PIC_SEPARATE
        mysqld_pic
        PATH_SUFFIXES mysql
    )

    if(MYSQLD_PIC_SEPARATE)
        string(REPLACE "lmysqld" "lmysqld_pic" MYSQL_EMBEDDED_LIBRARIES ${MYSQL_EMBEDDED_LIBSTEMP})
        # append link directory to variable as mysql_config is not always (since Ubuntu 12.04?)
        # reporting this directory with when being called with --libs
        get_filename_component(MYSQL_EMBEDDED_LIB_DIR_TMP "${MYSQLD_PIC_SEPARATE}" PATH)
        set(MYSQL_EMBEDDED_LIBRARIES "${MYSQL_EMBEDDED_LIBRARIES} -L${MYSQL_EMBEDDED_LIB_DIR_TMP}")
    else(MYSQLD_PIC_SEPARATE)
        set(MYSQL_EMBEDDED_LIBRARIES ${MYSQL_EMBEDDED_LIBSTEMP})
    endif(MYSQLD_PIC_SEPARATE)

    if (UNIX)
        # libmysqld wants -lpthread, but it is very likely it does not say that
        # explicitly in --libmysqld-libs
        find_package(Threads)
        set(MYSQL_EMBEDDED_LIBRARIES "${MYSQL_EMBEDDED_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT}")
    endif(UNIX)

else(MYSQLCONFIG_EXECUTABLE)

    if(WIN32)
        set(MYSQL_CLIENT_LIBRARY_NAME libmysql)
    else(WIN32)
        set(MYSQL_CLIENT_LIBRARY_NAME mysqlclient)
    endif(WIN32)

    find_library(MYSQL_LIBRARIES NAMES ${MYSQL_CLIENT_LIBRARY_NAME}
      PATHS
        ~/usr/lib/mysql
        /opt/mysql/mysql/lib 
        usr/mysql/lib/mysql
        opt/local/lib/mysql5/mysql
        opt/mysqle/lib/mysql
        usr/lib/mysql
        usr/lib64/mysql
        usr/lib64
        usr/local/lib/mysql
        opt/local/lib/mysql
        opt/ports/lib/mysql5/mysql
    )

    find_library(MYSQL_EMBEDDED_LIBRARIES NAMES mysqld_pic mysqld libmysqld
      PATHS
        ~/usr/lib/mysql
        /opt/local/lib/mysql5/mysql
        /opt/mysqle/lib/mysql
        /usr/lib/mysql
        /usr/lib64/mysql
        /usr/local/lib/mysql
        /opt/mysql/lib/mysql
        /opt/local/lib/mysql
        /opt/ports/lib/mysql5/mysql
    )

    macro_push_required_vars()
    set( CMAKE_REQUIRED_INCLUDES ${MYSQL_INCLUDE_DIR} )
    set( CMAKE_REQUIRED_LIBRARIES ${MYSQL_EMBEDDED_LIBRARIES} )
    include_directories( ${MYSQL_INCLUDE_DIR} )
    check_cxx_source_compiles( "#if (defined(_WIN32) || defined(_WIN64))\n#define __LCC__\n#endif\n#include <mysql.h>\nint main() { int i = MYSQL_OPT_USE_EMBEDDED_CONNECTION; }" HAVE_MYSQL_EMBEDDED )
    macro_pop_required_vars()

endif(MYSQLCONFIG_EXECUTABLE)

if(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARIES)
   set(MYSQL_FOUND TRUE)
   message(STATUS "Found MySQL: ${MYSQL_INCLUDE_DIR}, ${MYSQL_LIBRARIES}")
else(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARIES)
   set(MYSQL_FOUND FALSE)
   message(STATUS "MySQL not found.")
endif(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARIES)

if(MYSQL_INCLUDE_DIR AND MYSQL_EMBEDDED_LIBRARIES AND HAVE_MYSQL_EMBEDDED)
   set(MYSQL_EMBEDDED_FOUND TRUE)
   message(STATUS "Found MySQL Embedded: ${MYSQL_INCLUDE_DIR}, ${MYSQL_EMBEDDED_LIBRARIES}")
else(MYSQL_INCLUDE_DIR AND MYSQL_EMBEDDED_LIBRARIES AND HAVE_MYSQL_EMBEDDED)
   set(MYSQL_EMBEDDED_FOUND FALSE)
   message(STATUS "MySQL Embedded not found.")
endif(MYSQL_INCLUDE_DIR AND MYSQL_EMBEDDED_LIBRARIES AND HAVE_MYSQL_EMBEDDED)

#MYSQLAMAROK_FOUND has to be defined so that MYSQLAMAROK gets into the PACKAGES_FOUND property and the other properties that depend on <NAME>_FOUND variables. If this is not set to TRUE (when both MYSQL and MYSQLD are available) then on the subsequent call to feature_summary, MySQLAmarok will be missing from the PACKAGES_FOUND property and hence it will be marked as a missing required feature thus breaking the build when feature_summary has been used with FATAL_ON_MISSING_REQUIRED_PACKAGES.
if(MYSQL_EMBEDDED_FOUND AND MYSQL_FOUND)
   set(MYSQLAMAROK_FOUND TRUE)
endif(MYSQL_EMBEDDED_FOUND AND MYSQL_FOUND)

mark_as_advanced(MYSQL_INCLUDE_DIR MYSQL_LIBRARIES MYSQL_EMBEDDED_LIBRARIES)
