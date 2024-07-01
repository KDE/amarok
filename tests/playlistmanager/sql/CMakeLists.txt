include_directories( .
    ${AMAROK_SOURCE_TREE}/browsers/playlistbrowser
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_BINARY_DIR}/tests
)

#------------------------ Test SqlUserPlaylistProvider -----------------------------

set( testsqluserplaylistprovider_SRCS
     TestSqlUserPlaylistProvider.cpp
     ${AMAROK_SOURCE_TREE}/playlistmanager/sql/SqlUserPlaylistProvider.cpp
     ${AMAROK_SOURCE_TREE}/playlistmanager/sql/SqlPlaylistGroup.cpp
     ${AMAROK_SOURCE_TREE}/playlistmanager/sql/SqlPlaylist.cpp
     ${AMAROK_SOURCE_TREE}/core-impl/meta/timecode/TimecodeMeta.cpp
     ${AMAROK_SOURCE_TREE}/core-impl/capabilities/timecode/TimecodeBoundedPlaybackCapability.cpp
     ${AMAROK_SOURCE_TREE}/core/capabilities/BoundedPlaybackCapability.cpp
     ${AMAROK_SOURCE_TREE}/core-impl/playlists/types/file/PlaylistFileSupport.cpp
    )
add_executable( testsqluserplaylistprovider ${testsqluserplaylistprovider_SRCS} )
add_test(NAME testsqluserplaylistprovider COMMAND $<TARGET_FILE:testsqluserplaylistprovider>)
ecm_mark_as_test(testsqluserplaylistprovider)
target_link_libraries( testsqluserplaylistprovider  KF${KF_MAJOR_VERSION}::KIOCore Qt::Test Qt::Core amarokcore amaroklib )
