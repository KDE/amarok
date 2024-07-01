include_directories( .
    ${AMAROK_TEST_TREE}
    ${AMAROK_SOURCE_TREE}/statusbar
    ${CMAKE_BINARY_DIR}/tests
    ${CMAKE_BINARY_DIR}/src
)

#------------------------ Test PlaylistFileProvider -----------------------------

set( testplaylistfileprovider_SRCS
     TestPlaylistFileProvider.cpp
     ${AMAROK_SOURCE_TREE}/playlistmanager/file/PlaylistFileProvider.cpp
     ${AMAROK_SOURCE_TREE}/playlist/PlaylistDefines.cpp
     ${AMAROK_SOURCE_TREE}/playlist/PlaylistModelStack.cpp
     ${AMAROK_SOURCE_TREE}/playlist/proxymodels/GroupingProxy.cpp
     ${AMAROK_SOURCE_TREE}/playlist/proxymodels/ProxyBase.cpp
     ${AMAROK_SOURCE_TREE}/playlist/proxymodels/SearchProxy.cpp
     ${AMAROK_SOURCE_TREE}/playlist/proxymodels/SortFilterProxy.cpp
     ${AMAROK_SOURCE_TREE}/playlist/proxymodels/SortAlgorithms.cpp
     ${AMAROK_SOURCE_TREE}/playlist/proxymodels/SortScheme.cpp
     ${AMAROK_SOURCE_TREE}/core-impl/playlists/types/file/PlaylistFileSupport.cpp
    )
add_executable( testplaylistfileprovider ${testplaylistfileprovider_SRCS} )
add_test(NAME testplaylistfileprovider COMMAND $<TARGET_FILE:testplaylistfileprovider>)
ecm_mark_as_test(testplaylistfileprovider)
target_link_libraries( testplaylistfileprovider  KF${KF_MAJOR_VERSION}::KIOCore Qt::Test Qt::Core amarokcore amaroklib )
