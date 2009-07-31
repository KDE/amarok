# - Find MySQL / MySQL Embedded
# Find the MySQL includes and client library
# This module defines
#  MYSQL_INCLUDE_DIR, where to find mysql.h
#  MYSQL_LIBRARIES, the libraries needed to use MySQL.
#  MYSQL_EMBEDDED_LIBRARIES, the libraries needed to use MySQL Embedded.
#  MYSQL_FOUND, If false, do not try to use MySQL.
#  MYSQL_EMBEDDED_FOUND, If false, do not try to use MySQL Embedded.

# Copyright (c) 2006, Jaroslaw Staniek, <js@iidea.pl>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


find_path(MYSQL_INCLUDE_DIR mysql.h
   ~/usr/include/mysql
   /opt/local/include/mysql5/mysql
   /opt/mysqle/include/mysql
   /opt/mysql/mysql/include 
   /usr/mysql/include/mysql
   /usr/include/mysql
   /usr/local/include/mysql
   /opt/local/include/mysql
   /opt/ports/include/mysql5/mysql
)

if(WIN32)
    set(MYSQL_CLIENT_LIBRARY_NAME libmysql)
else(WIN32)
    set(MYSQL_CLIENT_LIBRARY_NAME mysqlclient)
endif(WIN32)

find_library(MYSQL_LIBRARIES NAMES ${MYSQL_CLIENT_LIBRARY_NAME}
   PATHS
   ~/usr/lib/mysql
   /opt/mysql/mysql/lib 
   /usr/mysql/lib/mysql
   /opt/local/lib/mysql5/mysql
   /opt/mysqle/lib/mysql
   /usr/lib/mysql
   /usr/lib64/mysql
   /usr/lib64
   /usr/local/lib/mysql
   /opt/local/lib/mysql
   /opt/ports/lib/mysql5/mysql
)

find_library(MYSQL_EMBEDDED_LIBRARIES NAMES mysqld libmysqld
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
check_cxx_source_compiles( "#if (defined(_WIN32) || defined(_WIN64))\n#define __LCC__\n#endif\n#include <mysql.h>\nint main() { int i = MYSQL_OPT_USE_EMBEDDED_CONNECTION; }" HAVE_MYSQL_OPT_EMBEDDED_CONNECTION )
macro_pop_required_vars()

if(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARIES)
   set(MYSQL_FOUND TRUE)
   message(STATUS "Found MySQL: ${MYSQL_INCLUDE_DIR}, ${MYSQL_LIBRARIES}")
else(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARIES)
   set(MYSQL_FOUND FALSE)
   message(STATUS "MySQL not found.")
endif(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARIES)

if(MYSQL_INCLUDE_DIR AND MYSQL_EMBEDDED_LIBRARIES AND HAVE_MYSQL_OPT_EMBEDDED_CONNECTION)
   set(MYSQL_EMBEDDED_FOUND TRUE)
   message(STATUS "Found MySQL Embedded: ${MYSQL_INCLUDE_DIR}, ${MYSQL_EMBEDDED_LIBRARIES}")
else(MYSQL_INCLUDE_DIR AND MYSQL_EMBEDDED_LIBRARIES AND HAVE_MYSQL_OPT_EMBEDDED_CONNECTION)
   set(MYSQL_EMBEDDED_FOUND FALSE)
   message(STATUS "MySQL Embedded not found.")
endif(MYSQL_INCLUDE_DIR AND MYSQL_EMBEDDED_LIBRARIES AND HAVE_MYSQL_OPT_EMBEDDED_CONNECTION)

mark_as_advanced(MYSQL_INCLUDE_DIR MYSQL_LIBRARIES MYSQL_EMBEDDED_LIBRARIES)
