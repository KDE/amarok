include_directories(
    ${KDE4_INCLUDES}
)

set( amarokshared_SRCS
    FileType.cpp
    MetaTagLib.cpp
    TagsFromFileNameGuesser.cpp
)
if( TAGLIB_FOUND )
    set( amarokshared_SRCS
        ${amarokshared_SRCS}
        FileTypeResolver.cpp
        MetaReplayGain.cpp
    )
endif( TAGLIB_FOUND )

set( amarokshared_collectionscanner_SRCS
    collectionscanner/Album.cpp
    collectionscanner/BatchFile.cpp
    collectionscanner/Directory.cpp
    collectionscanner/Playlist.cpp
    collectionscanner/ScanningState.cpp
    collectionscanner/Track.cpp
)

if( TAGLIB_FOUND )
    set( amarokshared_tag_helpers_SRCS
        tag_helpers/APETagHelper.cpp
        tag_helpers/ASFTagHelper.cpp
        tag_helpers/ID3v2TagHelper.cpp
        tag_helpers/MP4TagHelper.cpp
        tag_helpers/StringHelper.cpp
        tag_helpers/TagHelper.cpp
        tag_helpers/VorbisCommentTagHelper.cpp
    )
endif( TAGLIB_FOUND )

kde4_add_library( amarokshared SHARED ${amarokshared_SRCS}
    ${amarokshared_collectionscanner_SRCS} ${amarokshared_tag_helpers_SRCS} )

if( TAGLIB_FOUND )
    include_directories( ${TAGLIB_INCLUDES} )
    add_definitions( ${TAGLIB_CFLAGS} )
    target_link_libraries( amarokshared ${TAGLIB_LIBRARIES} )
endif( TAGLIB_FOUND )
if( TAGLIB-EXTRAS_FOUND )
    include_directories( ${TAGLIB-EXTRAS_INCLUDES} )
    add_definitions( ${TAGLIB-EXTRAS_CFLAGS} )
    target_link_libraries( amarokshared ${TAGLIB-EXTRAS_LIBRARIES} )
endif( TAGLIB-EXTRAS_FOUND )

target_link_libraries( amarokshared
    ${QT_QTCORE_LIBRARY}
    ${QT_QTGUI_LIBRARY} # for QImage, ...
    ${KDE4_KDECORE_LIBRARY} # for KMimeType, ...
)

set_target_properties( amarokshared PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install( TARGETS amarokshared ${INSTALL_TARGETS_DEFAULT_ARGS} )
