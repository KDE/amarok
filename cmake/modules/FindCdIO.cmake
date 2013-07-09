# Module to find the cdio library
#
# It defines
#  CDIO_INCLUDE_DIR - the include dir 
#  CDIO_LIBRARIES - the required libraries
#  CDIO_FOUND - true if both of the above have been found

if(CDIO_INCLUDE_DIR AND CDIO_LIBRARIES)
   set(CDIO_FIND_QUIETLY TRUE)
endif(CDIO_INCLUDE_DIR AND CDIO_LIBRARIES)

FIND_PATH(CDIO_INCLUDE_DIR cdio/cdio.h)

FIND_LIBRARY( CDIO_LIBRARIES NAMES cdio)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args( CdIO DEFAULT_MSG
                                   CDIO_INCLUDE_DIR CDIO_LIBRARIES)

MARK_AS_ADVANCED(CDIO_INCLUDE_DIR CDIO_LIBRARIES)
message(${CDIO_LIBRARIES})
