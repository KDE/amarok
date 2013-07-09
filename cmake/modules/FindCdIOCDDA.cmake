# Module to find the cdio library
#
# It defines
#  CDIOCDDA_LIBRARIES - the required libraries
#  CDIOCDDA_FOUND - true if both of the above have been found

if(CDIOCDDA_LIBRARIES)
   set(CDIOCDDA_FIND_QUIETLY TRUE)
endif(CDIOCDDA_LIBRARIES)


FIND_LIBRARY( CDIOCDDA_LIBRARIES NAMES cdio_cdda)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args( CdIOCDDA DEFAULT_MSG
                                   CDIOCDDA_LIBRARIES)

MARK_AS_ADVANCED(CDIOCDDA_LIBRARIES)
message(${CDIOCDDA_LIBRARIES})
