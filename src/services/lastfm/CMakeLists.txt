include_directories(SYSTEM
    ${LIBLASTFM_INCLUDE_DIR}
)

add_subdirectory( images )

########### next target ###############

find_package( KF${KF_MAJOR_VERSION} COMPONENTS Wallet REQUIRED )

set( amarok_service_lastfm_config_SRCS
    LastFmServiceConfig.cpp
)

add_library( amarok_service_lastfm_config SHARED ${amarok_service_lastfm_config_SRCS} )
generate_export_header(amarok_service_lastfm_config BASE_NAME amarok_service_lastfm_config)
target_link_libraries( amarok_service_lastfm_config
    amarokcore
    KF${KF_MAJOR_VERSION}::Wallet
)

install( TARGETS amarok_service_lastfm_config ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

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
    amarok_service_lastfm_config
    amarokcore
    amaroklib
    amarokpud
    ${LIBLASTFM_LIBRARY}
    KF${KF_MAJOR_VERSION}::KIOCore
    KF${KF_MAJOR_VERSION}::ThreadWeaver
    KF${KF_MAJOR_VERSION}::Wallet
    Qt::Network
)

install( TARGETS amarok_service_lastfm DESTINATION ${KDE_INSTALL_PLUGINDIR} )

########### next target ###############

set(kcm_amarok_service_lastfm_PART_SRCS
    LastFmServiceSettings.cpp
)

ki18n_wrap_ui( kcm_amarok_service_lastfm_PART_SRCS LastFmConfigWidget.ui )

add_library(kcm_amarok_service_lastfm MODULE ${kcm_amarok_service_lastfm_PART_SRCS} )

target_link_libraries( kcm_amarok_service_lastfm
    amarok_service_lastfm_config
    amarokcore
    amaroklib
    ${LIBLASTFM_LIBRARY}
    KF${KF_MAJOR_VERSION}::KIOCore
    Qt::Network
)

install( TARGETS kcm_amarok_service_lastfm DESTINATION ${KDE_INSTALL_PLUGINDIR} )

########### install files ###############

if(NOT BUILD_WITH_QT6)
  install( FILES amarok_service_lastfm.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR} )
  install( FILES amarok_service_lastfm_config.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR} )
  install( FILES amaroklastfm.protocol DESTINATION ${KDE_INSTALL_KSERVICES5DIR} )
endif()
