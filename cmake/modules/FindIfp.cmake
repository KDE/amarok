# - Find IPF library
# Find the native IFP includes and library
# This module defines
#  IFP_INCLUDE_DIR, where to find ifp.h, etc.
#  IFP_LIBRARIES, libraries to link against to use IFP.
#  IFP_FOUND, If false, do NOT try to use IFP.
# also defined, but NOT for general use are
#  IFP_LIBRARY, where to find the IFP library.

if (IFP_INCLUDE_DIR)
  # Already in cache, be silent
  set(IFP_FIND_QUIETLY TRUE)
endif (IFP_INCLUDE_DIR)

FIND_PATH(IFP_INCLUDE_DIR ifp.h
  /usr/local/include
  /usr/include
)

FIND_LIBRARY(IFP_LIBRARY
  PATHS 
  /usr/lib 
  /usr/local/lib
)

if (IFP_INCLUDE_DIR AND IFP_LIBRARY)
   set(IFP_FOUND TRUE)
   set(IFP_LIBRARIES ${IFP_LIBRARY} )
endif (IFP_INCLUDE_DIR AND IFP_LIBRARY)

if (IFP_FOUND)
   if (NOT Ifp_FIND_QUIETLY)
      message(STATUS "Found IFP: ${IFP_LIBRARY}")
   endif (NOT Ifp_FIND_QUIETLY)
else (IFP_FOUND)
   if (Ifp_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find IFP")
   endif (Ifp_FIND_REQUIRED)
endif (IFP_FOUND)

MARK_AS_ADVANCED(IFP_INCLUDE_DIR IFP_LIBRARY)
