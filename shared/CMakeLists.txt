set( amarokshared_SRCS
    FileType.cpp
    FileTypeResolver.cpp
    MetaReplayGain.cpp
    MetaTagLib.cpp
    TagsFromFileNameGuesser.cpp
)

set( amarokshared_collectionscanner_SRCS
    collectionscanner/Album.cpp
    collectionscanner/BatchFile.cpp
    collectionscanner/Directory.cpp
    collectionscanner/Playlist.cpp
    collectionscanner/ScanningState.cpp
    collectionscanner/Track.cpp
)


set( amarokshared_tag_helpers_SRCS
    tag_helpers/APETagHelper.cpp
    tag_helpers/ASFTagHelper.cpp
    tag_helpers/ID3v2TagHelper.cpp
    tag_helpers/MP4TagHelper.cpp
    tag_helpers/StringHelper.cpp
    tag_helpers/TagHelper.cpp
    tag_helpers/VorbisCommentTagHelper.cpp
)

add_library( amarokshared SHARED
    ${amarokshared_SRCS}
    ${amarokshared_collectionscanner_SRCS}
    ${amarokshared_tag_helpers_SRCS} )

generate_export_header(amarokshared BASE_NAME amarokshared)


target_include_directories( amarokshared PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} )

include_directories(SYSTEM ${TAGLIB_INCLUDES} )
add_definitions( ${TAGLIB_CFLAGS} )
target_link_libraries( amarokshared ${TAGLIB_LIBRARIES} KF5::Codecs)
if( TAGLIB-EXTRAS_FOUND )
    include_directories(SYSTEM ${TAGLIB-EXTRAS_INCLUDES} )
    add_definitions( ${TAGLIB-EXTRAS_CFLAGS} )
    target_link_libraries( amarokshared ${TAGLIB-EXTRAS_LIBRARIES} )
endif()

target_link_libraries( amarokshared
    Qt::Core
    Qt::Gui # for QImage, ...
)

set_target_properties( amarokshared PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install( TARGETS amarokshared ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )
