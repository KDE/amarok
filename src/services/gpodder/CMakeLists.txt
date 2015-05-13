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
        
        
        ${LIBMYGPO_QT_INCLUDE_DIRS}
        ${LIBMYGPO_QT_INCLUDE_DIRS}/../
        )

        add_subdirectory( images )

        set(amarok_service_gpodder_PART_SRCS
            GpodderService.cpp
            GpodderServiceConfig.cpp
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
            ${LIBMYGPO_QT_LIBRARIES}
            KF5::KDELibs4Support
            
            KF5::KIOCore
            ${KDE4_SOLID_LIBRARY}
            KF5::ThreadWeaver
            Qt5::Network
        )

         install( TARGETS amarok_service_gpodder DESTINATION ${PLUGIN_INSTALL_DIR} )

         set(kcm_amarok_service_gpodder_PART_SRCS
            GpodderServiceSettings.cpp
            GpodderServiceConfig.cpp
         )

         ki18n_wrap_ui( kcm_amarok_service_gpodder_PART_SRCS GpodderConfigWidget.ui )

         add_library(kcm_amarok_service_gpodder MODULE ${kcm_amarok_service_gpodder_PART_SRCS} )

         target_link_libraries( kcm_amarok_service_gpodder
            amarokcore
            amaroklib
            ${LIBMYGPO_QT_LIBRARIES}
            
            
            KF5::KIOCore
            Qt5::Network )

         install(TARGETS kcm_amarok_service_gpodder  DESTINATION ${PLUGIN_INSTALL_DIR})

         install( FILES amarok_service_gpodder.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
         install( FILES amarok_service_gpodder_config.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
