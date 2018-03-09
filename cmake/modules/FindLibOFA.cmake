find_path(LIBOFA_INCLUDE_DIR NAMES ofa.h
   HINTS
   ~/usr/include
   /opt/local/include
   /usr/include
   /usr/local/include
   PATH_SUFFIXES ofa1
)

find_library(LIBOFA_LIBRARY NAMES ofa
    PATHS
    ~/usr/lib
   /opt/local/lib
   /usr/lib
   /usr/lib64
   /usr/local/lib
)


if(LIBOFA_INCLUDE_DIR AND LIBOFA_LIBRARY)
   set(LIBOFA_FOUND TRUE)
   message(STATUS "Found libofa: ${LIBOFA_INCLUDE_DIR}, ${LIBOFA_LIBRARY}")
else()
   set(LIBOFA_FOUND FALSE)
   if (LIBOFA_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find required package LibOFA")
   endif()
endif()

mark_as_advanced(LIBOFA_INCLUDE_DIR LIBOFA_LIBRARY)
