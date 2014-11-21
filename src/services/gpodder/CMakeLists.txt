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
        ${KDE4_INCLUDE_DIR}
        ${QT_INCLUDES}
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

        kde4_add_plugin(amarok_service_gpodder ${amarok_service_gpodder_PART_SRCS})

        target_link_libraries(amarok_service_gpodder
            amarokcore
            amaroklib
            amarokpud
            ${LIBMYGPO_QT_LIBRARIES}
            ${KDE4_KDECORE_LIBS}
            ${KDE4_KDEUI_LIBS}
            ${KDE4_KIO_LIBS}
            ${KDE4_SOLID_LIBRARY}
            ${KDE4_THREADWEAVER_LIBRARIES}
            ${QT_QTNETWORK_LIBRARY}
        )

         install( TARGETS amarok_service_gpodder DESTINATION ${PLUGIN_INSTALL_DIR} )

         set(kcm_amarok_service_gpodder_PART_SRCS
            GpodderServiceSettings.cpp
            GpodderServiceConfig.cpp
         )

         kde4_add_ui_files( kcm_amarok_service_gpodder_PART_SRCS GpodderConfigWidget.ui )

         kde4_add_plugin( kcm_amarok_service_gpodder ${kcm_amarok_service_gpodder_PART_SRCS} )

         target_link_libraries( kcm_amarok_service_gpodder
            amarokcore
            amaroklib
            ${LIBMYGPO_QT_LIBRARIES}
            ${KDE4_KDEUI_LIBS}
            ${KDE4_KUTILS_LIBS}
            ${KDE4_KIO_LIBS}
            ${QT_QTNETWORK_LIBRARY} )

         install(TARGETS kcm_amarok_service_gpodder  DESTINATION ${PLUGIN_INSTALL_DIR})

         install( FILES amarok_service_gpodder.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
         install( FILES amarok_service_gpodder_config.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
