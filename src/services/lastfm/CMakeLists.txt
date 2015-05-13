include_directories(
    ${LIBLASTFM_INCLUDE_DIR}
)

add_subdirectory( images )

########### next target ###############

set( amarok_service_lastfm_shared_SRCS
    LastFmServiceConfig.cpp
)
add_library( amarok_service_lastfm_shared SHARED ${amarok_service_lastfm_shared_SRCS} )
target_link_libraries( amarok_service_lastfm_shared
    amarokcore
    KF5::KDELibs4Support
    
)
install( TARGETS amarok_service_lastfm_shared ${INSTALL_TARGETS_DEFAULT_ARGS} )

########### next target ###############

set( amarok_service_lastfm_PART_SRCS
    LastFmService.cpp
    LastFmServiceCollection.cpp
    ScrobblerAdapter.cpp
    SynchronizationAdapter.cpp
    SynchronizationTrack.cpp
    LastFmTreeModel.cpp
    LastFmTreeView.cpp
    AvatarDownloader.cpp

    meta/LastFmMeta.cpp
    meta/LastFmMultiPlayableCapability.cpp
    meta/LastFmStreamInfoCapability.cpp

    biases/LastFmBias.cpp
    biases/WeeklyTopBias.cpp

    SimilarArtistsAction.cpp
    LoveTrackAction.cpp
)
add_library(amarok_service_lastfm MODULE ${amarok_service_lastfm_PART_SRCS} )
target_link_libraries( amarok_service_lastfm
    amarok_service_lastfm_shared
    amarokcore
    amaroklib
    amarokpud
    ${LIBLASTFM_LIBRARY}
    KF5::KDELibs4Support
    
    KF5::KIOCore
    ${KDE4_SOLID_LIBRARY}
    KF5::ThreadWeaver
    Qt5::Network
)
install( TARGETS amarok_service_lastfm DESTINATION ${PLUGIN_INSTALL_DIR} )

########### next target ###############

set(kcm_amarok_service_lastfm_PART_SRCS
    LastFmServiceSettings.cpp
)
ki18n_wrap_ui( kcm_amarok_service_lastfm_PART_SRCS LastFmConfigWidget.ui )
add_library(kcm_amarok_service_lastfm MODULE ${kcm_amarok_service_lastfm_PART_SRCS} )
target_link_libraries( kcm_amarok_service_lastfm
    amarok_service_lastfm_shared
    amarokcore
    amaroklib
    ${LIBLASTFM_LIBRARY}
    
    
    KF5::KIOCore
    Qt5::Network )
install( TARGETS kcm_amarok_service_lastfm DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

install( FILES amarok_service_lastfm.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
install( FILES amarok_service_lastfm_config.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
install( FILES amaroklastfm.protocol DESTINATION ${SERVICES_INSTALL_DIR} )
