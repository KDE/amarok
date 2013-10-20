# Copyright (c) 2013, Vedant Agarwala <vedant.kota@gmail.com>
#
# Redistribution and use is allowed according to the terms of the GNU license.
#
# Including and linking of all the tagguessing classes starts from here.
# A new library "tagguessing" is created here and linked to amaroklib
#
# Macros related to external libraries (like LIBCHROMAPRINT_FOUND) should be added in amarok/cmake/modules/

set( Amarok_TAGGUESSING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} )

include_directories(
                    ${KDE4_INCLUDE_DIR}
                    ${QT_INCLUDES}
                    ${Amarok_SOURCE_DIR}/src )

set( tagguessing_SRCS
    Finder.cpp
    Provider.cpp
    WebRequestsHandler.cpp )

if( LIBOFA_FOUND )
    set( tagguessing_SRCS
        AudioToQStringDecoder.cpp )
endif()

kde4_add_library( tagguessing SHARED ${tagguessing_SRCS} )

target_link_libraries( tagguessing
    amaroklib )

add_subdirectory( musicbrainz )

if ( LIBCHROMAPRINT_FOUND )
    add_subdirectory( acustid )
endif( LIBCHROMAPRINT_FOUND )

install( TARGETS tagguessing ${INSTALL_TARGETS_DEFAULT_ARGS} )
