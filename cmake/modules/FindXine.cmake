# - Try to find the Xine library
# Once done this will define
#
#  XINE_FOUND - system has the Xine library
#  XINE_INCLUDE_DIR - the Xine include directory
#  XINE_LIBRARY - The libraries needed to use Xine

if (XINE_INCLUDE_DIR AND XINE_LIBRARY)
  # Already in cache, be silent
  set(Xine_FIND_QUIETLY TRUE)
endif (XINE_INCLUDE_DIR AND XINE_LIBRARY)

FIND_PATH(XINE_INCLUDE_DIR xine.h
)

FIND_LIBRARY(XINE_LIBRARY NAMES xine
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Xine DEFAULT_MSG XINE_LIBRARY XINE_INCLUDE_DIR)

MARK_AS_ADVANCED(XINE_INCLUDE_DIR XINE_LIBRARY)
