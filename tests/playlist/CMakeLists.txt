include_directories(
  ${AMAROK_TEST_TREE}
  ${AMAROK_SOURCE_TREE}
  ${AMAROK_UTILITY_TREE}
  ${CMAKE_BINARY_DIR}/src
  ${CMAKE_BINARY_DIR}/tests
  ${AMAROK_COLLECTION_SUPPORT_DIR}
  ${KDE4_INCLUDE_DIR}
  ${QT_INCLUDES}
  ${GOOGLEMOCK_INCLUDE_DIR}
  )

#------------------------ Test CueFileSupport -----------------------------

set( testplaylistmodels_SRCS
        TestPlaylistModels.cpp
	${AMAROK_SOURCE_TREE}/EngineController.cpp
        ${AMAROK_SOURCE_TREE}/EngineObserver.cpp
        ${AMAROK_SOURCE_TREE}/core/capabilities/BoundedPlaybackCapability.cpp
        ${AMAROK_SOURCE_TREE}/core/capabilities/MultiSourceCapability.cpp
        ${AMAROK_SOURCE_TREE}/playlist/PlaylistModel.cpp
        ${AMAROK_SOURCE_TREE}/playlist/PlaylistModelStack.cpp
	${AMAROK_SOURCE_TREE}/playlist/proxymodels/ProxyBase.cpp
	${AMAROK_SOURCE_TREE}/playlist/proxymodels/GroupingProxy.cpp
	${AMAROK_SOURCE_TREE}/playlist/proxymodels/SearchProxy.cpp
	${AMAROK_SOURCE_TREE}/playlist/proxymodels/SortAlgorithms.cpp
	${AMAROK_SOURCE_TREE}/playlist/proxymodels/SortFilterProxy.cpp
	${AMAROK_SOURCE_TREE}/playlist/proxymodels/SortScheme.cpp
        ${AMAROK_SOURCE_TREE}/core/playlists/impl/file/PlaylistFileSupport.cpp
        ${AMAROK_SOURCE_TREE}/DirectoryLoader.cpp
        ${AMAROK_SOURCE_TREE}/playlist/PlaylistItem.cpp
        ${AMAROK_SOURCE_TREE}/Amarok.cpp
    )

kde4_add_unit_test( testplaylistmodels ${testplaylistmodels_SRCS} )

add_dependencies( testplaylistmodels amaroklib )

target_link_libraries( testplaylistmodels
    amaroklib
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${QT_QTTEST_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    ${GOOGLEMOCK_LIBRARIES}
    ${CMAKE_DL_LIBS})
