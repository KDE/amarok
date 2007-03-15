# - Try to find the MP4V2 library
# Once done this will define
#
#  MP4V2_FOUND - system has the MP4V2 library
#  MP4V2_INCLUDE_DIR - the MP4V2 include directory
#  MP4V2_LIBRARIES - The libraries needed to use MP4V2

if (MP4V2_INCLUDE_DIR AND MP4V2_LIBRARIES)
  # Already in cache, be silent
  set(MP4V2_FIND_QUIETLY TRUE)
endif (MP4V2_INCLUDE_DIR AND MP4V2_LIBRARIES)

FIND_PATH(MP4V2_INCLUDE_DIR mp4.h
 /usr/include/
 /usr/local/include/
)

FIND_LIBRARY(MP4V2_LIBRARY NAMES mp4v2
 PATHS
 /usr/lib
 /usr/local/lib
)

if (MP4V2_INCLUDE_DIR AND MP4V2_LIBRARY)
   set(MP4V2_FOUND TRUE)
endif (MP4V2_INCLUDE_DIR AND MP4V2_LIBRARY)

if (MP4V2_FOUND)
   if (NOT Mp4v2_FIND_QUIETLY)
      message(STATUS "Found MP4V2: ${MP4V2_LIBRARY}")
   endif (NOT Mp4v2_FIND_QUIETLY)
else (MP4V2_FOUND)
   if (Mp4v2_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find MP4V2")
   endif (Mp4v2_FIND_REQUIRED)
endif (MP4V2_FOUND)

MARK_AS_ADVANCED(MP4V2_INCLUDE_DIR MP4V2_LIBRARY)
