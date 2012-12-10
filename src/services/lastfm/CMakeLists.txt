include_directories(
    ${LIBLASTFM_INCLUDE_DIR}
)

add_subdirectory( images )

########### next target ###############

set( amarok_service_lastfm_PART_SRCS
    LastFmService.cpp
    LastFmServiceCollection.cpp
    LastFmServiceConfig.cpp
    ScrobblerAdapter.cpp
    SynchronizationAdapter.cpp
    SynchronizationTrack.cpp
    LastFmTreeModel.cpp
    LastFmTreeView.cpp
    AvatarDownloader.cpp

    meta/LastFmMeta.cpp
    meta/LastFmStreamInfoCapability.cpp

    biases/LastFmBias.cpp
    biases/WeeklyTopBias.cpp

    SimilarArtistsAction.cpp
    LoveTrackAction.cpp
)
kde4_add_plugin( amarok_service_lastfm ${amarok_service_lastfm_PART_SRCS} )
target_link_libraries( amarok_service_lastfm
    amarokcore
    amaroklib
    amarokpud
    ${LIBLASTFM_LIBRARY}
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KIO_LIBS}
    ${KDE4_SOLID_LIBRARY}
    ${KDE4_THREADWEAVER_LIBRARIES}
    ${QT_QTNETWORK_LIBRARY}
)
install( TARGETS amarok_service_lastfm DESTINATION ${PLUGIN_INSTALL_DIR} )

########### next target ###############

set(kcm_amarok_service_lastfm_PART_SRCS
    LastFmServiceSettings.cpp
    LastFmServiceConfig.cpp
)
kde4_add_ui_files( kcm_amarok_service_lastfm_PART_SRCS LastFmConfigWidget.ui )
kde4_add_plugin( kcm_amarok_service_lastfm ${kcm_amarok_service_lastfm_PART_SRCS} )
target_link_libraries( kcm_amarok_service_lastfm
    amarokcore
    amaroklib
    ${LIBLASTFM_LIBRARY}
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KUTILS_LIBS}
    ${KDE4_KIO_LIBS}
    ${QT_QTNETWORK_LIBRARY} )
install( TARGETS kcm_amarok_service_lastfm DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

install( FILES amarok_service_lastfm.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
install( FILES amarok_service_lastfm_config.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
install( FILES amaroklastfm.protocol DESTINATION ${SERVICES_INSTALL_DIR} )
