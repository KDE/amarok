# Module to find the cddb library
#
# It defines
#  CDDB_INCLUDE_DIR - the include dir 
#  CDDB_LIBRARIES - the required libraries
#  CDDB_FOUND - true if both of the above have been found

if(CDDB_INCLUDE_DIR AND CDDB_LIBRARIES)
   set(CDDB_FIND_QUIETLY TRUE)
endif(CDDB_INCLUDE_DIR AND CDDB_LIBRARIES)

FIND_PATH(CDDB_INCLUDE_DIR cddb/cddb.h)

FIND_LIBRARY( CDDB_LIBRARIES NAMES cddb)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args( CDDB DEFAULT_MSG
                                   CDDB_INCLUDE_DIR CDDB_LIBRARIES)

MARK_AS_ADVANCED(CDDB_INCLUDE_DIR CDDB_LIBRARIES)
message(${CDDB_LIBRARIES})
