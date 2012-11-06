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
     SpotifySettingsDialog.cpp
     SpotifyDownloadDialog.cpp
     support/TrackProxy.cpp
     support/Query.cpp
     support/Controller.cpp )

kde4_add_ui_files( amarok_collection-spotifycollection_PART_SRCS
    SpotifySettingsWidget.ui
    SpotifyDownloadDialog.ui
)

kde4_add_plugin(amarok_collection-spotifycollection ${amarok_collection-spotifycollection_PART_SRCS})
#kde4_add_library(amarok_collection-spotifycollection SHARED ${amarok_collection-spotifycollection_PART_SRCS})

target_link_libraries( amarok_collection-spotifycollection
                       amarokcore
                       amaroklib
                       ${QJSON_LIBRARIES}
                       ${KDE4_KDECORE_LIBS}
                       ${KDE4_KDEUI_LIBS}
                       ${KDE4_THREADWEAVER_LIBRARIES}
                       ${KDE4_KIO_LIBS}
                       ${QT_QTNETWORK_LIBRARY})

install( TARGETS amarok_collection-spotifycollection DESTINATION ${PLUGIN_INSTALL_DIR} )

install( FILES amarok_collection-spotifycollection.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

