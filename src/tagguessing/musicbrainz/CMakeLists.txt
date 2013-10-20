# Copyright (c) 2013, Vedant Agarwala <vedant.kota@gmail.com>
#
# Redistribution and use is allowed according to the terms of the GNU license.
#
# Including and linking of all the tagguessing classes starts from here.
# A new library "tagguessing_musicbrainz" is created here
#
# Macros related to external libraries (like LIBCHROMAPRINT_FOUND) should be added in amarok/cmake/modules/

set( Amarok_TAGGUESSING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} )

include_directories(
                    ${KDE4_INCLUDE_DIR}
                    ${QT_INCLUDES}
                    ${Amarok_SOURCE_DIR}/src
                   )

set( tagguessing_musicbrainz_SRCS
    MusicBrainzFinder.cpp
    MusicBrainzTagsItem.cpp
    MusicBrainzTagsItem.cpp
    MusicBrainzTagsModel.cpp
    MusicBrainzTagsModelDelegate.cpp
    MusicBrainzTagsView.cpp
    MusicDNSXmlParser.cpp
    )

if ( LIBOFA_FOUND )
    set( tagguessing_musicbrainz_SRCS
        MusicDNSFinder.cpp
        MusicDNSXmlParser.cpp
    )
endif ( LIBOFA_FOUND )

kde4_add_library( tagguessing_musicbrainz SHARED ${tagguessing_musicbrainz_SRCS} )

target_link_libraries( tagguessing_musicbrainz
    amaroklib
)
