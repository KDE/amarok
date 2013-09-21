# Including and linking of all the tagguessing classes starts from here
#
# Copyright (c) 2013, Vedant Agarwala <vedant.kota@gmail.com>
#
# Redistribution and use is allowed according to the terms of the GNU license.

include_directories(
                    ${KDE4_INCLUDE_DIR}
                    ${QT_INCLUDES}
                    ${Amarok_SOURCE_DIR}/src
                   )
add_subdirectory( musicbrainz )

kde4_add_library( amarok_tagguessing

target_link_libraries()
    tagguessing/musicbrainz/MusicBrainzFinder.cpp
    tagguessing/musicbrainz/MusicBrainzTagsItem.cpp
    tagguessing/musicbrainz/MusicBrainzTagsModel.cpp
    tagguessing/musicbrainz/MusicBrainzTagsModelDelegate.cpp
    tagguessing/musicbrainz/MusicBrainzTagsView.cpp
    tagguessing/musicbrainz/MusicBrainzXmlParser.cpp
    tagguessing/Finder.cpp
    tagguessing/Provider.cpp
