# Copyright (c) 2013, Vedant Agarwala <vedant.kota@gmail.com>
#
# Redistribution and use is allowed according to the terms of the GNU license.
#
# Including and linking of all the tagguessing classes starts from here.
# A new library "tagguessing" is created here and linked to amaroklib
#
# Macros related to external libraries (like LIBCHROMAPRINT_FOUND) should be added in amarok/cmake/modules/

include_directories(
                    ${KDE4_INCLUDE_DIR}
                    ${QT_INCLUDES}
                    ${Amarok_SOURCE_DIR}/src )

# first target:

set( tagguessing_support_SRCS
    Provider.cpp
    WebRequestsHandler.cpp )

if( LIBOFA_FOUND )
    set( tagguessing_support_SRCS
        AudioToQStringDecoder.cpp )
endif()

kde4_add_library( tagguessing_support SHARED ${tagguessing_support_SRCS} )

target_link_libraries( tagguessing_support
    amarokcore
    amaroklib
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS} )

# add providers:

add_subdirectory( musicbrainz )
set( tagguessing_provider_LIBS tagguessing_musicbrainz )

if ( LIBCHROMAPRINT_FOUND )
    add_subdirectory( acustid )
    set( tagguessing_provider_LIBS tagguessing_acustid )
endif( LIBCHROMAPRINT_FOUND )

# next target:

set( tagguessing_core_SRCS
    Finder.cpp
    TagsItem.cpp
    TagsModel.cpp
    TagsModelDelegate.cpp
    TagsView.cpp )

kde4_add_library( tagguessing_core SHARED ${tagguessing_core_SRCS} )

target_link_libraries( tagguessing_core
    ${tagguessing_provider_LIBS}
    amarokcore
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS} )

#install( TARGETS tagguessing ${INSTALL_TARGETS_DEFAULT_ARGS} )
