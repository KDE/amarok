# - Find Googlemock
# Find the google mock includes and the google mock libraries
# This module defines
# GOOGLEMOCK_INCLUDE_DIR, root google mock include dir
# GOOGLEMOCK_LIBRARY, the path to Google Mock library
# GOOGLEMOCK_LIBRARIES, the path to Google Mock and Google Test library
# GOOGLEMOCK_FOUND, whether Google Mock was found

find_program(GMOCK-CONFIG_EXECUTABLE NAMES gmock-config PATHS
       ${BIN_INSTALL_DIR}
       /opt/local/bin
       /usr/bin
)

if(GMOCK-CONFIG_EXECUTABLE)
exec_program(${GMOCK-CONFIG_EXECUTABLE} ARGS --includedir OUTPUT_VARIABLE GOOGLEMOCK_INCLUDE_DIR)
exec_program(${GMOCK-CONFIG_EXECUTABLE} ARGS --libs OUTPUT_VARIABLE GOOGLEMOCK_LIBRARIES)

if(GOOGLEMOCK_INCLUDE_DIR AND GOOGLEMOCK_LIBRARIES)
   set(GOOGLEMOCK_FOUND TRUE)
   message(STATUS "Found libgmock: ${GOOGLEMOCK_INCLUDE_DIR}, ${GOOGLEMOCK_LIBRARIES}")
else(GOOGLEMOCK_INCLUDE_DIR AND GOOGLEMOCK_LIBRARIES)
   set(GOOGLEMOCK_FOUND FALSE)
   if (GOOGLEMOCK_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find required package Googlemock")
   endif(GOOGLEMOCK_FIND_REQUIRED)
endif(GOOGLEMOCK_INCLUDE_DIR AND GOOGLEMOCK_LIBRARIES)

else(GMOCK-CONFIG_EXECUTABLE)

find_path(GOOGLEMOCK_INCLUDE_DIR NAMES gmock.h
   HINTS
   ~/usr/include
   /opt/local/include
   /usr/include
   /usr/local/include
   /opt/kde4/include
   ${KDE4_INCLUDE_DIR}
   PATH_SUFFIXES gmock
)

find_library( GOOGLEMOCK_LIBRARY NAMES gmock
    PATHS
    ~/usr/lib
   /opt/local/lib
   /usr/lib
   /usr/lib64
   /usr/local/lib
   /opt/kde4/lib
   ${KDE4_LIB_DIR}
)

find_library( GOOGLEMOCK_DEP_GTEST_LIBRARY NAMES gtest
    PATHS
    ~/usr/lib
   /opt/local/lib
   /usr/lib
   /usr/lib64
   /usr/local/lib
   /opt/kde4/lib
   ${KDE4_LIB_DIR}
)


if(GOOGLEMOCK_INCLUDE_DIR AND GOOGLEMOCK_LIBRARY AND GOOGLEMOCK_DEP_GTEST_LIBRARY)
   set(GOOGLEMOCK_FOUND TRUE)
   set(GOOGLEMOCK_LIBRARIES ${GOOGLEMOCK_LIBRARY} ${GOOGLEMOCK_DEP_GTEST_LIBRARY})
   message(STATUS "Found libgmock: ${GOOGLEMOCK_INCLUDE_DIR}, ${GOOGLEMOCK_LIBRARIES}")
else(GOOGLEMOCK_INCLUDE_DIR AND GOOGLEMOCK_LIBRARY AND GOOGLEMOCK_DEP_GTEST_LIBRARY)
   set(GOOGLEMOCK_FOUND FALSE)
   if (GOOGLEMOCK_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find required package Googlemock")
   endif(GOOGLEMOCK_FIND_REQUIRED)
endif(GOOGLEMOCK_INCLUDE_DIR AND GOOGLEMOCK_LIBRARY AND GOOGLEMOCK_DEP_GTEST_LIBRARY)

endif(GMOCK-CONFIG_EXECUTABLE)

mark_as_advanced(GOOGLEMOCK_INCLUDE_DIR GOOGLEMOCK_LIBRARIES)
