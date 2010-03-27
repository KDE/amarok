include_directories( .
    ${AMAROK_TEST_TREE}
    ${AMAROK_SOURCE_TREE}/statusbar
    ${AMAROK_SOURCE_TREE}/collection
    ${CMAKE_BINARY_DIR}/tests
    ${CMAKE_BINARY_DIR}/src
)

#------------------------ Test PlaylistFileProvider -----------------------------

set( testplaylistfileprovider_SRCS
     TestPlaylistFileProvider.cpp
     ${AMAROK_SOURCE_TREE}/playlistmanager/file/PlaylistFileProvider.cpp
     ${AMAROK_SOURCE_TREE}/playlist/PlaylistModelStack.cpp
     ${AMAROK_SOURCE_TREE}/playlist/proxymodels/GroupingProxy.cpp
     ${AMAROK_SOURCE_TREE}/playlist/proxymodels/ProxyBase.cpp
     ${AMAROK_SOURCE_TREE}/playlist/proxymodels/SearchProxy.cpp
     ${AMAROK_SOURCE_TREE}/playlist/proxymodels/SortFilterProxy.cpp
     ${AMAROK_SOURCE_TREE}/playlist/proxymodels/SortAlgorithms.cpp
     ${AMAROK_SOURCE_TREE}/playlist/proxymodels/SortScheme.cpp
     ${AMAROK_SOURCE_TREE}/core-implementations/playlists/file/PlaylistFileSupport.cpp
    )
kde4_add_unit_test( testplaylistfileprovider ${testplaylistfileprovider_SRCS} )
target_link_libraries( testplaylistfileprovider ${QT_QTTEST_LIBRARY} amaroklib )
