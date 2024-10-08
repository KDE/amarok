# - Find Googlemock
# Find the google mock includes and the google mock libraries
# This module defines
# GOOGLEMOCK_INCLUDE_DIR, root google mock include dir
# GOOGLEMOCK_LIBRARY, the path to Google Mock library
# GOOGLEMOCK_LIBRARIES, the path to Google Mock and Google Test library
# GOOGLEMOCK_FOUND, whether Google Mock was found
#
# since google test and google mock is not supposed to be supplied pre-compiled
# we try to find the google mock sources as a fallback

find_program(GMOCK-CONFIG_EXECUTABLE NAMES gmock-config PATHS
       ${BIN_INSTALL_DIR}
       /opt/local/bin
       /usr/bin
)

if(GMOCK-CONFIG_EXECUTABLE)
exec_program(${GMOCK-CONFIG_EXECUTABLE} ARGS --includedir OUTPUT_VARIABLE GOOGLEMOCK_INCLUDE_DIR)
exec_program(${GMOCK-CONFIG_EXECUTABLE} ARGS --ldflags OUTPUT_VARIABLE GOOGLEMOCK_LDFLAGS)
exec_program(${GMOCK-CONFIG_EXECUTABLE} ARGS --libs OUTPUT_VARIABLE GOOGLEMOCK_libs_tmp)
set(GOOGLEMOCK_LIBRARIES ${GOOGLEMOCK_LDFLAGS} ${GOOGLEMOCK_libs_tmp})

if(GOOGLEMOCK_INCLUDE_DIR AND GOOGLEMOCK_LIBRARIES)
   set(GOOGLEMOCK_FOUND TRUE)
   message(STATUS "Found libgmock: ${GOOGLEMOCK_INCLUDE_DIR}, ${GOOGLEMOCK_LIBRARIES}")
