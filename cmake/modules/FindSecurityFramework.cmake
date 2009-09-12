# - Find OS X Security framework
# Find the Apple Security Framework, and guess the include path based on the Framework structure
# This module defines
# APPLE_SECURITY_INCLUDE_DIR, root lastfm include dir
# APPLE_SECURITY_LIBRARY, the path to liblastfm
# APPLE_SECURITY_FOUND, whether liblastfm was found

find_library(APPLE_SECURITY_LIBRARY NAMES Security)


if(APPLE_SECURITY_LIBRARY)
   set(APPLE_SECURITY_FOUND TRUE)
   set(APPLE_SECURITY_INCLUDE_DIR "${APPLE_SECURITY_LIBRARY}/Headers")
   message(STATUS "Found Apple Security Framwork: ${APPLE_SECURITY_INCLUDE_DIR}, ${APPLE_SECURITY_LIBRARY}")
else(APPLE_SECURITY_LIBRARY)
   set(APPLE_SECURITY_FOUND FALSE)
   if (APPLE_SECURITY_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find required Apple Security Framwork")
   endif(APPLE_SECURITY_FIND_REQUIRED)
endif(APPLE_SECURITY_LIBRARY)

mark_as_advanced(APPLE_SECURITY_INCLUDE_DIR APPLE_SECURITY_LIBRARY)
