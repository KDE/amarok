# Module to find the cdio library
#
# It defines
#  PARANOIA_INCLUDE_DIR - the include dir 
#  PARANOIA_LIBRARIES - the required libraries
#  PARANOIA_FOUND - true if both of the above have been found

if(PARANOIA_INCLUDE_DIR AND PARANOIA_LIBRARIES)
   set(PARANOIA_FIND_QUIETLY TRUE)
endif(PARANOIA_INCLUDE_DIR AND PARANOIA_LIBRARIES)

FIND_PATH(PARANOIA_INCLUDE_DIR cdio/paranoia/paranoia.h)

FIND_LIBRARY( PARANOIA_LIBRARIES NAMES cdio_paranoia)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args( PARANOIA DEFAULT_MSG
                                   PARANOIA_INCLUDE_DIR PARANOIA_LIBRARIES)

MARK_AS_ADVANCED(PARANOIA_INCLUDE_DIR PARANOIA_LIBRARIES)
message(${PARANOIA_LIBRARIES})
