add_definitions(${KDE4_ENABLE_EXCEPTIONS})

include_directories( ${Amarok_SOURCE_DIR}/src
                     ${KDE4_INCLUDE_DIR}
                     ${QT_INCLUDES}
                     ${QJSON_INCLUDE_DIR}
                     ${CMAKE_CURRENT_BINARY_DIR} )

set( amarok_collection-spotifycollection_PART_SRCS
     SpotifyCollection.cpp
     SpotifyQueryMaker.cpp
     SpotifyMeta.cpp
     SpotifyConfig.cpp
     #SpotifyPlaylist.cpp
     #SpotifyPlaylistProvider.cpp
     support/TrackProxy.cpp
     support/Query.cpp
     support/Controller.cpp )

kde4_add_plugin(amarok_collection-spotifycollection ${amarok_collection-spotifycollection_PART_SRCS})
#kde4_add_library(amarok_collection-spotifycollection SHARED ${amarok_collection-spotifycollection_PART_SRCS})

target_link_libraries( amarok_collection-spotifycollection
                       amarokcore
                       amaroklib
                       ${QJSON_LIBRARIES}
                       ${KDE4_KDECORE_LIBS}
                       ${KDE4_KDEUI_LIBS}
                       ${KDE4_THREADWEAVER_LIBRARIES}
                       ${KDE4_KIO_LIBS} )

install( TARGETS amarok_collection-spotifycollection DESTINATION ${PLUGIN_INSTALL_DIR} )

install( FILES amarok_collection-spotifycollection.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

# Config plugin
set( kcm_amarok_collection-spotifycollection_PART_SRCS
     SpotifyConfig.cpp
     SpotifySettings.cpp
    )

kde4_add_ui_files( kcm_amarok_collection-spotifycollection_PART_SRCS SpotifyConfigWidget.ui )

kde4_add_plugin( kcm_amarok_collection-spotifycollection ${kcm_amarok_collection-spotifycollection_PART_SRCS})

target_link_libraries( kcm_amarok_collection-spotifycollection
                       amarokcore
                       amaroklib
                       #amarok_collection-spotifycollection
                       ${KDE4_KDECORE_LIBS}
                       ${KDE4_KDEUI_LIBS}
                       ${KDE4_KUTILS_LIBS}
                       ${KDE4_THREADWEAVER_LIBRARIES}
                       ${KDE4_KIO_LIBS} )

install( TARGETS kcm_amarok_collection-spotifycollection DESTINATION ${PLUGIN_INSTALL_DIR} )

install( FILES amarok_collection-spotifycollection_config.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
