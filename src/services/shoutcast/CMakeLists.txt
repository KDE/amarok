        include_directories(
            ../
            ../..
            ../../plugin
            ../../meta
            ../../collection
            ../../servicebrowser
            ../../statusbar
            ${CMAKE_CURRENT_BINARY_DIR}/../../..
            ${KDE4_INCLUDE_DIR}
            ${STRIGI_INCLUDE_DIR}
            ${QT_INCLUDES} )

########### next target ###############

        set(amarok_service_shoutcast_PART_SRCS
            ShoutcastMeta.cpp
            ShoutcastService.cpp
            ShoutcastServiceCollection.cpp
            ShoutcastServiceQueryMaker.cpp
        )


        kde4_add_plugin(amarok_service_shoutcast ${amarok_service_shoutcast_PART_SRCS})
        target_link_libraries(amarok_service_shoutcast
            amaroklib
            ${KDE4_KDECORE_LIBS}
            ${KDE4_KDEUI_LIBS}
            ${KDE4_KIO_LIBS}
            ${KDE4_THREADWEAVER_LIBRARIES}
        )


        install(TARGETS amarok_service_shoutcast DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

        install( FILES amarok_service_shoutcast.desktop DESTINATION ${SERVICES_INSTALL_DIR})
