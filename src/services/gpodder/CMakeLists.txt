include_directories(
    ../
    ../../
    ../../core-impl/collections
    ../../statusbar
    ../../widgets
    ../../context
    ../../network
    ../../dynamic # for CustomBias.h
    ../../browsers/playlistbrowser
    ${CMAKE_CURRENT_BINARY_DIR}/../.. #for amarokconfig.h
)

add_subdirectory( images )

find_package( KF5 COMPONENTS Wallet REQUIRED )

set(libgpodder_service_config_SRCS
    GpodderServiceConfig.cpp
)

add_library(gpodder_service_config SHARED ${libgpodder_service_config_SRCS})

target_link_libraries(gpodder_service_config
    amaroklib
    amarokcore
    KF5::Wallet
)

install(TARGETS gpodder_service_config DESTINATION ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

set(amarok_service_gpodder_PART_SRCS
    GpodderService.cpp
    GpodderServiceModel.cpp
    GpodderServiceView.cpp
    GpodderProvider.cpp
    GpodderPodcastMeta.cpp
    GpodderTreeItem.cpp
    GpodderPodcastTreeItem.cpp
    GpodderTagTreeItem.cpp
    GpodderPodcastRequestHandler.cpp
    GpodderSortFilterProxyModel.cpp
)

add_library(amarok_service_gpodder MODULE ${amarok_service_gpodder_PART_SRCS})

target_link_libraries(amarok_service_gpodder
    amarokcore
    amaroklib
    amarokpud
    gpodder_service_config
    mygpo-qt5
    KF5::KIOCore
    KF5::ThreadWeaver
    Qt5::Network
)

install( TARGETS amarok_service_gpodder DESTINATION ${KDE_INSTALL_PLUGINDIR} )

set(kcm_amarok_service_gpodder_PART_SRCS
    GpodderServiceSettings.cpp
)

ki18n_wrap_ui( kcm_amarok_service_gpodder_PART_SRCS GpodderConfigWidget.ui )

add_library(kcm_amarok_service_gpodder MODULE ${kcm_amarok_service_gpodder_PART_SRCS} )

target_link_libraries( kcm_amarok_service_gpodder
    amarokcore
    amaroklib
    gpodder_service_config
    mygpo-qt5
    KF5::ConfigWidgets
    KF5::KIOCore
    Qt5::Network
)

install(TARGETS kcm_amarok_service_gpodder DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES amarok_service_gpodder_config.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR} )