else()
   set(GOOGLEMOCK_FOUND FALSE)
   if (GOOGLEMOCK_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find required package Googlemock")
   endif()
endif()

else()

find_path(GOOGLEMOCK_INCLUDE_DIR NAMES gmock/gmock.h
   HINTS
   ~/usr/include
   /opt/local/include
   /usr/include
   /usr/local/include
)

find_library( GOOGLEMOCK_LIBRARY NAMES gmock
    PATHS
    ~/usr/lib
   /opt/local/lib
   /usr/lib
   /usr/lib64
   /usr/local/lib
)

find_library( GOOGLEMOCK_DEP_GTEST_LIBRARY NAMES gtest
    PATHS
    ~/usr/lib
   /opt/local/lib
   /usr/lib
   /usr/lib64
   /usr/local/lib
)

# google-mock >= 1.5 requires pthread
# see: http://code.google.com/p/googlemock/source/browse/trunk/CHANGES
if( NOT WIN32 AND GOOGLEMOCK_LIBRARY )
    find_library( GOOGLEMOCK_DEP_PTHREAD_LIBRARY NAMES pthread
        PATHS
        ~/usr/lib
       /opt/local/lib
       /usr/lib
       /usr/lib64
       /usr/local/lib
    )

endif()

# Google recommends not to distribute a pre-build libary and ubuntu is following
# this advice with libgtest 1.6.0
# However they are distributing sources, so we are looking if we at least have
# them available
if( NOT GOOGLEMOCK_DEP_GTEST_LIBRARY )
    find_path( GOOGLEMOCK_SOURCES NAMES gmock
        PATHS /usr/src
        NO_DEFAULT_PATH
        NO_CMAKE_PATH
    )

    # found googlemock as sources. then we also have the gtest sources since they
    # are included
    if( GOOGLEMOCK_SOURCES )
        find_path( GOOGLEMOCK_DEP_GTEST_SOURCES NAMES gtest
            PATHS "${GOOGLEMOCK_SOURCES}/gmock"
            NO_DEFAULT_PATH
            NO_CMAKE_PATH
        )

        # make sure that we use the gtest supplied with googlemock
        set(GOOGLEMOCK_INCLUDE_DIR
            "${GOOGLEMOCK_INCLUDE_DIR}"
            "${GOOGLEMOCK_SOURCES}/gmock"
            "${GOOGLEMOCK_DEP_GTEST_SOURCES}/gtest/include"
        )

    elseif( GOOGLEMOCK_SOURCES )
        find_path( GOOGLEMOCK_DEP_GTEST_SOURCES NAMES gtest
            PATHS /usr/src
            NO_DEFAULT_PATH
            NO_CMAKE_PATH
        )

        # in this case we also have to use the static google mock library
        set( OLD_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
        set( CMAKE_FIND_LIBRARY_SUFFIXES .a)
        find_library( GOOGLEMOCK_LIBRARY_STATIC NAMES gmock
            PATHS
            ~/usr/lib
           /opt/local/lib
           /usr/lib
           /usr/lib64
           /usr/local/lib
        )
        set( CMAKE_FIND_LIBRARY_SUFFIXES ${OLD_CMAKE_FIND_LIBRARY_SUFFIXES})
    endif()

endif()

# -- googlemock and gtest library available
if(GOOGLEMOCK_INCLUDE_DIR AND GOOGLEMOCK_LIBRARY AND GOOGLEMOCK_DEP_GTEST_LIBRARY)
   set(GOOGLEMOCK_FOUND TRUE)
   set(GOOGLEMOCK_LIBRARIES ${GOOGLEMOCK_LIBRARY} ${GOOGLEMOCK_DEP_GTEST_LIBRARY} ${GOOGLEMOCK_DEP_PTHREAD_LIBRARY})
   message(STATUS "Found libgmock: ${GOOGLEMOCK_INCLUDE_DIR}, ${GOOGLEMOCK_LIBRARIES}")


# -- googlemock and gtest sources available
elseif(GOOGLEMOCK_INCLUDE_DIR AND GOOGLEMOCK_LIBRARY AND GOOGLEMOCK_DEP_GTEST_SOURCES)
   set(GOOGLEMOCK_FOUND TRUE)
   set(GOOGLEMOCK_LIBRARIES ${GOOGLEMOCK_LIBRARY_STATIC} gtest)
   set(GOOGLEMOCK_GTEST_SOURCES "${GOOGLEMOCK_DEP_GTEST_SOURCES}/gtest" CACHE PATH "Path to the gtest sources")
   message(STATUS "Found libgmock but need to build gtest: ${GOOGLEMOCK_INCLUDE_DIR}, ${GOOGLEMOCK_LIBRARIES} ${GOOGLEMOCK_DEP_GTEST_SOURCES}")

# -- googlemock sources and gtest sources available
elseif(GOOGLEMOCK_SOURCES)
   set(GOOGLEMOCK_FOUND TRUE)
   set(GOOGLEMOCK_LIBRARIES gtest)
   set(GOOGLEMOCK_SRCS "${GOOGLEMOCK_SOURCES}/gmock/src/gmock-all.cc" CACHE PATH "Google mock source file that needs to be added")
   set(GOOGLEMOCK_SOURCES "${GOOGLEMOCK_SOURCES}/gmock" CACHE PATH "Path to the google-mock sources")
   set(GOOGLEMOCK_GTEST_SOURCES "${GOOGLEMOCK_DEP_GTEST_SOURCES}/gtest" CACHE PATH "Path to the gtest sources")
   message(STATUS "Found gmock and gtest but need to build both: ${GOOGLEMOCK_INCLUDE_DIR}, ${GOOGLEMOCK_DEP_GTEST_SOURCES}")
   mark_as_advanced(GOOGLEMOCK_SRCS)

# -- googlemock but no gtest
else()
   set(GOOGLEMOCK_FOUND FALSE)
   if (GOOGLEMOCK_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find required package Googlemock or gtest")
   endif()
endif()

endif()

mark_as_advanced(GOOGLEMOCK_INCLUDE_DIR GOOGLEMOCK_LIBRARIES)
