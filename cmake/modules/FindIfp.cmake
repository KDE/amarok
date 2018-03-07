# - Find IFP library
# Find the native IFP includes and library
# This module defines
#  IFP_INCLUDE_DIR, where to find ifp.h, etc.
#  IFP_LIBRARIES, libraries to link against to use IFP.
#  IFP_FOUND, If false, do NOT try to use IFP.
# also defined, but NOT for general use are
#  IFP_LIBRARY, where to find the IFP library.

if (IFP_INCLUDE_DIR)
  # Already in cache, be silent
  set(Ifp_FIND_QUIETLY TRUE)
endif ()

find_path(IFP_INCLUDE_DIR ifp.h
)

find_library(IFP_LIBRARY ifp
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(IFP DEFAULT_MSG IFP_LIBRARY IFP_INCLUDE_DIR)

mark_as_advanced(IFP_INCLUDE_DIR IFP_LIBRARY)
