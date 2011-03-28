include_directories(
        ../
        ../../
        ../../core-impl/collections
        ../../statusbar
        ../../widgets
        ../../context
        ../../network
        ../../dynamic # for CustomBias.h
        ../../browsers/playlistbrowser # for DynamicBiasWidgets
        ${CMAKE_CURRENT_BINARY_DIR}/../.. #for amarokconfig.h
        ${KDE4_INCLUDE_DIR}
        ${STRIGI_INCLUDE_DIR}
        ${QT_INCLUDES}
        ${LIBMYGPO_QT_INCLUDE_DIR}
        ${LIBMYGPO_QT_INCLUDE_DIR}/../
        )

        add_subdirectory( images )

        add_definitions(${KDE4_ENABLE_EXCEPTIONS}) #not sure if needed

        set(amarok_service_gpodder_PART_SRCS
            GpodderService.cpp
            GpodderServiceModel.cpp
            GpodderServiceView.cpp
            GpodderTreeItem.cpp
            GpodderPodcastTreeItem.cpp
            GpodderTagTreeItem.cpp
            GpodderPodcastRequestHandler.cpp
            GpodderSortFilterProxyModel.cpp
        )

        
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}") #not sure if needed

        kde4_add_plugin(amarok_service_gpodder ${amarok_service_gpodder_PART_SRCS})
        
        target_link_libraries(amarok_service_gpodder
            amarokcore
            amaroklib
            amarokpud
            ${LIBMYGPO_QT_LIBRARY}
            ${KDE4_KDECORE_LIBS}
            ${KDE4_KDEUI_LIBS}
            ${KDE4_KIO_LIBS}
            ${KDE4_SOLID_LIBRARY}
            ${KDE4_THREADWEAVER_LIBRARIES}
            ${QT_QTNETWORK_LIBRARY}
        )

         install( TARGETS amarok_service_gpodder DESTINATION ${PLUGIN_INSTALL_DIR} )

         install( FILES amarok_service_gpodder.desktop DESTINATION ${SERVICES_INSTALL_DIR} )